.. _contribute_guidelines:

Contribution Guidelines
#######################

As an open-source project, we welcome and encourage the community to submit
patches directly to the project.  In our collaborative open source environment,
standards and methods for submitting changes help reduce the chaos that can result
from an active development community.

This document explains how to participate in project conversations, log bugs
and enhancement requests, and submit patches to the project so your patch will
be accepted quickly in the codebase.

.. _licensing_requirements:

Licensing
*********

Licensing is very important to open source projects. It helps ensure the
software continues to be available under the terms that the author desired.

.. _Apache 2.0 license:
   https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE

.. _GitHub repo: https://github.com/zephyrproject-rtos/zephyr

Zephyr uses the `Apache 2.0 license`_ (as found in the LICENSE file in
the project's `GitHub repo`_) to strike a balance between open
contribution and allowing you to use the software however you would like
to.  The Apache 2.0 license is a permissive open source license that
allows you to freely use, modify, distribute and sell your own products
that include Apache 2.0 licensed software.  (For more information about
this, check out articles such as `Why choose Apache 2.0 licensing`_ and
`Top 10 Apache License Questions Answered`_).

.. _Why choose Apache 2.0 licensing:
   https://www.zephyrproject.org/faqs/#1571346989065-9216c551-f523

.. _Top 10 Apache License Questions Answered:
   https://www.whitesourcesoftware.com/whitesource-blog/top-10-apache-license-questions-answered/

A license tells you what rights you have as a developer, as provided by the
copyright holder. It is important that the contributor fully understands the
licensing rights and agrees to them. Sometimes the copyright holder isn't the
contributor, such as when the contributor is doing work on behalf of a
company.

Components using other Licenses
===============================

There are some imported or reused components of the Zephyr project that
use other licensing, as described in :ref:`Zephyr_Licensing`.

Importing code into the Zephyr OS from other projects that use a license
other than the Apache 2.0 license needs to be fully understood in
context and approved by the Zephyr governing board.

By carefully reviewing potential contributions and also enforcing a
:ref:`DCO` for contributed code, we can ensure that
the Zephyr community can develop products with the Zephyr Project
without concerns over patent or copyright issues.

See :ref:`external-contributions` for more information about
this contributing and review process for imported components.

.. only:: latex

   .. toctree::
      :maxdepth: 1

      ../LICENSING.rst

.. _copyrights:

Copyrights Notices
*******************

Please follow this `Community Best Practice`_ for Copyright Notices from the
Linux Foundation.


.. _Community Best Practice:
   https://www.linuxfoundation.org/blog/copyright-notices-in-open-source-software-projects/

.. _DCO:

Developer Certification of Origin (DCO)
***************************************

To make a good faith effort to ensure licensing criteria are met, the Zephyr
project requires the Developer Certificate of Origin (DCO) process to be
followed.

The DCO is an attestation attached to every contribution made by every
developer. In the commit message of the contribution, (described more fully
later in this document), the developer simply adds a ``Signed-off-by``
statement and thereby agrees to the DCO.

When a developer submits a patch, it is a commitment that the contributor has
the right to submit the patch per the license.  The DCO agreement is shown
below and at http://developercertificate.org/.

.. code-block:: none

    Developer's Certificate of Origin 1.1

    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I
        have the right to submit it under the open source license
        indicated in the file; or

    (b) The contribution is based upon previous work that, to the
        best of my knowledge, is covered under an appropriate open
        source license and I have the right under that license to
        submit that work with modifications, whether created in whole
        or in part by me, under the same open source license (unless
        I am permitted to submit under a different license), as
        Indicated in the file; or

    (c) The contribution was provided directly to me by some other
        person who certified (a), (b) or (c) and I have not modified
        it.

    (d) I understand and agree that this project and the contribution
        are public and that a record of the contribution (including
        all personal information I submit with it, including my
        sign-off) is maintained indefinitely and may be redistributed
        consistent with this project or the open source license(s)
        involved.

DCO Sign-Off
============

The "sign-off" in the DCO is a "Signed-off-by:" line in each commit's log
message. The Signed-off-by: line must be in the following format::

   Signed-off-by: Your Name <your.email@example.com>

For your commits, replace:

- ``Your Name`` with your legal name (pseudonyms, hacker handles, and the
  names of groups are not allowed)

- ``your.email@example.com`` with the same email address you are using to
  author the commit (CI will fail if there is no match)

You can automatically add the Signed-off-by: line to your commit body using
``git commit -s``. Use other commits in the zephyr git history as examples.

Additional requirements:

- If you are altering an existing commit created by someone else, you must add
  your Signed-off-by: line without removing the existing one.

- If you forget to add the Signed-off-by: line, you can add it to your previous
  commit by running ``git commit --amend -s``.

- If you've pushed your changes to GitHub already you'll need to force push
  your branch after this with ``git push -f``.

Notes
=====

Any contributions made as part of submitted pull requests are considered free
for the Project to use. Developers are permitted to cherry-pick patches that
are included in pull requests submitted by other contributors. It is expected
that

* the content of the patches will not be substantially modified,
* the cherry-picked commits or portions of a commit shall preserve the original
  sign-off messages and the author identity.

:ref:`modifying_contributions` describes additional recommended policies
around working with contributions submitted by other developers.


Prerequisites
*************

.. _Zephyr Project website: https://zephyrproject.org

As a contributor, you'll want to be familiar with the Zephyr project, how to
configure, install, and use it as explained in the `Zephyr Project website`_
and how to set up your development environment as introduced in the Zephyr
:ref:`getting_started`.

You should be familiar with common developer tools such as Git and CMake, and
platforms such as GitHub.

If you haven't already done so, you'll need to create a (free) GitHub account
on https://github.com and have Git tools available on your development system.

.. note::
   The Zephyr development workflow supports all 3 major operating systems
   (Linux, macOS, and Windows) but some of the tools used in the sections below
   are only available on Linux and macOS. On Windows, instead of running these
   tools yourself, you will need to rely on the Continuous Integration (CI)
   service using Github Actions, which runs automatically on GitHub when you submit
   your Pull Request (PR).  You can see any failure results in the workflow
   details link near the end of the PR conversation list. See
   `Continuous Integration`_ for more information

.. _source_tree_v2:

Source Tree Structure
*********************

To clone the main Zephyr Project repository use the instructions in
:ref:`get_the_code`.

This section describes the main repository's source tree. In addition to the
Zephyr kernel itself, you'll also find the sources for technical documentation,
sample code, supported board configurations, and a collection of subsystem
tests.  All of these are available for developers to contribute to and enhance.

Understanding the Zephyr source tree can help locate the code
associated with a particular Zephyr feature.

At the top of the tree, several files are of importance:

:file:`CMakeLists.txt`
    The top-level file for the CMake build system, containing a lot of the
    logic required to build Zephyr.

:file:`Kconfig`
    The top-level Kconfig file, which refers to the file :file:`Kconfig.zephyr`
    also found in the top-level directory.

    See :ref:`the Kconfig section of the manual <kconfig>` for detailed Kconfig
    documentation.

:file:`west.yml`
    The :ref:`west` manifest, listing the external repositories managed by
    the west command-line tool.

The Zephyr source tree also contains the following top-level
directories, each of which may have one or more additional levels of
subdirectories not described here.

:file:`arch`
    Architecture-specific kernel and system-on-chip (SoC) code.
    Each supported architecture (for example, x86 and ARM)
    has its own subdirectory,
    which contains additional subdirectories for the following areas:

    * architecture-specific kernel source files
    * architecture-specific kernel include files for private APIs

:file:`soc`
    SoC related code and configuration files.

:file:`boards`
    Board related code and configuration files.

:file:`doc`
    Zephyr technical documentation source files and tools used to
    generate the https://docs.zephyrproject.org web content.

:file:`drivers`
    Device driver code.

:file:`dts`
    :ref:`devicetree <dt-guide>` source files used to describe non-discoverable
    board-specific hardware details.

:file:`include`
    Include files for all public APIs, except those defined under :file:`lib`.

:file:`kernel`
    Architecture-independent kernel code.

:file:`lib`
    Library code, including the minimal standard C library.

:file:`misc`
    Miscellaneous code that doesn't belong to any of the other top-level
    directories.

:file:`samples`
    Sample applications that demonstrate the use of Zephyr features.

:file:`scripts`
    Various programs and other files used to build and test Zephyr
    applications.

:file:`cmake`
    Additional build scripts needed to build Zephyr.

:file:`subsys`
    Subsystems of Zephyr, including:

    * USB device stack code
    * Networking code, including the Bluetooth stack and networking stacks
    * File system code
    * Bluetooth host and controller

:file:`tests`
    Test code and benchmarks for Zephyr features.

:file:`share`
    Additional architecture independent data. It currently contains Zephyr's CMake
    package.

Pull Requests and Issues
************************

.. _Zephyr Project Issues: https://github.com/zephyrproject-rtos/zephyr/issues

.. _open pull requests: https://github.com/zephyrproject-rtos/zephyr/pulls

.. _Zephyr devel mailing list: https://lists.zephyrproject.org/g/devel

.. _Zephyr Discord Server: https://chat.zephyrproject.org

Before starting on a patch, first check in our issues `Zephyr Project Issues`_
system to see what's been reported on the issue you'd like to address.  Have a
conversation on the `Zephyr devel mailing list`_ (or the `Zephyr Discord
Server`_) to see what others think of your issue (and proposed solution).  You
may find others that have encountered the issue you're finding, or that have
similar ideas for changes or additions.  Send a message to the `Zephyr devel
mailing list`_ to introduce and discuss your idea with the development
community.

It's always a good practice to search for existing or related issues before
submitting your own. When you submit an issue (bug or feature request), the
triage team will review and comment on the submission, typically within a few
business days.

You can find all `open pull requests`_ on GitHub and open `Zephyr Project
Issues`_ in Github issues.

 .. _Continuous Integration:


Tools and Git Setup
*******************

.. _git-name-and-email:

Name and email
==============

We need to know who you are, and how to contact you. To add this
information to your Git installation, set the Git configuration
variables ``user.name`` to your full name, and ``user.email`` to your
email address.

For example, if your name is ``Zephyr Developer`` and your email
address is ``z.developer@example.com``:

.. code-block:: console

   git config --global user.name "Zephyr Developer"
   git config --global user.email "z.developer@example.com"

gitlint
=========

When you submit a pull request to the project, a series of checks are
performed to verify your commit messages meet the requirements. The same step
done during the CI process can be performed locally using the ``gitlint``
command.

Run ``gitlint`` locally in your tree and branch where your patches have been
committed:

.. code-block:: console

     gitlint

Note, gitlint only checks HEAD (the most recent commit), so you should run it
after each commit, or use the ``--commits`` option to specify a commit range
covering all the development patches to be submitted.

twister
=======

.. note::
   twister support on windows is limited and execution of tests is not
   supported, only building.

To verify that your changes did not break any tests or samples, please run the
``twister`` script locally before submitting your pull request to GitHub.

Twister allows limiting the scope of the tests built and run by pointing it to
the tests related to the code or the platform you have modified. For example, to
limit tests to a single platform and an area in the kernel::

    source zephyr-env.sh
    west twister -p qemu_x86 -T tests/kernel/sched

Running tests on connected devices is also supported using the
``--device-testing`` options. Please consult with the :ref:`Twister
<twister_script>` documentation for more details.

To run the same tests the CI system runs, follow these steps from within your
local Zephyr source working directory:

.. code-block:: console

    source zephyr-env.sh
    west twister --integration

The above will execute the basic twister script, which will run various
tests using the QEMU emulator and other simulators supported in Zephyr.
It will also do some build tests on various samples with advanced features that
can't run in a simulator or QEMU.

We highly recommend you run these tests locally to avoid any CI failures
However, note that building and executing tests using twister requires
significant computing resources. When running locally and to get results in a
reasonable time, limit the scope to the areas and platforms you have modified.
In case of major changes to the kernel, build or configuration infrastructures
of Zephyr, it is advised to use twister for verifying majority the changes
before handing over to the dedicated CI resources provided by the Zephyr
project.

clang-format
============

The `clang-format tool <https://clang.llvm.org/docs/ClangFormat.html>`_ can
be helpful to quickly reformat large amounts of new source code to our
`Coding Style`_ standards together with the ``.clang-format`` configuration file
provided in the repository. ``clang-format`` is well integrated into most
editors, but you can also run it manually like this:

.. code-block:: bash

   clang-format -i my_source_file.c

``clang-format`` is part of LLVM, which can be downloaded from the project
`releases page <https://github.com/llvm/llvm-project/releases>`. Note that if
you are a Linux user, ``clang-format`` will likely be available as a package in
your distribution repositories.

.. _coding_style:

Coding Style
************

Use these coding guidelines to ensure that your development complies with the
project's style and naming conventions.

.. _Linux kernel coding style:
   https://kernel.org/doc/html/latest/process/coding-style.html

In general, follow the `Linux kernel coding style`_, with the following
exceptions:

* The line length is 100 columns or fewer. In the documentation, longer lines
  for URL references are an allowed exception.
* Add braces to every ``if``, ``else``, ``do``, ``while``, ``for`` and
  ``switch`` body, even for single-line code blocks. Use the ``--ignore BRACES``
  flag to make *checkpatch* stop complaining.
* Use spaces instead of tabs to align comments after declarations, as needed.
* Use C89-style single line comments, ``/*  */``. The C99-style single line
  comment, ``//``, is not allowed.
* Use ``/**  */`` for doxygen comments that need to appear in the documentation.
* Avoid using binary literals (constants starting with ``0b``).
* Avoid using non-ASCII symbols in code, unless it significantly improves
  clarity, avoid emojis in any case.

When there are differences between the guidelines above and the formatting
generated by code formatting tools, the guidelines above take precedence.

The Linux kernel GPL-licensed tool ``checkpatch`` is used to check
coding style conformity.

.. note::
   checkpatch does not currently run on Windows.

Checkpatch is available in the scripts directory. To invoke it when committing
code, make the file *$ZEPHYR_BASE/.git/hooks/pre-commit* executable and edit
it to contain:

.. code-block:: bash

    #!/bin/sh
    set -e exec
    exec git diff --cached | ${ZEPHYR_BASE}/scripts/checkpatch.pl -

Instead of running checkpatch at each commit, you may prefer to run it only
before pushing on zephyr repo. To do this, make the file
*$ZEPHYR_BASE/.git/hooks/pre-push* executable and edit it to contain:

.. code-block:: bash

    #!/bin/sh
    remote="$1"
    url="$2"

    z40=0000000000000000000000000000000000000000

    echo "Run push hook"

    while read local_ref local_sha remote_ref remote_sha
    do
        args="$remote $url $local_ref $local_sha $remote_ref $remote_sha"
        exec ${ZEPHYR_BASE}/scripts/series-push-hook.sh $args
    done

    exit 0

If you want to override checkpatch verdict and push you branch despite reported
issues, you can add option --no-verify to the git push command.

A more complete alternative to this is using check_compliance.py script from
ci-tools repo.

.. _static_analysis:

Static Code Analysis
********************

Coverity Scan is a free service for static code analysis of Open Source
projects. It is based on Coverity's commercial product and is able to analyze
C, C++ and Java code.

Coverity's static code analysis doesn't run the code. Instead of that it uses
abstract interpretation to gain information about the code's control flow and
data flow. It's able to follow all possible code paths that a program may take.
For example the analyzer understands that malloc() returns a memory that must
be freed with free() later. It follows all branches and function calls to see
if all possible combinations free the memory. The analyzer is able to detect
all sorts of issues like resource leaks (memory, file descriptors), NULL
dereferencing, use after free, unchecked return values, dead code, buffer
overflows, integer overflows, uninitialized variables, and many more.

The results are available on the `Coverity Scan
<https://scan.coverity.com/projects/zephyr>`_ website. In order to access the
results you have to create an account yourself.  From the Zephyr project page,
you may select "Add me to project" to be added to the project. New members must
be approved by an admin.

Static analysis of the Zephyr codebase is conducted on a bi-weekly basis. GitHub
issues are automatically created for any issues detected by static analysis
tools. These issues will have the same (or equivalent) priority initially
defined by the tool.

To ensure accountability and efficient issue resolution, they are assigned to
the respective maintainer who is responsible for the affected code.

A dedicated team comprising members with expertise in static analysis, code
quality, and software security ensures the effectiveness of the static
analysis process and verifies that identified issues are properly
triaged and resolved in a timely manner.

Workflow
========

If after analyzing the Coverity report it is concluded that it is a false
positive please set the classification to either "False positive" or
"Intentional", the action to "Ignore", owner to your own account and add a
comment why the issue is considered false positive or intentional.

Update the related Github issue in the zephyr project with the details, and only close
it after completing the steps above on scan service website. Any issues
closed without a fix or without ignoring the entry in the scan service will be
automatically reopened if the issue continues to be present in the code.

.. _Contribution Tools:

.. _Contribution workflow:

Contribution Workflow
*********************

One general practice we encourage, is to make small,
controlled changes. This practice simplifies review, makes merging and
rebasing easier, and keeps the change history clear and clean.

When contributing to the Zephyr Project, it is also important you provide as much
information as you can about your change, update appropriate documentation,
and test your changes thoroughly before submitting.

The general GitHub workflow used by Zephyr developers uses a combination of
command line Git commands and browser interaction with GitHub.  As it is with
Git, there are multiple ways of getting a task done.  We'll describe a typical
workflow here:

.. _Create a Fork of Zephyr:
   https://github.com/zephyrproject-rtos/zephyr#fork-destination-box

#. `Create a Fork of Zephyr`_
   to your personal account on GitHub. (Click on the fork button in the top
   right corner of the Zephyr project repo page in GitHub.)

#. On your development computer, change into the :file:`zephyr` folder that was
   created when you :ref:`obtained the code <get_the_code>`::

     cd zephyrproject/zephyr

   Rename the default remote pointing to the `upstream repository
   <https://github.com/zephyrproject-rtos/zephyr>`_ from ``origin`` to
   ``upstream``::

     git remote rename origin upstream

   Let Git know about the fork you just created, naming it ``origin``::

     git remote add origin https://github.com/<your github id>/zephyr

   and verify the remote repos::

     git remote -v

   The output should look similar to::

     origin   https://github.com/<your github id>/zephyr (fetch)
     origin   https://github.com/<your github id>/zephyr (push)
     upstream https://github.com/zephyrproject-rtos/zephyr (fetch)
     upstream https://github.com/zephyrproject-rtos/zephyr (push)

#. Create a topic branch (off of ``main``) for your work (if you're addressing
   an issue, we suggest including the issue number in the branch name)::

     git checkout main
     git checkout -b fix_comment_typo

   Some Zephyr subsystems do development work on a separate branch from
   ``main`` so you may need to indicate this in your checkout::

     git checkout -b fix_out_of_date_patch origin/net

