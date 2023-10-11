.. _bin-blobs:

Binary Blobs
************

In the context of an operating system that supports multiple architectures and
many different IC families, some functionality may be unavailable without the
help of executable code distributed in binary form.  Binary blobs (or blobs for
short) are files containing proprietary machine code or data in a binary format,
e.g. without corresponding source code released under an OSI approved license.

Zephyr supports downloading and using third-party binary blobs via its built-in
mechanisms, with some important caveats, described in the following sections. It
is important to note that all the information in this section applies only to
`upstream (vanilla) Zephyr <https://github.com/zephyrproject-rtos/zephyr>`_.

There are no limitations whatsoever (except perhaps license compatibility) in
the support for binary blobs in forks or third-party distributions of Zephyr. In
fact, Zephyr’s build system supports arbitrary use cases related to blobs. This
includes linking against libraries, flashing images to targets, etc. Users are
therefore free to create Zephyr-based downstream software which uses binary
blobs if they cannot meet the requirements described in this page.

Software license
================

Most binary blobs are distributed under proprietary licenses which vary
significantly in nature and conditions. It is up to the vendor to specify the
license as part of the blob submission process. Blob vendors may impose a
click-through or other EULA-like workflow when users fetch and install blobs.

Hosting
=======

Blobs must be hosted on the Internet and managed by third-party infrastructure.
Two potential examples are Git repositories and web servers managed by
individual hardware vendors.

The Zephyr Project does not host binary blobs in its Git repositories or
anywhere else.

Fetching blobs
==============

Blobs are fetched from official third-party sources by the :ref:`west blobs
command <west-blobs>` command.

The blobs themselves must be specified in the :ref:`module.yml
<modules-bin-blobs>` files included in separate Zephyr :ref:`module repositories
<modules>` maintained by their respective vendors.  This means that in order to
include a reference to a binary blob to the upstream Zephyr distribution, a
module repository must exist first or be created as part of the submission
process.

Each blob which may be fetched must be individually identified in the
corresponding :file:`module.yml` file. A specification for a blob must contain:

- An abstract description of the blob itself
- Version information
- A reference to vendor-provided documentation
- The blob’s :ref:`type <bin-blobs-types>`, which must be one of the allowed types
- A checksum for the blob, which ``west blobs`` checks after downloading.
  This is required for reproducibility and to allow bisecting issues as blobs
  change using Git and west
- License text applicable to the blob or a reference to such text, in SPDX
  format

See the :ref:`corresponding section <modules-bin-blobs>` for a more formal
definition of the fields.

The :ref:`west blobs <west-blobs>` command can be used to list metadata of
available blobs and to fetch blobs from user-selected modules.

The ``west blobs`` command only fetches and stores the binary blobs themselves.
Any accompanying code, including interface header files for the blobs, must be
present in the corresponding module repository.

Tainting
========

Inclusion of binary blobs will taint the Zephyr build. The definition of
tainting originates in the `Linux kernel
<https://www.kernel.org/doc/html/latest/admin-guide/tainted-kernels.html>`_ and,
in the context of Zephyr, a tainted image will be one that includes binary blobs
in it.

Tainting will be communicated to the user in the following manners:

- One or more Kconfig options ``TAINT_BLOBS_*`` will be set to ``y``
- The Zephyr build system, during its configuration phase, will issue a warning.
  It will be possible to disable the warning using Kconfig
- The ``west spdx`` command will include the tainted status in its output
- The kernel's default fatal error handler will also explicitly print out the
  kernel's tainted status

.. _bin-blobs-types:

Allowed types
=============

The following binary blob types are acceptable in Zephyr:

* Precompiled libraries: Hardware enablement libraries, distributed in
  precompiled binary form, typically for SoC peripherals. An example could be an
  enablement library for a wireless peripheral
* Firmware images: An image containing the executable code for a secondary
  processor or CPU.  This can be full or partial (typically delta or patch data)
  and is generally copied into RAM or flash memory by the main CPU. An example
  could be the firmware for the core running a Bluetooth LE Controller
* Miscellaneous binary data files. An example could be pre-trained neural
  network model data

Hardware agnostic features provided via a proprietary library are not
acceptable. For example, a proprietary and hardware agnostic TCP/IP stack
distributed as a static archive would be rejected.

Note that just because a blob has an acceptable type does not imply that it will
be unconditionally accepted by the project; any blob may be rejected for other
reasons on a case by case basis (see library-specific requirements below).
In case of disagreement, the TSC is the arbiter of whether a particular blob
fits in one of the above types.

Precompiled library-specific requirements
=========================================

This section contains additional requirements specific to precompiled library
blobs.

Any person who wishes to submit a precompiled library must represent that it
meets these requirements. The project may remove a blob from the upstream
distribution if it is discovered that the blob fails to meet these requirements
later on.

