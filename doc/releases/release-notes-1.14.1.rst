:orphan:

.. _zephyr_1.14.1:

Zephyr Kernel 1.14.1
####################

We are pleased to announce the release of Zephyr kernel version
1.14.1.

This is a patch release to the 1.14 stable release.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following security vulnerability (CVE) was addressed in this
release:

* Fixes CVE-2019-9506: The Bluetooth BR/EDR specification up to and
  including version 5.1 permits sufficiently low encryption key length
  and does not prevent an attacker from influencing the key length
  negotiation. This allows practical brute-force attacks (aka "KNOB")
  that can decrypt traffic and inject arbitrary ciphertext without the
  victim noticing.
