#!/usr/bin/env bash
# Add a release's row to the Releases table of main's README.
#
#   STACK=<this repository's name> scripts/readme-row.sh <README> <version> <tag-sha>
#
# The row follows the last release's. A README that already names the release is
# left as it is.
set -euo pipefail

USAGE="usage: scripts/readme-row.sh <README> <version> <tag-sha>"
README="${1:?$USAGE}"
VERSION="${2:?$USAGE}"
TAG="${3:?$USAGE}"
STACK="${STACK:?STACK names this repository}"

fail() { echo "readme-row: $*" >&2; exit 1; }

[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || fail "'$VERSION' is not a version"
[[ "$TAG" =~ ^[0-9a-f]{40}$ ]] || fail "'$TAG' is not a full commit SHA"
grep -q '^| \[`release/' "$README" || fail "$README has no Releases table"
if grep -Fq "\`release/$VERSION\`" "$README"; then
    exit 0
fi

url="https://github.com/cososo-ltd/$STACK/tree/release/$VERSION"
row="| [\`release/$VERSION\`]($url) | $VERSION | \`${TAG:0:7}\` (\`v$VERSION\`) |"
awk -v row="$row" '
    /^\| \[`release\// { last = NR }
    { line[NR] = $0 }
    END { for (i = 1; i <= NR; i++) { print line[i]; if (i == last) print row } }
' "$README" > "$README.new"
mv "$README.new" "$README"
