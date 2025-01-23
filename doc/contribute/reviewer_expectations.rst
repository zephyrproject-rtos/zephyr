.. _reviewer-expectations:

Reviewer Expectations
#####################

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

- Style changes that the reviewer disagrees with but that are not documented as
  part of the project can be pointed out as non-blocking, but cannot constitute
  a reason for a request for changes. The reviewer can optionally correct any
  potential inconsistencies in the tree, document the new guidelines or rules,
  and then enforce them as part of the review.

- Whenever requesting style related changes, reviewers should be able to point
  out the corresponding guideline, rule or rationale in the project's
  documentation.  This does not apply to certain types of requests for changes,
  notably those specific to the changes being submitted (e.g. the use of a
  particular data structure or the choice of locking primitives).

- Reviewers shall be *clear* about what changes they are requesting when the
  "Request Changes" option is used. Requested changes shall be in the scope of
  the PR in question and following the contribution and style guidelines of the
  project. Furthermore, reviewers must be able to point back to the exact issues
  in the PR that triggered a request for changes.

- Reviewers should not request changes for issues which are automatically
  caught by CI, as this causes the pull request to remain blocked even after CI
  failures have been addressed and may unnecessarily delay it from being merged.

- Reviewers shall not close a PR due to technical or structural disagreement.
  If requested changes cannot be resolved within the review process, the
  :ref:`pr_technical_escalation` path shall be used for any potential resolution
  path, which may include closing the PR.

.. _Code of Conduct: https://github.com/zephyrproject-rtos/zephyr/blob/main/CODE_OF_CONDUCT.md

.. _Zephyr Release Plan: https://github.com/orgs/zephyrproject-rtos/projects/13
