.. _safety_process-safety_manual:

Safety Manual
#############

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

This process aims at defining the how is the safety manual created, based on all the safety artefacts created as part of the software development lifecycle.

This process is applicable to Zephyr project safety releases and applies to all of the software life cycle data, software tools and hardware tools produced and used throughout the development life cycle.

Overview
========

.. figure:: ../images/TBD.svg
   :align: center
   :alt: Zephyr safety manual creation process activity diagram
   :figclass: align-center

   Safety manual creation activity diagram

.. tabs::

   .. group-tab:: Entry

      #. Safety claims are added
      #. Safety claims are updated
      #. Safety analysis is completed
      #. Safety analysis is updated
      #. Software baseline is updated
      #. Software configuration is updated
      #. Tools configuration is updated

   .. group-tab:: Input

      #. Safety plan
      #. Safety analyses
      #. Software baseline
      #. Software configuration
      #. Tools configuration

   .. group-tab:: Activities

      #. TBD

   .. group-tab:: Output

      #. Safety manual

   .. group-tab:: Exit

      #. Safety manual report is in configuration management
      #. Issues resulting from this process are resolved
      #. Non-resolved issues are logged and marked as non-gating
      #. Safety manual is released

   .. group-tab:: Tools

      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Tool
          - Purpose
          - Location

        * - Requirement repository
          - Requirements register
          - `Requirement repository <https://github.com/zephyrproject-rtos/reqmgmt>`__

   .. group-tab:: R & R
      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Role
          - Responsibilities

        * - Safety Manager
          - TBD

Activities
==========

.. admonition:: WIP
   :class: warning

   To be documented by the safety committee: Complete section
