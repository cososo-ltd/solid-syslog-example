# Pull request

## What this stage adds

<!-- The single capability added and why — which SolidSyslog component, and what it
gives a reader who cares about their logs. -->

## Checklist

- [ ] The diff is **application-only** — no change to board bring-up, config headers, or build infra (unless this PR *is* Baseline).
- [ ] A row added to `measurements/stages.tsv`.
- [ ] `./run.sh` green (build + QEMU + collector).
