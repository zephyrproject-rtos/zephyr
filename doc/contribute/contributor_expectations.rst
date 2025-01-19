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

.. _pr_requirements:

PR Requirements
***************

- Each commit in the PR must provide a commit message following the
  :ref:`commit-guidelines`.

- No fixup or merge commits are allowed, see :ref:`Contribution workflow` for
  more information.

- The PR description must include a summary of the changes and their rationales.

- All files in the PR must comply with :ref:`Licensing
  Requirements<licensing_requirements>`.

- The code must follow the Zephyr :ref:`coding_style` and :ref:`coding_guidelines`.

- The PR must pass all CI checks, as described in :ref:`merge_criteria`.
  Contributors may mark a PR as draft and explicitly request reviewers to
  provide early feedback, even with failing CI checks.

- When breaking up a PR into multiple commits, each commit must build cleanly. The
  CI system does not enforce this policy, so it is the PR author's
  responsibility to verify.

- Commits in a pull request should represent clear, logical units of change that are easy to review
  and maintain bisectability. The following guidelines expand on this principle:

  1. Distinct, Logical Units of Change

     Each commit should correspond to a self-contained, meaningful change. For example, adding a
     feature, fixing a bug, or refactoring existing code should be separate commits. Avoid mixing
     different types of changes (e.g., feature implementation and unrelated refactoring) in the same
     commit.

  2. Retain Bisectability

     Every commit in the pull request must build successfully and pass all relevant tests. This
     ensures that git bisect can be used effectively to identify the specific commit that introduced
     a bug or issue.

  3. Squash Intermediary or Non-Final Development History

     During development, commits may include intermediary changes (e.g., partial implementations,
     temporary files, or debugging code). These should be squashed or rewritten before submitting the
     pull request. Remove non-final artifacts, such as:

     * Temporary renaming of files that are later renamed again.
     * Code that was rewritten or significantly changed in later commits.

  4. Ensure Clean History Before Submission

     Use interactive rebasing (git rebase -i) to clean up the commit history before submitting the
     pull request. This helps in:

     * Squashing small, incomplete commits into a single cohesive commit.
     * Ensuring that each commit remains bisectable.
     * Maintaining proper attribution of authorship while improving clarity.

  5. Renaming and Code Rewrites

     If files or code are renamed or rewritten in later commits during development, squash or rewrite
     earlier commits to reflect the final structure. This ensures that:

     * The history remains clean and easy to follow.
     * Bisectability is preserved by eliminating redundant renaming or partial rewrites.

  6. Attribution of Authorship

     While cleaning up the commit history, ensure that authorship attribution remains accurate.

  7. Readable and Reviewable History

     The final commit history should be easy to understand for future maintainers. Logical units of
     change should be grouped into commits that tell a clear, coherent story of the work done.

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

- Documentation must be added and/or updated to reflect the changes in the code
  introduced by the PR. The documentation changes must use the proper
  terminology as present in the existing pages, and must be written in American
  English. If you include images as part of the documentation, those must follow
  the rules in :ref:`doc_images`. Please refer to :ref:`doc_guidelines` for
  additional information.

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

Getting PRs Reviewed
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

Contributors may request PRs to be reviewed outside of the Zephyr Dev Meeting
triage process as follows:

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
Zephyr Dev Meeting before making a request themselves.

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

.. _Zephyr Dev Meeting: https://github.com/zephyrproject-rtos/zephyr/wiki/Zephyr-Committee-and-Working-Groups#zephyr-dev-meeting

.. _Architecture Project: https://github.com/zephyrproject-rtos/zephyr/projects/18

.. _Architecture Working Group: https://github.com/zephyrproject-rtos/zephyr/wiki/Architecture-Working-Group
