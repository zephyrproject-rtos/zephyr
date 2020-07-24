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

Licensing
*********

Licensing is very important to open source projects. It helps ensure the
software continues to be available under the terms that the author desired.

.. _Apache 2.0 license:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/LICENSE

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

See :ref:`contribute_non-Apache` for more information about
this contributing and review process for imported components.

.. only:: latex

   .. toctree::
      :maxdepth: 1

      ../LICENSING.rst

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

DCO Sign-Off Methods
====================

The DCO requires a sign-off message in the following format appear on each
commit in the pull request::

   Signed-off-by: Zephyrus Zephyr <zephyrus@zephyrproject.org>

The DCO text can either be manually added to your commit body, or you can add
either ``-s`` or ``--signoff`` to your usual Git commit commands. If you forget
to add the sign-off you can also amend a previous commit with the sign-off by
running ``git commit --amend -s``. If you've pushed your changes to GitHub
already you'll need to force push your branch after this with ``git push -f``.

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
   service ``shippable``, which runs automatically on GitHub when you submit
   your Pull Request (PR).  You can see any failure results in the Shippable
   details link near the end of the PR conversation list. See
   `Continuous Integration`_ for more information

Repository layout
*****************

To clone the main Zephyr Project repositories use the instructions in
:ref:`get_the_code`.

The Zephyr project directory structure is described in :ref:`source_tree_v2`
documentation. In addition to the Zephyr kernel itself, you'll also find the
sources for technical documentation, sample code, supported board
configurations, and a collection of subsystem tests.  All of these are
available for developers to contribute to and enhance.

Pull Requests and Issues
************************

.. _Zephyr Project Issues: https://github.com/zephyrproject-rtos/zephyr/issues

.. _open pull requests: https://github.com/zephyrproject-rtos/zephyr/pulls

.. _Zephyr devel mailing list: https://lists.zephyrproject.org/g/devel

.. _Zephyr Slack channel: https://zephyrproject.slack.com

Before starting on a patch, first check in our issues `Zephyr Project Issues`_
system to see what's been reported on the issue you'd like to address.  Have a
conversation on the `Zephyr devel mailing list`_ (or the the `Zephyr Slack channel`_)
to see what others think of your issue (and proposed solution).  You may find
others that have encountered the issue you're finding, or that have similar
ideas for changes or additions.  Send a message to the `Zephyr devel mailing list`_
to introduce and discuss your idea with the development community.

It's always a good practice to search for existing or related issues before
submitting your own. When you submit an issue (bug or feature request), the
triage team will review and comment on the submission, typically within a few
business days.

You can find all `open pull requests`_ on GitHub and open `Zephyr Project
Issues`_ in Github issues.

 .. _Continuous Integration:

Continuous Integration (CI)
***************************

The Zephyr Project operates a Continuous Integration (CI) system that runs on
every Pull Request (PR) in order to verify several aspects of the PR:

* Git commit formatting
* Coding Style
* Sanity Check builds for multiple architectures and boards
* Documentation build to verify any doc changes

CI is run on the ``shippable`` cloud service and it uses the same tools
described in the `Contribution Tools`_ section.
The CI results must be green indicating "All checks have passed" before
the Pull Request can be merged.  CI is run when the PR is created, and
again every time the PR is modified with a commit.

The current status of the CI run can always be found at the bottom of the
GitHub PR page, below the review status. Depending on the success or failure
of the run you will see:

* "All checks have passed"
* "All checks have failed"

In case of failure you can click on the "Details" link presented below the
failure message in order to navigate to ``shippable`` and inspect the results.
Once you click on the link you will be taken to the ``shippable`` summary
results page where a table with all the different builds will be shown. To see
what build or test failed click on the row that contains the failed (i.e.
non-green) build and then click on the "Tests" tab to see the console output
messages indicating the failure.

The `builds@lists.zephyrproject.org mailing list
<https://lists.zephyrproject.org/g/builds>`_
archives the CI (shippable) nightly build results.

Coding Guidelines
*****************

