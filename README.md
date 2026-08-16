# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Origin

Name the device in the record with `SolidSyslogOriginSd` — the software, its version, and the
enterprise number.

```cmake
project(solid_syslog_example_baseline VERSION 0.2.0 LANGUAGES C ASM)

target_compile_definitions(baseline PRIVATE "SYSLOG_SW_VERSION=\"${PROJECT_VERSION}\"")
```

```c
#define SYSLOG_SOFTWARE "solid-syslog-example"

struct SolidSyslogOriginSdConfig originConfig = {
    .Software     = SYSLOG_SOFTWARE,
    .SwVersion    = SYSLOG_SW_VERSION,
    .EnterpriseId = SYSLOG_ENTERPRISE_ID,
};
sd[2] = SolidSyslogOriginSd_Create(&originConfig);
```

```text
... [origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473"] device started
```

This lands after the store rather than before it. While records went straight out, the answer to
"who sent this" was implied by the connection they arrived on. Once records can replay hours later
that is no longer so, and the record has to carry it.

The `ip` PARAM is left out here. The address the collector sees is still the address that reached
it; the next stage takes that assumption away.

`SYSLOG_ENTERPRISE_ID` is defined in its own header rather than beside the element that carries it,
because the number identifies the vendor rather than the logger — anything else this product puts
its own name on wants the same one. `SYSLOG_SW_VERSION` is the version the product already
carries in `project()`, passed in by CMake, for the same reason.

> Enterprise number 32473 is reserved for documentation and testing by RFC 5612. A shipping product
> uses its own, registered with IANA.

**When you need it.** If records will be correlated across devices, replayed after a delay, or
relayed through anything.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
