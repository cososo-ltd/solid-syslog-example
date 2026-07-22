# Epic: worked example — footprint deltas + porting guide, across three secure systems

## Goal

One example repository that does three jobs at once:

1. **Marketing numbers** — defensible flash / RAM / stack **deltas** for adding SolidSyslog to a
   realistic secure networked device, at each of three destination systems.
2. **Porting / getting-started guide** — the diff between two tags *is* the integration tutorial;
   the prose is collated from those diffs.
3. **Demo / memory** — a granular, single-change-per-tag history anyone can `git checkout` and
   re-measure.

Pitched at the three **worked combinations** in
[`security-levels.md`](https://docs.cososo.co.uk/solid-syslog/security-levels/), framed
CRA-forward: a device's contribution to a compliant system, not a component-level compliance
claim.

## The three destination systems

| # | System | Pitch | Reference SL |
|---|---|---|---|
| **1** | A lean edge sensor | the minimum — a valid, timestamped RFC 5424 record to the collector | SL1 |
| **2** | A device on an untrusted network | reasonably secure; suitable for a device targeted at SL2 systems or at CRA | SL2 |
| **3** | A device needing non-repudiation | the maximal offering we can give | SL3 or higher |

> IEC 62443 certifies systems, not components; the CRA is about the whole product. These examples
> show how a device *contributes* to a compliant system's audit-logging story — guidance, not a
> guarantee of compliance.

## Target (one only)

FreeRTOS + lwIP + mbedTLS + FatFS on **QEMU mps2-an385 (Cortex-M3)**, syslog-ng as the receiving
oracle. Reuse, don't rebuild:

- the SolidSyslog FreeRTOS + lwIP + netif + QEMU + syslog-ng BDD harness (S28.09),
- the generated integration manifest (`cmake --build --target manifest` →
  `docs/generated/beta-stack-manifest.txt`); the example consumes that source list — no hand-rolled
  includes,
- `integrating-lwip.md` / `integrating-mbedtls.md` / `integrating-plusfat.md`.

One target is enough — we sell **deltas**, not absolute numbers.

## The buildup principle (invariants)

- **Baseline is a clean cut.** Produce T1 however messily is needed (branches, stashes, squashes)
  until the whole stack is known-good, then land it as **one** commit and tag it. **Infra is
  frozen at T1.**
- **Every T2+ commit is application-only.** `git diff Tn..Tn+1` shows nothing but how SolidSyslog
  is wired in. An infra file changing in a later diff means something is wrong.
- **Single-change, attributable tags.** One component per tag, so each delta is owned by exactly
  one thing.
- **Pre-provision infra generously in T1; document forced requirements as a note, never a code
  delta.** If SolidSyslog forces a bigger `configTOTAL_HEAP_SIZE`, an lwIP option, or a deeper
  stack, size T1 for the full system-3 build up front and record the requirement as a note on the
  tag ("T4 needs heap ≥ X"). Honest number, clean diff.
- **We don't own the platform's cost.** mbedTLS / lwIP / FatFS live below the line, in the frozen
  baseline. Point at their docs, not ours.

## Baseline contents (T1)

A realistic secure networked device with **zero** SolidSyslog:

- FreeRTOS + lwIP with **TCP and UDP linked and exercised** (a heartbeat) — we only add to
  networked equipment, so these costs must not land in our deltas.
- FatFS with the **file API actually used** (open / write / `f_sync` / close).
- mbedTLS with a **live TLS client session** to the oracle.
- Harness: spawns two tasks — one carrying `// security event here` — delays, then prints per-task
  stack high-water marks, free heap (`xPortGetFreeHeapSize`), and image `.text` / `.data` / `.bss`
  from `size`; then cleanly deletes the tasks and exits via semihosting.

## Tag sequence

| Tag | Adds (component from `security-levels.md`) | Destination |
|---|---|---|
| **T1** | Baseline, frozen (above) | — |
| **T2** | `UdpSender` + `PassthroughBuffer` + `NullSecurityPolicy`, injected clock, no SD, default size | **System 1 reached** (lean edge sensor) |
| **T3** | tune message size down for the no-SD case (`SOLIDSYSLOG_FORMATTER_STORAGE_SIZE` + buffer max-msg) | System 1, minimal |
| **T4** | UDP → server-auth `TlsStream` (reuses baseline mbedTLS) | System 2 → |
| **T5** | `BlockStore` store-and-forward over the baseline FatFS file (still `NullSecurityPolicy`) | System 2 → |
| **T6** | `NullSecurityPolicy` → `Crc16Policy` (accidental-corruption detection at rest) | System 2 → |
| **T7** | `TimeQualitySd` (trusted-time evidence) | System 2 → |
| **T8** | `MetaSd` with `sequenceId` (gap-visible delivery) | **System 2 reached** (untrusted network / SL2 / CRA) |
| **T9** | server-auth → mutual TLS (per-device client cert/key) | System 3 → |
| **T10** | `Crc16Policy` → keyed HMAC policy (tamper-evident at rest; exercises the `GetKey` seam) | System 3 → |
| **T11** | `OriginSd` + sysUpTime (device attribution / origin verification) | System 3 → |
| **T12** | discard policy + store-full response (`OnStoreFull` / threshold callback) | **System 3 reached** (non-repudiation / SL3+) |

**SL4 is doc-only.** Write-once/immutable storage, protected time source, hardware-held keys are
platform/hardware, not library code — undemonstrable on QEMU. Appendix: "this is System 3 plus
these hardware requirements; here are the seams you wire (`GetKey` → secure element, a write-once
`BlockDevice` adapter, an alternate clock source)."

## Measurement method

Per tag, one build + one QEMU run, committed as `measurements/T<n>.txt`:

- `.text` / `.data` / `.bss` from `size` on the linked image,
- per-task stack **high-water mark** (from the harness), validated against a static worst-case pass
  (`-fstack-usage` + call-graph) so we never under-quote a deep path (TLS handshake, DNS),
- free heap at steady state.

The marketing table is **deltas vs the previous tag** (and vs T1 for the cumulative "cost to reach
system 2 / system 3"). Every published number carries its config footnote (backends, pool sizes,
compiler, `-Os`) — a number without its config is a claim a customer can disprove.

**mbedTLS framing.** Because mbedTLS is in the frozen baseline, the only TLS number we publish is
the marginal **"you already have mbedTLS"** one (T4: adapter flash + one TLS session's RAM). The
"from scratch" figure is not headlined; if ever needed it is one auxiliary build (baseline minus
mbedTLS), captured once in an appendix as platform cost.

## Work checklist

Granular steps live here (this is a marketing/docs artifact; it does not carry the library's
MISRA/TDD/board ceremony). Any gap this example surfaces in the **library itself** (a missing
BYO-callback, a needed tunable, a manifest hole) is filed as a normal story on the
[solid-syslog](https://github.com/cososo-ltd/solid-syslog) board — that is library work.

- [ ] **Scaffold + baseline (T1)** — repo layout, CMake/Make consuming the generated manifest,
      QEMU + syslog-ng harness (reuse S28.09), measurement harness, the messy-then-clean cut, land
      T1 tagged.
- [ ] **System 1 (T2–T3)** — lean edge sensor.
- [ ] **System 2 (T4–T8)** — untrusted network.
- [ ] **System 3 (T9–T12)** — non-repudiation.
- [ ] **SL4 appendix + marketing delta table + porting guide** collated from the diffs.

## Open items

- SL4 strictly doc-only (recommended) vs a thin software-seam tag.
- Granularity: keep all 12 tags split (recommended — the point is attribution) vs merge T7+T8
  ("System 2 SD bundle") and T5+T6 ("store + at-rest").
- Flip this repo public once T1 is landed and known-good.