Beyond the :ref:`coding_style` that Zephyr enforces for all code that is
submitted for inclusion, the project targets compliance with a series of
coding guidelines. Refer to the :ref:`coding_guidelines` section of the
documentation for additional details.

 .. _Contribution Tools:

Contribution Tools and Git Setup
********************************

Signed-off-by
=============

The name in the commit message ``Signed-off-by:`` line and your email must
match the change authorship information. Make sure your :file:`.gitconfig`
is set up correctly:

.. code-block:: console

   git config --global user.name "David Developer"
   git config --global user.email "david.developer@company.com"

gitlint
=========

When you submit a pull request to the project, a series of checks are
performed to verify your commit messages meet the requirements. The same step
done during the CI process can be performed locally using the the ``gitlint``
command.

Run ``gitlint`` locally in your tree and branch where your patches have been
committed:

.. code-block:: console

     gitlint

Note, gitlint only checks HEAD (the most recent commit), so you should run it
after each commit, or use the ``--commits`` option to specify a commit range
covering all the development patches to be submitted.

sanitycheck
===========

.. note::
   sanitycheck does not currently run on Windows.

To verify that your changes did not break any tests or samples, please run the
``sanitycheck`` script locally before submitting your pull request to GitHub. To
run the same tests the CI system runs, follow these steps from within your
local Zephyr source working directory:

.. code-block:: console

    source zephyr-env.sh
    ./scripts/sanitycheck

The above will execute the basic sanitycheck script, which will run various
kernel tests using the QEMU emulator.  It will also do some build tests on
various samples with advanced features that can't run in QEMU.

We highly recommend you run these tests locally to avoid any CI
failures.

uncrustify
==========

The `uncrustify tool <https://sourceforge.net/projects/uncrustify>`_ can
be helpful to quickly reformat large amounts of new source code to our
`Coding Style`_
standards together with a configuration file we've provided:

.. code-block:: bash

   # On Linux/macOS
   uncrustify --replace --no-backup -l C -c $ZEPHYR_BASE/.uncrustify.cfg my_source_file.c
   # On Windows
   uncrustify --replace --no-backup -l C -c %ZEPHYR_BASE%\.uncrustify.cfg my_source_file.c

But note that you should not use uncrustify to reformat existing Zephyr code,
or to modify files in which you only introduce a small fix. This would create a
lot of unwelcome extra changed lines.

On Linux systems, you can install uncrustify with

.. code-block:: bash

   sudo apt install uncrustify

For Windows installation instructions see the `sourceforge listing for
uncrustify <https://sourceforge.net/projects/uncrustify>`_.

.. _coding_style:

Coding Style
************

Use these coding guidelines to ensure that your development complies with the
project's style and naming conventions.

.. _Linux kernel coding style:
   https://kernel.org/doc/html/latest/process/coding-style.html

In general, follow the `Linux kernel coding style`_, with the
following exceptions:

* Add braces to every ``if``, ``else``, ``do``, ``while``, ``for`` and
  ``switch`` body, even for single-line code blocks. Use the ``--ignore BRACES``
  flag to make *checkpatch* stop complaining.
* Use spaces instead of tabs to align comments after declarations, as needed.
* Use C89-style single line comments, ``/*  */``. The C99-style single line
  comment, ``//``, is not allowed.
* Use ``/**  */`` for doxygen comments that need to appear in the documentation.


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

#. Create a topic branch (off of master) for your work (if you're addressing
   an issue, we suggest including the issue number in the branch name)::

     git checkout master
     git checkout -b fix_comment_typo

   Some Zephyr subsystems do development work on a separate branch from
   master so you may need to indicate this in your checkout::

     git checkout -b fix_out_of_date_patch origin/net

#. Make changes, test locally, change, test, test again, ...  (Check out the
   prior chapter on `sanitycheck`_ as well).

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
   agreement with the `DCO`_.  See the `Commit Guidelines`_ section for
   specific guidelines for writing your commit messages.

#. Push your topic branch with your changes to your fork in your personal
   GitHub account::

     git push origin fix_comment_typo

#. In your web browser, go to your forked repo and click on the
   ``Compare & pull request`` button for the branch you just worked on and
   you want to open a pull request with.

