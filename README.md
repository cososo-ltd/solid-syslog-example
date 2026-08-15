# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Sequence numbers

Add the first structured-data element, `SolidSyslogMetaSd`, carrying a sequence number. Elements
are supplied to the logger as an array and read on every record, so they must outlive the call that
creates the logger.

```c
static struct SolidSyslogStructuredData* sd[1];

struct SolidSyslogMetaSdConfig metaConfig = {
    .Counter = SolidSyslogStdAtomicCounter_Create(),
};
sd[0] = SolidSyslogMetaSd_Create(&metaConfig);

struct SolidSyslogConfig config = {
    /* ... */
    .Sd      = sd,
    .SdCount = 1U,
};
```

```text
... BOOT [meta sequenceId="1"] device started
```

The sequence number is incremented once per record *formatted*, not once per record delivered. A
record that never arrives therefore leaves a gap in the sequence rather than no trace at all, which
is why it is worth adding before any buffering or storage that could drop one. Instrument first,
then introduce the failure mode.

Unlike a header field, an SD PARAM has no nil value: one that is unset is omitted entirely rather
than written as `-`.

The counter comes from `SolidSyslogStdAtomicCounter`. If your toolchain has no atomics, supply your
own to the contract `SolidSyslogAtomicCounter_Increment` states — and note that logging from more
than one task is what makes the atomic part of it necessary.

`StdAtomic` joins the platform list. It is named but not linked — a platform over a stable system
API is already inside `libSolidSyslog.a`, and only the header-configured packs have a
`SolidSyslog::<Platform>` target.

**When you need it.** If anyone needs to know that records have gone missing.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
