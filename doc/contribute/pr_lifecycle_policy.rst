.. _pr_lifecycle_policy:

Pull Request Lifecycle Policy
#############################

This policy keeps the open pull request list focused on contributions that are actively
progressing and have a realistic path to merge.

Goals
*****

* Keep open pull requests manageable for contributors and maintainers.
* Prioritize review bandwidth for changes that are moving forward.
* Provide clear expectations for draft pull requests and inactive pull requests.

Scope
*****

This policy applies to all pull requests in the repository, including draft pull requests.

Definitions
***********

Active pull request
  A pull request with meaningful progress such as commits, review responses, or updates
  addressing requested changes.

Draft pull request
  A pull request opened for work in progress and early feedback.

Stalled pull request
  A pull request without meaningful activity inside the inactivity window.

Closed inactive pull request
  A pull request closed due to inactivity, supersession, or no clear merge path.

Draft Pull Request Expectations
*******************************

* Draft pull requests should include a clear problem statement, current status, and known gaps.
* Draft pull requests should move to ready-for-review within 30 days.
* If a draft pull request is inactive for 30 days, it may be marked stale and a reminder is posted.
* If there is no meaningful progress for 7 additional days after stale notice, the pull request
  may be closed.

Ready-for-Review Pull Request Expectations
******************************************

* Authors should respond to review feedback within 21 days.
* If a ready pull request has no meaningful activity for 30 days, it may be marked stale.
* If no meaningful update is made within 7 days after stale notice, the pull request
  may be closed.
* Pull requests with unresolved blocking feedback for more than 45 days may be closed if no
  concrete plan is provided.

What Counts as Meaningful Activity
**********************************

Examples of meaningful activity include:

* New commits that address review feedback.
* Concrete technical responses and follow-up updates.
* Significant revision updates that materially change the pull request.

Examples that typically do not count as meaningful activity include:

* Ping-only comments with no technical update.
* Trivial rebases with no substantive change.

Exceptions
**********

The following pull requests may be exempt from automatic inactive closure:

* Security-critical fixes.
* Release blockers.
* Pull requests explicitly waiting on an external dependency with a linked tracking item.

Exempt pull requests still need a status update at least every 30 days.

Superseded Pull Requests
************************

When work is replaced by another pull request:

* Close the old pull request as superseded.
* Link the replacement pull request.
* Add a short summary comment preserving context.

Reopening Closed Inactive Pull Requests
***************************************

Closing a pull request due to inactivity is an administrative step to keep the open list
manageable. It is not a rejection of the contribution.

Closed inactive pull requests may be reopened when:

* The author posts an updated plan.
* Prior blocking feedback is addressed.
* Maintainers confirm that review can continue.

Contributors are encouraged to reopen at any time when they can provide meaningful updates.

Responsibilities
****************

Contributors are expected to:

* Keep pull request descriptions up to date.
* Respond to review comments in a timely manner.
* Move draft pull requests to ready-for-review once they are reviewable.

Consult with the :ref:`contributor expectations <contributor-expectations>` document
for additional guidance.

Maintainers are expected to:

* Apply stale and closure decisions consistently.
* Provide clear blocking feedback.
* Close with explicit rationale and a reopen path.

Consult with the :ref:`maintainer responsibilities <maintainer>`  document for
additional guidance.
