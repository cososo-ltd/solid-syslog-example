#!/usr/bin/env bash
# Check that app/ is the same in this stack and its sibling, stage by stage.
#
#   scripts/app-invariant.sh <ref> <sibling-ref>
#
# The two stacks are one application built two ways, so at every stage app/ is
# byte-identical between them, bar the file below, which names its own build.
# Stages are paired by slug. Exits 0 where every stage matches. Where some do
# not, prints the slug of each stage whose app/ differs or that only one stack
# has, one per line, and exits 2. Exits 1 if the comparison could not be made.
set -Eeuo pipefail
# Exit 2 is this script's own answer, and also how sort and awk fail.
trap 'exit 1' ERR

REF="${1:?usage: scripts/app-invariant.sh <ref> <sibling-ref>}"
SIBLING="${2:?usage: scripts/app-invariant.sh <ref> <sibling-ref>}"
STAGES=measurements/stages.tsv
EXCEPT=app/net/smsc9220/KNOWN-LIMITATIONS.md
TAB="$(printf '\t')"

fail() { echo "app-invariant: $*" >&2; exit 1; }

# slug <TAB> commit, for every commit that adds a row: an evidence commit adds none.
stages_of() {
    local tip commits c slug last=""
    tip="$(git rev-parse -q --verify "$1^{commit}")" || fail "'$1' is not a commit"
    commits="$(git rev-list --first-parent --reverse "$tip")"
    for c in $commits; do
        slug="$(git show "$c:$STAGES" | awk -F'\t' '!/^[[:space:]]*#/ && NF { s = $1 } END { print s }')"
        [ -n "$slug" ] || fail "$STAGES at $c has no rows"
        [ "$slug" = "$last" ] || printf '%s\t%s\n' "$slug" "$c"
        last="$slug"
    done
}

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
stages_of "$REF" > "$WORK/ours"
stages_of "$SIBLING" > "$WORK/theirs"
sort -o "$WORK/ours" "$WORK/ours"
sort -o "$WORK/theirs" "$WORK/theirs"
join -t "$TAB" -a 1 -a 2 -e - -o 0,1.2,2.2 "$WORK/ours" "$WORK/theirs" > "$WORK/pairs"
[ -s "$WORK/pairs" ] || fail "no stages to compare"

differing=0
while IFS="$TAB" read -r slug ours theirs; do
    if [ "$ours" != - ] && [ "$theirs" != - ]; then
        rc=0
        git diff --quiet "$ours" "$theirs" -- app ":!$EXCEPT" || rc=$?
        [ "$rc" -le 1 ] || fail "cannot compare app/ at '$slug'"
        [ "$rc" -eq 1 ] || continue
    fi
    echo "$slug"
    differing=2
done < "$WORK/pairs"
exit "$differing"
