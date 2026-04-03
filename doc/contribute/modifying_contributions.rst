.. _modifying_contributions:

Modifying Contributions made by other developers
************************************************

Scenarios
#########

Zephyr contributors and collaborators are encouraged to assist
as reviewers in pull requests, so that patches may be approved and merged
to Zephyr's main branch as part of the original pull requests. The authors
of the pull requests are responsible for amending their original commits
following the review process.

There are occasions, however, when a contributor might need to modify patches
included in pull requests that are submitted by other Zephyr contributors.
For instance, this is the case when:

* a developer cherry-picks commits submitted by other contributors into their
  own pull requests in order to:

  * integrate useful content which is part of a stale pull request, or
  * get content merged to the project's main branch as part of a larger
    patch

* a developer pushes to a branch or pull request opened by another
  contributor in order to:

  * assist in updating pull requests in order to get the patches merged
    to the project's main branch
  * drive stale pull requests to completion so they can be merged


Accepted policies
#################

A developer who intends to cherry-pick and potentially modify patches sent by
another contributor shall:

* clarify in their pull request the reason for cherry-picking the patches,
  instead of assisting in getting the patches merged in their original
  pull request, and
* invite the original author of the patches to their pull request review.

A developer who intends to force-push to a branch or pull request of
another Zephyr contributor shall clarify in the pull request the reason
for pushing and for modifying the existing patches (e.g. stating that it
is done to drive the pull request review to completion, when the pull
request author is not able to do so).

.. note::
  Developers should try to limit the above practice to pull requests identified
  as *stale*. Read about how to identify pull requests as stale in
  :ref:`development processes and tools <dev-environment-and-tools>`

If the original patches are substantially modified, the developer can either:

* (preferably) reach out to the original author and request them to
  acknowledge that the modified patches may be merged while having
  the original sign-off line and author identity, or
* submit the modified patches as their *own* work (i.e. with their
  *own* sign-off line and author identity). In this case, the developer
  shall identify in the commit message(s) the original source the
  submitted work is based on (mentioning, for example, the original PR
  number).

.. note::
  Contributors should uncheck the box *“Allow Edits By Maintainers"*
  to indicate that they do not wish their patches to be amended,
  inside their original branch or pull request, by other Zephyr developers.
