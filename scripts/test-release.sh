#!/usr/bin/env bash
# Tests scripts/release.sh against a scratch stack and a scratch SolidSyslog: a
# Baseline and two stages on wip/next, a sweep of them, and a library whose
# v0.2.0 tag differs from the commit swept only in its documentation.
#
#   scripts/test-release.sh
set -uo pipefail

TOOLS="$(cd "$(dirname "$0")" && pwd)"
SCRATCH="$(mktemp -d)"
trap 'rm -rf "$SCRATCH"' EXIT
export GIT_AUTHOR_NAME=Test GIT_AUTHOR_EMAIL=test@example.invalid
export GIT_COMMITTER_NAME=Test GIT_COMMITTER_EMAIL=test@example.invalid
export STACK=solid-syslog-example

CROSS=ghcr.io/cososo-ltd/cpputest-freertos-cross
DIGEST="sha256:$(printf 'a%.0s' {1..64})"
OTHER_DIGEST="sha256:$(printf 'b%.0s' {1..64})"
IMAGE="$CROSS:sha-0000000@$DIGEST"
LINK="https://github.com/cososo-ltd/$STACK/tree/release"

failures=0
check() { # check <what> <command...>
    if "${@:2}" >/dev/null 2>&1; then echo "ok    $1"; else echo "FAIL  $1"; failures=$((failures + 1)); fi
}
value() { sed -n "s/^$1=//p" "$SCRATCH/out"; }
refused() { grep -Fxq "refused=$1" "$SCRATCH/out"; }
# release <version> <tag-sha> [VAR=value...]
release() {
    rm -rf "$SCRATCH/made"
    (cd "$SCRATCH/stack" && git fetch -q --prune origin && env "${@:3}" bash "$TOOLS/release.sh" "$1" "$2" \
        "$SCRATCH/sweep" "$SCRATCH/run" "$SCRATCH/library" "$SCRATCH/made") > "$SCRATCH/out" 2> "$SCRATCH/err"
}
run_is() { printf 'id=4242\nevent=%s\nconclusion=%s\ntitle=%s\n' "$1" "$2" "$3" > "$SCRATCH/run"; }

# ---- the scratch SolidSyslog -------------------------------------------------
git init -q "$SCRATCH/library" && cd "$SCRATCH/library" || exit 1
mkdir -p .github/workflows Core docs
printf 'on:\n  push:\n    branches: [ main ]\n    paths:\n      - Core/**\n      - CMakeLists.txt\n      - .github/workflows/ci.yml\n  workflow_dispatch:\n' > .github/workflows/example-sweep.yml
printf 'jobs:\n  cross:\n    container:\n      image: %s@%s # sha-0000000\n' "$CROSS" "$DIGEST" > .github/workflows/ci.yml
echo "int core;" > Core/Core.c
echo "project(SolidSyslog)" > CMakeLists.txt
echo "See [the example]($LINK/0.1.0)." > README.md
echo "Stage by stage, on [the example]($LINK/0.1.0)." > docs/hardening-path.md
git add -A && git commit -q -m "feat: the library the stack was swept at"
PIN="$(git rev-parse HEAD)"
sed -i "s#release/0.1.0#release/0.2.0#" README.md docs/hardening-path.md
echo "0.2.0" > version.txt
git add -A && git commit -q -m "chore: release 0.2.0" && git tag v0.2.0
TAG="$(git rev-parse HEAD)"

# ---- the scratch stack -------------------------------------------------------
git init -q --bare "$SCRATCH/origin.git"
git init -q "$SCRATCH/seed" && cd "$SCRATCH/seed" || exit 1
printf '# Example\n\n## Releases\n\n| Branch | SolidSyslog | Pinned at |\n|---|---|---|\n| [`release/0.1.0`](%s/0.1.0) | 0.1.0 | `1111111` (`v0.1.0`) |\n\nRelease branches are permanent.\n' "$LINK" > README.md
git add -A && git commit -q -m "docs: this page"
git push -q "$SCRATCH/origin.git" HEAD:refs/heads/main
git checkout -q --orphan stages && git rm -q -rf .
mkdir -p docker .devcontainer measurements app
echo "$PIN" > solid-syslog.pin
printf 'x-cross-image: &cross-image %s\nservices:\n  run:\n    image: *cross-image\n' "$IMAGE" > docker/docker-compose.yml
printf 'services:\n  dev:\n    image: %s\n' "$IMAGE" > .devcontainer/docker-compose.yml
printf '# slug\tname\tgives\nBaseline\tBaseline\ta device\n' > measurements/stages.tsv
echo "int main(void) { return 0; }" > app/main.c
git add -A
git commit -q -m "Baseline: a working product

