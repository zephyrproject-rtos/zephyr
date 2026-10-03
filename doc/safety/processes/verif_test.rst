.. _safety_process-verif_test:

Verification Test
#################

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

The verification test process aims at defining when and how shall contributors develop and perform verification test activities.
This process also defines activities to be performed when test is not the appropriate mean to verify a requirement (e.g. verification through analysis).

This process is applicable to Zephyr project safety releases and applies to all of the software life cycle data, software tools and hardware tools produced and used throughout the development life cycle.

Overview
========

.. figure:: ../images/TBD.svg
   :align: center
   :alt: Zephyr verification test process activity diagram
   :figclass: align-center

   Verification test activity diagram

.. tabs::

   .. group-tab:: Entry

      #. New development artifact (requirement, architecture, design)
      #. Updated development artifact (requirement, architecture, design)

   .. group-tab:: Input

      #. Development artifact

   .. group-tab:: Activities

      #. Develop test cases
      #. Develop test procedures

   .. group-tab:: Output

      #. Test cases
      #. Procedures

   .. group-tab:: Exit

      #. Test cases and procedures are configuration management
      #. Issues resulting from this process are resolved
      #. Non-resolved issues are logged and marked as non-gating
      #. Test cases and procedures are released

   .. group-tab:: Tools

      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Tool
          - Purpose
          - Location

        * - Git
          - Test cases and procedures repository
          - https://github.com/zephyrproject-rtos/<REPOSITORY>/pulls

   .. group-tab:: R & R
      .. list-table::
        :header-rows: 1
        :width: 100%

        * - Role
          - Responsibilities

        * - Contributor
          - Develops and submits test cases and procedures for review and merge

        * - Maintainer
          - Review and approves changes

Activities
==========

Develop test cases
------------------

Test cases shall be documented using doxygen as follows:

.. code-block:: c

    /**
    @brief Test multi-threads to get data from a queue

    @details Define thre threads and do stuff...

    Pass / Fail criteria:
    1. When X then Y
    2. When A then B

    @verbatim embed:rst
    - external+req:ref:'ZEP-SRS-20-6'
    - external+req:ref:'ZEP-SRS-20-7'
    @endverbatim
    */

Test cases shall be developed such that they cover all possible scenarios from a requirement or set of requirements.

.. list-table:: Acitivity Audit & Review checklist
        :header-rows: 1
        :widths: 10 50 40
        :width: 100%

        * - ID
          - Checklist item
          - Example (optional)

        * - 1
          - Is the test case, in conunction with other cases, covering the requirement(s) intent?
          - None

        * - 2
          - Is the test case traced to requirement(s) it is covering?
          - None

.. admonition:: WIP
   :class: warning

   To be documented by the safety committee: Complete checklist

Develop test procedures
-----------------------

Test procedures shall be developed such that they cover all possible scenarios from a requirement or set of requirements.

.. list-table:: Acitivity Audit & Review checklist
        :header-rows: 1
        :widths: 10 50 40
        :width: 100%

        * - ID
          - Checklist item
          - Example (optional)

        * - 1
          - Is the test procedure implementing the test cases?
          - None

        * - 2
          - Is the test procedure execution result a PASS?
          - None

.. admonition:: WIP
   :class: warning

   To be documented by the safety committee: Complete checklist
