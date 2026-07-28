#!/usr/bin/env bash
# The one way to run this repo (in-container half; drive it via ./run.sh).
#
# The syslog-ng oracle is already up (this runs in the `run` compose service,
# which shares the oracle's network namespace). Build the baseline, run it under
# QEMU, then capture BOTH:
#   - the device's self-measured figures (and a size cross-check), and
#   - whatever the oracle received (nothing until the device sends its first record),
# self-check the figures against the frozen baseline, and print it all. Identical
# behaviour locally and in CI.
#
# CAPTURE=1 (re)freezes measurements/<TAG>.csv from this run — but only from a
# run that completed successfully.
set -euo pipefail

REPO="${REPO:-/w}"

# Which stage are we building? The last row of measurements/stages.tsv — stages in
# the order they were added, so the newest is the one under construction. Its row
# is added when its work starts and its measurements/<slug>.csv frozen at the end,
# so until then the self-check has nothing to compare against and says so.
#
# Defaulting to Baseline instead would drift-check every later stage against the
# baseline image and fail for the whole of its development, which is the guard
# firing at the one time it has nothing to protect.
TAG="${TAG:-$(awk -F'\t' '!/^[[:space:]]*#/ && NF { state = $1 } END { print state }' "${REPO}/measurements/stages.tsv")}"
TOL="${TOL:-0}"

# Tolerance per key, because they are not all reproducible to the same degree.
# Flash and static RAM are exact and are held to it — a byte of drift there is a
# real change. The mbedTLS figures cannot be: scripts/gen-certs.sh builds a fresh
# PKI every run and DER lengths shift with the key material, which moves the
# allocator's high-water mark by ~100 bytes either way. Holding those to zero
# would fail runs for a reason that has nothing to do with the device.
key_tolerance() {
    case "$1" in
        mbedtls_peak | mbedtls_free | heap_used) echo 256 ;;
        lwip_pbufs_free) echo 4 ;;
        *) echo "$TOL" ;;
    esac
}
CAPTURE="${CAPTURE:-0}"
BUILD_DIR="$REPO/build/baseline-cross"
ELF="$BUILD_DIR/baseline.elf"
EXPECTED="$REPO/measurements/${TAG}.csv"
ORACLE_LOG_DIR="${ORACLE_LOG_DIR:-/collector}"

cd "$REPO"

# This harness has twice published a different tree's figures and called it PASS:
# once from a conflicted apply that left markers in CMakeLists.txt, once from a
# configure older than the sources, which rebuilt nothing and measured the
# previous stage's binary. A repository whose whole claim is that its numbers can
# be trusted has to refuse those runs rather than report them.
if markers="$(grep -rlE '^(<<<<<<<|>>>>>>>) ' CMakeLists.txt CMakePresets.json cmake app scripts 2>/dev/null)"; then
    echo "FAIL: unresolved conflict markers in: $(tr '\n' ' ' <<<"$markers")" >&2
    exit 1
fi
for f in CMakeLists.txt CMakePresets.json cmake/arm-none-eabi.cmake; do
    if [ -f "$BUILD_DIR/CMakeCache.txt" ] && [ "$f" -nt "$BUILD_DIR/CMakeCache.txt" ]; then
        echo "=== $f is newer than the configure — discarding $BUILD_DIR ==="
        rm -rf "$BUILD_DIR"
    fi
done

echo "=== build (${TAG}) ==="
[ -d "$BUILD_DIR" ] || cmake --preset baseline-cross
cmake --build "$BUILD_DIR" -j"$(nproc)"
[ -f "$ELF" ] || { echo "FAIL: $ELF not built" >&2; exit 1; }

echo "=== prove the listeners ==="
set +e
SMOKE_OUT="$(ORACLE_LOG_DIR="$ORACLE_LOG_DIR" bash "${REPO}/scripts/smoke-oracle.sh")"
SMOKE_RC=$?
set -e
printf '%s\n' "$SMOKE_OUT"

echo "=== run under QEMU (oracle up; the app reaches it via slirp) ==="
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

# A run is usable only if the oracle proved out, QEMU exited cleanly, AND the device
# emitted a full report. One definition, honoured by both the freeze below and the
# self-check: figures from a run whose collector was not listening are not figures
# worth keeping, and once the device sends over TLS they are not even the same
# figures, because a dead listener changes what the device does and not just what
# the collector heard.
run_ok=1
[ "$SMOKE_RC" -eq 0 ] || run_ok=0
[ "$RC" -eq 0 ] || run_ok=0
grep -q '\[report\] --- end ---' <<<"$APP_OUT" || run_ok=0
grep -q '\[device\] ready' <<<"$APP_OUT" || run_ok=0

# A device that says it logged a record must have delivered one. It can be
# perfectly healthy while its records go nowhere — a failed handshake, a sender
# left unwired, a drain window too short for a TLS connect — and until this check
# existed the run said PASS and printed an empty collector section.
#
# Keyed on what the device did, not on which stage is building: the stages before
# the logger sends its first record send nothing, and are not failures for it.
delivered=1
if grep -q '\[device\]   first record logged: yes' <<<"$APP_OUT" && [ -z "$ORACLE_OUT" ]; then
    delivered=0
    run_ok=0
