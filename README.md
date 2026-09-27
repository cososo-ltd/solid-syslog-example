# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Right-sized

Fit the compile-time sizes to what this device uses, now that every collaborator is in place.

The message cap comes first, because the ring, the store's record buffer and the formatter frame on
both task stacks all follow it.

```c
/* app/config/solid_syslog_tunables.h */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 400U

#define SOLIDSYSLOG_ADDRESS_POOL_SIZE 1U
#define SOLIDSYSLOG_TCP_STREAM_POOL_SIZE 1U
#define SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE 1U
```

The worst case measured here is 379 octets: the four SD-ELEMENTs with both counters at full 32-bit
width and both addresses at fifteen characters, plus a short message. 400 allows for longer messages
on this device. Anything longer is truncated rather than dropped.

The pool defaults suit a device running several transports at once. This one runs a single sender
over a single stream to a single destination.

The overrides reach the library through `SOLIDSYSLOG_USER_TUNABLES_FILE`, which the library carries
on an INTERFACE target, so Core, the platform packs and this application all compile against the
same values. They change struct sizes, and a build where only some translation units saw them would
disagree about how big those structs are.

The ring drops from eight records to four. The store holds a backlog, so the ring only has to absorb
what can be logged while the service task is sending.

The task stacks go last, at twice their measured high-water marks rounded up to a whole
`configMINIMAL_STACK_SIZE`.

**When you need it.** Once the pipeline is complete. Sizing earlier means sizing against a device
that is still missing collaborators.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