solid-syslog.pin names the SolidSyslog commit the stages build against:
${PIN:0:8}, a commit on SolidSyslog's main.

  Flash  1 B
  RAM    1 B"
stage() { # stage <slug> <figures block>
    printf '%s\t%s\t%s\n' "$1" "$1" "what $1 gives" >> measurements/stages.tsv
    echo "/* $1 */" >> app/main.c
    git add -A && git commit -q -m "feat: $1

$2"
}
stage linked "  Flash  +1 B  (+1 on the previous stage)
  RAM    +1 B  (+1)"
stage logger "  Flash  +1 B  (+1 on the previous stage)
  RAM    +1 B  (+1)

The ring costs 512 bytes of that."
git push -q "$SCRATCH/origin.git" HEAD:refs/heads/wip/next
git clone -q "$SCRATCH/origin.git" "$SCRATCH/stack" 2>/dev/null
cd "$SCRATCH/stack" || exit 1

# ---- the sweep of it ----------------------------------------------------------
sweep_of() { # sweep_of <ref>: the output of a sweep that passed every stage of it
    rm -rf "$SCRATCH/sweep" && mkdir -p "$SCRATCH/sweep/reports"
    echo "index,slug,commit,verdict" > "$SCRATCH/sweep/stages.csv"
    echo "index,slug,commit,key,current,baseline,above_baseline" > "$SCRATCH/sweep/figures.csv"
    local k=0 c slug
    for c in $(git rev-list --reverse "$1"); do
        slug="$(git show "$c:measurements/stages.tsv" | tail -1 | cut -f1)"
        echo "$k,$slug,$c,\"PASS\"" >> "$SCRATCH/sweep/stages.csv"
        for key in flash_text flash_data static_bss mbedtls_peak stack_log stack_service; do
            echo "$k,$slug,$c,$key,$((1000 + 100 * k)),1000,$((100 * k))" >> "$SCRATCH/sweep/figures.csv"
        done
        echo "report of $slug" > "$SCRATCH/sweep/reports/$(printf '%02d' "$k")-$slug.md"
        k=$((k + 1))
    done
    echo "the table" > "$SCRATCH/sweep/figures.md"
}
sweep_of origin/wip/next
run_is repository_dispatch success "Sweep wip/next at library $PIN"
tip_show() { git -C "$SCRATCH/stack" show "$(value tip):$1"; }
message() { git -C "$SCRATCH/stack" log -1 --format=%B "$1"; }
baseline() { git -C "$SCRATCH/stack" rev-list --max-parents=0 "$(value tip)"; }

