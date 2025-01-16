.. _project_roles:

TSC Project Roles
*****************

Project Roles
#############

TSC projects generally will involve *Maintainers*, *Collaborators*, and
*Contributors*:

**Maintainer**: lead Collaborators on an area identified by the TSC (e.g.
Architecture, code subsystems, etc.). Maintainers shall also serve as the
area’s representative on the TSC as needed. Maintainers may become voting
members of the TSC under the guidelines stated in the project Charter.

**Collaborator**: A highly involved Contributor in one or more areas.
May become a Maintainer with approval of existing TSC voting members.

**Contributor**: anyone in the community that contributes code or
documentation to the project. Contributors may become Collaborators
by approval of the existing Collaborators and Maintainers of the
particular code base areas or subsystems.


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

Contributors are initially only given `Read
<https://docs.github.com/en/organizations/managing-access-to-your-organizations-repositories/repository-permission-levels-for-an-organization>`_
access to the Zephyr GitHub repository. Specifically, at the Read access level,
Contributors are not allowed to assign reviewers to their own pull requests. An
automated process will assign reviewers. You may also share the pull request on
the `Zephyr devel mailing list <https://lists.zephyrproject.org/g/devel>`_ or on
the `Zephyr Discord Server <https://chat.zephyrproject.org>`_.

Contributors who show dedication and skill are granted the Triage permission
level to the Zephyr GitHub repository.

You may nominate yourself, or another GitHub user, for promotion to the Triage
permission level by creating a GitHub issue, using the :github:`nomination
template <new?assignees=&labels=Role+Nomination&template=006_nomination.md&title=>`.

Contributors granted the Triage permission level are permitted to add reviewers
to a pull request and can be added as a reviewer by other GitHub users.
Contributor change requests or approval on pull requests are not counted with
respect to accepting and merging a pull request. However, Contributors comments
and requested changes should still be considered by the pull request author.

Zephyr Contributor Badge
++++++++++++++++++++++++

When your first contribution to the Zephyr project gets merged, you'll become eligible to claim your
Zephyr Contributor Badge. This digital badge can be displayed on your website, blog, social media
profile, etc. It will allow you to showcase your involvement in the Zephyr project and help raise
its awareness.

You may apply for your Contributor Badge by filling out the `Zephyr Contributor Badge form`_.

.. _collaborator:

Collaborator
++++++++++++

A *Collaborator* is a Contributor who is also responsible for the maintenance
of Zephyr source code. Their opinions weigh more when decisions are made, in a
fully meritocratic fashion.

Collaborators have the following rights and responsibilities,
in addition to those listed for Contributors:

* Right to set goals for the short and medium terms for the project being
  maintained, alongside the Maintainer.
* Responsibility to participate in the feature development process.
* Responsibility to review relevant code changes within reasonable time.
* Responsibility to ensure the quality of the code to expected levels.
* Responsibility to participate in community discussions.
* Responsibility to mentor new contributors when appropriate
* Responsibility to participate in the quality verification and release
  process, when those happen.

Contributors are promoted to the Collaborator role by adding the GitHub user
name to one or more ``collaborators`` sections of the :ref:`maintainers_file` in
the Zephyr repository.

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
* Responsibility to enforce the code of conduct.
* Responsibility to triage static analysis issues in their code area.
  See :ref:`static_analysis`.

Contributors or Collaborators are promoted to the Maintainer role by adding the
GitHub user name to one or more ``maintainers`` sections of the
:ref:`maintainers_file` in the Zephyr repository. Candidates who are neither
Contributors nor Collaborators must be approved by the TSC before they can
assume the role of Maintainer.

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

An *Assignee* is one of the maintainers of a subsystem or code being changed.
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
        ..           Release Engineering      **x**      **x**          x

    ================ =================== =========== ================ =========== =========== ============


.. _maintainers_file:

MAINTAINERS File
################

Generic guidelines for deciding and filling in the Maintainers' list

* We should keep the granularity of code maintainership at a manageable level
* We should be looking for maintainers for areas of code that
  are orphaned (i.e. without an explicit maintainer)

  * Un-maintained areas should be indicated clearly in the MAINTAINERS file

