#!/usr/bin/env bash
# Measure every stage of a release branch in one run.
#
#   scripts/sweep.sh <ref> [out-dir]
#
# Rows of <ref>'s measurements/stages.tsv map positionally onto its last commits,
# one row per commit, an evidence commit at the tip excepted. Each stage commit is
# checked out into its own worktree and runs its own scripts/run.sh from a clean
# build directory. Every stage is measured against the same PKI, so the figures
# are comparable across stages.
#
# Writes <out-dir>/stages.csv, figures.csv, figures.md and reports/NN-<slug>.md.
# Exits non-zero if the stack does not match its stage list, or any stage fails.
# MAP_ONLY=1 prints the row-to-commit mapping and stops.
set -euo pipefail

REF="${1:?usage: scripts/sweep.sh <ref> [out-dir]}"
OUT="${2:-sweep-out}"
TOOLS="$(cd "$(dirname "$0")" && pwd)"
STAGES=measurements/stages.tsv
PYTHON="${PYTHON:-$(for p in python3 python; do "$p" -c "import sys; assert sys.version_info >= (3, 8)" 2>/dev/null && { echo "$p"; break; }; done)}"

# Docker Desktop wants C:/... where Git Bash has /c/...
hostpath() { if command -v cygpath >/dev/null; then cygpath -m "$1"; else printf '%s' "$1"; fi; }
slugs_at() { git show "$1:$STAGES" | awk -F'\t' '!/^[[:space:]]*#/ && NF { print $1 }'; }
fail() { echo "sweep: $*" >&2; exit 1; }

# ---- map rows to commits -----------------------------------------------------
tip="$(git rev-parse --verify "$REF^{commit}")"
# A release branch is one commit per stage, so a merge anywhere means the rows
# cannot be mapped.
[ -z "$(git rev-list --merges "$tip")" ] || fail "$REF is not linear: $(git rev-list --merges "$tip" | head -1) is a merge"
if [ -z "$(git diff-tree --root --no-commit-id --name-only -r "$tip" | grep -v '^evidence/' || true)" ]; then
    tip="$(git rev-parse "$tip^")"
fi

mapfile -t rows < <(slugs_at "$tip")
mapfile -t chain < <(git rev-list --first-parent --reverse "$tip")
n=${#rows[@]}
[ "$n" -gt 0 ] || fail "$STAGES at $tip has no rows"
if [ "${#chain[@]}" -lt "$n" ]; then
    fail "$(( n - ${#chain[@]} )) of $n rows have no commit, '${rows[0]}' to '${rows[$(( n - ${#chain[@]} - 1 ))]}'"
fi
commits=("${chain[@]: -$n}")

for k in "${!rows[@]}"; do
    mapfile -t own < <(slugs_at "${commits[$k]}")
    if [ "${#own[@]}" -ne $(( k + 1 )) ] || [ "${own[*]}" != "${rows[*]:0:$(( k + 1 ))}" ]; then
        last=""
        [ "${#own[@]}" -eq 0 ] || last="${own[${#own[@]}-1]}"
        fail "row $k '${rows[$k]}' maps to $(git log -1 --format='%h %s' "${commits[$k]}"), whose $STAGES ends at '$last' after ${#own[@]} rows"
    fi
done

if [ "${MAP_ONLY:-0}" = 1 ]; then
    for k in "${!rows[@]}"; do printf '%2d  %-14s %s\n' "$k" "${rows[$k]}" "$(git log -1 --format='%h %s' "${commits[$k]}")"; done
    exit 0
fi

# ---- one PKI for every stage -------------------------------------------------
WORK="$(mktemp -d)"
compose() { docker compose -p solid-syslog-sweep -f "$(hostpath "$1/docker/docker-compose.yml")" -f "$(hostpath "$WORK/override.yml")" "${@:2}"; }
# The build is root-owned, so a root container removes it: certs, because it has
# no network_mode and so starts whether or not the stack is still up.
discard() {
    compose "$1" run --rm --no-deps certs bash -c 'rm -rf /w/build /w/baseline-disk.img' >/dev/null 2>&1 || true
    compose "$1" down --volumes --remove-orphans >/dev/null 2>&1 || true
    git worktree remove --force "$1" >/dev/null 2>&1 || true
}
# Best effort: the containers write as root, so on Linux some of what they leave
# cannot be removed here. Whatever cleanup manages, the sweep's own result stands.
cleanup() {
    for wt in "$WORK"/wt-*; do
        [ -d "$wt" ] && discard "$wt"
    done
    rm -rf "$WORK" 2> /dev/null
    git worktree prune
}
trap 'rc=$?; cleanup || true; exit "$rc"' EXIT

mkdir -p "$WORK/pki"
cat > "$WORK/override.yml" <<EOF
services:
  certs:
    volumes:
      - $(hostpath "$WORK/pki"):/pki
    command: bash -c "mkdir -p /w/build/certs && { [ -f /pki/ca.crt ] || bash scripts/gen-certs.sh /pki; } && cp -a /pki/. /w/build/certs/"
EOF

# ---- run each stage ----------------------------------------------------------
mkdir -p "$OUT" && OUT="$(cd "$OUT" && pwd)"
rm -rf "$OUT/reports"
mkdir -p "$OUT/reports"
echo "index,slug,commit,verdict" > "$OUT/stages.csv"
echo "index,slug,commit,key,current,baseline,above_baseline" > "$OUT/figures.csv"
failed=0

for k in "${!rows[@]}"; do
    slug="${rows[$k]}" c="${commits[$k]}"
    name="$(printf '%02d-%s' "$k" "$slug")"
    wt="$WORK/wt-$name"
    echo "=== [$((k + 1))/$n] $slug  $(git log -1 --format='%h %s' "$c")"

    out="$(git worktree add --detach "$wt" "$c" 2>&1)" || fail "cannot check out $c: $out"
    compose "$wt" down --volumes --remove-orphans >/dev/null 2>&1 || true
    set +e
    compose "$wt" run --rm -e TAG= -e TOL= -e CAPTURE=0 run > "$OUT/reports/$name.log" 2>&1
    rc=$?
    set -e

    report="$wt/build/run-report.md"
    if [ -f "$report" ]; then
        cp "$report" "$OUT/reports/$name.md"
        verdict="$(sed -n 's/^\*\*RESULT: \(.*\)\*\*$/\1/p' "$report")"
        sed -n 's/^\[report\] \([a-z_][a-z_]*\),\([0-9-]*\),\([0-9-]*\),\([0-9-]*\)$/\1,\2,\3,\4/p' "$report" |
            sed "s/^/$k,$slug,$c,/" >> "$OUT/figures.csv"
    else
        verdict="FAIL (no run report; see reports/$name.log)"
    fi
    [ "$rc" -eq 0 ] && [[ "$verdict" == PASS* ]] || { failed=$((failed + 1)); [[ "$verdict" == PASS* ]] && verdict="FAIL (exit $rc)"; }
    echo "$k,$slug,$c,\"$verdict\"" >> "$OUT/stages.csv"
    echo "    $verdict"

    discard "$wt"
done

"$PYTHON" "$(hostpath "$TOOLS/figures.py")" table "$(hostpath "$OUT")" > "$OUT/figures.md"
cat "$OUT/figures.md"

[ "$failed" -eq 0 ] || fail "$failed of $n stages failed"
