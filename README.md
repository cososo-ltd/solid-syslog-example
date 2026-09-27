# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Linked

SolidSyslog is linked into the application without any of it being called. The stage is broken out
for clarity: it separates getting the build to accept the library from getting the device to use
it, so anything that goes wrong here is a build problem and nothing else.

Three lines carry it. `FetchContent` nests the library under this build. `SOLIDSYSLOG_PLATFORMS`
names the platforms, and a named list is authoritative: lwIP alone,
because nothing at this stage reaches any other pack. Then one link line, for the core library and
that pack.

`--gc-sections` discards what nothing calls, so a platform pack that is linked but unused does not
reach the image.

For now you need only the core and a network platform. The pin is the SHA in `solid-syslog.pin`,
which the build reads, and a change to that file reconfigures it.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
