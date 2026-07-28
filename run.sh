#!/usr/bin/env bash
# The one way to run this repo, locally or in CI.
#
# Brings up the syslog-ng oracle + the cross container, builds and runs the
# baseline under QEMU, and prints one combined report: the device's self-measured
# figures and whatever the oracle received (nothing at Baseline). Same command
# everywhere; exits non-zero if the run fails.
#
#   ./run.sh                 # build and run
#   CAPTURE=1 ./run.sh       # at the Baseline, also re-freeze measurements/Baseline.csv
#   TAG=<slug> ./run.sh      # name the run as a particular stage
set -euo pipefail

cd "$(dirname "$0")"
compose=(docker compose -f docker/docker-compose.yml)

cleanup() { "${compose[@]}" down --volumes --remove-orphans >/dev/null 2>&1 || true; }
trap cleanup EXIT
cleanup

# Pass TAG / CAPTURE through to the in-container script.
"${compose[@]}" run --rm \
    -e "TAG=${TAG:-}" -e "CAPTURE=${CAPTURE:-0}" \
    run
