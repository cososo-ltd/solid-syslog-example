#!/usr/bin/env bash
# The one way to run this repo (in-container half; drive it via ./run.sh).
#
# Fire up nothing here — the syslog-ng oracle is already up (this runs in the
# `run` compose service, which shares the oracle's network namespace). Build the
# baseline, run it under QEMU, then capture BOTH:
#   - the device's self-measured figures (and a size cross-check), and
#   - whatever the oracle received (nothing at Baseline; real records from Minimal),
# self-check the figures against the frozen baseline, and print it all. Identical
# behaviour locally and in CI.
#
# CAPTURE=1 (re)freezes measurements/<TAG>.csv from this run.
set -euo pipefail

REPO="${REPO:-/w}"
TAG="${TAG:-Baseline}"
TOL="${TOL:-64}"
CAPTURE="${CAPTURE:-0}"
BUILD_DIR="$REPO/build/baseline-cross"
ELF="$BUILD_DIR/baseline.elf"
EXPECTED="$REPO/measurements/${TAG}.csv"
ORACLE_LOG_DIR="${ORACLE_LOG_DIR:-/collector}"

cd "$REPO"

echo "=== build (${TAG}) ==="
[ -d "$BUILD_DIR" ] || cmake --preset baseline-cross
cmake --build "$BUILD_DIR" -j"$(nproc)"
[ -f "$ELF" ] || { echo "FAIL: $ELF not built" >&2; exit 1; }

echo "=== run under QEMU (oracle up; app reaches it via slirp from Minimal) ==="
rm -f baseline-disk.img
set +e
APP_OUT="$(timeout 120 qemu-system-arm -M mps2-an385 -m 16M -display none -serial stdio \
    -icount shift=auto,sleep=off,align=off \
    -netdev user,id=net0 -net nic,netdev=net0,model=lan9118 \
    -semihosting-config enable=on,target=native \
    -kernel "$ELF")"
RC=$?
set -e
rm -f baseline-disk.img

# Let the oracle flush, then read whatever it recorded.
sleep 1
ORACLE_OUT="$(cat "$ORACLE_LOG_DIR"/received*.log 2>/dev/null || true)"

# (Re)freeze the baseline from this run if asked.
if [ "$CAPTURE" = "1" ]; then
    {
        echo "# ${TAG} figures (bytes) — captured by scripts/run.sh (CAPTURE=1)."
        echo "# The device reads measurements/Baseline.csv as its frozen baseline and reports current-minus-Baseline."
        sed -n 's/^\[report\] \([a-z_][a-z_]*\),\([0-9][0-9-]*\),.*/\1,\2/p' <<<"$APP_OUT"
    } > "$EXPECTED"
fi

# ---- self-check ----
selfcheck="$(
    if [ ! -f "$EXPECTED" ]; then
        echo "  (no committed measurements/${TAG}.csv yet — rerun with CAPTURE=1 to freeze it)"
        exit 3
    fi
    drift=0
    while IFS=, read -r key cur; do
        exp="$(sed -n "s/^${key},\([0-9-]*\).*/\1/p" "$EXPECTED" | head -1)"
        if [ -z "$exp" ]; then
            echo "  ?     $key: no expected value in measurements/${TAG}.csv"
            drift=$((drift + 1))
            continue
        fi
        d=$((cur - exp)); d=${d#-}
        if [ "$d" -le "$TOL" ]; then
            echo "  OK    $key: $cur (expected $exp, Δ$d)"
        else
            echo "  DRIFT $key: $cur vs expected $exp (Δ$d > $TOL)"
            drift=$((drift + 1))
        fi
    done < <(sed -n 's/^\[report\] \([a-z_][a-z_]*\),\([0-9][0-9-]*\),.*/\1,\2/p' <<<"$APP_OUT")
    [ "$drift" -eq 0 ] || exit 1
)" && selfcheck_rc=0 || selfcheck_rc=$?

# ---- verdict ----
verdict="PASS"
exit_code=0
if [ "$RC" -ne 0 ]; then
    verdict="FAIL (qemu exit $RC)"; exit_code=1
elif ! grep -q '\[report\] --- end ---' <<<"$APP_OUT"; then
    verdict="FAIL (no measurement report)"; exit_code=1
elif ! grep -q '\[device\] ready' <<<"$APP_OUT"; then
    verdict="FAIL (baseline not ready)"; exit_code=1
elif [ "$selfcheck_rc" -eq 1 ]; then
    verdict="FAIL (baseline drift)"; exit_code=1
elif [ "$selfcheck_rc" -eq 3 ]; then
    verdict="PASS (self-check skipped — no committed baseline)"
fi

# ---- assemble + emit the combined report (stdout + build/run-report.txt) ----
report="$(
    echo "================ solid-syslog-example :: run (${TAG}) ================"
    echo
    echo "--- Device (self-measured; app talks to no collector at Baseline) ---"
    grep -E '^\[device\]|^\[report\]' <<<"$APP_OUT" || true
    echo
    echo "  size cross-check:"
    arm-none-eabi-size "$ELF" | sed 's/^/    /'
    echo
    echo "--- Collector (syslog-ng) received ---"
    if [ -n "$ORACLE_OUT" ]; then
        sed 's/^/  /' <<<"$ORACLE_OUT"
    else
        echo "  (nothing — the Baseline sends no records; SolidSyslog logs from Minimal)"
    fi
    echo
    echo "--- Baseline self-check (vs measurements/${TAG}.csv, tolerance ${TOL} B) ---"
    echo "$selfcheck"
    echo
    echo "RESULT: ${verdict}"
    echo "==================================================================="
)"

printf '%s\n' "$report"
mkdir -p "$REPO/build"
printf '%s\n' "$report" > "$REPO/build/run-report.txt"

exit "$exit_code"
