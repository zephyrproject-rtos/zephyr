.. _safety_process-quality_mgmt:

Quality Management
##################

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

The quality management process includes establishing quality policies and objectives and processes to achieve these objectives through quality planning, quality assurance, quality control and quality improvement.
Quality planning, part of quality management, also includes establishing a quality plan (i.e. this document).

This process is applicable to Zephyr project safety releases and applies to all of the software life cycle data, software tools and hardware tools produced and used throughout the development life cycle.

Overview
========

.. figure:: ../images/TBD.svg
   :align: center
   :alt: Zephyr quality management process activity diagram
   :figclass: align-center

   Quality management activity diagram


.. tabs::

   .. group-tab:: Entry

      #. Project start

   .. group-tab:: Input

      #. Standards activities and requirements
      #. Released software development lifecycle processes
      #. Project lifecycle activities list

   .. group-tab:: Activities

      #. Define quality objectives
      #. Specify operational processes, and related resources
      #. Plan & Perform quality assurance activities
      #. Plan & Perform quality control activities
      #. Plan & Perform quality improvements activities

   .. group-tab:: Output   

      #. Quality objectives
      #. Operational processes for each lifecycle activity
      #. Quality plan
      #. Quality assurance report
      #. Quality control report
      #. Quality issues and related improvement plan

   .. group-tab:: Exit

      #. Project release

   .. group-tab:: Tools

      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Tool
          - Purpose
          - Location

        * - Safety Committee Git
          - Safety plan register
          - | Documented in safety plan
            | (Disclosed to Platinum members and assessors only)

        * - Git issues
          - Quality issues
          - https://github.com/zephyrproject-rtos/<REPOSITORY>/issues

   .. group-tab:: R & R
      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Role
          - Responsibilities

        * - Quality Assurance Manager
          - Plans and performs quality activities, with independence from the project team responsible for any lifecycle activity (other than quality management activities)

.. note:: 

   For this release and for the safety scope, the Safety manager is acting as the Quality assurance manager.

Activities
==========

Quality Objectives
------------------

The quality objectives are defined as follows:

#. Ensuring compliance of plans with applicable standards
#. Enforcing compliance to project plans, processes and procedures
#. Continuously improving quality 

Operational processes, Quality assurance & control
--------------------------------------------------

Operational processes, Quality assurance & control
Compliance of plans to applicable standards and compliance to project plans, processes and procedures is ensured by process audits.

These audits consists of the following quality assurance and controls activities:

#. **Plans, processes and procedures reviews:** Ensures that 
   
   #. All the processes are defined for all the software lifecycle activities to be performed by the project team 
   #. All the processes comply to and collaboratively achieve compliance to the target standards referenced in the project and/or safety plan 

#. **Process exit gate reviews:** To ensure that the lifecycle process activities were performed, in compliance with the plans
#. **Samples inspections:** To ensure that the lifecycle artifacts produced as part of the process activities comply with the plans

These activities may be performed altogether by the quality manager, for a single process, no later than at process exit.

Each process may be reviewed by inspection and/or personal judgement in reference to the applicable standards. If any, inspection checklist for specific processes shall be defined within the :ref:`process_inspection_checklist` section.

The quality assurance manager shall record the quality assurance activities within the quality reports tool.

**Process exit audits**

The quality assurance manager shall perform process exit audits for each process and record them within the quality report tool.

Each process exit audit checklist shall be documented within the process documents themselves and used by the quality assurance manager to perform the audit.

**Samples inspection**

The quality assurance manager shall perform samples inspection process exit audits and record them within the quality report tool.

Each process exit audit checklist shall be documented within the process documents themselves and used by the quality assurance manager to perform the audit.


Quality Issues & Improvement
----------------------------

**Issues process**

Continuous improvement of quality is ensured by recording issues within the quality issue tool :

#. As a result of quality assurance & controls activities
#. As a result of process gaps findings, by any project personnel, from lifecycle activities execution

These issues are then managed to closure or logged as deviations through the change management process.

**Escalations**

For issues that cannot be addressed by the quality and project personnel, escalation is available to the technical steering committee.


.. _process_inspection_checklist:

Process inspection checklist
============================

.. admonition:: WIP
   :class: warning

   To be documented by the safety committee: Inspection checklists to be defined for each process.
