#!/usr/bin/env bash
# How far a sweep's Baseline measured from its own measurements/Baseline.csv.
#
#   scripts/baseline-adrift.sh <figures.csv>
#
# Every stage's cost is counted from that file, so a Baseline that has moved
# since the file was captured is charged to every stage after it. Prints each of
# the Baseline's flash and static RAM figures that differs, and by how much, and
# nothing where the file is this Baseline's own.
set -euo pipefail

FIGURES="${1:?usage: scripts/baseline-adrift.sh <figures.csv>}"
[ -f "$FIGURES" ] || { echo "baseline-adrift: no $FIGURES" >&2; exit 1; }

awk -F, 'NR > 1 && $1 == 0 && $4 ~ /^(flash_text|flash_data|static_bss)$/ && $7 != 0 {
    printf "%s%s %+d B", (c++ ? ", " : ""), $4, $7
} END { if (c) print "" }' "$FIGURES"
