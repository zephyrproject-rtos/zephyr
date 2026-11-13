.. _reporting:

Security Vulnerability Reporting
################################

Introduction
============

Vulnerabilities in the Zephyr project should preferably be reported by opening
a draft `security advisory`_ on the Zephyr repository's
`security advisories page`_.
Alternatively, reports may be submitted to the project security incident
response team via email to vulnerabilities@zephyrproject.org.
These reports will be acknowledged and analyzed by the security response team
within 1 week.

All vulnerabilities are tracked and managed through the Zephyr project's
`security advisories page`_ on GitHub, where the original submitter will be
granted access to view the issues that they have reported.

.. _security advisory: https://docs.github.com/en/code-security/security-advisories/guidance-on-reporting-and-writing/privately-reporting-a-security-vulnerability#privately-reporting-a-security-vulnerability
.. _security advisories page: https://github.com/zephyrproject-rtos/zephyr/security/advisories

.. _vulnerability_scope:

Vulnerability Scope
===================

Not every defect that can crash or corrupt a Zephyr system is a security
vulnerability.  The security response team treats a report as a
vulnerability when a released version of Zephyr lets an attacker cross a
trust boundary that Zephyr is designed to enforce.  Reports that do not
meet this definition are still welcome, but they are handled as regular
bugs through the public `issue tracker`_: no CVE is assigned and no
embargo applies.

.. _issue tracker: https://github.com/zephyrproject-rtos/zephyr/issues

Trust model
-----------

Zephyr is one component of a product, and the product integrator is
responsible for the security of the system as a whole.  Unless a
Zephyr feature explicitly documents otherwise, the following are
trusted.  A report in which the attacker is one of them describes a
bug, not a vulnerability:

- The application and all code linked into the firmware image.  The
  kernel, drivers, subsystems and the application share a single trust
  domain.  Passing invalid arguments to an API, misusing an API, or
  corrupting memory from code running in supervisor mode is a bug in
  that code; the API is not expected to defend against its callers.

- Everything supplied at build time: Kconfig options, devicetree,
  linker scripts, generated files and the toolchain.

- The hardware the image runs on: the SoC, memory-mapped peripherals,
  and devices attached to the board over buses such as I2C, SPI, UART,
  SDIO or PCIe, including sensors, displays, flash and Ethernet
  PHYs.  A device that returns malformed register contents, bogus
  lengths or breaks its bus protocol is a faulty or malicious part of
  the product.  Zephyr drivers are not hardened against it, and a crash
  or memory corruption caused by such a device is a robustness bug.

- Co-processors and companion firmware that are part of the same
  product and that Zephyr talks to over a local transport, such as a
  Bluetooth controller behind HCI, a cellular modem behind an AT or
  CMUX link, or a remote core reached through IPC.  A rogue Bluetooth
  controller sending malformed HCI events, for instance, is a defect of
  the controller, not a vulnerability in the host stack.  Data these
  components relay from outside the device, such as IP packets from a
  modem or packets from a remote Bluetooth peer, remains untrusted.

- Parties with unrestricted access through debug ports or a privileged
  console or shell, and attackers who bypass the assumed hardware
  protections through fault injection, glitching or hardware side-channel
  attacks.  Physical access alone does not make a party trusted: peers on
  exposed communication interfaces remain subject to the rules below.

The following are not trusted.  A defect that an attacker can trigger
from one of these positions is a vulnerability:

- Peers on communication interfaces that leave the device: IP
  networking, Bluetooth and other wireless links, IEEE 802.15.4, the
  USB host when Zephyr runs as a USB device, and comparable interfaces.
  This includes data received over such an interface and then processed
  later, for example a downloaded file or a received firmware image.

- Threads running in user mode when :ref:`user mode <usermode_api>` is
  enabled.  The kernel and system call handlers must validate every
  argument that comes from user mode, and a user thread gaining
  supervisor privileges, or reading or writing memory in violation of its
  configured access permissions, is a vulnerability.  See the
  :ref:`user mode threat model <usermode_overview>` for the protections
  provided and their limitations.

Some interfaces are trusted in one product and exposed to attackers in
another: a file system on removable media, a USB device attached to a
Zephyr USB host, or data read from an external bus that is physically
accessible.  When a report depends on such an interface, say so and
describe the configuration.  The security response team decides these
cases individually.

Defects in unreleased code
--------------------------

The vulnerability process exists to protect users of released
software.  A defect introduced on the ``main`` branch after the most
recent release, and therefore not present in any tagged release, is a
regular bug: report it in the public `issue tracker`_ following the
:ref:`bug reporting <bug_reporting>` guidelines, where it is fixed
through the normal pull request process.  No CVE is assigned and no
embargo applies, whatever the severity of the defect.

