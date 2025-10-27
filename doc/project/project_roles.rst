.. _project_roles:

TSC Project Roles
*****************

Project Roles
#############

You can participate in the Zephyr Project as a *Contributor*, *Collaborator*, or *Maintainer*.

**Contributor**: Any community member who contributes code, documentation, or other assets to the
project.

**Collaborator**: An active Contributor who is highly involved in one or more areas of the project.

**Maintainer**: A lead Collaborator responsible for a specific area or subsystem within the project.
Maintainers also serve as representatives for their area on the Technical Steering Committee (TSC)
as needed.

Areas in the context of this document refer to specific subsystems, components, or modules within
the Zephyr Project codebase. Examples of areas include, but are not limited to, networking,
file systems, device drivers, architecture support, and board support packages (boards and SoC
definitions).

Areas in the project should at least have one Maintainer and may have multiple Collaborators.
Depending on the size and complexity of the area, there may be multiple Maintainers as well,
however, the number of maintainers should be kept to a practical minimum to ensure effective
management and decision-making.

.. _contributor:

Contributor
+++++++++++

A *Contributor* is a developer who wishes to contribute to the project,
at any level.

Contributors are granted the following rights and responsibilities:

* Right to contribute code, documentation, translations, artwork, etc.
* Right to report defects (bugs) and suggestions for enhancement.
* Right to participate in the process of reviewing contributions by others.
* Right to initiate and participate in discussions in any communication
  methods.
* Right to approach any member of the community with matters they believe
  to be important.
* Right to participate in the feature development process.
* Responsibility to abide by decisions, once made. They are welcome to
  provide new, relevant information to reopen decisions.
* Responsibility for issues and bugs introduced by one’s own contributions.
* Responsibility to respect the rules of the community.
* Responsibility to provide constructive advice whenever participating in
  discussions and in the review of contributions.
