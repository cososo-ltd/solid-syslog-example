#!/usr/bin/env bash
# Build a release branch from wip/next and the sweep run that measured it.
#
#   scripts/release.sh <version> <tag-sha> <sweep-out> <run-facts> <library> <out-dir>
#
#   <tag-sha>    the commit SolidSyslog's v<version> tag names
#   <sweep-out>  the output of the Sweep run, as its artifact holds it
#   <run-facts>  that run, as lines of id=, event=, conclusion= and title=
#   <library>    a clone of solid-syslog holding <tag-sha> and the commit swept
#
# The stages are origin/wip/next's, commit for commit the ones the run measured.
# Each stage's message takes its figures from the run, the stages are rebased onto
# a Baseline pinned at <tag-sha>, and the run's output is committed at the tip
# under evidence/. Nothing is measured here: the figures are the run's, and they
# hold at the tag because nothing that reaches this stack differs between the
# commit the run measured and the tag.
#
# Moves no ref and pushes nothing. Writes notes.md, README.md and prose.txt to
# <out-dir>: the Release's description, main's README with the release's row, and
# the lines of stage messages that quote a figure outside their figures block.
# Prints tip=<the evidence commit>, pin=<the commit the run measured> and
# stages=<how many>.
#
# A release that must not be cut prints refused=<why> for each reason and exits 2:
#   exists   origin/release/<version> or this stack's v<version> tag is already there
#   run      the run is not a green sweep the chain started at wip/next's pin
#   stages   the run did not measure wip/next as it stands, or a stage failed
#   baseline the Baseline measured above measurements/Baseline.csv, so that file
#            was captured from another Baseline and every stage is charged for it
#   tag      SolidSyslog's v<version> is not <tag-sha>
#   library  a path that reaches this stack differs between the pin and the tag
#   image    wip/next is not on the image SolidSyslog's CI pins at the tag
#   links    SolidSyslog's documentation at the tag does not link release/<version>
# DRY_RUN=1 prints would-refuse=<why> in their place and builds the branch
# anyway, except where the run did not measure wip/next. Any other failure exits 1.
set -euo pipefail

USAGE="usage: scripts/release.sh <version> <tag-sha> <sweep-out> <run-facts> <library> <out-dir>"
VERSION="${1:?$USAGE}"
TAG="${2:?$USAGE}"
SWEEP="${3:?$USAGE}"
FACTS="${4:?$USAGE}"
LIBRARY="${5:?$USAGE}"
OUT="${6:?$USAGE}"
TOOLS="$(cd "$(dirname "$0")" && pwd)"
STACK="${STACK:-solid-syslog-example}"
BUILT_WITH=CMake
CROSS='ghcr\.io/cososo-ltd/cpputest-freertos-cross'
SWEEP_WORKFLOW=.github/workflows/example-sweep.yml
PYTHON="${PYTHON:-$(for p in python3 python; do "$p" -c "import sys; assert sys.version_info >= (3, 8)" 2>/dev/null && { echo "$p"; break; }; done)}"

fail() { echo "release: $*" >&2; exit 1; }
note() { echo "release: $*" >&2; }
refusals=0
refuse() { # refuse <why> <message>
    refusals=$((refusals + 1))
    if [ "${DRY_RUN:-0}" = 1 ]; then echo "would-refuse=$1"; else echo "refused=$1"; fi
    note "$2"
}
library() { git -C "$LIBRARY" "$@"; }
fact() { sed -n "s/^$1=//p" "$FACTS"; }