A defect that is present in a tagged release is a vulnerability even
if the code has since been changed or removed on ``main``.

If you are unsure whether the affected code was ever released, report
through the security channels and say so.  The security response team
checks the release history, and when the code was never released it
rejects the advisory and reproduces the report as a public issue, or
asks the reporter to do so.

Other reports that are not vulnerabilities
------------------------------------------

The following are also handled outside the vulnerability process:

- Missing hardening or security features, such as a Kconfig option
  whose default is not the most secure choice, or a mitigation Zephyr
  does not implement.  These are enhancement requests, unless a feature
  Zephyr documents as a security mechanism does not work as described.

- Defects that can only be triggered by a trusted party, for example
  resource exhaustion or a deadlock caused by the application's own
  misuse of a kernel object.

- Defects in a function that lacks input validation when no untrusted
  input reaches that function.  A report must show a path from an
  untrusted position, as described above, to the defect.

- Defects originating in upstream code that Zephyr imports from
  third-party projects, such as the HAL modules, Mbed TLS or other
  repositories listed in ``west.yml``.  Report these to the upstream
  project's security contact.  The security response team tracks the
  integration of the upstream fix into Zephyr.  Vulnerabilities introduced
  by Zephyr-specific changes to imported code remain within Zephyr's
  vulnerability process.  If the origin is uncertain, report privately
  through Zephyr's security channels so the response team can determine
  responsibility.

- Defects that affect only releases no longer listed as supported in
  the project `security policy`_.  These releases do not receive fixes;
  users are expected to upgrade.

.. _security policy: https://github.com/zephyrproject-rtos/zephyr/blob/main/.github/SECURITY.md

Vulnerability Reporting Guidelines
==================================

Security research and vulnerability reporting are valued contributions to the
Zephyr project. To keep this process safe and productive for everyone, all
reports and related activities must comply with the Zephyr project
`Code of Conduct`_.

The guidelines below describe what is expected when researching and reporting
security vulnerabilities in Zephyr.

.. _Code of Conduct: https://github.com/zephyrproject-rtos/zephyr/blob/main/CODE_OF_CONDUCT.md

Reporting expectations
----------------------

To help the security response team triage and resolve issues efficiently, a
report should:

- Describe the affected component, version, board or platform, and
  configuration needed to reproduce the issue.

- Include clear, step-by-step reproduction instructions and a minimal
  proof-of-concept where possible.

- Explain the security impact (for example, loss of confidentiality,
  integrity, or availability), who the attacker is and which trust
  boundary described in :ref:`vulnerability_scope` is crossed, and any
  known mitigations.

- Avoid including destructive payloads or components that could cause harm to
  users or systems if the report were inadvertently disclosed.

Reporters are expected to make a good-faith effort to give the project a
reasonable opportunity to investigate and remediate the issue before any public
disclosure, consistent with the embargo and disclosure process described in the
following sections.

Security Issue Management
=========================

Issues within this bug tracking system will transition through a
number of states according to this diagram:

.. graphviz::

   digraph {
      node [style = rounded];
      init [shape = point];
      New [shape = box];
      Triage [shape = box];
      {
        rank = same;
        rankdir = LR;
        Assigned [shape = box];
        Rejected [shape = box];
      }
      Review [shape = box];
      Accepted [shape = box];
      Public [shape = box];

      init -> New;
      New -> Triage;
      Triage -> Rejected [dir = both];
      Triage -> Assigned;
      Assigned -> Review [dir = both];
      Review -> Accepted;
      Review -> Rejected;
      Accepted -> Public;

   }

- New: This state represents new reports that have been entered
  directly by a reporter.  When entered by the response team in
  response to an email, the issue shall be transitioned directly to
  Triage.

- Triage: This issue is awaiting Triage by the response team.  The
  response team will analyze the issue, determine a responsible
  entity, assign it to that individual, and move the
  issue to the Assigned state.  Part of triage will be to set the
  issue's priority.

- Assigned: The issue has been assigned, and is awaiting a fix by the
  assignee.

- Rejected: The report is not a vulnerability as defined in
  :ref:`vulnerability_scope`, or could not be reproduced.  When the
  report describes a bug, it is reproduced in the public issue tracker.

- Review: Once there is a Zephyr pull request for the issue, the PR
  link will be added to a comment in the issue, and the issue moved to
  the Review state.

