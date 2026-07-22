# solid-syslog-example

A worked example of integrating [SolidSyslog](https://github.com/cososo-ltd/solid-syslog)
into a realistic secure embedded device — **FreeRTOS + lwIP + mbedTLS + FatFS**, running on
QEMU with syslog-ng as the receiving collector.

The repository is built as a sequence of tagged steps. It starts from a working, already-secured
device with **no** SolidSyslog, then adds logging one component at a time. Diffing between two
tags shows exactly how each capability is wired in; the committed measurements at each tag show
exactly what it costs in flash, RAM, and stack.

## Three worked systems

Mirroring the [Security Levels guidance](https://docs.cososo.co.uk/solid-syslog/security-levels/),
the steps climb through three destination systems:

1. **A lean edge sensor — the minimum system.** UDP, no store, no structured data. The least
   that emits a valid, timestamped RFC 5424 record to your collector.
2. **A device on an untrusted network — reasonably secure.** Server-auth TLS, store-and-forward
   so records survive outages, and evidence the collector can trust. Aimed at a device destined
   for **SL2** systems or a **CRA**-facing deployment.
3. **A device needing non-repudiation — the maximal offering.** Mutual TLS with a per-device
   identity, tamper-evident at-rest integrity, origin evidence, and a defined storage-full
   response. Aimed at **SL3 or higher**.

> IEC 62443 certifies systems, not components, and the Cyber Resilience Act is about the whole
> product. These examples show how a device *contributes* to a compliant system's audit-logging
> story. They are guidance, not a guarantee of compliance.

## What the numbers mean

Every measurement is a **delta** against a frozen baseline — a device that already runs a TCP/UDP
stack, a filesystem, and TLS. So the figures answer the real question: *what does adding secure
remote syslog cost on a device that already does networking, storage, and crypto?* The platform's
own footprint (lwIP, mbedTLS, FatFS) lives below the line — see those projects' docs, not ours.

See [`EPIC.md`](EPIC.md) for the full plan, tag sequence, and measurement method.
