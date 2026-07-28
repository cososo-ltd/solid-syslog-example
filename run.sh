#!/usr/bin/env bash
# The one way to run this repo, locally or in CI.
#
# Brings up the syslog-ng oracle + the cross container, builds and runs the
# baseline under QEMU, and prints one combined report: the device's self-measured
# figures, whatever the oracle received (nothing at Baseline), and the baseline
# self-check. Same command everywhere; exits non-zero if the run fails or the
# baseline drifts.
#
#   ./run.sh                 # build, run, self-check
#   CAPTURE=1 ./run.sh       # also (re)freeze measurements/<TAG>.csv
#   TAG=<slug> ./run.sh      # run/check a particular stage
set -euo pipefail

cd "$(dirname "$0")"
compose=(docker compose -f docker/docker-compose.yml)

cleanup() { "${compose[@]}" down --volumes --remove-orphans >/dev/null 2>&1 || true; }
trap cleanup EXIT
cleanup

# Pass TAG / TOL / CAPTURE through to the in-container script.
"${compose[@]}" run --rm \
    -e "TAG=${TAG:-}" -e "TOL=${TOL:-}" -e "CAPTURE=${CAPTURE:-0}" \
    run