#. Review the pull request changes, and verify that you are opening a
   pull request for the appropriate branch. The title and message from your
   commit message should appear as well.

#. If you're working on a subsystem branch that's not ``master``,
   you may need to change the intended branch for the pull request
   here, for example, by changing the base branch from ``master`` to ``net``.

#. GitHub will assign one or more suggested reviewers (based on the
   CODEOWNERS file in the repo). If you are a project member, you can
   select additional reviewers now too.

#. Click on the submit button and your pull request is sent and awaits
   review.  Email will be sent as review comments are made, or you can check
   on your pull request at https://github.com/zephyrproject-rtos/zephyr/pulls.

#. While you're waiting for your pull request to be accepted and merged, you
   can create another branch to work on another issue. (Be sure to make your
   new branch off of master and not the previous branch.)::

     git checkout master
     git checkout -b fix_another_issue

   and use the same process described above to work on this new topic branch.

#. If reviewers do request changes to your patch, you can interactively rebase
   commit(s) to fix review issues.  In your development repo::

     git fetch --all
     git rebase --ignore-whitespace upstream/master

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

Contributions to External Modules
**********************************

Follow the guidelines in the :ref:`modules` section for contributing
:ref:`new modules <submitting_new_modules>` and
submitting changes to :ref:`existing modules <changes_to_existing_module>`.

Commit Guidelines
*****************

Changes are submitted as Git commits. Each commit message must contain:

* A short and descriptive subject line that is less than 72 characters,
  followed by a blank line. The subject line must include a prefix that
  identifies the subsystem being changed, followed by a colon, and a short
  title, for example:  ``doc: update wiki references to new site``.
  (If you're updating an existing file, you can use
  ``git log <filename>`` to see what developers used as the prefix for
  previous patches of this file.)

* A change description with your logic or reasoning for the changes, followed
  by a blank line.

* A Signed-off-by line, ``Signed-off-by: <name> <email>`` typically added
  automatically by using ``git commit -s``

* If the change addresses an issue, include a line of the form::

      Fixes #<issue number>.


All changes and topics sent to GitHub must be well-formed, as described above.

Commit Message Body
===================

When editing the commit message, please briefly explain what your change
does and why it's needed. A change summary of ``"Fixes stuff"`` will be rejected.

.. warning::
   An empty change summary body is not permitted. Even for trivial changes, please
   include a summary body in the commit message.

The description body of the commit message must include:

* **what** the change does,
* **why** you chose that approach,
* **what** assumptions were made, and
* **how** you know it works -- for example, which tests you ran.

For examples of accepted commit messages, you can refer to the Zephyr GitHub
`changelog <https://github.com/zephyrproject-rtos/zephyr/commits/master>`__.

Other Commit Expectations
=========================

* Commits must build cleanly when applied on top of each other, thus avoiding
  breaking bisectability.

* Commits must pass all CI checks (see `Continuous Integration`_ for more
  information)

* Each commit must address a single identifiable issue and must be
  logically self-contained. Unrelated changes should be submitted as
  separate commits.

* You may submit pull request RFCs (requests for comments) to send work
  proposals, progress snapshots of your work, or to get early feedback on
  features or changes that will affect multiple areas in the code base.

* When major new functionality is added, tests for the new functionality MUST be
  added to the automated test suite. All new APIs MUST be documented and tested
  and tests MUST cover at least 80% of the added functionality using the code
  coverage tool and reporting provided by the project.

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

In cases where the file is imported from an external project, the commit
message shall contain details regarding the original project, the location of
the project, the SHA-id of the origin commit for the file, the intended
purpose, and if the file will be maintained by the Zephyr project,
(whether or not the Zephyr project will contain a localized branch or if
it is a downstream copy).

For example, a copy of a locally maintained import::

    Origin: Contiki OS
    License: BSD 3-Clause
    URL: http://www.contiki-os.org/
    commit: 853207acfdc6549b10eb3e44504b1a75ae1ad63a
    Purpose: Introduction of networking stack.
    Maintained-by: Zephyr

