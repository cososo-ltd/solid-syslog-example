#!/usr/bin/env bash
# Freeze a Baseline's measurements/Baseline.csv from a run of it.
#
#   scripts/capture.sh <worktree>
#
# <worktree> is the Baseline checked out. It is built and run in the image its own
# compose file names, and the figures of that run are written over the file. A run
# that fails writes nothing and exits non-zero. The build is removed afterwards.
set -euo pipefail

WT="$(cd "${1:?usage: scripts/capture.sh <worktree>}" && pwd)"

# Docker Desktop wants C:/... where Git Bash has /c/...
hostpath() { if command -v cygpath >/dev/null; then cygpath -m "$1"; else printf '%s' "$1"; fi; }
compose() { docker compose -p "solid-syslog-capture-$$" -f "$(hostpath "$WT/docker/docker-compose.yml")" "$@"; }
# The build is root-owned, so a root container removes it.
cleanup() {
    compose run --rm --no-deps certs bash -c 'rm -rf /w/build /w/baseline-disk.img' >/dev/null 2>&1 || true
    compose down --volumes --remove-orphans >/dev/null 2>&1 || true
}
trap 'rc=$?; cleanup; exit "$rc"' EXIT

LOG="$(mktemp)"
rc=0
compose run --rm -e TAG=Baseline -e TOL= -e CAPTURE=1 run > "$LOG" 2>&1 || rc=$?
frozen=0
grep -Fxq "froze measurements/Baseline.csv" "$LOG" || frozen=1
if [ "$rc" -ne 0 ] || [ "$frozen" -ne 0 ]; then
    tail -40 "$LOG" >&2
    rm -f "$LOG"
    echo "capture: the Baseline did not run cleanly (exit $rc)" >&2
    exit 1
fi
rm -f "$LOG"
