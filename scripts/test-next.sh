#!/usr/bin/env bash
# Tests scripts/next.sh against a scratch stack: a Baseline and two stages on
# release/0.1.0 with an evidence commit at its tip.
#
#   scripts/test-next.sh
set -uo pipefail

TOOLS="$(cd "$(dirname "$0")" && pwd)"
SCRATCH="$(mktemp -d)"
trap 'rm -rf "$SCRATCH"' EXIT
export GIT_AUTHOR_NAME=Test GIT_AUTHOR_EMAIL=test@example.invalid
export GIT_COMMITTER_NAME=Test GIT_COMMITTER_EMAIL=test@example.invalid

OLD_SHA=1111111111111111111111111111111111111111
NEW_SHA=2222222222222222222222222222222222222222
NEWER_SHA=3333333333333333333333333333333333333333
CROSS=ghcr.io/cososo-ltd/cpputest-freertos-cross
OLD_IMAGE="$CROSS:sha-0000000@sha256:$(printf 'a%.0s' {1..64})"
NEW_IMAGE="$CROSS@sha256:$(printf 'b%.0s' {1..64})"

failures=0
check() { # check <what> <command...>
    if "${@:2}" >/dev/null 2>&1; then echo "ok    $1"; else echo "FAIL  $1"; failures=$((failures + 1)); fi
}
value() { sed -n "s/^$1=//p" "$SCRATCH/out"; }
run_next() { (cd "$SCRATCH/stack" && git fetch -q --prune origin && bash "$TOOLS/next.sh" "$@") > "$SCRATCH/out" 2> "$SCRATCH/err"; }

stage() { # stage <slug> <file> <text>
    printf '%s\t%s\t%s\n' "$1" "$1" "what $1 gives" >> measurements/stages.tsv
    echo "$3" >> "$2"
    git add -A && git commit -q -m "feat: $1"
}

# ---- the scratch stack -------------------------------------------------------
git init -q --bare "$SCRATCH/origin.git"
git init -q "$SCRATCH/seed" && cd "$SCRATCH/seed" || exit 1
mkdir -p docker .devcontainer measurements app
echo "$OLD_SHA" > solid-syslog.pin
printf 'x-cross-image: &cross-image %s\nservices:\n  run:\n    image: *cross-image\n' "$OLD_IMAGE" > docker/docker-compose.yml
printf 'services:\n  dev:\n    image: %s\n' "$OLD_IMAGE" > .devcontainer/docker-compose.yml
printf '# slug\tname\tgives\nBaseline\tBaseline\ta device\n' > measurements/stages.tsv
echo "int main(void) { return 0; }" > app/main.c
git add -A
git commit -q -m "Baseline: a working product

solid-syslog.pin names the SolidSyslog commit the stages build against:
v0.1.0, 11111111.

  Flash  1 B"
stage linked app/main.c "/* linked */"
stage logger app/main.c "/* logger */"
mkdir evidence && echo figures > evidence/figures.csv
git add -A && git commit -q -m "evidence: the sweep"
git push -q "$SCRATCH/origin.git" HEAD:refs/heads/release/0.1.0
git clone -q "$SCRATCH/origin.git" "$SCRATCH/stack" 2>/dev/null
cd "$SCRATCH/stack" || exit 1
in_tip() { git -C "$SCRATCH/stack" show "$(value tip):$1"; }
root_of() { git -C "$SCRATCH/stack" rev-list --max-parents=0 "$1"; }

