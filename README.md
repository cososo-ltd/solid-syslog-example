# solid-syslog-example

A worked integration of [SolidSyslog](https://github.com/cososo-ltd/solid-syslog), built up in
stages — from a device with no syslog at all to one whose records are authenticated and encrypted.

Each stage is one commit. It says what it does, what it changes, what it gives you, and what it
costs. The costs are measured by the device itself, not estimated.

It builds on a baseline that simulates the sort of device you might be adding this to, and that
measures itself: see [docs/baseline.md](docs/baseline.md) for what the baseline is, how the
figures are made, and how to run it.

## This stage — Discard policy

Decide what a full queue does with the record that does not fit, and keep the decision changeable
while the device runs.

```c
static enum SolidSyslogDiscardPolicy SyslogDiscardPolicy(void* context)
{
    (void) context;
    return s_discardPolicy;
}

.Buffer = SolidSyslogCircularBuffer_Create(SolidSyslogFreeRtosMutex_Create(), s_ring, sizeof(s_ring), SyslogDiscardPolicy, NULL),

/* on the store */
.GetDiscardPolicy = SyslogDiscardPolicy,

/* on the logger */
.GetDiscardPolicy = SyslogDiscardPolicy,
.OnHalt           = SyslogHalted,
```

Three places can be full: the ring, on the task that logs; the store, on the service task; and the
logger itself, which holds a record for the sender between passes. Each asks the policy at the
moment it has to decide, from the task it runs on. Give all three the same function, as here, so
the pipeline applies one policy; the library does not check that they agree.

The answers are the library's `SolidSyslogDiscardPolicy`. Oldest, the default the earlier stages
left in place, drops the oldest records to make room, so the newest survive. Newest refuses the
record being written. Halt refuses it too, and the logger then stops moving records: it calls
`OnHalt` once, on the service task, after the refusal has been reported, and moves nothing more
until the policy answers otherwise or the device restarts. Records lost under any of the three are
reported once when a run of losses starts, by the queue that lost them.

This device stays on Oldest, and `Syslog_SetDiscardPolicy` changes it from any task. `OnHalt` is
the device's own reaction; here it is a line on the console, and on a device whose records must
not be lost it is where the device stops.

**When you need it.** If what happens to records on a full queue is a decision for the site or the
commissioning engineer rather than the firmware, or if a device must stop rather than lose a
record. A device content to keep the newest records can leave every policy argument `NULL`.

## License

This example's own code is [0BSD](LICENSE) — completely open, no conditions.

Third-party code keeps its own license: the vendored Arm SMSC9220 driver (`app/net/smsc9220/`) is
Apache-2.0 (see its `LICENSE`). FreeRTOS, lwIP, mbedTLS, and FatFs are consumed from the build
container under their own upstream licenses and are not redistributed here.

SolidSyslog is fetched at build time and is likewise not redistributed here. It is offered under
three alternative licenses, which its own
[LICENSE.md](https://github.com/cososo-ltd/solid-syslog/blob/main/LICENSE.md) sets out.
