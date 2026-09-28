# SolidSyslog example

An example of integrating [SolidSyslog](https://github.com/cososo-ltd/solid-syslog) into an
application that already exists, built with CMake. Each release of the library has its own
branch here, and on that branch every commit after the Baseline is one stage of the
integration, stating what it changes, what it gives you and what it costs.

This branch holds only this page. The integrations are on the release branches.

## Releases

| Branch | SolidSyslog | Pinned at |
|---|---|---|
| [`release/0.1.0`](https://github.com/cososo-ltd/solid-syslog-example/tree/release/0.1.0) | 0.1.0 | `a8d3979` |
| [`release/0.2.0`](https://github.com/cososo-ltd/solid-syslog-example/tree/release/0.2.0) | 0.2.0 | `25676cc` (`v0.2.0`) |

`release/0.1.0` pins `a8d3979`, seven commits before `v0.1.0`. Those seven change
documentation, CI and release metadata, plus one library source file: the FatFs adapter
lost a compile-time check on its block size. This device passes that check, so the image
built from `a8d3979` is the image `v0.1.0` would build.

`release/0.1.0` commits each stage's measured figures and run report alongside its
source. From 0.2.0 onward a stage commit carries source only, and a release's figures are
measured in one run over the finished branch and committed once at its tip, under
`evidence/`, and attached to that release's
[GitHub release](https://github.com/cososo-ltd/solid-syslog-example/releases/tag/v0.2.0).

## Release branches are permanent

The SolidSyslog documentation links these branches by name, and that documentation ships
in signed, version-stamped bundles. A release branch is created once, when its release is
published, and is never deleted or rewritten.
