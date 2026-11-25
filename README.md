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
| [`release/0.3.0`](https://github.com/cososo-ltd/solid-syslog-example/tree/release/0.3.0) | 0.3.0 | `83a0239` (`v0.3.0`) |
| [`release/0.4.0`](https://github.com/cososo-ltd/solid-syslog-example/tree/release/0.4.0) | 0.4.0 | `f681bb4` (`v0.4.0`) |

`release/0.1.0` pins `a8d3979`, seven commits before `v0.1.0`. Those seven change
documentation, CI and release metadata, plus one library source file: the FatFs adapter
lost a compile-time check on its block size. This device passes that check, so the image
built from `a8d3979` is the image `v0.1.0` would build.

`release/0.1.0` commits each stage's measured figures and run report alongside its
source. From 0.2.0 onward a stage commit carries source only, and a release's figures are
measured in one run over the finished branch and committed once at its tip, under
`evidence/`, and attached to that release's
[GitHub release](https://github.com/cososo-ltd/solid-syslog-example/releases), tagged
`v<version>` at the branch tip.

## Release branches are permanent

The SolidSyslog documentation links these branches by name, and that documentation ships
in signed, version-stamped bundles. A release branch is created once, when its release is
published, and is never deleted or rewritten.

## A release is cut by the Release workflow

Once SolidSyslog's `v<version>` tag exists, the **Release** workflow makes this example's
release of the same version. It is run from the Actions tab with the version, the commit
that tag names, and the **Sweep** run whose output is the release's evidence:

```bash
gh workflow run release.yml --repo cososo-ltd/solid-syslog-example \
  -f version=<version> -f tag=<full SHA of SolidSyslog's v<version>> \
  -f run=<Sweep run id> -f dry-run=false
```

It takes the stages from `wip/next`, writes each stage's figures into its commit
message, rebases them onto a Baseline pinned at the tag, and commits the run's output at
the tip under `evidence/`. It then pushes `release/<version>`, creates this repository's
`v<version>` tag and GitHub Release with `sweep-<version>.zip`, and adds the row to the
table above.

Nothing is measured by the workflow. The run it is given must be the green sweep that
SolidSyslog's Example Sweep workflow started for `wip/next` as it stands, and the
release is refused unless that run's figures hold at the tag:

- SolidSyslog's `v<version>` is the commit named, and neither `release/<version>` nor a
  `v<version>` tag exists here.
- Between the commit the run measured and the tag, SolidSyslog changed nothing under the
  paths its `example-sweep.yml` names, which are the ones a change reaches this example
  through.
- `wip/next` is on the container image SolidSyslog's CI pins at the tag.
- The Baseline measured nothing above `measurements/Baseline.csv`. Every stage's cost is
  counted from that file, so it has to be this Baseline's own: where the image or an
  upstream has moved since it was captured, capture it again on `wip/next`'s Baseline.
- Every link from SolidSyslog's documentation at the tag to a release branch here is to
  `release/<version>`.

Where SolidSyslog changed, or the image moved, the answer is to sweep again: run
SolidSyslog's Example Sweep workflow for the commit the tag names, and give the Release
workflow that run.

`dry-run` is ticked unless it is cleared. A dry run makes every check, reports each one
that would refuse the release, builds the branch and pushes nothing; the branch is in the
run's `release` artifact as a Git bundle, beside the Release's description, the zip and
the README. The run's summary lists the stage messages that quote a figure in their
prose, which nothing rewrites: correct those on `wip/next`, sweep again, and release from
that run.

## `wip/next` is rewritten

Between releases, `wip/next` holds the next release's stages on a Baseline pinned at a
recent commit of SolidSyslog's `main`. The Sweep workflow writes it again whenever a
change to the library could reach this example, so nothing should link it or build on
it. A release branch is the published form.

A stage is one commit, so a stage is fixed on `wip/next` by amending its commit. The
next rewrite starts from what it finds there, and the fix is carried forward:

```bash
git fetch origin && git switch --detach origin/wip/next
# edit, then fold the change into the stage's own commit
git commit -a --fixup <stage commit>
GIT_SEQUENCE_EDITOR=true git rebase -i --autosquash --root
git push --force-with-lease=wip/next origin HEAD:wip/next
```
