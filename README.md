# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Error handler

Install the handler before any other call into SolidSyslog.

```c
SolidSyslog_SetErrorHandler(OnSyslogError, NULL);
```

Nothing in the library fails loudly. A `_Create` that cannot succeed substitutes a Null object and
reports it rather than returning `NULL`, so a logger that has silently stopped looks exactly like
one with nothing to say. The handler is what tells the two apart, which is why it goes in before
the first `_Create` and not after something looks wrong. It reports many misconfiguration errors,
and can save significant time while integrating.

This is not only an integration aid. The handler is the seam into the device's own error and health
reporting, and it stays valuable at run time: later stages raise an edge-triggered warning when the
collector becomes unreachable and a notice when delivery recovers. Route it wherever the device
already routes faults — here, the same console as everything else, so a fault lands in the run
report next to the rest of what the device did.

The handler names the four lifecycle categories a misconfigured integration raises and prints the
rest numerically. A device reacting to a fault would switch on the category rather than the text.

**When you need it.** Every device, and first. It is the only thing standing between a
misconfigured logger and a silent one.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