* All submitted pull requests should have an assignee
* We Introduce an area/subsystem hierarchy to address the above point

  * Parent-area maintainer should be acting as default substitute/fallback
    assignee for un-maintained sub-areas
  * Area maintainer gets precedence over parent-area maintainer

* Pull requests may be re-assigned if this is needed or more appropriate

  * Re-assigned by original assignee

* In general, updates to the MAINTAINERS file should be
  in a standalone commit alongside other changes introducing new files and
  directories to the tree.
* Major changes to the file, including the addition of new areas with new maintainers
  should come in as standalone pull requests and require TSC review.
* If additional review by the TSC is required, the maintainers of the file
  should send the requested changes to the TSC and give members of the TSC two
  (2) days to object to any of the changes to maintainership of areas or the
  addition of new maintainers or areas.
* Path, collaborator and name changes do not require a review by the TSC.
* Addition of new areas without a maintainer do not require review by the TSC.
* The MAINTAINERS file itself shall have a maintainer
* Architectures, core components, sub-systems, samples, tests

  * Each area shall have an explicit maintainer

* Boards (incl relevant samples, tests), SoCs (incl DTS)
  * May have a maintainer, shall have a higher-level platform maintainer
* Drivers

  * Shall have a driver-area (and API) maintainer
  * Could have individual driver implementation
    maintainers but preferably collaborator/contributors
  * In the above case, platform-specific PRs may be
    re-assigned to respective collaborator/contributor of driver
    implementation


Release Activity
################

    .. figure:: img/img_release_activity.png
         :width: 663px
         :align: center
         :alt: Release Activity

.. _merge_criteria:

Merge Criteria
++++++++++++++

* All :ref:`pr_requirements` must be met.
* Minimal of 2 approvals, including an approval by the designated assignee.
* Pull requests should be reviewed by at least a maintainer or collaborator of
  each affected area; Unless the changes to a given area are considered trivial
  enough, in which case approvals by other affected subsystems
  maintainers/collaborators would suffice.
* Four eye principle on the organisation level. We already require at least 2
  approvals (basic four eye principle), however, such reviews and approvals
  might be unintentionally biased in the case where the submitter is from the
  same organisation as the approvers. To allow for project wide review and
  approvals, the merge criteria is extended with the guidelines below:

  * Changes or additions to common and shared code shall have approvals from
    different organisations (at least one approval from an
    organisation different than the submitters').
    Common and shared code is defined as anything that does not fall under
    :file:`soc`, :file:`boards` and :file:`drivers/*/*`.
  * Changes or additions to hardware support (driver, SoC, boards) shall at
    least have the merger be from a different organisation. This applies only
    to implementation of an API supporting vendor specific hardware and not the
    APIs.
  * Release engineers may make exceptions for areas with contributions primarily
    coming from one organisation and where reviews from other organisations are
    not possible, however, merges shall be completed by a person from a different
    organisation. In such cases, the minimum review period of at least 2 days
    shall be strictly followed to allow for additional reviews.
  * Release engineers shall not merge code changes originating and reviewed
    only by their own organisation. To be able to merge such changes, at least
    one review shall be from a different organisation.

* A minimum review period of 2 business days, 4 hours for trivial changes (see
  :ref:`review_time`).
* Hotfixes can be merged at any time after CI has passed and are excluded from
  most of the conditions listed above.
* All required checks are passing:

  * Device Tree
  * Documentation
  * Code linters (Gitlint, Pylint, Ruff, Sphinx, etc.)
  * Identity/Emails
  * Kconfig
  * License checks
  * Checkpatch (Coding Style)
  * Integration Tests (Via twister) on emulation/simulation platforms
  * Simulated Bluetooth Tests

* Planned

  * Footprint
  * Code coverage
  * Coding Guidelines
  * Static Analysis (Coverity)
  * Documentation coverage (APIs)


.. _Zephyr Contributor Badge form: https://forms.gle/oCw9iAPLhUsHTapc8
