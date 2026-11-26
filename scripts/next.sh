#!/usr/bin/env bash
# Rebase the stack's stages onto a Baseline pinned at a library commit.
#
#   scripts/next.sh <library-sha> <image>
#
# The stages are those of origin/wip/next where it exists, and otherwise those of
# the newest origin/release/<version> without its evidence commit. The Baseline is
# the root commit and the only one that names the library commit and the cross
# image, so it is written again with solid-syslog.pin at <library-sha> and both
# compose files on <image>, and every commit after it is rebased onto the result.
#
# Every stage's cost is counted from the Baseline's measurements/Baseline.csv, so
# where the Baseline changes in anything but the pin, scripts/capture.sh runs it
# and the file is written again in the same commit.
#
# STAGES_FROM=<commit> takes the stages from that commit in place of either
# branch. PIN_LABEL=<name> writes the Baseline's pin line as a release's,
# "<name>, <short sha>.", in place of a commit on main's.
#
# Moves no ref and pushes nothing. Prints
#   source=<the ref the stages came from>
#   was=<its tip>
#   baseline=<each figure a capture moved, as "<key> <was> -> <now>">
# and then, on success, tip=<the rebased tip>, which the caller decides what to
# do with. Where a stage does not apply it prints conflict=<NN-slug> and
# commit=<the stage commit> instead, and exits 2. Where the Baseline did not run
# it prints capture=failed and exits 3. Any other failure exits 1.
set -euo pipefail

SHA="${1:?usage: scripts/next.sh <library-sha> <image>}"
IMAGE="${2:?usage: scripts/next.sh <library-sha> <image>}"
TOOLS="$(cd "$(dirname "$0")" && pwd)"
STAGES=measurements/stages.tsv
BASELINE=measurements/Baseline.csv
CROSS='ghcr\.io/cososo-ltd/cpputest-freertos-cross'
PIN_LINE='solid-syslog.pin names the SolidSyslog commit the stages build against:'

fail() { echo "next: $*" >&2; exit 1; }
note() { echo "next: $*" >&2; }

[[ "$SHA" =~ ^[0-9a-f]{40}$ ]] || fail "'$SHA' is not a full commit SHA"
[[ "$IMAGE" =~ ^${CROSS}(:[A-Za-z0-9._-]+)?@sha256:[0-9a-f]{64}$ ]] || fail "'$IMAGE' is not the cross image by digest"

# ---- choose the stages -------------------------------------------------------
if [ -n "${STAGES_FROM:-}" ]; then
    source="$STAGES_FROM"
    git rev-parse -q --verify "$source^{commit}" >/dev/null || fail "'$source' is not a commit"
elif git rev-parse -q --verify origin/wip/next^{commit} >/dev/null; then
    source=origin/wip/next
else
    source="$(git for-each-ref --format='%(refname:short)' 'refs/remotes/origin/release/*' | sort -V | tail -1)"
    [ -n "$source" ] || fail "no origin/wip/next and no origin/release/ branch"
fi
was="$(git rev-parse "$source^{commit}")"
tip="$was"
[ -z "$(git rev-list --merges "$tip")" ] || fail "$source is not linear"
if [ -z "$(git diff-tree --root --no-commit-id --name-only -r "$tip" | grep -v '^evidence/' || true)" ]; then
    tip="$(git rev-parse "$tip^")"
fi
root="$(git rev-list --max-parents=0 "$tip")"
[ "$(echo "$root" | wc -l)" -eq 1 ] || fail "$source has more than one root"
count="$(git rev-list --count "$tip")"
note "$count commits of $source, Baseline $(git log -1 --format=%h "$root")"
printf 'source=%s\nwas=%s\n' "$source" "$was"