* Responsibility to follow the project’s code of conduct
  (https://github.com/zephyrproject-rtos/zephyr/blob/main/CODE_OF_CONDUCT.md)


Zephyr Contributor Badge
------------------------

When your first contribution to the Zephyr project gets merged, you'll become
eligible to claim your Zephyr Contributor Badge. This digital badge can be
displayed on your website, blog, social media profile, etc. It will allow you to
showcase your involvement in the Zephyr project and help raise its awareness.

You may apply for your Contributor Badge by filling out the `Zephyr Contributor
Badge form`_.

   .. _collaborator:

Collaborator
++++++++++++

A *Collaborator* is a Contributor who is also participating in the maintenance
of Zephyr source code. Their input weighs more when decisions are made, in a
fully meritocratic fashion.

You become a collaborator by showing consistent involvement in the project,
including making significant contributions to the project over a period of
time and by participating in the development and review of at least one area.

To request to become a Collaborator, submit a pull request adding yourself
to the ``collaborators`` section of an area in the :ref:`maintainers_file` and
notify the maintainers of that area. The addition needs to be approved by
the maintainers of that area.

Collaborators have the following rights and responsibilities, in addition to
those listed for Contributors:

* Right to participate in setting goals for the short and medium terms for the
  area being maintained, alongside the maintainers of that area.
* Responsibility to participate in the feature development process.
* Responsibility to review relevant code changes within reasonable time.
* Responsibility to ensure the quality of the code to expected levels.
* Responsibility to participate in community discussions.
* Responsibility to mentor new contributors when appropriate
* Responsibility to participate in the quality verification and release
  process, when those happen.

Collaborator change requests on pull requests should
be addressed by the original submitter. In cases where the changes requested do
not follow the :ref:`expectations <reviewer-expectations>` and the guidelines
of the project or in cases of disagreement, it is the responsibility of the
assignee to advance the review process and resolve any disagreements.

Collaborator approval of pull requests are counted toward the minimum required
approvals needed to merge a PR. Other criteria for merging may apply.


.. _maintainer:

Maintainer
++++++++++

A *Maintainer* is a Collaborator who is also responsible for knowing,
directing and anticipating the needs of a given zephyr source code area.

You become a maintainer of an area by

- showing consistent involvement in the project.
- making significant contributions to that area over a period of time and
  participating in the development and review of contributions.
- when introducing new areas to the project where you are the original author
  or have significant expertise.

To request to become a Maintainer, submit a pull request adding yourself
to the ``maintainers`` section of an area in the :ref:`maintainers_file` and
notify the existing maintainers of that area. The addition needs to be approved by
the maintainers of that area.

Maintainers have the following rights and responsibilities,
in addition to those listed for Contributors and Collaborators:

* Right to set the overall architecture of the relevant subsystems or areas
  of involvement.
* Right to make decisions in the relevant subsystems or areas of involvement,
  in conjunction with the collaborators and submitters.
  See :ref:`pr_technical_escalation`.
* Responsibility to convey the direction of the relevant subsystem or areas to
  the TSC
* Responsibility to ensure all contributions of the project have been reviewed
  within reasonable time.
* Responsibility to enforce the Code of Conduct. As leaders in the community,
  maintainers are expected to be role models in upholding the project's code
  of conduct and creating a welcoming and inclusive environment for everyone.
* Responsibility to triage static analysis issues in their code area.
  See :ref:`static_analysis`.

Maintainer approval of pull requests are counted toward the minimum
required approvals needed to merge a PR. Other criteria for merging may apply.

Role Retirement
###############

* Individuals elected to the following Project roles, including, Maintainer,
  Release Engineering Team member, Release Manager, but are no longer engaged
  in the project as described by the rights and responsibilities of that role,
  may be requested by the TSC to retire from the role they are elected.
* Such a request needs to be raised as a motion in the TSC and be
  approved by the TSC voting members.
  By approval of the TSC the individual is considered to be retired
  from the role they have been elected.
* The above applies to elected TSC Project roles that may be defined
  in addition.


Teams and Supporting Activities
###############################

Assignee
++++++++

An *Assignee* is one of the maintainers of an area being changed in a pull request.

Assignees are set either automatically based on the code being changed or set
by the other Maintainers, the Release Engineering team can set an assignee when
the latter is not possible.

* Responsibility to drive the pull request to a mergeable state
* Right to dismiss stale and unrelated reviews or reviews not following
  :ref:`expectations <reviewer-expectations>` from reviewers and seek reviews
  from additional maintainers, developers and contributors
* Right to block pull requests from being merged until issues or changes
  requested are addressed
* Responsibility to re-assign a pull request if they are the original submitter
  of the code
* Solicit approvals from maintainers of the subsystems affected
* Responsibility to drive the :ref:`pr_technical_escalation` process

Static Analysis Audit Team
++++++++++++++++++++++++++

The Static Analysis Audit team works closely with the release engineering
team to ensure that static analysis defects opened during a release
cycle are properly addressed. The team has the following rights and
responsibilities:

* Right to revert any triage in a static analysis tool (e.g: Coverity)
  that does not follow the project expectations.
* Responsibility to inform code owners about improper classifications.
* Responsibility to alert TSC if any issues are not adequately addressed by the
  responsible code owners.

Joining the Static Analysis Audit team

* Contributors highly involved in the project with some expertise
  in static analysis.


.. _release-engineering-team:

Release Engineering Team
++++++++++++++++++++++++

A team of active Maintainers involved in multiple areas.

* The members of the Release Engineering team are expected to fill
  the Release Manager role based on a defined cadence and selection process.
* The cadence and selection process are defined by the Release Engineering
  team and are approved by the TSC.
* The team reports directly into the TSC.

Release Engineering team has the following rights and responsibilities:

* Right to merge code changes to the zephyr tree following the project rules.
* Right to revert any changes that have broken the code base
* Right to close any stale changes after <N> months of no activity
* Responsibility to take directions from the TSC and follow them.
* Responsibility to coordinate code merges with maintainers.
* Responsibility to merge all contributions regardless of their
  origin and area if they have been approved by the respective
  maintainers and follow the merge criteria of a change.
* Responsibility to keep the Zephyr code base in a working and passing state
  (as per CI)

Joining the Release Engineering team

* Maintainers highly involved in the project may be nominated
  by a TSC voting member to join the Release Engineering team.
  Nominees may become members of the team by approval of the
  existing TSC voting members.
* To ensure a functional Release Engineering team the TSC shall
  periodically review the team’s followed processes,
  the appropriate size, and the membership
  composition (ensure, for example, that team members are
  geographically distributed across multiple locations and
  time-zones).


Release Manager
+++++++++++++++

A *Maintainer* responsible for driving a specific release to
completion following the milestones and the roadmap of the
project for this specific release.

* TSC has to approve a release manager.

A Release Manager is a member of the Release Engineering team and has
the rights and responsibilities of that team in addition to
the following:

* Right to manage and coordinate all code merges after the
  code freeze milestone (M3, see `program management overview <https://wiki.zephyrproject.org/Program-Management>`_.)
* Responsibility to drive and coordinate the triaging process
  for the release
* Responsibility to create the release notes of the release
* Responsibility to notify all stakeholders of the project,
  including the community at large about the status of the
  release in a timely manner.
* Responsibility to coordinate with QA and validation and
  verify changes either directly or through QA before major
  changes and major milestones.

Roles / Permissions
+++++++++++++++++++

.. table:: Project Roles vs GitHub Permissions
    :widths: 20 20 10 10 10 10 10
    :align: center

    ================ =================== =========== ================ =========== =========== ============
          ..             ..               **Admin**  **Merge Rights**   Member      Owner     Collaborator
    ---------------- ------------------- ----------- ---------------- ----------- ----------- ------------
    Main Roles       Contributor                                                                 x
    ---------------- ------------------- ----------- ---------------- ----------- ----------- ------------
        ..           Collaborator                                       x
    ---------------- ------------------- ----------- ---------------- ----------- ----------- ------------
        ..           Maintainer                                         x
    Supportive Roles QA/Validation                                      x                        x
        ..           DevOps                   **x**
        ..           System Admin             **x**                                      x
        ..           Release Engineering                 **x**          x

    ================ =================== =========== ================ =========== =========== ============


.. _maintainers_file:

MAINTAINERS File
################

The following guidelines apply to the structure, scope, and maintenance of the
MAINTAINERS file.

- The MAINTAINERS file shall have designated individuals responsible for the
  accuracy, structure, and upkeep of the file, in accordance with the Zephyr
  Project Charter. These individuals shall be appointed by the TSC.
- The granularity of maintainership should remain practical and manageable.
- The TSC, in collaboration with existing maintainers and contributors, should
  actively identify and encourage contributors to step up as maintainers for
  orphaned areas of the codebase and should facilitate the assignment of
  maintainers to those components.
- Unmaintained areas shall be clearly marked as such in the MAINTAINERS file.
- Updates to the MAINTAINERS file should:

  - Generally be included as standalone commits when introducing new files or
    directories.
  - For major structural changes, including the addition of new areas and new
    maintainers, should be submitted as separate pull requests, requiring
    approval by the file’s maintainers. Such activities might be the result of
    splitting existing large areas into smaller ones or merging smaller areas.

Guidelines for assigning maintainers to different areas of the codebase:

Architectures, core components, subsystems, samples, and tests:
  Each area shall have an explicitly assigned maintainer.

Boards (including related samples and tests) and SoCs (including DTS definitions)
  Each board and SoC should have an explicitly assigned maintainer through a
  platform area covering the boards, SoCs, and their related components and
  drivers.

Drivers / Backends
  The area of the API level Shall have a maintainer and specific driver
  implementations or backends shall be covered through a platform area covering
  the driver implementation.

.. _Zephyr Contributor Badge form: https://forms.gle/oCw9iAPLhUsHTapc8
