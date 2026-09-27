# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Pin rotation

A pinned collector's certificate is renewed without any device stopping. A renewal changes the
fingerprint, so the device takes the renewed certificate's pin beside the current one before the
collector switches, and retires the old one afterwards.

```c
static const char* s_collectorPins[2];   /* either authorises the collector */

.PeerFingerprintCount = 2U,              /* on the credentials */
.Version              = SyslogStreamVersion,   /* on the TLS stream */

bool Syslog_ProvisionNextCollectorPin(const char* pin);   /* any task */
bool Syslog_RetireCollectorPin(void);                     /* any task */
void Syslog_ApplyPinChanges(void);                        /* service task, each pass */
```

Both slots hold the current pin until a renewal is under way. The pins and the stream version are
read on the service task as it connects, so a change is queued and the service task applies it
between passes. Both calls return false when the change is not queued - a full queue, or a NULL
pin, which would read as a retirement - so a refused change is seen rather than lost. The version
moves with every change; the sender checks it before every record and reconnects when it has
moved, so new pins apply without a restart.

**When you need it.** Wherever the collector is pinned. Without it, every device pinned to a
collector stops on the day that collector's certificate is replaced, and on an OT network that is
every device on the site at once.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
