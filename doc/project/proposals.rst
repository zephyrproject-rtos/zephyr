.. _feature-tracking:

Feature Tracking
#################

To propose something new to the Zephyr Project:

#. Start by searching existing `issues
   <https://github.com/zephyrproject-rtos/zephyr/issues>`_ to see if there
   is prior work on the area.
#. If not, feel free to create a `new issue
   <https://github.com/zephyrproject-rtos/zephyr/issues/new/choose>`_ of
   the appropriate :ref:`type <issue_types>`.

You should contact an assigned owner if you'd like to discuss or contribute to
an existing issue's implementation.

.. _issue_types:

Issue Types
***********

Every issue has exactly one GitHub issue type, selected by the template used to
create it:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Type
     - Use for

   * - Bug
     - An unexpected problem or behavior in something that exists. See
       :ref:`bug_reporting`.

   * - Feature
     - A new capability that Zephyr does not have.

   * - Enhancement
     - An addition or improvement to something Zephyr already has.

   * - RFC
     - A proposal that needs design discussion and consensus before it is
       implemented. See :ref:`rfcs`.

   * - Task
     - Work that does not change what Zephyr provides, such as process,
       infrastructure or maintenance work.

.. _feature_vs_enhancement:

Feature or Enhancement
======================

What separates the two types is whether the capability is new to Zephyr, not the
size of the change or whether it is new to one platform.

A **Feature** adds a capability that has no counterpart in the tree. It usually
introduces new API surface, new concepts or new conventions for users, and is
therefore often preceded by an :ref:`RFC <rfcs>`. Examples:

- Support for a new CPU architecture.
- A new subsystem, kernel service, library or protocol stack.
- A new device driver class, that is, a new driver API.
- A new tool or build system capability.

An **Enhancement** extends, completes or improves a capability that is already
part of Zephyr, within its existing APIs and design. Examples:

- A new option, mode or function in an existing subsystem or API.
- Better performance, footprint, diagnostics, test coverage or documentation.
- Support for new hardware within an existing architecture and driver class.

Adding hardware support is not a Feature. A new board, SoC, SoC series, shield
or device driver, including a sensor, a peripheral controller or the
implementation of an existing driver class for one more SoC, plugs into APIs
and infrastructure that Zephyr already provides. Such work is an Enhancement
when it is tracked in an issue, and it does not need an issue to be submitted as
a pull request.

.. note::

   Previously, the project used `GitHub labels
   <https://docs.github.com/en/issues/using-labels-and-milestones-to-track-work/managing-labels>`_
   to track feature-related work. You may find issues with labels like
   "Feature" or "Enhancement".

   These labels are deprecated for new issues, which should be of the
   appropriate type instead. For example, create an issue of type Enhancement
   instead of adding the Enhancement label to a new issue.

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
