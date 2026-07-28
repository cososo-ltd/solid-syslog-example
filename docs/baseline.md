# The baseline, and how the numbers are made

Every figure this repository publishes is a delta above one frozen baseline. This
page is what that baseline is, how the device measures itself, and how to run it.

## What the baseline is

A device you might plausibly be adding syslog to, with none of SolidSyslog in it:
**FreeRTOS + lwIP + mbedTLS + FatFs on QEMU's mps2-an385** (Cortex-M3). It brings
its network interface up, mounts a filesystem, and holds a mutual-TLS session to
a broker for as long as it runs.

That last part matters more than it looks. A device that merely *links* a TLS
stack has not paid for a TLS session, and adding one would then charge syslog for
the whole of mbedTLS's session memory. Because the baseline holds a real session,
what gets added on top is charged only for a **second concurrent** session, which
is far less. The session is held rather than opened and closed, because two
sessions that never overlap would measure the larger of the two rather than the
sum.

The far end is an `openssl s_server` requiring a client certificate, proved to be
up and enforcing before the device runs. On a real device it would be an MQTT
broker or a cloud gateway carrying application traffic; here it is one exchange,
because the session is what is being measured and not the protocol.

Two application tasks — a **log** source and a **service** worker — sit idle,
ready to be occupied. Their stacks are at the FreeRTOS floor, so whatever
occupies them is charged for the depth it adds.

## What the device measures

The device measures itself and prints a `[report]` block; `scripts/run.sh`
captures it, checks it against the frozen figures, and writes
[`run-report.md`](../run-report.md).

| Key | What it is |
|---|---|
| `flash_text` | `.text` + `.rodata` + init arrays — the read-only image |
| `flash_data` | `.data` — initialisers, stored in flash and copied to RAM |
| `static_bss` | `.bss` — including the FreeRTOS heap array, the mbedTLS buffer, and every task stack |
| `heap_used` | FreeRTOS heap in use. heap_1 never frees, so this is also its peak |
| `mbedtls_peak` | high-water mark of the static mbedTLS buffer |
| `mbedtls_free` | what was left of that buffer at the high-water mark |
| `lwip_mem_free` | what was left of lwIP's heap at *its* high-water mark |
| `lwip_pbufs_free` | pool pbufs never used, of `PBUF_POOL_SIZE` — entries, not bytes |
| `stack_log` | peak stack used by the log seam |
| `stack_service` | peak stack used by the service seam |
| `stack_harness` | peak stack used by the harness, where bring-up and the handshake run |

The README's headline pair is derived from these:

- **Flash** = `flash_text` + `flash_data`
- **RAM** = `flash_data` + `static_bss`

`.data` is counted in both because it exists in both — an image in flash, and the
live copy in RAM. `heap_used` is deliberately *not* added to RAM: the FreeRTOS
heap is a static array inside `.bss`, so it is already there, and adding it would
count it twice.

## How sizes are chosen

Nothing here is a round number someone liked. Each is derived from what the
device reports, and every one of them is reported with its remaining margin so a
bad choice shows up as a number rather than as a crash.

- **mbedTLS buffer** — measured peak × 1.5, rounded up to the next KiB. The
  margin is fragmentation headroom, not spare capacity: `buffer_alloc` hands out
  contiguous space, and 24 KiB against a 22.2 KiB peak failed where 32 KiB
  worked.
- **Task stacks** — roughly twice the measured high-water mark, with
  `configCHECK_FOR_STACK_OVERFLOW` at 2 so a bad estimate fails loudly instead of
  corrupting memory. Both idle seams sit at the FreeRTOS floor, because twice
  what an idle task uses is below it.
- **FreeRTOS heap** — twice what the device reports using. Only lwIP allocates
  from it, and heap_1 never frees, so the reported figure is the peak by
  construction.
- **lwIP `MEM_SIZE`** — sized against `TCP_SND_BUF`, not against what a run
  happens to use: `tcp_write` copies into `PBUF_RAM` out of this heap, so
  anything smaller is a send window lwIP could never fill.
- **lwIP `PBUF_POOL_SIZE`** — lwIP's own default. It absorbs receive bursts, and
  a workload of one handshake has no standing to shrink it.

Two figures are **not byte-reproducible**: `heap_used` and `mbedtls_peak` move by
around a hundred bytes between runs of an identical image, because the test PKI
is regenerated every run and DER lengths shift with the key material. Flash and
RAM are exact.

## Running it

One command, the same locally and in CI. No host toolchain — it uses a public
cross image and a syslog-ng collector in Docker:

```bash
./run.sh
```

It brings up the collector and the broker, proves every listener is up and
enforcing *before* the device runs, builds and runs the device under QEMU, and
prints one report: the device's own figures, whatever the collector received, and
the self-check against the frozen figures. It exits non-zero if the run fails or
a figure drifts, so CI gates on it.

- `CAPTURE=1 ./run.sh` — freeze `measurements/<stage>.csv` from this run. It
  refuses to freeze a run that did not complete cleanly.
- `TAG=<slug> ./run.sh` — build and check a particular stage rather than the last
  row of `measurements/stages.tsv`.
- `TOL=<bytes> ./run.sh` — drift tolerance for the figures that are exact,
  default 0. The mbedTLS figures and `heap_used` keep their own allowance: a
  fresh test PKI every run moves the allocator's high-water mark either way.

An unproved listener, a dead broker, or a device that says it logged a record
while the collector received nothing all fail the run. A dead broker would
otherwise read as "the baseline uses less memory" rather than as a failure.