# ---- the release --------------------------------------------------------------
release 0.2.0 "$TAG"
check "cut: succeeds"                              test $? -eq 0
check "cut: nothing is refused"                    test -z "$(grep refuse "$SCRATCH/out")"
check "cut: the Baseline, the stages and evidence" test "$(git rev-list --count "$(value tip)")" -eq 4
check "cut: names the commit swept"                test "$(value pin)" = "$PIN"
check "cut: the pin is the tag"                    test "$(tip_show solid-syslog.pin)" = "$TAG"
check "cut: the pin line is the release's"         grep -Fxq "v0.2.0, ${TAG:0:8}." <(message "$(baseline)")
check "cut: the Baseline's figures are the run's"  grep -Eq '^  Flash +2,000 B$' <(message "$(baseline)")
check "cut: a stage's figures are the run's"       grep -Eq '^  Flash +\+400 B +\(\+200 on the previous stage\)$' <(message "$(value tip)~1")
check "cut: the stages are kept"                   test "$(tip_show app/main.c | tail -1)" = "/* logger */"
check "cut: the tip adds only evidence"            test -z "$(git diff --name-only "$(value tip)~1" "$(value tip)" | grep -v '^evidence/')"
check "cut: the evidence is the run's output"      test "$(git ls-tree -r --name-only "$(value tip)" evidence | wc -l)" -eq "$(find "$SCRATCH/sweep" -type f | wc -l)"
check "cut: figures.csv is unedited"               cmp -s "$SCRATCH/sweep/figures.csv" <(tip_show evidence/figures.csv)
check "cut: every report is there"                 test "$(git ls-tree -r --name-only "$(value tip)" evidence/reports | wc -l)" -eq 3
check "cut: the evidence names the run"            grep -Fq "Sweep run 4242" <(message "$(value tip)")
check "cut: the evidence names the commit swept"   grep -Fq "pinned to ${PIN:0:8}" <(message "$(value tip)")
check "cut: prose quoting a figure is listed"      grep -Fq "512 bytes" "$SCRATCH/made/prose.txt"
check "cut: the notes name the branch"             grep -Fq "$LINK/0.2.0" "$SCRATCH/made/notes.md"
check "cut: the notes name the run"                grep -Fq "Sweep run 4242, 3/3 pass" "$SCRATCH/made/notes.md"
check "cut: the README gains the row"              grep -Fxq "| [\`release/0.2.0\`]($LINK/0.2.0) | 0.2.0 | \`${TAG:0:7}\` (\`v0.2.0\`) |" "$SCRATCH/made/README.md"
check "cut: the row follows the last release"      test "$(grep -n 'release/0.2.0' "$SCRATCH/made/README.md" | cut -d: -f1)" -eq 8
check "cut: the rest of the README stands"         test "$(wc -l < "$SCRATCH/made/README.md")" -eq 10
STACK="$STACK" bash "$TOOLS/readme-row.sh" "$SCRATCH/made/README.md" 0.2.0 "$TAG"
check "cut: the row is not added twice"            test "$(grep -c 'release/0.2.0' "$SCRATCH/made/README.md")" -eq 1
check "cut: no ref moved"                          test "$(git rev-parse origin/wip/next)" = "$(git ls-remote origin refs/heads/wip/next | cut -f1)"
check "cut: no release branch was made"            test -z "$(git ls-remote origin 'refs/heads/release/*')"
cut="$(value tip)"

# ---- swept at the tag itself --------------------------------------------------
git -C "$SCRATCH/library" tag v0.1.5 "$PIN"
release 0.1.5 "$PIN" DRY_RUN=1
check "at the tag: only its links would refuse it" test "$(grep refuse "$SCRATCH/out")" = "would-refuse=links"
check "at the tag: the stages are not rebased"     test "$(git rev-parse "$(value tip)~1^{tree}")" = "$(git rev-parse origin/wip/next^{tree})"
check "at the tag: the evidence says so"           grep -Fq "The trees are identical." <(message "$(value tip)")

# ---- a release that is already cut --------------------------------------------
git push -q origin "$cut:refs/heads/release/0.2.0"
release 0.2.0 "$TAG"
check "exists: exits 2"                            test $? -eq 2
check "exists: says so"                            refused exists
check "exists: builds nothing"                     test -z "$(value tip)"
check "exists: the branch is as it was"            test "$(git ls-remote origin refs/heads/release/0.2.0 | cut -f1)" = "$cut"
release 0.2.0 "$TAG" DRY_RUN=1
check "dry run: succeeds"                          test $? -eq 0
check "dry run: says what would refuse it"         grep -Fxq "would-refuse=exists" "$SCRATCH/out"
check "dry run: builds the branch anyway"          test -n "$(value tip)"
git push -q origin :refs/heads/release/0.2.0
git push -q origin origin/wip/next:refs/tags/v0.2.0
release 0.2.0 "$TAG"
check "exists: a tag of the version alone is refused" refused exists
git push -q origin :refs/tags/v0.2.0 && git tag -d v0.2.0 >/dev/null