# ---- seeded from the release branch ------------------------------------------
run_next "$NEW_SHA" "$NEW_IMAGE"
check "seed: succeeds"                         test $? -eq 0
check "seed: the stages come from the release" test "$(value source)" = origin/release/0.1.0
check "seed: the evidence commit is dropped"   test "$(git rev-list --count "$(value tip)")" -eq 3
check "seed: no evidence in the tree"          test -z "$(git ls-tree -r --name-only "$(value tip)" evidence)"
check "seed: the pin is the commit"            test "$(in_tip solid-syslog.pin)" = "$NEW_SHA"
check "seed: the stack's image moved"          grep -Fq "&cross-image $NEW_IMAGE" <(in_tip docker/docker-compose.yml)
check "seed: the devcontainer's image moved"   grep -Fq "image: $NEW_IMAGE" <(in_tip .devcontainer/docker-compose.yml)
check "seed: the old image is gone"            test -z "$(git grep -l "$OLD_IMAGE" "$(value tip)")"
check "seed: the message names the commit"     grep -Fxq "22222222, a commit on SolidSyslog's main." <(git log -1 --format=%B "$(root_of "$(value tip)")")
check "seed: the old pin line is gone"         test -z "$(git log -1 --format=%B "$(root_of "$(value tip)")" | grep -F v0.1.0)"
check "seed: the rest of the message stands"   grep -Fxq "  Flash  1 B" <(git log -1 --format=%B "$(root_of "$(value tip)")")
check "seed: the stages are kept"              test "$(in_tip app/main.c | tail -1)" = "/* logger */"
check "seed: the release branch is untouched"  test "$(git rev-parse origin/release/0.1.0)" = "$(value was)"
git push -q origin "$(value tip):refs/heads/wip/next"

# ---- a hand fix on wip/next is what the next rebase starts from ---------------
git checkout -q -B fix origin/wip/next
echo "/* fixed by hand */" >> app/main.c
git commit -q -am "fixup" && git reset -q --soft HEAD~2 && git commit -q -m "feat: logger"
git push -q --force origin HEAD:refs/heads/wip/next
git checkout -q --detach
run_next "$NEWER_SHA" "$NEW_IMAGE"
check "fix: succeeds"                          test $? -eq 0
check "fix: the stages come from wip/next"     test "$(value source)" = origin/wip/next
check "fix: the pin is the newer commit"       test "$(in_tip solid-syslog.pin)" = "$NEWER_SHA"
check "fix: the hand fix survives"             test "$(in_tip app/main.c | tail -1)" = "/* fixed by hand */"
check "fix: still one commit per stage"        test "$(git rev-list --count "$(value tip)")" -eq 3
git push -q --force origin "$(value tip):refs/heads/wip/next"

# ---- nothing to do ------------------------------------------------------------
run_next "$NEWER_SHA" "$NEW_IMAGE"
check "same: succeeds"                         test $? -eq 0
check "same: the tip does not move"            test "$(value tip)" = "$(value was)"

# ---- a stage that does not apply ----------------------------------------------
git checkout -q -B clash origin/wip/next
printf 'clash\tclash\twhat clash gives\n' >> measurements/stages.tsv
sed -i 's#cpputest-freertos-cross@#cpputest-freertos-cross:clash@#' docker/docker-compose.yml
git commit -q -am "feat: clash"
clash="$(git rev-parse HEAD)"
git push -q origin HEAD:refs/heads/wip/next
git checkout -q --detach
run_next "$NEW_SHA" "$OLD_IMAGE"
check "clash: exits 2"                         test $? -eq 2
check "clash: names the stage"                 test "$(value conflict)" = 03-clash
check "clash: names the commit"                test "$(value commit)" = "$clash"
check "clash: names the source"                test "$(value source)" = origin/wip/next
check "clash: reports no tip"                  test -z "$(value tip)"
check "clash: leaves no worktree behind"       test "$(git worktree list | wc -l)" -eq 1

# ---- arguments ----------------------------------------------------------------
run_next 2222222 "$NEW_IMAGE"
check "args: an abbreviated commit is refused" test $? -eq 1
run_next "$NEW_SHA" "$CROSS:sha-0000000"
check "args: an image without a digest is refused" test $? -eq 1

[ "$failures" -eq 0 ] || { echo "$failures failed"; cat "$SCRATCH/err"; exit 1; }
echo "all passed"
