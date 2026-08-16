# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — AES-GCM at rest

Replace the HMAC policy with authenticated encryption. Tamper-evidence establishes that a stored
record was not altered; it does nothing to stop anyone reading it. AES-256-GCM encrypts the body,
authenticates the record header as associated data, and puts the nonce and tag in the trailer.

```c
struct SolidSyslogMbedTlsAesGcmPolicyConfig gcmConfig = {.GetKey = SyslogStoreKey, .Rng = rng};

.SecurityPolicy = SolidSyslogMbedTlsAesGcmPolicy_Create(&gcmConfig),
```

GCM needs a fresh nonce per record and mbedTLS has no context-free RNG, so the policy takes the
device's DRBG as well as the key. That is the only wiring difference from the HMAC policy.

The store key does not change. Its name states what it protects rather than which algorithm protects
it, so escalating the policy needs no new key provisioned.

These are separate decisions and the second does not follow from the first. A device that only needs
to prove records were not altered can stop at the HMAC.

The pipeline element now derives both of its values from what the device holds, and each falls back
to the weakest honest answer when the credential behind it is missing:

```c
s_sd[3] = SyslogPipelineSd_Init(
    ((clientChain != NULL) && (clientKey != NULL)) ? "mtls" : "tls", (rng != NULL) ? "aes-256-gcm" : "none"
);
```

```text
... [logPipeline@32473 transport="mtls" atRest="aes-256-gcm"] device started
```

**When you need it.** If a disk that leaves the device would give something away — records naming
users, addresses, process values, or anything else you would not publish.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
