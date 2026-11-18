.. _contributor-expectations:

Contributor Expectations
########################

The Zephyr project encourages :ref:`contributors <contributor>` to submit
changes as smaller pull requests. Smaller pull requests (PRs) have the following
benefits:

- Reviewed more quickly and reviewed more thoroughly. It's easier for reviewers
  to set aside a few minutes to review smaller changes several times than it is
  to allocate large blocks of time to review a large PR.

- Less wasted work if reviewers or maintainers reject the direction of the
  change.

- Easier to rebase and merge. Smaller PRs are less likely to conflict with other
  changes in the tree.

- Easier to revert if the PR breaks functionality.

.. note::
  This page does not apply to draft PRs which can have any size, any number of
  commits and any combination of smaller PRs for testing and preview purposes.
  Draft PRs have no review expectation and PRs created as drafts from the start
  do not notify anyone by default.

Defining Smaller PRs
********************

A PR should address one coherent change: a single bug fix, improvement, or
useful increment of a larger feature. Reviewers should be able to understand
its purpose and evaluate it without having to review unrelated work.

A smaller PR does not necessarily mean fewer lines of code. The important
thing is a clear scope. For example, a large feature might be introduced
through a series of PRs: one for a core interface and an initial working
implementation, followed by separate PRs for additional drivers or
capabilities. Each PR should leave the codebase in a working state.

Avoid splitting changes into tightly coupled PRs solely to make them smaller.
If several parts must be reviewed and merged together to make sense, keeping
them in one PR may be easier. For changes that need an overall design proposal,
see the "Large Changes" section below. Testing and documentation obligations
are covered under :ref:`pr_requirements`.

Multiple Commits on a Single PR
*******************************

A PR can contain one commit or several. Use multiple commits when they make a
cohesive change easier to understand, not simply to preserve the order in
which the work was developed. Each commit should represent a meaningful step
in the final implementation.

For example, a PR extending a driver API might contain:

#. Introduce the API and an initial implementation, with relevant tests.
#. Extend support to another driver, with its corresponding tests.
#. Add a usage sample and update the user documentation.

This is only an example, not a required structure. A small fix may need just
one commit. If parts of a larger change can be reviewed and merged
independently, separate PRs may be a better choice.

Requirements for commit history, buildability, testing, and documentation are
listed under :ref:`pr_requirements`.

Large Changes
*************

Large changes to the Zephyr project must submit an :ref:`RFC proposal <rfcs>`
describing the full scope of change and future work.  The RFC proposal provides
the required context to reviewers, but allows for smaller, incremental, PRs to
get reviewed and merged into the project. The RFC should also define the minimum
viable implementation.

Changes which require an RFC proposal include:

- Submitting a new feature.
- Submitting a new API.
- :ref:`treewide-changes`.
- Other large changes that can benefit from the RFC proposal process.

Maintainers have the discretion to request that contributors create an RFC for
PRs that are too large or complicated.

.. _pr_requirements:

PR Requirements
***************

Before requesting a review, self-review your PR and make sure it meets the
requirements below. A reviewer should be able to understand the change, follow
the commit history, and see how the change was validated.

.. important::

   PRs that do not meet these expectations may receive less reviewer attention,
   and reviewers may request changes without providing detailed feedback.

   Draft PRs can still be used to request early feedback, even when CI is
   failing.

Explain the change
==================

**Describe what changed and why.** The PR description must summarize the change
and explain its rationale. Give reviewers enough context to understand the
problem being solved and the approach you chose.

Follow project standards
========================

**Use the required project conventions.** Each commit must follow the
:ref:`commit-guidelines`. All files must comply with the
:ref:`licensing_requirements`, and code must follow the :ref:`coding_style` and
:ref:`coding_guidelines`.

**Keep the commit history useful.** Each commit should represent one logical,
self-contained step. Do not combine unrelated work in the same commit. Every
commit must build and pass the relevant tests so that the history remains
useful for ``git bisect``.

Before requesting review, clean up temporary development history. Squash
intermediate commits, remove debugging artifacts, and avoid repeated renames or
rewrites that do not help explain the final change. Fixup and merge commits are
not allowed in the final PR history. If necessary, use ``git rebase -i`` to
reorganize the commits, while preserving correct authorship attribution. See
:ref:`Contribution workflow` for more information.

Test the change
===============

**Include tests and examples for changes in behavior.** Major new
functionality must be covered by automated tests. Other new features and
changes to existing behavior should include appropriate tests or samples,
especially when an API's behavior contract changes. Hardware-independent bug
fixes should include a regression test that fails without the fix and passes
with it. Any new API must include an example showing how it is used. API
tests should check individual functions and their behavior contracts, not
just whether the code builds. Maintainers and reviewers determine whether
this coverage is sufficient.

For examples, see the :zephyr_file:`kernel timer tests <tests/kernel/timer/timer_behavior>`
and the :zephyr_file:`fuel gauge tests <tests/drivers/fuel_gauge/sbs_gauge>`,
which use a :zephyr_file:`smart battery emulator <drivers/fuel_gauge/sbs_gauge/emul_sbs_gauge.c>`
to test driver behavior. Project-wide coverage reports are available on
`Codecov <https://app.codecov.io/gh/zephyrproject-rtos/zephyr>`_.

**Check CI results.** The PR must pass all required CI checks as described in
the :ref:`merge_criteria`. If you need feedback before CI passes, use a draft PR
and explicitly request early review.

Update documentation for users
==============================

**Keep documentation in sync with the code.** Add or update documentation for
any behavior or functionality changed by the PR. Use terminology consistent
with existing documentation, write in American English, and follow the
:ref:`doc_guidelines`, including the :ref:`doc_images` rules for images.

