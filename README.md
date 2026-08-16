# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Origin address

Add the `ip` PARAM to the origin element, sourced from the same interface address the HOSTNAME field
reports.

```c
struct SolidSyslogOriginSdConfig originConfig = {
    /* ... as the previous stage ... */
    .GetIpCount = SyslogOriginIpCount,
    .GetIpAt    = SyslogOriginIpAt,
};
```

```text
... [origin software="solid-syslog-example" swVersion="0.2.0" enterpriseId="32473" ip="10.0.2.15"] device started
```

A relay or NAT between the device and the collector rewrites the address the collector observes.
`ip` is what the device says about itself, and that survives the hop.

The PARAM is repeatable, so the library asks for a count and then one value per index rather than
taking a single string. This device has one address and returns one, and returns none before the
interface has an address — a count of zero omits the PARAM rather than emitting an empty one.

`SyslogFields_IpAddress` becomes the single place that reads the address, and HOSTNAME formats the
same string through it. Two fields that must agree now cannot disagree.

**When you need it.** If anything sits between the device and the collector — a relay, a gateway, or
NAT — and the source address the collector sees can no longer be trusted to identify the device.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
