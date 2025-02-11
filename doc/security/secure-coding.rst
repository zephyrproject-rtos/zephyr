.. _secure code:

Secure Coding
#############

Traditionally, microcontroller-based systems have not placed much
emphasis on security.
They have usually been thought of as isolated, disconnected
from the world, and not very vulnerable, just because of the
difficulty in accessing them.  The Internet of Things has changed
this.  Now, code running on small microcontrollers often has access to
the internet, or at least to other devices (that may themselves have
vulnerabilities).  Given the volume they are often deployed at,
uncontrolled access can be devastating [#attackf]_.

This document describes the requirements and process for ensuring
security is addressed within the Zephyr project.  All code submitted
should comply with these principles.

Much of this document comes from [CIIBPB]_.

Introduction and Scope
**********************

This document covers guidelines for the `Zephyr Project`_, from a
security perspective.  Many of the ideas contained herein are captured
from other open source efforts.

.. todo: Reference main document here

.. _Zephyr Project: https://www.zephyrproject.org/

We begin with an overview of secure design as it relates to
Zephyr.  This is followed by
a section on `Secure development knowledge`_, which
gives basic requirements that a developer working on the project will
need to have.  This section gives references to other security
documents, and full details of how to write secure software are beyond
the scope of this document.  This section also describes
vulnerability knowledge that at least one of the primary developers
should have.  This knowledge will be necessary for the review process
described below this.

Following this is a description of the review process used to
incorporate changes into the Zephyr codebase.  This is followed by
documentation about how security-sensitive issues are handled by the
project.

Finally, the document covers how changes are to be made to this
document.

Secure Coding
*************

Designing an open software system such as Zephyr to be secure requires
adhering to a defined set of design standards. In [SALT75]_, the following,
widely accepted principles for protection mechanisms are defined to
help prevent security violations and limit their impact:

- **Open design** as a design guideline incorporates the maxim that
  protection mechanisms cannot be kept secret on any system in
  widespread use. Instead of relying on secret, custom-tailored
  security measures, publicly accepted cryptographic algorithms and
  well established cryptographic libraries shall be used.

- **Economy of mechanism** specifies that the underlying design of a
  system shall be kept as simple and small as possible. In the context
  of the Zephyr project, this can be realized, e.g., by modular code
  [PAUL09]_ and abstracted APIs.

- **Complete mediation** requires that each access to every object and
  process needs to be authenticated first. Mechanisms to store access
  conditions shall be avoided if possible.

- **Fail-safe defaults** defines that access is restricted by default
  and permitted only in specific conditions defined by the system
  protection scheme, e.g., after successful authentication.
  Furthermore, default settings for services shall be chosen in a way
  to provide maximum security.  This corresponds to the "Secure by
  Default" paradigm [MS12]_.

- **Separation of privilege** is the principle that two conditions or
  more need to be satisfied before access is granted. In the context
  of the Zephyr project, this could encompass split keys [PAUL09]_.

- **Least privilege** describes an access model in which each user,
  program, and thread, shall have the smallest possible subset
  of permissions in the system required to perform their task. This
  positive security model aims to minimize the attack surface of the
  system.

- **Least common mechanism** specifies that mechanisms common to more
  than one user or process shall not be shared if not strictly
  required. The example given in [SALT75]_ is a function that should be
  implemented as a shared library executed by each user and not as a
  supervisor procedure shared by all users.

- **Psychological acceptability** requires that security features are
  easy to use by the developers in order to ensure their usage and the
  correctness of its application.

In addition to these general principles, the following points are
specific to the development of a secure RTOS:

- **Complementary Security/Defense in Depth**: do not rely on a single
  threat mitigation approach. In case of the complementary security
  approach, parts of the threat mitigation are performed by the
  underlying platform. In case such mechanisms are not provided by the
  platform, or are not trusted, a defense in depth [MS12]_ paradigm
  shall be used.

- **Less commonly used services off by default**: to reduce the
  exposure of the system to potential attacks, features or services
  shall not be enabled by default if they are only rarely used (a
  threshold of 80% is given in [MS12]_). For the Zephyr project, this can
  be realized using the configuration management. Each functionality
  and module shall be represented as a configuration option and needs
  to be explicitly enabled. Then, all features, protocols, and drivers
  not required for a particular use case can be disabled. The user
  shall be notified if low-level options and APIs are enabled but not
  used by the application.

- **Change management**: to guarantee a traceability of changes to the
  system, each change shall follow a specified process including a
  change request, impact analysis, ratification, implementation, and
  validation phase. In each stage, appropriate documentation shall be
  provided. All commits shall be related to a bug report or change
  request in the issue tracker. Commits without a valid reference
  shall be denied.

Secure development knowledge
****************************

Secure designer
===============

The Zephyr project must have at least one primary developer who knows
how to design secure software.

This requires understanding the following design principles,
including the 8 principles from [SALT75]_:

- economy of mechanism (keep the design as simple and small as
  practical, e.g., by adopting sweeping simplifications)

- fail-safe defaults (access decisions shall deny by default, and
  projects' installation shall be secure by default)

- complete mediation (every access that might be limited must be
  checked for authority and be non-bypassable)

.. todo: Explain better the constraints of embedded devices, and that
   we typically do edge detection, not at each function. Perhaps
   relate this to input validation below.

- open design (security mechanisms should not depend on attacker
  ignorance of its design, but instead on more easily protected and
  changed information like keys and passwords)

- separation of privilege (ideally, access to important objects should
  depend on more than one condition, so that defeating one protection
  system won't enable complete access. For example, multi-factor
  authentication, such as requiring both a password and a hardware
  token, is stronger than single-factor authentication)

- least privilege (processes should operate with the least privilege
  necessary)

- least common mechanism (the design should minimize the mechanisms
  common to more than one user and depended on by all users, e.g.,
  directories for temporary files)

- psychological acceptability (the human interface must be designed
  for ease of use - designing for "least astonishment" can help)

- limited attack surface (the set of the
  different points where an attacker can try to enter or extract data)

- input validation with whitelists (inputs should typically be checked
  to determine if they are valid before they are accepted; this
  validation should use whitelists (which only accept known-good
  values), not blacklists (which attempt to list known-bad values)).

Vulnerability Knowledge
=======================

A "primary developer" in a project is anyone who is familiar with the
project's code base, is comfortable making changes to it, and is
acknowledged as such by most other participants in the project. A
primary developer would typically make a number of contributions over
the past year (via code, documentation, or answering questions).
Developers would typically be considered primary developers if they
initiated the project (and have not left the project more than three
years ago), have the option of receiving information on a private
vulnerability reporting channel (if there is one), can accept commits
on behalf of the project, or perform final releases of the project
software. If there is only one developer, that individual is the
primary developer.

At least one of the primary developers **must** know of common kinds of
errors that lead to vulnerabilities in this kind of software, as well
as at least one method to counter or mitigate each of them.

Examples (depending on the type of software) include SQL
injection, OS injection, classic buffer overflow, cross-site
scripting, missing authentication, and missing authorization. See the
`CWE/SANS top 25`_ or `OWASP Top 10`_ for commonly used lists.

A free class from the nonprofit OpenSecurityTraining2 for C/C++ developers
is available at `OST2_1001`_. It teaches how to prevent, detect, and
mitigate linear stack/heap buffer overflows, non-linear out of bound writes,
integer overflows, and other integer issues. The follow-on class, `OST2_1002`_,
covers uninitialized data access, race conditions, use-after-free, type confusion,
and information disclosure vulnerabilities.

.. Turn this into something specific. Can we find examples of
   mistakes.  Perhaps an example of things static analysis tool has sent us.

.. _CWE/SANS top 25: http://cwe.mitre.org/top25/

.. _OWASP Top 10: https://owasp.org/www-project-top-ten/

.. _OST2_1001: https://ost2.fyi/Vulns1001

.. _OST2_1002: https://ost2.fyi/Vulns1002

Zephyr Security Subcommittee
============================

There shall be a "Zephyr Security Subcommittee", responsible for
enforcing this guideline, monitoring reviews, and improving these
guidelines.

This team will be established according to the Zephyr Project charter.

Code Review
***********

The Zephyr project shall use a code review system that all changes are
required to go through.  Each change shall be reviewed by at least one
primary developer that is not the author of the change.  This
developer shall determine if this change affects the security of the
system (based on their general understanding of security), and if so,
shall request the developer with vulnerability knowledge, or the
secure designer to also review the code.  Any of these individuals
shall have the ability to block the change from being merged into the
mainline code until the security issues have been addressed.

Issues and Bug Tracking
***********************

The Zephyr project shall have an issue tracking system (such as GitHub_)
that can be used to record and track defects that are found in the
system.

.. _GitHub: https://www.github.com

Because security issues are often sensitive, this issue tracking
system shall have a field to indicate a security issue.  Setting this
field shall result in the issue only being visible to the Zephyr Security
Subcommittee. In addition, there shall be a
field to allow the Zephyr Security Subcommittee to add additional users that will
have visibility to a given issue.

This embargo, or limited visibility, shall only be for a fixed
duration, with a default being a project-decided value.  However,
because security considerations are often external to the Zephyr
project itself, it may be necessary to increase this embargo time.
The time necessary shall be clearly annotated in the issue itself.

The list of issues shall be reviewed at least once a month by the
Zephyr Security Subcommittee.  This review should focus on
tracking the fixes, determining if any external parties need to be
notified or involved, and determining when to lift the embargo on the
issue.  The embargo should **not** be lifted via an automated means, but
the review team should avoid unnecessary delay in lifting issues that
have been resolved.

Modifications to This Document
******************************

Changes to this document shall be reviewed by the Zephyr Security Subcommittee,
and approved by consensus.

.. [#attackf]  An attack_ resulted in a significant portion of DNS
   infrastructure being taken down.

.. _attack: http://www.theverge.com/2016/10/21/13362354/dyn-dns-ddos-attack-cause-outage-status-explained
