# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Mutual TLS

Add a client certificate and its key to the credentials, beside the collector's pins. The handshake
then authenticates the device to the collector, as well as the collector to the device.

```c
struct SolidSyslogMbedTlsHandleCredentialsConfig credentialsConfig = {
    .ClientCertChain = clientChain,
    .ClientKey       = clientKey,
    /* ... the Rng and the collector's pins, as before ... */
};
```

Both fields must be set. Supplying one and not the other is reported on every connection, and the
device then presents nothing, so the pipeline element is given what the device holds rather than
what was configured:

```c
s_sd[3] = SyslogPipelineSd_Init((clientChain != NULL) && (clientKey != NULL));
```

```text
... [logPipeline@32473 transport="mtls" atRest="hmac-sha256"] device started
```

The handshake authenticates the TLS peer. Where a relay, gateway or broker terminates the
connection, the collector authenticates that hop rather than the device behind it, and the `origin`
element carries the device's own identity across it.

The collector port used here requires a client certificate and refuses a client that presents none.

**When you need it.** When the receiver has to authenticate the device rather than accept the
identity the record claims. It requires a certificate per device, protected storage for the private
key, and an issuing and revocation process behind both.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