WORK="$(mktemp -d)"
cleanup() {
    for wt in "$WORK"/*/; do
        git worktree remove --force "$wt" >/dev/null 2>&1 || true
    done
    rm -rf "$WORK"
    git worktree prune
}
trap 'rc=$?; cleanup || true; exit "$rc"' EXIT

# ---- write the Baseline ------------------------------------------------------
base="$WORK/baseline"
git worktree add -q --detach "$base" "$root"
[ -f "$base/solid-syslog.pin" ] || fail "the Baseline has no solid-syslog.pin"
printf '%s\n' "$SHA" > "$base/solid-syslog.pin"
for compose in docker/docker-compose.yml .devcontainer/docker-compose.yml; do
    grep -Eq "$CROSS[:@]" "$base/$compose" || fail "the Baseline's $compose does not name the cross image"
    sed -E -i "s#$CROSS[:@][^[:space:]]*#$IMAGE#" "$base/$compose"
done

# The line under PIN_LINE says which commit the pin is, up to the next blank line.
pin_line="${SHA:0:8}, a commit on SolidSyslog's main."
[ -z "${PIN_LABEL:-}" ] || pin_line="$PIN_LABEL, ${SHA:0:8}."
git log -1 --format=%B "$root" > "$WORK/message.was"
grep -Fxq "$PIN_LINE" "$WORK/message.was" || fail "the Baseline's message has no pin line"
awk -v marker="$PIN_LINE" -v line="$pin_line" '
    replacing && NF == 0 { replacing = 0 }
    replacing            { next }
                         { print }
    $0 == marker         { print line; replacing = 1 }
' "$WORK/message.was" > "$WORK/message"

git -C "$base" add -A
# The pin does not reach the Baseline, which has no SolidSyslog in it.
if [ -n "$(git -C "$base" diff --cached --name-only | grep -Fxv solid-syslog.pin || true)" ]; then
    note "the Baseline is another build, so $BASELINE is captured again"
    if ! bash "${CAPTURE:-$TOOLS/capture.sh}" "$base" >&2; then
        echo "capture=failed"
        note "the Baseline did not run"
        exit 3
    fi
    moved="$(git show "$root:$BASELINE" | awk -F, '
        /^#/ || NF != 2 { next }
        FNR == NR       { was[$1] = $2; next }
        $2 != was[$1]   { printf "%s%s %s -> %s", (n++ ? ", " : ""), $1, was[$1], $2 }
    ' - "$base/$BASELINE")"
    [ -z "$moved" ] || echo "baseline=$moved"
    git -C "$base" add -A
fi
if git -C "$base" diff --cached --quiet && cmp -s "$WORK/message.was" "$WORK/message"; then
    new="$root"
    note "the Baseline is already at ${SHA:0:8} on this image"
else
    new="$(GIT_AUTHOR_NAME="$(git log -1 --format=%an "$root")" \
           GIT_AUTHOR_EMAIL="$(git log -1 --format=%ae "$root")" \
           GIT_AUTHOR_DATE="$(git log -1 --format=%aI "$root")" \
           git commit-tree "$(git -C "$base" write-tree)" -F "$WORK/message")"
fi

# ---- rebase the stages onto it -----------------------------------------------
if [ "$new" != "$root" ] && [ "$tip" != "$root" ]; then
    stages="$WORK/stages"
    git worktree add -q --detach "$stages" "$tip"
    if ! git -C "$stages" rebase --onto "$new" "$root" >"$WORK/rebase.log" 2>&1; then
        stopped="$(git -C "$stages" rev-parse -q --verify REBASE_HEAD || true)"
        [ -n "$stopped" ] || { cat "$WORK/rebase.log" >&2; fail "the rebase failed before any stage"; }
        slug="$(git show "$stopped:$STAGES" | awk -F'\t' '!/^[[:space:]]*#/ && NF { s = $1 } END { print s }')"
        printf 'conflict=%02d-%s\ncommit=%s\n' "$(git rev-list --count "$root..$stopped")" "$slug" "$stopped"
        note "$(git log -1 --format='%h %s' "$stopped") does not apply"
        exit 2
    fi
    tip="$(git -C "$stages" rev-parse HEAD)"
elif [ "$new" != "$root" ]; then
    tip="$new"
fi

# A stage the rebase emptied or dropped would shift every row after it.
[ "$(git rev-list --count "$tip")" -eq "$count" ] || fail "the rebase kept $(git rev-list --count "$tip") of $count commits"
MAP_ONLY=1 bash "$TOOLS/sweep.sh" "$tip" >/dev/null || fail "the rebased stages no longer match $STAGES"

printf 'tip=%s\n' "$tip"
