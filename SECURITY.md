# Security Policy

This repository is a **worked example / reference** for integrating SolidSyslog —
not a supported product. It is licensed [0BSD](LICENSE) and provided as-is.

## Reporting a vulnerability

**Please do not report security issues in public GitHub issues, pull requests,
or discussions.** Use one of the private channels below — the same ones as the
[SolidSyslog](https://github.com/cososo-ltd/solid-syslog) policy:

1. **GitHub private vulnerability reporting (preferred).** On this repository, go
   to the **Security** tab → **Report a vulnerability**. This opens a private
   advisory visible only to you and the maintainer.
2. **Web form.** If you cannot use GitHub, submit the form at
   **<https://cososo.co.uk/security/report>**. It routes to a private inbox.

We do not publish a security email address. Both channels reach the maintainer
privately.

## Scope

- **This example's own code** (0BSD) — reported via the channels above and fixed
  on a best-effort basis.
- **SolidSyslog itself** — report against the
  [solid-syslog](https://github.com/cososo-ltd/solid-syslog/security) repository,
  whose security policy governs treatment (CVE, advisory, coordinated disclosure).
- **Third-party code** — FreeRTOS, lwIP, mbedTLS, and FatFs (consumed from the
  build container) and the vendored Arm SMSC9220 driver are the responsibility of
  their upstream projects and your own supply-chain process.

Build infrastructure, CI configuration, and documentation are not in scope.
