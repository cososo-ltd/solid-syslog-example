# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — File store

Spool to a `SolidSyslogBlockStore` over a `SolidSyslogFileBlockDevice` over the library's FatFs
port, replacing the Null store. The service task drains the ring into storage and sends from there,
so a failed send costs a retry rather than the record: the audit trail survives an outage instead of
ending at it.

```c
#define SYSLOG_STORE_PREFIX "syslog"
#define SYSLOG_STORE_BLOCKS 4U

struct SolidSyslogBlockStoreConfig storeConfig = {
    .BlockDevice    = SolidSyslogFileBlockDevice_Create(SolidSyslogFatFsFile_Create(), SYSLOG_STORE_PREFIX, 0U),
    .MaxBlocks      = SYSLOG_STORE_BLOCKS,
    .DiscardPolicy  = SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
    .SecurityPolicy = SolidSyslogCrc16Policy_Create(),
};
```

Three decisions come with it: how much to store, which is capacity on the medium rather than RAM;
what happens when it fills — discard oldest, discard newest, or halt; and whether to be warned
before that point, via the capacity-threshold callback.

This device stores four blocks, one file per block, `syslog00.log` upward on the volume it already
mounts, and discards the oldest when full.

The CRC-16 detects corruption, not tampering. It catches a truncated write or bit-rot; anyone who
can edit a stored record can recompute it. It establishes that a record came back the way it went
in, which is the prerequisite for spooling at all. Making stored records tamper-evident, and then
unreadable, are later stages.

Storing happens on the service task, so a task that calls `SolidSyslog_Log` still knows nothing
about what happens after it returns and its stack does not move. The RAM is pool allocation and
handles rather than buffers — nothing holds a block in memory, so the store costs its handles
rather than its capacity.

`SolidSyslog::FatFs` is a header-configured upstream, so it is both named in
`SOLIDSYSLOG_PLATFORMS` and linked.

**When you need it.** If losing the records raised during an outage is not acceptable, or if they
must survive a reboot.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
