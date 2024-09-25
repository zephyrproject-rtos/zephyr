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

- Smaller PRs should encompass one self-contained logical change.

- When adding a new large feature or API, the PR should address only one part of
  the feature. In this case create an :ref:`RFC proposal <rfcs>` to describe the
  additional parts of the feature for reviewers.

- PRs should include tests or samples under the following conditions:

  - Adding new features or functionality.

  - Modifying a feature, especially for API behavior contract changes.

  - Fixing a hardware agnostic bug. The test should fail without the bug fixed
    and pass with the fix applied.

- PRs must update any documentation affected by a functional code change.

- If introducing a new API, the PR must include an example usage of the API.
  This provides context to the reviewer and prevents submitting PRs with unused
  APIs.


Multiple Commits on a Single PR
*******************************

Contributors are further encouraged to break up PRs into multiple commits.  Keep
in mind each commit in the PR must still build cleanly and pass all the CI
tests.

For example, when introducing an extension to an API, contributors can break up
the PR into multiple commits targeting these specific changes:

#. Introduce the new APIs, including shared devicetree bindings
#. Update driver implementation X, with driver specific devicetree bindings
#. Update driver implementation Y
#. Add tests for the new API
#. Add a sample using the API
#. Update the documentation

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

PR Requirements
***************

- Each commit in the PR must provide a commit message following the
  :ref:`commit-guidelines`.

- The PR description must include a summary of the changes and their rationales.

- All files in the PR must comply with :ref:`Licensing
  Requirements<licensing_requirements>`.

- Follow the Zephyr :ref:`coding_style` and :ref:`coding_guidelines`.

- PRs must pass all CI checks. This is a requirement to merge the PR.
  Contributors may mark a PR as draft and explicitly request reviewers to
  provide early feedback, even with failing CI checks.

- When breaking a PR into multiple commits, each commit must build cleanly. The
  CI system does not enforce this policy, so it is the PR author's
  responsibility to verify.

- When major new functionality is added, tests for the new functionality shall
  be added to the automated test suite. All API functions should have test cases
  and there should be tests for the behavior contracts of the API. Maintainers
  and reviewers have the discretion to determine if the provided tests are
  sufficient. The examples below demonstrate best practices on how to test APIs
  effectively.

    - :zephyr_file:`Kernel timer tests <tests/kernel/timer/timer_behavior>`
      provide around 85% test coverage for the
      :zephyr_file:`kernel timer <kernel/timer.c>`, measured by lines of code.
    - Emulators for off-chip peripherals are an effective way to test driver
      APIs. The :zephyr_file:`fuel gauge tests <tests/drivers/fuel_gauge/sbs_gauge>`
      use the :zephyr_file:`smart battery emulator <drivers/fuel_gauge/sbs_gauge/emul_sbs_gauge.c>`,
      providing test coverage for the
      :zephyr_file:`fuel gauge API <include/zephyr/drivers/fuel_gauge.h>`
      and the :zephyr_file:`smart battery driver <drivers/fuel_gauge/sbs_gauge/sbs_gauge.c>`.
    - Code coverage reports for the Zephyr project are available on `Codecov`_.

- Incompatible changes to APIs must also update the release notes for the
  next release detailing the change.  APIs marked as experimental are excluded
  from this requirement.

- Changes to APIs must increment the API version number according to the API
  version rules.

- PRs must also satisfy all :ref:`merge_criteria` before a member of the release
  engineering team merges the PR into the zephyr tree.

Maintainers may request that contributors break up a PR into smaller PRs and may
request that they create an :ref:`RFC proposal <rfcs>`.

.. _`Codecov`: https://app.codecov.io/gh/zephyrproject-rtos/zephyr

Workflow Suggestions That Help Reviewers
========================================

- Unless they applied the reviewer's recommendation exactly, authors must not
  resolve and hide comments, they must let the initial reviewer do it. The
  Zephyr project does not require all comments to be resolved before merge.
  Leaving some completed discussions open can sometimes be useful to understand
  the greater picture.

- Respond to comments using the "Start Review" and "Add Review" green buttons in
  the "Files changed" view. This allows responding to multiple comments and
  publishing the responses in bulk. This reduces the number of emails sent to
  reviewers.

- As GitHub does not implement |git range-diff|_, try to minimize rebases in the
  middle of a review. If a rebase is required, push this as a separate update
  with no other changes since the last push of the PR. When pushing a rebase
  only, add a comment to the PR indicating which commit is the rebase.

.. |git range-diff| replace:: ``git range-diff``
.. _`git range-diff`: https://git-scm.com/docs/git-range-diff

PR Review Escalation
====================

The Zephyr community is a diverse group of individuals, with different levels of
commitment and priorities. As such, reviewers and maintainers may not get to a
PR right away.

