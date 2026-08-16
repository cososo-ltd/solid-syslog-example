# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Private SD-ELEMENT

Write a private enterprise SD-ELEMENT. RFC 5424 reserves this form for definitions of your own, and
`SyslogPipelineSd.c` is a complete example of one: it implements the library's structured-data
extension point in its own translation unit.

```c
static void SyslogPipelineSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base;

    SolidSyslogSdElement_Begin(element, "logPipeline", SYSLOG_ENTERPRISE_NUMBER);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "transport"), "tls");
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "atRest"), "hmac-sha256");
    SolidSyslogSdElement_End(element);
}

static struct SolidSyslogStructuredData s_pipelineSd = {SyslogPipelineSd_Format};
```

```text
... [logPipeline@32473 transport="tls" atRest="hmac-sha256"] device started
```

The vtable has one entry, `Format`, and the library never allocates the object. A stateless source
therefore needs no `_Create` and no pool slot; it is a static this application owns and points the
config at. A source with per-instance state puts that state alongside the vtable in the same struct
and reads it back from the `base` parameter.

A non-zero enterprise number is what produces a private SD-ID: `_Begin` emits `name@number` for one
and a bare IANA `name` for zero. `SyslogEnterprise.h` now defines the number and derives the string
that `origin`'s `enterpriseId` carries, so the two forms cannot drift.

What the element reports is the state of the logging path. A collector can confirm that a record
arrived over TLS and was sealed at rest, and can alert on a device whose pipeline has weakened.
The remaining stages change both values as the protection changes.

**When you need it.** If a collector has to verify the protection a record travelled and rested
under rather than assume it.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
