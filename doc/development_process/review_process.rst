

Development Environment and Tools
#################################

Code Review
************

GitHub is intended to provide a framework for reviewing every commit before it
is accepted into the code base. Changes, in the form of Pull Requests (PR) are
uploaded to GitHub but don’t actually become a part of the project until they’ve
been reviewed, passed a series of checks (CI), and are approved by maintainers.
GitHub is used to support the standard open source practice of submitting
patches, which are then reviewed by the project members before being applied to
the code base.

The Zephyr project uses GitHub for code reviews and Git tree management. When
submitting a change or an enhancement to any Zephyr component, a developer
should use GitHub. GitHub automatically assigns a responsible reviewer on a
component basis, as defined in the :file:`CODEOWNERS` file stored with the code
tree in the Zephyr project repository. A limited set of release managers are
allowed to merge a pull request into the master branch once reviews are complete.


Give reviewers time to review before code merge
================================================

The Zephyr project is a global project that is not tied to a certain geography
or timezone. We have developers and contributors from across the globe. When
changes are proposed using pull request, we need to allow for a minimal review
time to give developers and contributors the opportunity to review and comment
on changes. There are different categories of changes and we know that some
changes do require reviews by subject matter experts and owners of the subsystem
being changed. Many changes fall under the “trivial” category that can be
addressed with general reviews and do not need to be queued for a maintainer or
code-owner review. Additionally, some changes might require further discussions
and a decision by the TSC or the Security working group. To summarize the above,
the diagram below proposes minimal review times for each category:


.. figure:: pull_request_classes.png
    :align: center
    :alt: Pull request classes
    :figclass: align-center

    Pull request classes

Workflow
---------

- An author of a change can suggest in his pull-request which category a change
  should belong to. A project maintainers or TSC member monitoring the inflow of
  changes can change the label of a pull request by adding a comment justifying
  why a change should belong to another category.
- The project will use the label system to categorize the pull requests.
- Changes should not be merged before the minimal time has expired.

Categories/Labels
-----------------

Hotfix
++++++

Any change that is a fix to an issue that blocks developers from doing their
daily work, for example CI breakage, Test breakage, Minor documentation fixes
that impact the user experience.

Such fixes can be merged at any time after they have passed CI checks. Depending
on the fix, severity, and availability of someone to review them (other than the
author) they can be merged with justification without review by one of the
project owners.

Trivial
+++++++

Trivial changes are those that appear obvious enough and do not require maintainer or code-owner
involvement. Such changes should not change the logic or the design of a
subsystem or component. For example a trivial change can be:

- Documentation changes
- Configuration changes
- Minor Build System tweaks
- Minor optimization to code logic without changing the logic
- Test changes and fixes
- Sample modifications to support additional configuration or boards etc.

Maintainer
+++++++++++

Any changes that touch the logic or the original design of a subsystem or
component will need to be reviewed by the code owner or the designated subsystem
maintainer. If the code changes is initiated by a contributor or developer other
than the owner the pull request needs to be assigned to the code owner who will
have to drive the pull request to a mergeable state by giving feedback to the
author and asking for more reviews from other developers.

Security
+++++++++++

Changes that appear to have an impact to the overall security of the system need
to be reviewed by a security expert from the security working group.

TSC
++++

Changes that introduce new features or functionality or change the way the
overall system works need to be reviewed by the TSC or the responsible Working
Group. For example for API changes, the API working group needs to be consulted
and made aware of such changes.


A Pull-Request should have an Assignee
=======================================

- An assignee to a pull request should not be the same as the
  author of the pull-request
- An assignee to a pull request is responsible for driving the
  pull request to a mergeable state
- An assignee is responsible for dismissing stale reviews and seeking reviews
  from additional developers and contributors
- Pull requests should not be merged without an approval by the assignee.

Pull Request should not be merged by author without review
===========================================================

All pull requests need to be reviewed and should not be merged by the author
without a review. The following exceptions apply:

- Hot fixes: Fixing CI issues, reverts, and system breakage
- Release related changes: Changing version file, applying tags and release
  related activities without any code changes.

Developers and contributors should always seek review, however there are cases
when reviewers are not available and there is a need to get a code change into
the tree as soon as possible.

Reviewers shall not ‘Request Changes’ without comments or justification
=======================================================================

Any change requests (-1) on a pull request have to be justified. A reviewer
should avoid blocking a pull-request with no justification. If a reviewer feels
that a change should not be merged without their review, then: Request change
of the category: for example:

- Trivial -> Maintainer
- Assign Pull Request to yourself, this will mean that a pull request should
  not be merged without your approval.


Pull Requests should have at least 2 approvals before they are merged
======================================================================

A pull-request shall be merged only with two positive reviews (approval). Beside
the person merging the pull-request (merging != approval), two additional
approvals are required to be able to merge a pull request. The person merging
the request can merge without approving or approve and merge to get to the 2
approvals required.

Reviewers should keep track of pull requests they have provided feedback to
===========================================================================

If a reviewer has requested changes in a pull request, he or she should monitor
the state of the pull request and/or respond to mention requests to see if his
feedback has been addressed. Failing to do so, negative reviews shall be
dismissed by the assignee or an owner of the repository. Reviews will be
dismissed following the criteria below:

- The feedback or concerns were visibly addressed by the author
- The reviewer did not revisit the pull request after 1 week and multiple pings
  by the author
- The review is unrelated to the code change or asking for unjustified
  structural changes such as:

  - Split the PR
  - Split the commits
  - Can you fix this unrelated code that happens to appear in the diff
  - Can you fix unrelated issues
  - Etc.

Closing Stale Issues and Pull Requests
=======================================

- The Pull requests and issues sections on Github are NOT discussion forums.
  They are items that we need to execute and drive to closure.
  Use the mailing lists for discussions.
- In case of both issues and pull-requests the original poster needs to respond
  to questions and provide clarifications regarding the issue or the change.
  Failing to do so, an issue or a pull request will be closed automatically
  after 6 months.

Continuous Integration
***********************

All changes submitted to GitHub are subject to sanity tests that are run on
emulated platforms and architectures to identify breakage and regressions that
can be immediately identified. Sanity testing additionally performs build tests
of all boards and platforms. Documentation changes are also verified
through review and build testing to verify doc generation will be successful.

Any failures found during the CI test run will result in a negative review
assigned automatically by the CI system.
Developers are expected to fix issues and rework their patches and submit again.

The CI infrastructure currently runs the following tests:

- Run ''checkpatch'' for code style issues (can vote -1 on errors)
- Gitlint: Git commit style based on project requirements
- License Check: Check for conflicting licenses
- Run ''sanitycheck'' script

  - Run kernel tests in QEMU (can vote -1 on errors)
  - Build various samples for different boards (can vote -1 on errors)

- Verify documentation builds correctly.