The `Zephyr Dev Meeting`_ performs a triage of PRs missing reviewer approval,
following this process:

#. Identify and update PRs missing an Assignee.
#. Identify PRs without any comments or reviews, ping the PR Assignee to start a
   review or assign to a different maintainer.
#. For PRs that have otherwise stalled, the Zephyr Dev Meeting pings the
   Assignee and any reviewers that have left comments on the PR.

Contributors may escalate PRs outside of the Zephyr Dev Meeting triage process
as follows:

- After 1 week of inactivity, ping the Assignee or reviewers on the PR by adding
  a comment to the PR.

- After 2 weeks of inactivity, post a message on the `#pr-help`_ channel on
  Discord linking to the PR.

- After 2 weeks of inactivity, add the `dev-review`_ label to the PR. This
  explicitly adds the PR to the agenda for the next `Zephyr Dev Meeting`_
  independent of the triage process. Not all contributors have the required
  privileges to add labels to PRs, in this case the contributor should request
  help on Discord or send an email to the `Zephyr devel mailing list`_.

Note that for new PRs, contributors should generally wait for at least one
Zephyr Dev Meeting before escalating the PR themselves.

.. _Zephyr devel mailing list: https://lists.zephyrproject.org/g/devel


.. _pr_technical_escalation:

PR Technical Escalation
=======================

In cases where a contributor objects to change requests from reviewers, Zephyr
defines the following escalation process for resolving technical disagreements.

Before escalation of technical disagreements, follow the steps below:

- Resolve in the PR among assignee, maintainers and reviewer.

  - Assignee to act as moderator if applicable.

- Optionally resolve in the next `Zephyr Dev Meeting`_  meeting with more
  Maintainers and project stakeholders.

  - The involved parties and the Assignee to be present when the  issue is
    discussed.

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

.. _dev-review: https://github.com/zephyrproject-rtos/zephyr/labels/dev-review

.. _Zephyr Dev Meeting: https://github.com/zephyrproject-rtos/zephyr/wiki/Zephyr-Committee-and-Working-Group-Meetings#zephyr-dev-meeting

.. _Architecture Project: https://github.com/zephyrproject-rtos/zephyr/projects/18

.. _Architecture Working Group: https://github.com/zephyrproject-rtos/zephyr/wiki/Architecture-Working-Group


.. _reviewer-expectations:

Reviewer Expectations
*********************

- Be respectful when commenting on PRs. Refer to the Zephyr `Code of Conduct`_
  for more details.

- The Zephyr Project recognizes that reviewers and maintainers have limited
  bandwidth. As a reviewer, prioritize review requests in the following order:

    #. PRs related to items in the `Zephyr Release Plan`_ or those targeting
       the next release during the stabilization period (after RC1).
    #. PRs where the reviewer has requested blocking changes.
    #. PRs assigned to the reviewer as the area maintainer.
    #. All other PRs.

- Reviewers shall strive to advance the PR to a mergeable state with their
  feedback and engagement with the PR author.

- Try to provide feedback on the entire PR in one shot. This provides the
  contributor an opportunity to address all comments in the next PR update.

- Partial reviews are permitted, but the reviewer must add a comment indicating
  what portion of the PR they reviewed. Examples of useful partial reviews
  include:

  - Domain specific reviews (e.g. Devicetree).
  - Code style changes that impact the readability of the PR.
  - Reviewing commits separately when the requested changes cascade into the
    later commits.

- Avoid increasing scope of the PR by requesting new features, especially when
  there is a corresponding :ref:`RFC <rfcs>` associated with the PR. Instead,
  reviewers should add suggestions as a comment to the :ref:`RFC <rfcs>`. This
  also encourages more collaboration as it is easier for multiple contributors
  to work on a feature once the minimum implementation has merged.

- When using the "Request Changes" option, mark trivial, non-functional,
  requests as "Non-blocking" in the comment. Reviewers should approve PRs once
  only non-blocking changes remain. The PR author has discretion as to whether
  they address all non-blocking comments. PR authors should acknowledge every
  review comment in some way, even if it's just with an emoticon.

- Reviewers shall be *clear* and *concise* what changes they are requesting when the
  "Request Changes" option is used. Requested changes shall be in the scope of
  the PR in question and following the contribution and style guidelines of the
  project.

- Reviewers shall not close a PR due to technical or structural disagreement.
  If requested changes cannot be resolved within the review process, the
  :ref:`pr_technical_escalation` path shall be used for any potential resolution
  path, which may include closing the PR.

.. _Code of Conduct: https://github.com/zephyrproject-rtos/zephyr/blob/main/CODE_OF_CONDUCT.md

.. _Zephyr Release Plan: https://github.com/orgs/zephyrproject-rtos/projects/13