#. Make changes, test locally, change, test, test again, ...  (Check out the
   prior chapter on `twister`_ as well).

#. When things look good, start the pull request process by adding your changed
   files::

     git add [file(s) that changed, add -p if you want to be more specific]

   You can see files that are not yet staged using::

     git status

#. Verify changes to be committed look as you expected::

     git diff --cached

#. Commit your changes to your local repo::

     git commit -s

   The ``-s`` option automatically adds your ``Signed-off-by:`` to your commit
   message.  Your commit will be rejected without this line that indicates your
   agreement with the :ref:`DCO`.  See the :ref:`commit-guidelines` section for
   specific guidelines for writing your commit messages.

#. Push your topic branch with your changes to your fork in your personal
   GitHub account::

     git push origin fix_comment_typo

#. In your web browser, go to your forked repo and click on the
   ``Compare & pull request`` button for the branch you just worked on and
   you want to open a pull request with.

#. Review the pull request changes, and verify that you are opening a pull
   request for the ``main`` branch. The title and message from your commit
   message should appear as well.

#. A bot will assign one or more suggested reviewers (based on the
   MAINTAINERS file in the repo). If you are a project member, you can
   select additional reviewers now too.

#. Click on the submit button and your pull request is sent and awaits
   review.  Email will be sent as review comments are made, or you can check
   on your pull request at https://github.com/zephyrproject-rtos/zephyr/pulls.

   .. note:: As more commits are merged upstream, the GitHub PR page will show
      a ``This branch is out-of-date with the base branch`` message and a
      ``Update branch`` button on the PR page. That message should be ignored,
      as the commits will be rebased as part of merging anyway, and triggering
      a branch update from the GitHub UI will cause the PR approvals to be
      dropped.

