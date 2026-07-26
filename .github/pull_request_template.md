## What this tag adds

<!-- The single capability added and why — which SolidSyslog component (see the
security-levels guidance), and which configuration it moves toward
(minimal / secure / hardened). -->

## Checklist

- [ ] The diff is **application-only** — no change to board bring-up, config headers, or build infra (unless this PR *is* Baseline).
- [ ] `measurements/<State>.csv` committed, and a row added to `measurements/tags.tsv`.
- [ ] README cost table regenerated: `python3 scripts/gen-cost-table.py`.
- [ ] `./run.sh` green (build + QEMU + baseline self-check).
