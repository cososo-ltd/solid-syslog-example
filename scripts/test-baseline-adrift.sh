#!/usr/bin/env bash
# Tests scripts/baseline-adrift.sh against a sweep's figures.csv.
#
#   scripts/test-baseline-adrift.sh
set -uo pipefail

TOOLS="$(cd "$(dirname "$0")" && pwd)"
SCRATCH="$(mktemp -d)"
trap 'rm -rf "$SCRATCH"' EXIT

failures=0
check() { # check <what> <command...>
    if "${@:2}" >/dev/null 2>&1; then echo "ok    $1"; else echo "FAIL  $1"; failures=$((failures + 1)); fi
}
figures() { # figures <index,slug,key,above>...
    echo "index,slug,commit,key,current,baseline,above_baseline" > "$SCRATCH/figures.csv"
    local row index slug key above
    for row in "$@"; do
        IFS=, read -r index slug key above <<<"$row"
        echo "$index,$slug,c$index,$key,$((1000 + above)),1000,$above" >> "$SCRATCH/figures.csv"
    done
}
adrift() { bash "$TOOLS/baseline-adrift.sh" "$SCRATCH/figures.csv" > "$SCRATCH/out" 2> "$SCRATCH/err"; }
says() { adrift && [ "$(cat "$SCRATCH/out")" = "$1" ]; }

figures 0,Baseline,flash_text,0 0,Baseline,flash_data,0 0,Baseline,static_bss,0 1,udp,flash_text,7000
check "its own: succeeds"                       adrift
check "its own: says nothing"                   says ""

figures 0,Baseline,flash_text,4192 0,Baseline,flash_data,-84 0,Baseline,static_bss,316
check "grown: succeeds"                         adrift
check "grown: names each figure and how far"    says "flash_text +4192 B, flash_data -84 B, static_bss +316 B"

figures 0,Baseline,flash_text,0 0,Baseline,stack_harness,200 0,Baseline,mbedtls_peak,-364
check "a stack or heap figure is not counted"   says ""

rm "$SCRATCH/figures.csv"
check "no figures: fails"                       bash -c "! bash '$TOOLS/baseline-adrift.sh' '$SCRATCH/figures.csv'"

echo
if [ "$failures" -eq 0 ]; then echo "all passed"; else echo "$failures failed"; exit 1; fi
