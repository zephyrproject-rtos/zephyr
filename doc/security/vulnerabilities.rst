.. _vulnerabilities:

Vulnerabilities
###############

This page collects all of the vulnerabilities that are discovered and
fixed in each release.  It will also often have more details than is
available in the releases.  Some vulnerabilities are deemed to be
sensitive, and will not be publically discussed until there is
sufficient time to fix them.  Because the release notes are locked to
a version, the information here can be updated after the embargo is
lifted.

Release 1.14.0 and 2.0.0
------------------------

The following security vulnerability (CVE) was addressed in this
release:

* Fixes CVE-2019-9506: The Bluetooth BR/EDR specification up to and
  including version 5.1 permits sufficiently low encryption key length
  and does not prevent an attacker from influencing the key length
  negotiation. This allows practical brute-force attacks (aka "KNOB")
  that can decrypt traffic and inject arbitrary ciphertext without the
  victim noticing.
