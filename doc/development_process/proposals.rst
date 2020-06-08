.. _feature-tracking:

Feature Tracking
#################

For feature tracking we use Github labels to classify new features and
enhancements. The following is the description of each category:

Enhancement
  Changes to existing features that are not considered a bug and would not
  block a release. This is an incremental enhancement to a feature that already
  exists in Zephyr.

Feature request
  A request for the implementation or inclusion of a new unit of functionality
  that is not part of any release plans yet, that has not been vetted, and needs
  further discussion and details.

Feature
  A committed and planned unit of functionality with a detailed design and
  implementation proposal and an owner. Features must go through an RFC process
  and must be vetted and discussed in the TSC before a target milestone is set.

Hardware Support
  A request or plan to port an existing feature or enhancement to a particular
  hardware platform. This ranges from porting Zephyr itself to a new
  architecture, SoC or board to adding an implementation of a peripheral driver
  API for an existing hardware platform.

Meta
  A label to group other GitHub issues that are part of a single feature or unit
  of work.

The following workflow should be used to process features:.

This is the formal way for asking for a new feature in Zephyr and indicating its
importance to the project.  Often, the requester may have a readiness and
willingness to drive implementation of the feature in an upcoming release, and
should assign the request to themselves.
If not though, an owner will be assigned after evaluation by the TSC.
A feature request can also have a companion RFC with more details on the feature
and a proposed design or implementation.

- Label new features requests as ``feature-request``
- The TSC discusses new ``feature-request`` items regularly and triages them.
  Items are examined for similarity with existing features, how they fit with
  the project goals and other timeline considerations. The priority is
  determined as follows:

  - High = Next milestone
  - Medium = As soon as possible
  - Low = Best effort

- After the initial discussion and triaging, the label is moved from
  ``feature-request`` to ``feature`` with the target milestone and an assignee.

All items marked as ``feature-request`` are non-binding and those without an
assignee are open for grabs, meaning that they can be picked up and implemented
by any project member or the community. You should contact an assigned owner if
you'd like to discuss or contribute to that feature's implementation


.. _rfcs:

Proposals and RFCs
*******************

Many changes, including bug fixes and documentation improvements can be
implemented and reviewed via the normal GitHub pull request workflow.

Many changes however are "substantial" and need to go through a
design process and produce a consensus among the project stakeholders.

The "RFC" (request for comments) process is intended to provide a consistent and
controlled path for new features to enter the project.

Contributors and project stakeholders should consider using this process if
they intend to make "substantial" changes to Zephyr or its documentation. Some
examples that would benefit from an RFC are:

- A new feature that creates new API surface area, and would require a feature
  flag if introduced.
- The modification of an existing stable API
- The removal of features that already shipped as part of Zephyr.
- The introduction of new idiomatic usage or conventions, even if they do not
  include code changes to Zephyr itself.

The RFC process is a great opportunity to get more eyeballs on proposals coming
from contributors before it becomes a part of Zephyr. Quite often, even
proposals that seem "obvious" can be significantly improved once a wider group
of interested people have a chance to weigh in.

The RFC process can also be helpful to encourage discussions about a proposed
feature as it is being designed, and incorporate important constraints into the
design while it's easier to change, before the design has been fully
implemented.

Some changes do not require an RFC:

- Rephrasing, reorganizing or refactoring
- Addition or removal of warnings
- Addition of new boards, SoCs or drivers to existing subsystems
- ...

The process in itself consists in creating a GitHub issue with the :ref:`RFC
label <gh_labels>` that documents the proposal thoroughly. There is an `RFC
template`_ included in the main Zephyr GitHub repository that serves as a
guideline to write a new RFC.

As with Pull Requests, RFCs might require discussion in the context of one of
the `Zephyr meetings`_ in order to move it forward in cases where there is
either disagreement or not enough voiced opinions in order to proceed. Make sure
to either label it appropriately or include it in the corresponding GitHub
project in order for it to be examined during the next meeting.

Roadmap and Release Plans
*************************

Project roadmaps and release plans are both important tools for the project, but
they have very different purposes and should not be confused. A project roadmap
communicates the high-level overview of a project's strategy, while a release
plan is a tactical document designed to capture and track the features planned
for upcoming releases.

- The project roadmap communicates the why; a release plan details the what
- A release plan spans only a few months; a product roadmap might cover a year
  or more


Project Roadmap
================

The project roadmap should serve as a high-level, visual summary of the
project's strategic objectives and expectations.

If built properly, the roadmap can be a valuable tool for several reasons. It
can help the project present its plan in a compelling way to existing and new
stakeholders, to help recruit new members and it can be a helpful resource the
team and community can refer to throughout the project's development, to ensure
they are still executing according to plan.

As such, the roadmap should contain only strategic-level details, major project
themes, epics, and goals.


Release Plans
==============

The release plan comes into play when the project roadmap's high-level strategy
is translated into an actionable plan built on specific features, enhancements,
and fixes that need to go into a specific release or milestone.

The release plan communicates those features and enhancements slated for your
project' next release (or the next few releases). So it acts as more of a
project plan, breaking the big ideas down into smaller projects the community
and main stakeholders of the project can make progress on.

Items labeled as ``features`` are short or long term release items that shall
have an assignee and a milestone set.

.. _`RFC template`: https://github.com/zephyrproject-rtos/zephyr/blob/master/.github/ISSUE_TEMPLATE/rfc---proposal.md
.. _`Zephyr meetings`: https://github.com/zephyrproject-rtos/zephyr/wiki/Zephyr-Committee-and-Working-Group-Meetings