Interface header files
----------------------

The precompiled library must be accompanied by one or more header files,
distributed under a non-copyleft OSI approved license, that define the interface
to the library.

Allowed dependencies
--------------------

This section defines requirements related to external symbols that a library
blob requires the build system to provide.

* The blob must not depend on Zephyr APIs directly. In other words, it must have
  been possible to build the binary without any Zephyr source code present at
  all. This is required for loose coupling and maintainability, since Zephyr
  APIs may change and such blobs cannot be modified by all project maintainers
* Instead, if the code in the precompiled library requires functionality
  provided by Zephyr (or an RTOS in general), an implementation of an OS
  abstraction layer (aka porting layer) can be provided alongside the library.
  The implementation of this OS abstraction layer must be in source code form,
  released under an OSI approved license and documented using Doxygen

Toolchain requirements
----------------------

Precompiled library blobs must be in a data format which is compatible with and
can be linked by a toolchain supported by the Zephyr Project.  This is required
for maintainability and usability. Use of such libraries may require special
compiler and/or linker flags, however. For example, a porting layer may require
special flags, or a static archive may require use of specific linker flags.

Limited scope
-------------

Allowing arbitrary library blobs carries a risk of degrading the degree to
which the upstream Zephyr software distribution is open source. As an extreme
example, a target with a zephyr kernel clock driver that is just a porting layer
around a library blob would not be bootable with open source software.

To mitigate this risk, the scope of upstream library blobs is limited. The
project maintainers define an open source test suite that an upstream
target must be able to pass using only open source software included in the
mainline distribution and its modules. The open source test suite currently
consists of:

- :file:`samples/philosophers`
- :file:`tests/kernel`

The scope of this test suite may grow over time. The goal is to specify
tests for a minimal feature set which must be supported via open source software
for any target with upstream Zephyr support.

At the discretion of the release team, the project may remove support for a
hardware target if it cannot pass this test suite.

Support and maintenance
=======================

The Zephyr Project is not expected to be responsible for the maintenance and
support of contributed binary blobs. As a consequence, at the discretion of the
Zephyr Project release team, and on a case-by-case basis:

- GitHub issues reported on the zephyr repository tracker that require use of
  blobs to reproduce may not be treated as bugs
- Such issues may be closed as out of scope of the Zephyr project

This does not imply that issues which require blobs to reproduce will be closed
without investigation. For example, the issue may be exposing a bug in a Zephyr
code path that is difficult or impossible to trigger without a blob. Project
maintainers may accept and attempt to resolve such issues.

However, some flexibility is required because project maintainers may not be
able to determine if a given issue is due to a bug in Zephyr or the blob itself,
may be unable to reproduce the bug due to lack of hardware, etc.

Blobs must have designated maintainers that must be responsive to issue reports
from users and provide updates to the blobs to address issues. At the discretion
of the Zephyr Project release team, module revisions referencing blobs may be
removed from :file:`zephyr/west.yml` at any time due to lack of responsiveness or
support from their maintainers. This is required to maintain project control
over bit-rot, security issues, etc.

The submitter of the proposal to integrate a binary blob must commit to maintain
the integration of such blob for the foreseeable future.

Regarding Continuous Integration, binary blobs will **not** be fetched in the
project's CI infrastructure that builds and optionally executes tests and samples
to prevent regressions and issues from entering the codebase. This includes
both CI ran when a new GitHub Pull Request is opened as well as any other
regularly scheduled execution of the CI infrastructure.

.. _blobs-process:

Submission and review process
=============================

For references to binary blobs to be included in the project, they must be
reviewed and accepted by the Technical Steering Committee (TSC). This process is
only required for new binary blobs, updates to binary blobs follow the
:ref:`module update procedure <modules_changes>`.

A request for integration with binary blobs must be made by creating a new
issue in the Zephyr project issue tracking system on GitHub with details
about the blobs and the functionality they provide to the project.

Follow the steps below to begin the submission process:

#. Make sure to read through the :ref:`bin-blobs` section in
   detail, so that you are informed of the criteria used by the TSC in order to
   approve or reject a request
#. Use the :github:`New Binary Blobs Issue
   <new?assignees=&labels=RFC&template=008_bin-blobs.md&title=>` to open an issue
#. Fill out all required sections, making sure you provide enough detail for the
   TSC to assess the merit of the request. Additionally you must also create a Pull
   Request that demonstrates the integration of the binary blobs and then
   link to it from the issue
#. Wait for feedback from the TSC, respond to any additional questions added as
   GitHub issue comments

If, after consideration by the TSC, the submission of the binary blob(s) is
approved, the submission process is complete and the binary blob(s) can be
integrated.