#. While you're waiting for your pull request to be accepted and merged, you
   can create another branch to work on another issue. (Be sure to make your
   new branch off of ``main`` and not the previous branch.)::

     git checkout main
     git checkout -b fix_another_issue

   and use the same process described above to work on this new topic branch.

#. If reviewers do request changes to your patch, you can interactively rebase
   commit(s) to fix review issues.  In your development repo::

     git fetch --all
     git rebase --ignore-whitespace upstream/main

   The ``--ignore-whitespace`` option stops ``git apply`` (called by rebase)
   from changing any whitespace. Continuing::

     git rebase -i <offending-commit-id>^

   In the interactive rebase editor, replace ``pick`` with ``edit`` to select
   a specific commit (if there's more than one in your pull request), or
   remove the line to delete a commit entirely.  Then edit files to fix the
   issues in the review.

   As before, inspect and test your changes. When ready, continue the
   patch submission::

     git add [file(s)]
     git rebase --continue

   Update commit comment if needed, and continue::

     git push --force origin fix_comment_typo

   By force pushing your update, your original pull request will be updated
   with your changes so you won't need to resubmit the pull request.

   .. note:: While amending commits and force pushing is a common review model
      outside GitHub, and the one recommended by Zephyr, it's not the main
      model supported by GitHub. Forced pushes can cause unexpected behavior,
      such as not being able to use "View Changes" buttons except for the last
      one - GitHub complains it can't find older commits. You're also not
      always able to compare the latest reviewed version with the latest
      submitted version. When rewriting history GitHub only guarantees access
      to the latest version.

#. If the CI run fails, you will need to make changes to your code in order
   to fix the issues and amend your commits by rebasing as described above.
   Additional information about the CI system can be found in
   `Continuous Integration`_.

.. _commit-guidelines:

Commit Message Guidelines
*************************

Changes are submitted as Git commits. Each commit has a *commit
message* describing the change. Acceptable commit messages look like
this:

.. code-block:: none

   [area]: [summary of change]

   [Commit message body (must be non-empty)]

   Signed-off-by: [Your Full Name] <[your.email@address]>

You need to change text in square brackets (``[like this]``) above to
fit your commit.

Examples and more details follow.

Example
=======

Here is an example of a good commit message.

.. code-block:: none

   drivers: sensor: abcd1234: fix bus I/O error handling

   The abcd1234 sensor driver is failing to check the flags field in
   the response packet from the device which signals that an error
   occurred. This can lead to reading invalid data from the response
   buffer. Fix it by checking the flag and adding an error path.

   Signed-off-by: Zephyr Developer <z.developer@example.com>

[area]: [summary of change]
===========================

This line is called the commit's *title*. Titles must be:

* one line
* less than 72 characters long
* followed by a completely blank line

[area]
  The ``[area]`` prefix usually identifies the area of code
  being changed. It can also identify the change's wider
  context if multiple areas are affected.

  Here are some examples:

  * ``doc: ...`` for documentation changes
  * ``drivers: foo:`` for ``foo`` driver changes
  * ``Bluetooth: Shell:`` for changes to the Bluetooth shell
  * ``net: ethernet:`` for Ethernet-related networking changes
  * ``dts:`` for treewide devicetree changes
  * ``style:`` for code style changes

  If you're not sure what to use, try running ``git log FILE``, where
  ``FILE`` is a file you are changing, and using previous commits that
  changed the same file as inspiration.

[summary of change]
  The ``[summary of change]`` part should be a quick description of
  what you've done. Here are some examples:

  * ``doc: update wiki references to new site``
  * ``drivers: sensor: sensor_shell: fix channel name collision``

Commit Message Body
===================

.. warning::

   An empty commit message body is not permitted. Even for trivial
   changes, please include a descriptive commit message body. Your
   pull request will fail CI checks if you do not.

This part of the commit should explain what your change does, and why
it's needed. Be specific. A body that says ``"Fixes stuff"`` will be
rejected. Be sure to include the following as relevant:

* **what** the change does,
* **why** you chose that approach,
* **what** assumptions were made, and
* **how** you know it works -- for example, which tests you ran.

Each line in your commit message should usually be 75 characters or
less. Use newlines to wrap longer lines. Exceptions include lines
with long URLs, email addresses, etc.

For examples of accepted commit messages, you can refer to the Zephyr GitHub
`changelog <https://github.com/zephyrproject-rtos/zephyr/commits/main>`__.

If the change addresses a GitHub issue, include a line of the form:

.. code-block:: none

   Fixes #[issue number]

Where ``[issue number]`` is the relevant GitHub issue's number. For
example:

.. code-block:: none

   Fixes: #1234

Signed-off-by: ...
==================

.. tip::

   You should have set your :ref:`git-name-and-email`
   already. Create your commit with ``git commit -s`` to add the
   Signed-off-by: line automatically using this information.

For open source licensing reasons, your commit must include a
Signed-off-by: line that looks like this:

.. code-block:: none

   Signed-off-by: [Your Full Name] <[your.email@address]>

For example, if your full name is ``Zephyr Developer`` and your email
address is ``z.developer@example.com``:

.. code-block:: none

   Signed-off-by: Zephyr Developer <z.developer@example.com>

This means that you have personally made sure your change complies
with the :ref:`DCO`. For this reason, you must use your legal name.
Pseudonyms or "hacker aliases" are not permitted.

Your name and the email address you use must match the name and email
in the Git commit's ``Author:`` field.

Other Commit Expectations
=========================

See the :ref:`contributor-expectations` for a more complete discussion of
contributor and reviewer expectations.

Submitting Proposals
====================

You can request a new feature or submit a proposal by submitting an issue to
our GitHub Repository.
If you would like to implement a new feature, please submit an issue with a
proposal (RFC) for your work first, to be sure that we can use it. Please
consider what kind of change it is:

* For a Major Feature, first open an issue and outline your proposal so that it
  can be discussed. This will also allow us to better coordinate our efforts,
  prevent duplication of work, and help you to craft the change so that it is
  successfully accepted into the project. Providing the following information
  will increase the chances of your issue being dealt with quickly:

  * Overview of the Proposal
  * Motivation for or Use Case
  * Design Details
  * Alternatives
  * Test Strategy

* Small Features can be crafted and directly submitted as a Pull Request.

Identifying Contribution Origin
===============================

When adding a new file to the tree, it is important to detail the source of
origin on the file, provide attributions, and detail the intended usage. In
cases where the file is an original to Zephyr, the commit message should
include the following ("Original" is the assumption if no Origin tag is
present)::

      Origin: Original

In cases where the file is :ref:`imported from an external project
<external-contributions>`, the commit message shall contain details regarding
the original project, the location of the project, the SHA-id of the origin
commit for the file and the intended purpose.

For example, a copy of a locally maintained import::

      Origin: Contiki OS
      License: BSD 3-Clause
      URL: http://www.contiki-os.org/
      commit: 853207acfdc6549b10eb3e44504b1a75ae1ad63a
      Purpose: Introduction of networking stack.

For example, a copy of an externally maintained import in a module repository::

      Origin: Tiny Crypt
      License: BSD 3-Clause
      URL: https://github.com/01org/tinycrypt
      commit: 08ded7f21529c39e5133688ffb93a9d0c94e5c6e
      Purpose: Introduction of TinyCrypt


Continuous Integration (CI)
***************************

The Zephyr Project operates a Continuous Integration (CI) system that runs on
every Pull Request (PR) in order to verify several aspects of the PR:

* Git commit formatting
* Coding Style
* Twister builds for multiple architectures and boards
* Documentation build to verify any doc changes

CI is run on Github Actions and it uses the same tools described in the
`Contribution Tools`_ section.  The CI results must be green indicating "All
checks have passed" before the Pull Request can be merged.  CI is run when the
PR is created, and again every time the PR is modified with a commit.

The current status of the CI run can always be found at the bottom of the
GitHub PR page, below the review status. Depending on the success or failure
of the run you will see:

* "All checks have passed"
* "All checks have failed"

In case of failure you can click on the "Details" link presented below the
failure message in order to navigate to ``Github Actions`` and inspect the
results.
Once you click on the link you will be taken to the ``Github actions`` summary
results page where a table with all the different builds will be shown. To see
what build or test failed click on the row that contains the failed (i.e.
non-green) build.

Contributions to External Modules
**********************************

Follow the guidelines in the :ref:`modules` section for contributing
:ref:`new modules <submitting_new_modules>` and
submitting changes to :ref:`existing modules <changes_to_existing_module>`.

.. _treewide-changes:

Treewide Changes
****************

This section describes contributions that are treewide changes and some
additional associated requirements that apply to them. These requirements exist
to try to give such changes increased review and user visibility due to their
large impact.

Definition and Decision Making
==============================

A *treewide change* is defined as any change to Zephyr APIs, coding practices,
or other development requirements that either implies required changes
throughout the zephyr source code repository or can reasonably be expected to
do so for a wide class of external Zephyr-based source code.

This definition is informal by necessity. This is because the decision on
whether any particular change is treewide can be subjective and may depend on
additional context.

Project maintainers should use good judgement and prioritize the Zephyr
developer experience when deciding when a proposed change is treewide.
Protracted disagreements can be resolved by the Zephyr Project's Technical
Steering Committee (TSC), but please avoid premature escalation to the TSC.

Requirements for Treewide Changes
=================================

- The zephyr repository must apply the 'treewide' GitHub label to any issues or
  pull requests that are treewide changes

- The person proposing a treewide change must create an `RFC issue
  <https://github.com/zephyrproject-rtos/zephyr/issues/new?assignees=&labels=RFC&template=003_rfc-proposal.md&title=>`_
  describing the change, its rationale and impact, etc. before any pull
  requests related to the change can be merged

- The project's `Architecture Working Group (WG)
  <https://github.com/zephyrproject-rtos/zephyr/wiki/Architecture-Working-Group>`_
  must include the issue on the agenda and discuss whether the project will
  accept or reject the change before any pull requests related to the change
  can be merged (with escalation to the TSC if consensus is not reached at the
  WG)

- The Architecture WG must specify the procedure for merging any PRs associated
  with each individual treewide change, including any required approvals for
  pull requests affecting specific subsystems or extra review time requirements

- The person proposing a treewide change must email
  devel@lists.zephyrproject.org about the RFC if it is accepted by the
  Architecture WG before any pull requests related to the change can be merged

Examples
========

Some example past treewide changes are:

- the deprecation of version 1 of the :ref:`Logging API <logging_api>` in favor
  of version 2 (see commit `262cc55609
  <https://github.com/zephyrproject-rtos/zephyr/commit/262cc55609b73ea61b5f999c6c6daaba20bc5240>`_)
- the removal of support for a legacy :ref:`dt-bindings` syntax
  (`6bf761fc0a
  <https://github.com/zephyrproject-rtos/zephyr/commit/6bf761fc0a2811b037abec0c963d60b00c452acb>`_)

Note that adding a new version of a widely used API while maintaining
support for the old one is not a treewide change. Deprecation and removal of
such APIs, however, are treewide changes.

Specialized driver requirements
*******************************

Drivers for standalone devices should use the Zephyr bus APIs (SPI, I2C...)
whenever possible so that the device can be used with any SoC from any vendor
implementing a compatible bus.

If it is not technically possible to achieve full performance using the Zephyr
APIs due to specialized accelerators in a particular SoC family, one could
extend the support for an external device by providing a specialized path for
that SoC family. However, the driver must still provide a regular path (via
Zephyr APIs) for all other SoCs. Every exception must be approved by the
Architecture WG in order to be validated and potentially to be learned/improved
from.