[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || fail "'$VERSION' is not a version"
[[ "$TAG" =~ ^[0-9a-f]{40}$ ]] || fail "'$TAG' is not a full commit SHA"
[ -f "$SWEEP/stages.csv" ] && [ -f "$SWEEP/figures.csv" ] || fail "$SWEEP holds no sweep output"
[ -f "$FACTS" ] || fail "$FACTS is not a file"
library cat-file -e "$TAG^{commit}" 2>/dev/null || fail "$LIBRARY does not hold $TAG"
git rev-parse -q --verify origin/wip/next^{commit} >/dev/null || fail "there is no origin/wip/next"
SWEEP="$(cd "$SWEEP" && pwd)"
mkdir -p "$OUT" && OUT="$(cd "$OUT" && pwd)"

# ---- the run measured wip/next as it stands -----------------------------------
source="$(git rev-parse origin/wip/next^{commit})"
mapfile -t chain < <(git rev-list --first-parent --reverse "$source")
mapfile -t swept < <(awk -F, 'NR > 1 { print $3 }' "$SWEEP/stages.csv")
n=${#chain[@]}
if [ "${swept[*]}" != "${chain[*]}" ]; then
    echo "refused=stages"
    note "the run measured ${#swept[@]} commits ending at ${swept[${#swept[@]}-1]:-none}; wip/next is $n ending at $source"
    exit 2
fi
unpassed="$(awk -F, 'NR > 1 && $4 !~ /^"PASS/ { printf "%s%02d-%s", (c++ ? ", " : ""), $1, $2 }' "$SWEEP/stages.csv")"
[ -z "$unpassed" ] || refuse stages "the run did not pass $unpassed"

adrift="$(awk -F, 'NR > 1 && $1 == 0 && $4 ~ /^(flash_text|flash_data|static_bss)$/ && $7 != 0 { printf "%s%s %+d B", (c++ ? ", " : ""), $4, $7 }' "$SWEEP/figures.csv")"
[ -z "$adrift" ] || refuse baseline "the Baseline measured $adrift above measurements/Baseline.csv, which is not this Baseline's"

pin="$(git show "$source:solid-syslog.pin" | tr -d '[:space:]')"
[[ "$pin" =~ ^[0-9a-f]{40}$ ]] || fail "wip/next's solid-syslog.pin is not a full commit SHA"
library cat-file -e "$pin^{commit}" 2>/dev/null || fail "$LIBRARY does not hold $pin, the commit wip/next pins"

run="$(fact id)"
[[ "$run" =~ ^[0-9]+$ ]] || fail "$FACTS names no run"
if [ "$(fact event)" != repository_dispatch ] || [ "$(fact conclusion)" != success ]; then
    refuse run "run $run is a $(fact event) that ended in $(fact conclusion), not a green sweep the chain started"
elif [ "$(fact title)" != "Sweep wip/next at library $pin" ]; then
    refuse run "run $run is '$(fact title)', and wip/next pins $pin"
fi

# ---- the release is the one asked for, and not yet cut -----------------------
if git rev-parse -q --verify "origin/release/$VERSION^{commit}" >/dev/null; then
    refuse exists "release/$VERSION is already at $(git rev-parse --short "origin/release/$VERSION")"
fi
if git rev-parse -q --verify "refs/tags/v$VERSION^{commit}" >/dev/null; then
    refuse exists "this stack's v$VERSION tag is already at $(git rev-parse --short "refs/tags/v$VERSION^{commit}")"
fi
tagged="$(library rev-parse -q --verify "refs/tags/v$VERSION^{commit}" || true)"
if [ "$tagged" != "$TAG" ]; then
    refuse tag "SolidSyslog's v$VERSION is ${tagged:-not a tag}, not $TAG"
fi

# ---- the figures hold at the tag ------------------------------------------------
mapfile -t reaches < <(library show "$TAG:$SWEEP_WORKFLOW" | awk '
    /^[[:space:]]+paths:[[:space:]]*$/ { listing = 1; next }
    listing && /^[[:space:]]+-[[:space:]]/ { sub(/^[[:space:]]+-[[:space:]]+/, ""); gsub(/["\047]|[[:space:]]+$/, ""); print ":(glob)" $0; next }
    listing { exit }')
[ "${#reaches[@]}" -gt 0 ] || fail "$SWEEP_WORKFLOW at $TAG names no paths"
changed="$(library diff --name-only "$pin" "$TAG" -- "${reaches[@]}")"
if [ -n "$changed" ]; then
    refuse library "between ${pin:0:8} and the tag SolidSyslog changed $(echo "$changed" | paste -sd' ' -)"
fi

root="$(git rev-list --max-parents=0 "$source")"
images="$(git show "$root:docker/docker-compose.yml" | grep -Eo "$CROSS[:@][^[:space:]]*" | sort -u)"
[ "$(echo "$images" | grep -c .)" -eq 1 ] || fail "wip/next's Baseline names the cross image as: $images"
tested="$(library show "$TAG:.github/workflows/ci.yml" | grep -Eo "$CROSS@sha256:[0-9a-f]{64}" | sort -u)"
[ "$(echo "$tested" | grep -c .)" -eq 1 ] || fail "SolidSyslog's ci.yml at $TAG pins the cross image as: $tested"
if [ "${images##*@}" != "${tested##*@}" ]; then
    refuse image "wip/next is on ${images##*@}, and SolidSyslog's CI at the tag pins ${tested##*@}"
fi

# ---- SolidSyslog's documentation at the tag links this release ----------------
branch_url="https://github.com/cososo-ltd/$STACK/tree/release"
linked="$(library grep -ohE "$branch_url/[0-9A-Za-z._-]+" "$TAG" -- README.md docs | sed "s#^$branch_url/##; s#[.]\$##" | sort -u | paste -sd' ' - || true)"
if [ "$linked" != "$VERSION" ]; then
    refuse links "SolidSyslog's documentation at $TAG links release/ ${linked:-nothing}, not $VERSION alone"
fi

if [ "$refusals" -gt 0 ] && [ "${DRY_RUN:-0}" != 1 ]; then
    exit 2
fi

# ---- the stages, with the run's figures, on a Baseline at the tag -------------
: > "$OUT/prose.txt"
amended="$("$PYTHON" "$TOOLS/figures.py" amend "$SWEEP" "$source" 2> "$OUT/amend.log")" || { cat "$OUT/amend.log" >&2; fail "the figures could not be written into the stage messages"; }
awk '/^[0-9a-f]+ / { stage = $0 } /^  check prose: / { sub(/^  check prose: /, ""); print stage ": " $0 }' "$OUT/amend.log" > "$OUT/prose.txt"
rm -f "$OUT/amend.log"

STAGES_FROM="$amended" PIN_LABEL="v$VERSION" bash "$TOOLS/next.sh" "$TAG" "$images" > "$OUT/next.out" || fail "the stages do not apply to a Baseline pinned at the tag"
branch="$(sed -n 's/^tip=//p' "$OUT/next.out")"
rm -f "$OUT/next.out"
mapfile -t released < <(git rev-list --first-parent --reverse "$branch")
[ "${#released[@]}" -eq "$n" ] || fail "the release has ${#released[@]} commits and the run measured $n"
for k in "${!chain[@]}"; do
    differing="$(git diff --name-only "${chain[$k]}" "${released[$k]}")"
    [ -z "$differing" ] || [ "$differing" = solid-syslog.pin ] || fail "stage $k differs from the one measured in $differing"
done

# ---- the evidence commit ------------------------------------------------------
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
if [ "$pin" = "$TAG" ]; then
    held="The run measured the chain as pushed to wip/next, pinned to ${TAG:0:8}, the
commit SolidSyslog's v$VERSION tag names, as this branch is. Row k's commit
column names the SHA of wip/next's k-th commit as it stood for the run; the
figures were then written into the commit messages. The trees are identical."
else
    held="The run measured the chain as pushed to wip/next, pinned to ${pin:0:8} on
SolidSyslog's main. This branch pins the Baseline to SolidSyslog's v$VERSION
tag, ${TAG:0:8}. Between the two SolidSyslog changed nothing under the paths
its example-sweep.yml names, which are the ones a change reaches this example
through, and its CI pins the same container image at both.
Row k's commit column names the SHA of wip/next's k-th commit as it stood
for the run; the figures were then written into the commit messages and the
chain rebased onto the new pin. Apart from the pin the trees are identical."
fi
cat > "$WORK/message" <<EOF
evidence: the $VERSION sweep, every stage measured in one run

The output of Sweep run $run, unedited: figures.md is the table,
figures.csv every key at every stage, and reports/ each stage's run report
and log. $n of $n stages pass.

$held
EOF

export GIT_INDEX_FILE="$WORK/index"
git read-tree "$branch"
while IFS= read -r -d '' file; do
    path="evidence/${file#"$SWEEP"/}"
    git update-index --add --cacheinfo "100644,$(git hash-object -w --path="$path" "$file"),$path"
done < <(find "$SWEEP" -type f -print0 | sort -z)
tip="$(git commit-tree "$(git write-tree)" -p "$branch" -F "$WORK/message")"
unset GIT_INDEX_FILE

# ---- what the Release says, and main's README with its row --------------------
if [ "$pin" = "$TAG" ]; then
    measured="The sweep measured the chain at the tag."
else
    measured="The sweep measured the chain at \`${pin:0:8}\` on SolidSyslog's \`main\`; the evidence commit states why the figures hold at the tag."
fi
cat > "$OUT/notes.md" <<EOF
The SolidSyslog $VERSION integration, built with $BUILT_WITH: the Baseline and one commit per stage on [\`release/$VERSION\`]($branch_url/$VERSION), pinned to SolidSyslog [\`v$VERSION\`](https://github.com/cososo-ltd/solid-syslog/tree/v$VERSION) (\`${TAG:0:8}\`).

\`sweep-$VERSION.zip\` is the output of the sweep that measured every stage in one run (Sweep run $run, $n/$n pass), also committed at the branch tip under \`evidence/\`. $measured
EOF

git show origin/main:README.md > "$OUT/README.md"
STACK="$STACK" bash "$TOOLS/readme-row.sh" "$OUT/README.md" "$VERSION" "$TAG"

printf 'tip=%s\npin=%s\nstages=%s\n' "$tip" "$pin" "$n"