**Explain changes that affect existing applications.** Changes that require
users to adapt their applications must be described in the migration guide for
the next release. New capabilities with a user-visible impact belong in the
release notes, following :ref:`release_documentation`. Experimental APIs are
excluded from these release documentation requirements. Do not manually add
lists of new, deprecated, or removed APIs and options; these are generated
automatically.

**Account for API versioning.** Changes to APIs must increment the API version
number according to the API version rules.

Every PR must satisfy all :ref:`merge_criteria` before merging. Maintainers
may also ask for a large PR to be divided into smaller changes or supported
by an :ref:`RFC proposal <rfcs>`.

.. _`Codecov`: https://app.codecov.io/gh/zephyrproject-rtos/zephyr

Workflow Suggestions That Help Reviewers
========================================

A reviewer should be able to tell what changed, how it was tested, and what has
changed since the previous review. Making this information easy to find saves
reviewers time and helps your PR move forward without unnecessary back-and-forth.

Before requesting a review
--------------------------

**Explain what you tested.** In the PR description, say how you verified your
change. Distinguish between building the code, running automated tests, and
testing on real hardware. Name the boards or emulators, Twister scenarios, or
samples you used. Also mention anything you could not test, such as hardware
you do not have access to. For a bug fix, explain how you reproduced the problem
and, when adding a regression test, confirm that it fails without the fix.

**Identify dependencies between PRs.** If your PR depends on another open PR,
make that clear at the beginning of its description. This is particularly
important when the dependent PR includes commits from the other PR: reviewers
need to know which changes to review here and which to review elsewhere. For
example::

   Depends on #1234. The first two commits belong to that PR;
   please review them there.

When possible, keep the dependent PR in draft until its dependency is merged.
Alternatively, consider combining the changes into one PR if that makes them
easier to review.

While a review is in progress
-----------------------------

**Keep changes focused and the history clean.** When addressing review feedback,
amend the commits to which the changes belong rather than adding new commits
such as "fix review comments". See :ref:`Contribution workflow` for guidance
on updating commits.

**Avoid rebasing onto ``main`` during review.** A rebase makes it harder for
reviewers to identify what changed since the last round of feedback. GitHub
does not provide an equivalent of |git range-diff|_ in its review interface,
so reviewers may have to inspect the entire PR again, and existing approvals
may be dismissed. Rebase during an active review only when necessary, such as
to resolve a merge conflict or to pick up a change on ``main`` required by CI.
If you must rebase, push the rebase separately from other changes and leave a
comment explaining what you did. Rebasing after the PR has been approved is
fine, subject to any resulting changes in approval status.

After receiving review feedback
-------------------------------

**Respond to every comment.** Tell the reviewer what you changed, or explain
why you chose a different approach. Do not resolve a discussion yourself unless
you followed the reviewer's recommendation exactly; otherwise, let the
reviewer who raised the concern decide when it is resolved. Zephyr does not
require every discussion to be marked resolved before merging. Some completed
discussions are useful to leave visible for future context.

**Group your replies when practical.** In GitHub's **Files changed** view, use
**Start a review** and **Add review comment** to prepare several responses,
then submit them together. This lets reviewers read one coherent response
instead of receiving a separate notification for every comment.

**Let reviewers know when you're ready for another look.** After pushing
changes that address their feedback, use **Re-request review** for each reviewer
who requested changes. If your update includes anything beyond the requested
fixes, such as unrelated changes or a rebase, call that out in a PR comment so
reviewers know what else has changed.

.. |git range-diff| replace:: ``git range-diff``
.. _`git range-diff`: https://git-scm.com/docs/git-range-diff

Getting PRs Reviewed
====================

The Zephyr community is a diverse group of individuals, with different levels of
commitment and priorities. As such, reviewers and maintainers may not get to a
PR right away.

- After 1 week of inactivity, ping the Assignee or reviewers on the PR by adding
  a comment to the PR.

- After 2 weeks of inactivity, post a message on the `#pr-help`_ channel on
  Discord linking to the PR.

.. _pr_technical_escalation:

PR Technical Escalation
=======================

In cases where a contributor objects to change requests from reviewers, Zephyr
defines the following escalation process for resolving technical disagreements.

Before escalation of technical disagreements, follow the steps below:

- Resolve in the PR among assignee, maintainers and reviewer.

  - Assignee to act as moderator if applicable.

- If no progress is made, the assignee (maintainer) has the right to dismiss
  stale, unrelated or irrelevant change requests by reviewers giving the
  reviewers a minimum of 1 business day to respond and revisit their initial
  change requests or start the escalation process.

  The assignee has the responsibility to document the reasoning for dismissing
  any reviews in the PR and should notify the reviewer about their review being
  dismissed.

  To give the reviewers time to respond and escalate, the assignee is
  expected to block the PR from being merged either by not
  approving the PR or by setting the *DNM* label.

Escalation can be triggered by any party participating in the review
process (assignee, reviewers or the original author of the change) following
the steps below:

- Escalate to the `Architecture Working Group`_ by adding the `Architecture
  Review` label on the PR. Beside the weekly meeting where such escalations are
  processed, the `Architecture Working Group`_  shall facilitate an offline
  review of the escalation if requested, especially if any of the parties can't
  attend the meeting.

- If all avenues of resolution and escalation have failed, assignees can escalate
  to the TSC and get a binding resolution in the TSC by adding the *TSC* label
  on the PR.

- The Assignee is expected to ensure the resolution of the escalation and the
  outcome is documented in the related pull request or issues on Github.

.. _#pr-help: https://discord.com/channels/720317445772017664/997527108844798012

.. _Architecture Project: https://github.com/zephyrproject-rtos/zephyr/projects/18

.. _Architecture Working Group: https://github.com/zephyrproject-rtos/zephyr/wiki/Architecture-Working-Group
