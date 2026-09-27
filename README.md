# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — CA chain

For a site that runs its own PKI: the collector's certificate must now chain to the site CA as well
as match its pin. Either failing stops delivery.

```c
struct SolidSyslogMbedTlsHandleCredentialsConfig credentialsConfig = {
    .CaChain = caChain,
    /* ... the client credential, the Rng and the collector's pins, as before ... */
};
```

A pin never waives the chain, and a matching pin with a chain that does not verify is reported as
`PEER_CERTIFICATE_UNTRUSTED`, distinct from `PEER_FINGERPRINT_MISMATCHED`. The pipeline element says
which the device requires, so a collector can tell a device on the pin alone from one on both:

```text
... [logPipeline@32473 transport="mtls" collectorAuth="fingerprint+chain" atRest="aes-256-gcm"] ...
```

**When you need it.** When the site runs a PKI and wants its devices under its own certificate
policy. It adds a way for delivery to stop that the pin alone does not have: when the site CA or an
intermediate expires or is replaced, every device holding it stops at once, and the store holds what
it can until the new CA is provisioned, discarding the oldest when it fills. Without a PKI, the pin
is enough.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