- Accepted: Indicates that this issue has been merged into the
  appropriate branch within Zephyr.

- Public: The embargo period has ended.  The issue will be made
  publicly visible, the associated CVE updated, and the
  vulnerabilities page in the docs updated to include the detailed
  information.

The security advisories created are kept private, due to the
sensitive nature of security reports.  The issues are only visible to
certain parties:

- Members of the PSIRT mailing list

- the reporter

- others, as proposed and ratified by the Zephyr Security
  Subcommittee.  In the general case, this will include:

  - The code owner responsible for the fix.

  - The Zephyr release owners for the relevant releases affected by
    this vulnerability.

The Zephyr Security Subcommittee shall review the reported
vulnerabilities during any meeting with more than three people in
attendance.  During this review, they shall determine if new issues
need to be embargoed.

The guideline for embargo will be based on: 1. Severity of the issue,
and 2. Exploitability of the issue.  Issues that the subcommittee
decides do not need an embargo will be reproduced in the regular
Zephyr project bug tracking system.

.. _vulnerability_timeline:

Security sensitive vulnerabilities shall be made public after an
embargo period of at most 90 days.  The intent is to allow 30 days
within the Zephyr project to fix the issues, and 60 days for external
parties building products using Zephyr to be able to apply and
distribute these fixes.

.. _vulnerability_fix_recommendations:

Fixes to the code shall be made through pull requests PR in the Zephyr
project github.  Developers shall make an attempt to not reveal the
sensitive nature of what is being fixed, and shall not refer to CVE
numbers that have been assigned to the issue.  The developer instead
should merely describe what has been fixed.

The security subcommittee will maintain information mapping embargoed
CVEs to these PRs (this information is within the Github security
advisories), and produce regular reports of the state of security
issues.

Each issue that is considered a security vulnerability shall be
assigned a CVE number.  As fixes are created, it may be necessary to
allocate additional CVE numbers, or to retire numbers that were
assigned.

Vulnerability Notification
==========================

Each Zephyr release shall contain a report of CVEs that were fixed in
that release.  Because of the sensitive nature of these
vulnerabilities, the release shall merely include a list of CVEs that
have been fixed.  After the embargo period, the vulnerabilities page
shall be updated to include additional details of these
vulnerabilities.  The vulnerability page shall give credit to the
reporter(s) unless a reporter specifically requests anonymity.

The Zephyr project shall maintain a vulnerability-alerts mailing list.
This list will be seeded initially with a contact from each project
member.  Additional parties can request to join this list by filling
out the form at the `Vulnerability Registry`_.  These parties will be
vetted by the project director to determine that they have a
legitimate interest in knowing about security vulnerabilities during
the embargo period.

.. _Vulnerability Registry: https://www.zephyrproject.org/vulnerability-registry/ 

Periodically, the security subcommittee will send information to this
mailing list describing known embargoed issues, and their backport
status within the project.  This information is intended to allow them
to determine if they need to backport these changes to any internal
trees.

When issues have been triaged, this list will be informed of:

- The Zephyr Project security advisory link (GitHub).

- The CVE number assigned.

- The subsystem involved.

- The severity of the issue.

After acceptance of a PR fixing the issue (merged), in addition to the
above, the list will be informed of:

- The association between the CVE number and the PR fixing it.

- Backport plans within the Zephyr project.

Backporting of Security Vulnerabilities
=======================================

Each security issue fixed within zephyr shall be backported to the
following releases:

- The current Long Term Stable (LTS) release.

- The most recent two releases.

The developer of the fix shall be responsible for any necessary
backports, and apply them to any of the above listed release branches,
unless the fix does not apply (the vulnerability was introduced after
this release was made). All recommendations for
:ref:`vulnerability fixes <vulnerability_fix_recommendations>` apply
for backport pull requests (and associated issues). Additionally, it is
recommended that the developer privately informs the responsible
release manager that the backport pull request and issue are addressing
a vulnerability.

Backports will be tracked on the security advisory.

Need to Know
============

Due to the sensitive nature of security vulnerabilities, it is
important to share details and fixes only with those parties that have
a need to know.  The following parties will need to know details about
security vulnerabilities before the embargo period ends:

- Maintainers will have access to all information within their domain
  area only.

- The current release manager, and the release manager for historical
  releases affected by the vulnerability (see backporting above).

- The Project Security Incident Response (PSIRT) team will have full
  access to information.  The PSIRT is made up of representatives from
  platinum members, and volunteers who do work on triage from other
  members.

- As needed, release managers and maintainers may be invited to attend
  additional security meetings to discuss vulnerabilities.
