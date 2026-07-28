# Pull request

## What this stage adds

<!-- The single capability added and why — which SolidSyslog component, and what it
gives a reader who cares about their logs. -->

## Checklist

- [ ] The diff is **application-only** — no change to board bring-up, config headers, or build infra (unless this PR *is* Baseline).
- [ ] `measurements/<slug>.csv` committed, and a row added to `measurements/stages.tsv`.
- [ ] README regenerated: `python3 scripts/gen-cost-table.py`.
- [ ] `./run.sh` green (build + QEMU + baseline self-check).
