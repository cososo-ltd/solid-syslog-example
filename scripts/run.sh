#!/usr/bin/env bash
# The one way to run this repo (in-container half; drive it via ./run.sh).
#
# The syslog-ng oracle is already up (this runs in the `run` compose service,
# which shares the oracle's network namespace). Build the baseline, run it under
# QEMU, then capture BOTH:
#   - the device's self-measured figures (and a size cross-check), and
#   - whatever the oracle received (nothing until the device sends its first record),
# and print it all. Identical behaviour locally and in CI.
#
# CAPTURE=1 at the Baseline re-freezes measurements/Baseline.csv, the figures the
# device subtracts from its own — but only from a run that completed successfully.
set -euo pipefail

REPO="${REPO:-/w}"

# Which stage are we building? The last row of measurements/stages.tsv — stages in
# the order they were added, so the newest is the one under construction.
TAG="${TAG:-$(awk -F'\t' '!/^[[:space:]]*#/ && NF { state = $1 } END { print state }' "${REPO}/measurements/stages.tsv")}"
CAPTURE="${CAPTURE:-0}"
BUILD_DIR="$REPO/build/baseline-cross"
ELF="$BUILD_DIR/baseline.elf"
BASELINE="$REPO/measurements/Baseline.csv"
ORACLE_LOG_DIR="${ORACLE_LOG_DIR:-/collector}"

cd "$REPO"

if [ "$CAPTURE" = "1" ] && [ "$TAG" != "Baseline" ]; then
    echo "FAIL: CAPTURE=1 re-freezes measurements/Baseline.csv, so it runs only at the Baseline (this is ${TAG})" >&2
    exit 1
fi

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
# emitted a full report. Figures from a run whose collector was not listening are
# not figures worth keeping, and once the device sends over TLS they are not even
# the same figures, because a dead listener changes what the device does and not
# just what the collector heard.
run_ok=1
[ "$SMOKE_RC" -eq 0 ] || run_ok=0
[ "$RC" -eq 0 ] || run_ok=0
grep -q '\[report\] --- end ---' <<<"$APP_OUT" || run_ok=0
grep -q '\[device\] ready' <<<"$APP_OUT" || run_ok=0

# Every record the device logged must have arrived. A device can be perfectly
# healthy while its records go nowhere — a failed handshake, a sender left
# unwired, a drain window too short for a TLS connect — or while some of them do,
# which is the harder case: a run that delivers the first record and loses the
# rest looks like success from the collector's side alone.
#
# Keyed on what the device did, not on which stage is building: the stages before
# the logger sends its first record log nothing, and are not failures for it.
# Distinct records, so a replay after a reconnect cannot hide one that was lost.
logged="$(sed -n 's/^\[device\]   records logged: \([0-9]*\)$/\1/p' <<<"$APP_OUT")"
received="$({ grep '^wire ' <<<"$ORACLE_OUT" || true; } | sort -u | wc -l)"
delivered=1
if [ "${logged:-0}" -gt 0 ] && [ "$received" -lt "$logged" ]; then
    delivered=0
    run_ok=0
fi

# Re-freeze the Baseline from this run if asked — never from a bad run.
if [ "$CAPTURE" = "1" ]; then
    figures="$(sed -n 's/^\[report\] \([a-z_][a-z_]*\),\([0-9][0-9-]*\),.*/\1,\2/p' <<<"$APP_OUT")"
    if [ "$run_ok" = "1" ] && [ -n "$figures" ]; then
        {
            echo "# Baseline figures (bytes) — captured by scripts/run.sh (CAPTURE=1)."
            echo "# The device reads measurements/Baseline.csv as its frozen baseline and reports current-minus-Baseline."
            printf '%s\n' "$figures"
        } > "$BASELINE"
        echo "froze measurements/Baseline.csv"
    else
        echo "refusing to freeze measurements/Baseline.csv — the run did not complete cleanly" >&2
    fi
fi

# ---- verdict ----
verdict="PASS"
exit_code=0
if [ "$SMOKE_RC" -ne 0 ]; then
    verdict="FAIL (${SMOKE_RC} listener(s) unproved)"; exit_code=1
elif [ "$RC" -ne 0 ]; then
    verdict="FAIL (qemu exit $RC)"; exit_code=1
elif [ "$delivered" != "1" ]; then
    verdict="FAIL (${received} of ${logged} records reached the collector)"; exit_code=1
elif [ "$run_ok" != "1" ]; then
    verdict="FAIL (incomplete device report)"; exit_code=1
fi

# ---- assemble + emit the combined report (stdout + build/run-report.md) ----
# Markdown, so it renders where it is read — in the CI job summary — while
# staying exactly as readable in a terminal. Every block the device or the
# collector produced is fenced verbatim: the formatting is around the evidence,
# never applied to it.
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
    echo "**RESULT: ${verdict}**"
)"

printf '%s\n' "$report"
mkdir -p "$REPO/build"
printf '%s\n' "$report" > "$REPO/build/run-report.md"

exit "$exit_code"
