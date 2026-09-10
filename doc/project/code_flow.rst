.. _code-flow-and-branches:

Code Flow and Branches
######################

Introduction
************

The zephyr Git repository has three types of branches:

main
  Which contains the latest state of development

collab-\*
  Collaboration branches that are used for shared development
  of new features to be introduced into the main branch when ready. Creating a new
  collaboration branch requires a justification and TSC approval. Collaboration branches
  shall be based off the main branch and any changes developed in the collab
  branch shall target the main development branch. For released versions of
  Zephyr, the introduction of fixes and new features, if approved by the TSC,
  shall be done using backport pull requests.

vx.y-branch
  Branches which track maintenance releases based on a major
  release

Development in collaboration branches before features go to mainline allows teams to
work independently on a subsystem or a feature, improves efficiency and
turnaround time, and encourages collaboration and streamlines communication
between developers.

Changes submitted to a collaboration branch can evolve and improve
incrementally in a branch, before they are submitted to the mainline tree for
final integration.

By dedicating an isolated branch to complex features, it's
possible to initiate in-depth discussions around new additions before
integrating them into the official project.

Collaboration branches are ephemeral and shall be removed once the collaboration work
has been completed. When a branch is requested, the proposal should include the
following:

* Define exit criteria for merging the collaboration branch changes back into the main branch.
* Define a timeline for the expected life cycle of the branch. It is
  recommended to select a Zephyr release to set the timeline. Extensions to
  this timeline requires TSC approval.

Roles and Responsibilities
**************************

Collaboration branch owners have the following responsibilities:

- Use the infrastructure and tools provided by the project (GitHub, Git)
- All changes to collaboration branches shall come in form of github pull requests.
- Force pushing a collaboration branch is only allowed when rebasing against the main branch.
- Review changes coming from team members and request review from branch owners
  when submitting changes.
- Keep the branch in sync with upstream and update on a regular basis.
- Push changes frequently to upstream using the following methods:

  - GitHub pull requests: for example, when reviews have not been done in the local
    branch (one-man branch).
  - Merge requests: When a set of changes has been done in a local branch and
    has been reviewed and tested in a collaboration branch.