For example, a copy of an externally maintained import::

    Origin: Tiny Crypt
    License: BSD 3-Clause
    URL: https://github.com/01org/tinycrypt
    commit: 08ded7f21529c39e5133688ffb93a9d0c94e5c6e
    Purpose: Introduction of TinyCrypt
    Maintained-by: External

.. _contribute_non-Apache:

Contributing non-Apache 2.0 licensed components
***********************************************

Importing code into the Zephyr OS from other projects that use a license
other than the Apache 2.0 license needs to be fully understood in
context and approved by the `Zephyr governing board`_. The board will
automatically reject licenses that have not been approved by the `Open Source
Initiative (OSI)`_.

.. _Zephyr governing board:
   https://www.zephyrproject.org/governance/

.. _Open Source Initiative (OSI):
   https://opensource.org/licenses/alphabetical

By carefully reviewing potential contributions and also enforcing a
:ref:`DCO` for contributed code, we ensure that
the Zephyr community can develop products with the Zephyr Project
without concerns over patent or copyright issues.

Submission and review process
=============================

All contributions to the Zephyr project are submitted through GitHub
pull requests (PR) following the Zephyr Project's :ref:`Contribution workflow`.

Before you begin working on including a new component to the Zephyr
Project (Apache-2.0 licensed or not), you should start up a conversation
on the `developer mailing list <https://lists.zephyrproject.org/g/devel>`_
to see what the Zephyr community thinks about the idea.  Maybe there's
someone else working on something similar you can collaborate with, or a
different approach may make the new component unnecessary.

If the conclusion is that including a new component is the best
solution, and this new component uses a license other than Apache-2.0,
these additional steps must be followed:

#. Complete a README for your code component and add it to your source
   code pull request (PR).  A recommended README template can be found in
   :zephyr_file:`doc/contribute/code_component_README` (and included
   `below`_ for reference)

#. The Zephyr Technical Steering Committee (TSC) will evaluate the code
   component README as part of the PR
   commit and vote on accepting it using the GitHub PR review tools.

   - If rejected by the TSC, a TSC member will communicate this to
     the contributor and the PR will be closed.

   - If approved by the TSC, the TSC chair will forward the README to
     the Zephyr governing board for further review.

#. The Zephyr governing board has two weeks to review and ask questions:

   - If there are no objections, the matter is closed. Approval can be
     accelerated by unanimous approval of the board before the two
     weeks are up.

   - If a governing board member raises an objection that cannot be resolved
     via email, the board will meet to discuss whether to override the
     TSC approval or identify other approaches that can resolve the
     objections.

#. On approval of the Zephyr TSC and governing board, final review of
   the PR may be made to ensure its proper placement in the
   Zephyr Project :ref:`source_tree_v2`, (in the ``ext`` folder), and
   inclusion in the :ref:`zephyr_licensing` document.

.. note::

   External components not under the Apache-2.0 license **cannot** be
   included in a Zephyr OS release without approval of both the Zephyr TSC
   and the Zephyr governing board.

.. _below:

Code component README template
==============================

.. literalinclude:: code_component_README


Contribution Roles and Responsibilities
***************************************

.. toctree::
   :maxdepth: 1

   modifying_contributions.rst
   project_roles.rst

The Zephyr project defines a development process workflow using GitHub
**Issues** to track feature, enhancement, and bug reports together with GitHub
**Pull Requests** (PRs) for submitting and reviewing changes.  Zephyr
community members work together to review these Issues and PRs, managing
feature enhancements and quality improvements of Zephyr through its regular
releases, as outlined in the
`program management overview <https://wiki.zephyrproject.org/Program-Management>`_.

We can only manage the volume of Issues and PRs, by requiring timely reviews,
feedback, and responses from the community and contributors, both for initial
submissions and for followup questions and clarifications.  Read about the
project's :ref:`development processes and tools <dev-environment-and-tools>`
and specifics about :ref:`review timelines <review_time>` to learn about the
project's goals and guidelines for our active developer community.

:ref:`project_roles` describes in detail the Zephyr project roles and associated permissions
with respect to the development process workflow.
