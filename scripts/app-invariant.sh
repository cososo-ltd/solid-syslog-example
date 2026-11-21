#!/usr/bin/env bash
# Check that app/ is the same in this stack and its sibling, stage by stage.
#
#   scripts/app-invariant.sh <ref> <sibling-ref>
#
# The two stacks are one application built two ways, so at every stage app/ is
# byte-identical between them, bar the file below, which names its own build.
# Stages are paired by slug. Prints the slug of each stage whose app/ differs or
# that only one stack has, one per line, and exits 1 if there are any.
set -euo pipefail

REF="${1:?usage: scripts/app-invariant.sh <ref> <sibling-ref>}"
SIBLING="${2:?usage: scripts/app-invariant.sh <ref> <sibling-ref>}"
STAGES=measurements/stages.tsv
EXCEPT=app/net/smsc9220/KNOWN-LIMITATIONS.md

# slug <TAB> commit, for every commit that adds a row: an evidence commit adds none.
stages_of() {
    local c slug last=""
    for c in $(git rev-list --first-parent --reverse "$1"); do
        slug="$(git show "$c:$STAGES" | awk -F'\t' '!/^[[:space:]]*#/ && NF { s = $1 } END { print s }')"
        [ "$slug" = "$last" ] || printf '%s\t%s\n' "$slug" "$c"
        last="$slug"
    done
}

differing="$(join -t "$(printf '\t')" -a 1 -a 2 -e - -o 0,1.2,2.2 \
                 <(stages_of "$REF" | sort) <(stages_of "$SIBLING" | sort) |
    while IFS="$(printf '\t')" read -r slug ours theirs; do
        if [ "$ours" = - ] || [ "$theirs" = - ] || ! git diff --quiet "$ours" "$theirs" -- app ":!$EXCEPT"; then
            echo "$slug"
        fi
    done)"

[ -z "$differing" ] || { echo "$differing"; exit 1; }
