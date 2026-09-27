# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — TLS

Wrap the byte stream in TLS over the TCP stream from the previous stage, and accept the collector by
the fingerprint of its certificate. Records can be read only by that collector and cannot be altered
in transit. The fingerprint was provisioned at commissioning, so no CA is involved and the site needs
no PKI. The collector is authenticated to the device; the device is not yet authenticated to the
collector.

```c
struct SolidSyslogMbedTlsHandleCredentialsConfig credentialsConfig = {
    .Rng                  = DeviceCertStore_Rng(),
    .PeerFingerprints     = s_collectorPins,
    .PeerFingerprintCount = 1U,
};

struct SolidSyslogMbedTlsStreamConfig tlsConfig = {
    .Transport   = SolidSyslogLwipRawTcpStream_Create(&tcpConfig),
    .Sleep       = SyslogSleep,
    .Rng         = DeviceCertStore_Rng(),
    .Credentials = SolidSyslogMbedTlsHandleCredentials_Create(&credentialsConfig),
    .Profile     = CollectorProfile,
};

.Stream = SolidSyslogMbedTlsStream_Create(&tlsConfig),
```

The credentials carry the trust decision. `CollectorProfile` is asked at every connection and sets
`ServerName`, which is checked against the certificate as well: the pin says which certificate, the
name which peer it was issued to. A pinned certificate is still checked against its validity dates,
so its lifetime is the operator's to choose.

A second concurrent session has to be paid for upstream. The mbedTLS allocator and the task that
carries the handshake both need sizing for it; both fail loudly when they are not, and neither can
be sized from the run that fails.

**When you need it.** If the log path crosses a network you do not control, or if someone reading
records in transit would learn something they should not. Also if the device needs to know it is
talking to the real collector rather than to whatever answered on that address. A fingerprint gives
that without a PKI, which many OT sites do not run.

> If your device does not already run TLS, the library and its trust material will dominate
> everything on this page. This device already holds a TLS session for its own broker, so what this
> stage adds is the adapter and a second session.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