# ---- a run that is not the one ------------------------------------------------
run_is workflow_dispatch success "Sweep wip/next at library $PIN"
release 0.2.0 "$TAG"
check "run: one started by hand is refused"        refused run
run_is repository_dispatch failure "Sweep wip/next at library $PIN"
release 0.2.0 "$TAG"
check "run: one that failed is refused"            refused run
run_is repository_dispatch success "Sweep wip/next at library $TAG"
release 0.2.0 "$TAG"
check "run: one at another commit is refused"      refused run
run_is repository_dispatch success "Sweep wip/next at library $PIN"

sed -i '3s/"PASS"/"FAIL (exit 1)"/' "$SCRATCH/sweep/stages.csv"
release 0.2.0 "$TAG"
check "stages: a stage that failed is refused"     refused stages
check "stages: and named"                          grep -Fq "01-linked" "$SCRATCH/err"
sweep_of origin/wip/next

sed -i 's/^0,Baseline,\([0-9a-f]*\),flash_text,1000,1000,0$/0,Baseline,\1,flash_text,1042,1000,42/' "$SCRATCH/sweep/figures.csv"
release 0.2.0 "$TAG"
check "baseline: one that grew since its capture is refused" refused baseline
check "baseline: and by how much"                  grep -Fq "flash_text +42 B" "$SCRATCH/err"
sweep_of origin/wip/next

git checkout -q -B moved origin/wip/next
git commit -q --amend -m "feat: logger, reworded"
git push -q --force origin HEAD:refs/heads/wip/next
git checkout -q --detach
release 0.2.0 "$TAG" DRY_RUN=1
check "stages: a wip/next that moved exits 2"      test $? -eq 2
check "stages: even on a dry run"                  refused stages
git fetch -q origin && sweep_of origin/wip/next

# ---- a tag that is not the one ------------------------------------------------
release 0.2.0 "$PIN"
check "tag: a commit the tag does not name is refused" refused tag
release 0.3.0 "$TAG"
check "tag: a version with no tag is refused"      refused tag

# ---- a library that changed after the sweep -----------------------------------
library_tag() { # library_tag <version> <message>: commit what is staged and tag it
    git -C "$SCRATCH/library" commit -q -m "$2" && git -C "$SCRATCH/library" tag "v$1"
    git -C "$SCRATCH/library" rev-parse HEAD
}
cd "$SCRATCH/library" || exit 1
pages() { sed -i "s#release/[0-9.]*[0-9]#release/$1#" "${@:2}"; }
pages 0.2.1 README.md docs/hardening-path.md
echo "int more;" >> Core/Core.c && git add -A
changed="$(library_tag 0.2.1 "fix: a change to Core")"
git checkout -q v0.2.0 -- Core
pages 0.2.2 README.md docs/hardening-path.md
sed -i "s#$DIGEST#$OTHER_DIGEST#" .github/workflows/ci.yml && git add -A
moved="$(library_tag 0.2.2 "chore: bump the cross image")"
git checkout -q v0.2.0 -- .github
pages 0.2.3 README.md && git add -A
stale="$(library_tag 0.2.3 "docs: one page brought to the release")"
pages 0.2.3 docs/hardening-path.md && git add -A
behind="$(library_tag 0.2.4 "docs: both pages, a release behind")"
cd "$SCRATCH/stack" || exit 1

release 0.2.1 "$changed"
check "library: a change to Core is refused"       refused library
check "library: and nothing else is"               test "$(grep -c '^refused=' "$SCRATCH/out")" -eq 1
check "library: and the file named"                grep -Fq "Core/Core.c" "$SCRATCH/err"
release 0.2.2 "$moved"
check "library: a change to ci.yml is refused"     refused library
check "image: another image is refused"            refused image
release 0.2.3 "$stale"
check "links: a page left on another release is refused" refused links
check "links: and nothing else is"                 test "$(grep -c '^refused=' "$SCRATCH/out")" -eq 1
release 0.2.4 "$behind"
check "links: pages that name the release before are refused" refused links

# ---- arguments ----------------------------------------------------------------
release v0.2.0 "$TAG"
check "args: a version with a prefix is refused"   test $? -eq 1
release 0.2.0 "${TAG:0:7}"
check "args: an abbreviated commit is refused"     test $? -eq 1

[ "$failures" -eq 0 ] || { echo "$failures failed"; cat "$SCRATCH/err"; exit 1; }
echo "all passed"
