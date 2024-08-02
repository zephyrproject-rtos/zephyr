.. _reporting:

Security Vulnerability Reporting
################################

Introduction
============

Vulnerabilities to the Zephyr project may be reported via email to the
vulnerabilities@zephyrproject.org mailing list.  These reports will be
acknowledged and analyzed by the security response team within 1 week.
Each vulnerability will be entered into the Zephyr Project security
advisory GitHub_.  The original submitter will be granted permission to
view the issues that they have reported.

.. _GitHub: https://github.com/zephyrproject-rtos/zephyr/security

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
