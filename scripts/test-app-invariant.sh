#!/usr/bin/env bash
# Tests scripts/app-invariant.sh against two scratch stacks sharing one app/.
#
#   scripts/test-app-invariant.sh
set -uo pipefail

TOOLS="$(cd "$(dirname "$0")" && pwd)"
SCRATCH="$(mktemp -d)"
trap 'rm -rf "$SCRATCH"' EXIT
export GIT_AUTHOR_NAME=Test GIT_AUTHOR_EMAIL=test@example.invalid
export GIT_COMMITTER_NAME=Test GIT_COMMITTER_EMAIL=test@example.invalid

failures=0
check() { # check <what> <command...>
    if "${@:2}" >/dev/null 2>&1; then echo "ok    $1"; else echo "FAIL  $1"; failures=$((failures + 1)); fi
}
compare() { bash "$TOOLS/app-invariant.sh" "$@" > "$SCRATCH/out" 2> "$SCRATCH/err"; }
named() { paste -sd, - < "$SCRATCH/out"; }

stage() { # stage <slug> <file> <text>
    printf '%s\t%s\t%s\n' "$1" "$1" "what $1 gives" >> measurements/stages.tsv
    mkdir -p "$(dirname "$2")" && echo "$3" >> "$2"
    git add -A && git commit -q -m "feat: $1"
}
stack() { # stack <branch> <build file>: a Baseline and two stages
    git checkout -q --orphan "$1" && git rm -rq --cached . 2>/dev/null; rm -rf ./*
    mkdir -p measurements app/net/smsc9220
    printf '# slug\tname\tgives\n' > measurements/stages.tsv
    echo "built by $2" > "$2"
    echo "built by $2" > app/net/smsc9220/KNOWN-LIMITATIONS.md
    stage Baseline app/main.c "int main(void) { return 0; }"
    stage linked app/main.c "/* linked */"
    stage logger app/log.c "/* logger */"
}

git init -q "$SCRATCH/stacks" && cd "$SCRATCH/stacks" || exit 1
stack cmake CMakeLists.txt
stack make Makefile

compare cmake make
check "same: exits 0"                          test $? -eq 0
check "same: names nothing"                    test -z "$(named)"

git checkout -q -b evidence make
mkdir evidence && echo figures > evidence/figures.csv && git add -A && git commit -q -m "evidence: the sweep"
compare cmake evidence
check "evidence: a commit that adds no row is not a stage" test $? -eq 0

git checkout -q -b edited make
echo "/* only here */" >> app/log.c && git commit -q -am "feat: logger" --amend
compare cmake edited
check "edited: exits 2"                        test $? -eq 2
check "edited: names the stage"                test "$(named)" = logger

git checkout -q -b longer make
stage tcp app/tcp.c "/* tcp */"
compare cmake longer
check "longer: exits 2"                        test $? -eq 2
check "longer: names the stage only one has"   test "$(named)" = tcp
compare longer cmake
check "longer: either way round"               test "$(named)" = tcp

compare cmake no-such-branch
check "unknown: a sibling that is no commit exits 1" test $? -eq 1
check "unknown: names nothing"                 test -z "$(named)"
compare no-such-branch no-other-branch
check "unknown: two refs that are no commit exit 1" test $? -eq 1

[ "$failures" -eq 0 ] || { echo "$failures failed"; cat "$SCRATCH/err"; exit 1; }
echo "all passed"