fi

# "key,current" pairs from the report — reused for both capture and self-check.
figures="$(sed -n 's/^\[report\] \([a-z_][a-z_]*\),\([0-9][0-9-]*\),.*/\1,\2/p' <<<"$APP_OUT")"

# (Re)freeze the baseline from this run if asked — never from a bad run.
if [ "$CAPTURE" = "1" ]; then
    if [ "$run_ok" = "1" ] && [ -n "$figures" ]; then
        {
            echo "# ${TAG} figures (bytes) — captured by scripts/run.sh (CAPTURE=1)."
            echo "# The device reads measurements/Baseline.csv as its frozen baseline and reports current-minus-Baseline."
            printf '%s\n' "$figures"
        } > "$EXPECTED"
        echo "froze measurements/${TAG}.csv"
    else
        echo "refusing to freeze measurements/${TAG}.csv — the run did not complete cleanly" >&2
    fi
fi

# ---- self-check (only meaningful on a good run) ----
if [ "$run_ok" != "1" ]; then
    selfcheck="  (skipped — the run did not complete successfully)"
    selfcheck_rc=2
elif [ ! -f "$EXPECTED" ]; then
    selfcheck="  (no committed measurements/${TAG}.csv yet — rerun with CAPTURE=1 to freeze it)"
    selfcheck_rc=3
else
    selfcheck="$(
        drift=0
        while IFS=, read -r key cur; do
            exp="$(sed -n "s/^${key},\([0-9-]*\).*/\1/p" "$EXPECTED" | head -1)"
            if [ -z "$exp" ]; then
                echo "  ?     $key: no expected value in measurements/${TAG}.csv"
                drift=$((drift + 1))
                continue
            fi
            tol="$(key_tolerance "$key")"
            d=$((cur - exp)); d=${d#-}
            if [ "$d" -le "$tol" ]; then
                echo "  OK    $key: $cur (expected $exp, Δ$d)"
            else
                echo "  DRIFT $key: $cur vs expected $exp (Δ$d > $tol)"
                drift=$((drift + 1))
            fi
        done <<<"$figures"
        [ "$drift" -eq 0 ] || exit 1
    )" && selfcheck_rc=0 || selfcheck_rc=$?
fi

# ---- verdict ----
verdict="PASS"
exit_code=0
if [ "$SMOKE_RC" -ne 0 ]; then
    verdict="FAIL (${SMOKE_RC} listener(s) unproved)"; exit_code=1
elif [ "$RC" -ne 0 ]; then
    verdict="FAIL (qemu exit $RC)"; exit_code=1
elif [ "$delivered" != "1" ]; then
    verdict="FAIL (nothing reached the collector)"; exit_code=1
elif [ "$run_ok" != "1" ]; then
    verdict="FAIL (incomplete device report)"; exit_code=1
elif [ "$selfcheck_rc" -eq 1 ]; then
    verdict="FAIL (baseline drift)"; exit_code=1
elif [ "$selfcheck_rc" -eq 3 ]; then
    verdict="PASS (self-check skipped — no committed baseline)"
fi

# ---- assemble + emit the combined report (stdout + build/run-report.md) ----
# Markdown, so the committed copy renders where it is read — on GitHub, in the CI
# job summary, and linked from the README — while staying exactly as readable in a
# terminal. Every block the device or the collector produced is fenced verbatim:
# the formatting is around the evidence, never applied to it.
report="$(
    echo "# solid-syslog-example — run (${TAG})"
    echo
    echo '## Device (self-measured)'
    echo
    echo '```text'
    grep -E '^\[device\]|^\[sim\]|^\[report\]|^\[syslog\]' <<<"$APP_OUT" || true
    echo '```'
    echo
    echo '### Size cross-check'
    echo
    echo '```text'
    arm-none-eabi-size "$ELF"
    echo '```'
    echo
    echo '## Listeners (proved before the device ran)'
    echo
    echo '```text'
    printf '%s\n' "$SMOKE_OUT"
    echo '```'
    echo
    echo '## Collector (syslog-ng) received'
    echo
    echo '```text'
    if [ -n "$ORACLE_OUT" ]; then
        printf '%s\n' "$ORACLE_OUT"
    else
        echo "(nothing — this device sends no records yet)"
    fi
    echo '```'
    echo
    echo "## Self-check (vs measurements/${TAG}.csv)"
    echo
    echo '```text'
    echo "$selfcheck"
    echo '```'
    echo
    echo "**RESULT: ${verdict}**"
)"

printf '%s\n' "$report"
mkdir -p "$REPO/build"
printf '%s\n' "$report" > "$REPO/build/run-report.md"

# A second, committed copy. Each stage in this repository carries the run it
# produced, so the diff between two commits shows what actually changed on the
# device — a line the logger started printing, a figure that moved — and not
# just the source that caused it.
#
# Verbatim, timestamps included. They differ on every run, which is honest: the
# device reads a real clock, and how long it took to get a record out is exactly
# the kind of thing worth being able to see move.
printf '%s\n' "$report" > "$REPO/run-report.md"

exit "$exit_code"
