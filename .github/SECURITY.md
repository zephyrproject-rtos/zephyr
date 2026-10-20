# Security Policy

## Supported versions

The Zephyr project supports the following versions with security
updates:

  - The most recent release, and the release prior to that.
  - Active LTS releases.

At this time, with the latest release of v4.4, the supported
versions are:

  - v4.4: Current release
  - v4.3: Prior release
  - v3.7: Current LTS

## Reporting process

Please see our [Security Vulnerability
Reporting](https://docs.zephyrproject.org/latest/security/reporting.html)
page for details on the process.

Not every bug is a security vulnerability. Defects that require a trusted
party and defects only present on `main` and not in any tagged release are
handled as regular bugs through the public issue tracker. Physical access
alone does not make a party trusted; peers on exposed communication
interfaces, including a USB host connected to a Zephyr USB device, remain
untrusted. See the
[Vulnerability Scope](https://docs.zephyrproject.org/latest/security/reporting.html#vulnerability-scope)
section before filing an advisory.
