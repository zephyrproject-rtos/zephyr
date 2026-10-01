.. _safety_process-cfg_chg_mgmt:

Configuration & Change Management
#################################

Document Identification
***********************

.. list-table::
   :header-rows: 1
   :width: 100%

   * - Version
     - Status
     - Release date (DD/MM/YYYY)
     - Review & Approval

   * - 1.0
     - Draft
     - 18/09/2026
     - See merge request history [#f1]_

.. [#f1] Refer to Configuration & Change Management process for how to identify commit and merge request related to a release.

Version history
***************

.. list-table::
   :header-rows: 1
   :widths: 10 90
   :width: 100%

   * - Version
     - Reason for change

   * - 1.0
     - Initial release

Process
*******

Purpose & Scope
===============

The configuration management process aims at defining and controlling the configuration of development lifecycle artifacts throughout the development life cycle.

Configuration management shall enable consistently replicating/regenerating the released software (including its configuration) file, providing unique identification of inputs and outputs to ensure accuracy and repeatability of development lifecycle activities, recording issues and changes and evidence of approval of the software by control of the outputs of the software life cycle processes and ensuring that archiving, recovery, and control mechanisms are in place

This process is based on the established :ref:`contribute_guidelines`.

This processed is refined in the SWG_ConfigurationManagementPlan.sdoc within the safety committee repository.

This process is applicable to any Zephyr related software project and applies to all of the software life cycle data, software tools and hardware tools produced and used throughout the development life cycle.


Overview
========

.. figure:: ../images/TBD.svg
   :align: center
   :alt: Zephyr configuration and change management process activity diagram
   :figclass: align-center

   Configuration and change management activity diagram

.. tabs::

   .. group-tab:: Activities

      #. Unique identification
      #. Naming convention
      #. Version
      #. Issues / Issues reports
      #. Backup / Archive / Retrieval
      #. Change control
      #. Software load validity
      #. Release

   .. group-tab:: Tools

      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Tool
          - Purpose
          - Location

        * - Git
          - | Configuration management
            | Archival
            | Backup / Recovery
            | Release / Delivery
          - https://github.com/zephyrproject-rtos/<REPOSITORY>

        * - Git issues
          - | Change requests
            | Issue / Issues reports
            | Change impact analysis

          - https://github.com/zephyrproject-rtos/<REPOSITORY>/issues

   .. group-tab:: R & R
      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Role
          - Responsibilities

        * - Safety Manager
          - Plan and coordinate safety activities related to changes to the configuration for the release

        * - Release owner
          - Coordinate changes to the configuration for the release

Plan
****

Unique identification
=====================

Software lifecycle artefacts are uniquely identified by their full path and revision in the CM tool.
As such, any reference to an artefact during a lifecycle activity shall point to both the full path and the revision.
For example:
https://github.com/zephyrproject-rtos/zephyr/blob/3269b49b660de1868785a588df1c01e6b6665339/doc/safety/safety_overview.rst is a proper reference
https://github.com/zephyrproject-rtos/zephyr/blob/main/doc/safety/safety_overview.rst is not a proper reference

Naming convention
=================

Since all files are identified by their full path, and given that there is no possibility of 2 files with the same name at the same location, then no particular naming convention is required.
The project personnel are encouraged, but not required, to streamline file names.

For example:
tool_qual_report-<TOOL_NAME> pattern for tools qualification reports
quality_audit-<AUDIT_TOPIC> for QA audits reviews

Version
=======

Artifacts are versioned by the CM tool and do not require further versioning.
A release date and/or version may be added to documents to help project personnel easily identify whether a reference artifact was updated.

Issues / Issues reports
=======================

Issue / Issues reports shall be logged in the tool identified within the tools section.

.. NOTE::
   Add activity diagram for issues / change management.

The issue process is documented in `Issues <https://docs.zephyrproject.org/latest/project/issues.html>`_.

Backup / Archive / Retrieval
============================

The backup, archive and retrieval duties are entrusted to the CM tool provider through their `ISO 27001 Certification <https://github.com/github/roadmap/issues/245>`_.

Change control
==============

Change to the development lifecycle artefacts is protected through the CM tool login and roles policy and roles are granted as defined in :ref:`Project Roles <project_roles>`.

Changes are proposed and reviewed through the process defined in :ref:`dev-environment-and-tools`.

Software load validity
======================

.. admonition:: WIP
   :class: warning

   To be documented by the safety committee: How to capture the software configuration and as well as ensure that SW that is tested is the one that is released

Software environment configuration
==================================

.. admonition:: WIP
   :class: warning

   To be documented by the safety committee: How to capture the software development environment configuration
