# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Time quality

Add `SolidSyslogTimeQualitySd`, and give `MetaSd` an uptime source alongside its counter.

```c
struct SolidSyslogMetaSdConfig metaConfig = {
    .Counter      = SolidSyslogStdAtomicCounter_Create(),
    .GetSysUpTime = SolidSyslogFreeRtos_GetSysUpTime,   /* new */
};
sd[1] = SolidSyslogTimeQualitySd_Create(SyslogTimeQuality);
```

```text
... BOOT [meta sequenceId="1" sysUpTime="385"][timeQuality tzKnown="1" isSynced="0"] device started
```

Time quality states how far the clock can be trusted, which matters when comparing events from
different devices.

This device reads the host clock once at boot and then free-runs on the FreeRTOS tick, so `isSynced`
is `0` and the callback writes no `syncAccuracy`. `tzKnown` is `1`; the device works in UTC
throughout.

`sysUpTime` accompanies the sequence number. After a reboot the sequence restarts at one, and an
uptime near zero distinguishes that from a counter wrap.

The element lands before the store because store-and-forward breaks the assumption that a record
reaches the collector shortly after it was raised. A record can arrive hours later, so the device
states what its clock is worth first.

**When you need it.** If events from this device will be ordered against events from others, or if a
record's timestamp will be relied on after a delay.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
