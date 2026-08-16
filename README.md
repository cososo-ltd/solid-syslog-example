# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — HMAC at rest

Replace the CRC-16 with a keyed HMAC. The checksum established that a record came back the way it
went in; the HMAC establishes that nobody has changed it since. An edit made without the key fails
verification, so stored records become tamper-evident rather than merely intact.

```c
struct SolidSyslogMbedTlsHmacSha256PolicyConfig hmacConfig = {.GetKey = SyslogStoreKey};

.SecurityPolicy = SolidSyslogMbedTlsHmacSha256Policy_Create(&hmacConfig),
```

The key is fetched per seal and per verify rather than held, so it never sits on the policy
instance. Key custody, rotation and provisioning are yours; the library consumes a key you supply
and never stores one.

Holding a named symmetric key and handing it out is the device's own mechanism — a device already
doing mTLS has provisioned secrets and somewhere to keep them, so the key slot, the loader and the
accessor all sit below the line. What SolidSyslog is charged for is the policy and the callback that
reaches for the key.

**When you need it.** If an attacker could reach the medium — removable, unattended, or stealable —
and stored records must be provably unaltered.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
