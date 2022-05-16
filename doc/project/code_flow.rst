.. _code-flow-and-branches:

Code Flow and Branches
######################

Introduction
************

The zephyr Git repository has three types of branches:

main
  Which contains the latest state of development

topic-\*
  Topic branches that are used for shared development of a new feature

vx.y-branch
  Branches which track maintenance releases based on a major
  release

Development in topic branches before features go to mainline allows teams to
work independently on a subsystem or a feature, improves efficiency and
turnaround time, and encourages collaboration and streamlines communication
between developers.

Changes submitted to a development topic branch can evolve and improve
incrementally in a branch, before they are submitted to the mainline tree for
final integration.

By dedicating an isolated branch to complex features, it's
possible to initiate in-depth discussions around new additions before
integrating them into the official project.


Roles and Responsibilities
**************************

Development topic branch owners have the following responsibilities:

- Use the infrastructure and tools provided by the project (GitHub, Git)
- Review changes coming from team members and request review from branch owners
  when submitting changes.
- Keep the branch in sync with upstream and update on a regular basis.
- Push changes frequently to upstream using the following methods:

  - GitHub pull requests: for example, when reviews have not been done in the local
    branch (one-man branch).
  - Merge requests: When a set of changes has been done in a local branch and
    has been reviewed and tested in a topic branch.
