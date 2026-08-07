.. _twister_statuses:

Twister Status
##############

What is a Twister Status?
=========================

Twister Status formulates the current state of

- ``Harness``
- ``TestCase``
- ``TestSuite``
- ``TestInstance``

in a comprehensive and easy-to understand way.
In practice, most users will be interested in Statuses
of Instances and Cases after the conclusion of their Twister runs.

.. tip::

   Nomenclature reminder:

   .. tabs::

      .. tab:: ``Harness``

         ``Harness`` is a Python class inside Twister that allows us
         to capture and analyse output from a program external to Twister.
         It has been excised from this page for clarity,
         as it does not appear in final reports.

      .. tab:: ``TestCase``

         ``TestCase``, also called Case, is a piece of code that aims to verify some assertion.
         It is the smallest subdivision of testing in Zephyr.

      .. tab:: ``TestSuite``

         ``TestSuite``, also called Suite, is a grouping of Cases.
         One can modify Twister's behaviour on a per-Suite basis via ``testcase.yaml`` files.
         Such grouped Cases should have enough in common
         for it to make sense to treat them all the same by Twister.

      .. tab:: ``TestInstance``

         ``TestInstance``, also called Instance, is a Suite on some platform.
         Twister typically reports its results for Instances,
         despite them being called "Suites" there.
         If a status is marked as applicable for Suites, it is also applicable for Instances.
         As the distinction between them is not useful in this section,
         whenever you read "Suite", assume the same for Instances.

   More detailed explanation can be found :ref:`here <twister_tests_long_version>`.

Possible Twister Statuses
=========================

.. list-table:: Twister Statuses
   :widths: 10 10 66 7 7
   :header-rows: 1

   * - In-code
     - In-text
     - Description
     - Suite
     - Case
   * - FILTER
     - filtered
     - A static or runtime filter has eliminated the test from the list of tests to use.
     - ✓
     - ✓
   * - NOTRUN
     - not run
     - The test wasn't run because it was not runnable in current configuration.
       It was, however, built correctly.
     - ✓
     - ✓
   * - BLOCK
     - blocked
     - The test was not run because of an error or crash in the test suite.
     - ✕
     - ✓
   * - SKIP
     - skipped
     - Test was skipped by some other reason than previously delineated.
     - ✓
     - ✓
   * - ERROR
     - error
     - The test produced an error in running the test itself.
     - ✓
     - ✓
   * - FAIL
     - failed
     - The test produced results different than expected.
     - ✓
     - ✓
   * - PASS
     - passed
     - The test produced results as expected.
     - ✓
     - ✓

**In-code** and **In-text** are the naming contexts of a given status:
the former is rather internal for Twister and appears in logs,
whereas the latter is used in the JSON reports.

.. note::

   There are two more Statuses that are internal to Twister.
   ``NONE`` (A starting status for Cases and Suites) and
   ``STARTED`` (Indicating an in-progress Case).
   Those should not appear in the final Twister report.
   Appearance of those non-terminal Statuses in one's report files indicates a problem with Twister.


Case and Suite Status combinations
============================================

.. list-table:: Case and Suite Status combinations
   :widths: 22 13 13 13 13 13 13
   :align: center
   :header-rows: 1
   :stub-columns: 1

   * - ↓ Case\\Suite →
     - FILTER
     - ERROR
     - FAIL
     - PASS
     - NOTRUN
     - SKIP
   * - FILTER
     - ✓
     - ✕
     - ✕
     - ✕
     - ✕
     - ✕
   * - ERROR
     - ✕
     - ✓
     - ✕
     - ✕
     - ✕
     - ✕
   * - BLOCK
     - ✕
     - ✓
     - ✓
     - ✕
     - ✕
     - ✕
   * - FAIL
     - ✕
     - ✓
     - ✓
     - ✕
     - ✕
     - ✕
   * - PASS
     - ✕
     - ✓
     - ✓
     - ✓
     - ✕
     - ✕
   * - NOTRUN
     - ✕
     - ✕
     - ✕
     - ✕
     - ✓
     - ✕
   * - SKIP
     - ✕
     - ✓
     - ✓
     - ✓
     - ✕
     - ✓

✕ indicates that such a combination should not happen in a proper Twister run. In other words,
no Suite of a status indicated by the table column should contain any Cases of a status indicated
by the table row.

✓ indicates a proper combination.

Detailed explanation, per Suite Status
-------------------------------------------

``FILTER``:
  This status indicates that the whole Suite has been statically filtered
  out of a given Twister run. Thus, any Case within it should also have such a status.

``ERROR``:
  Suite encountered a problem when running the test. It requires at least one case with
  ``ERROR`` or ``BLOCK`` status. As this takes precedence over all other Case statuses, all valid
  terminal Case statuses can be within such a Suite.

``FAIL``:
  Suite has at least one Case that did not meet its assertions. This takes precedence over
  all other Case statuses, given that the conditions for an ERROR status have not been met.

``PASS``:
  Suite has passed properly. It cannot contain any Cases with ``BLOCK``, ``ERROR``, or ``FAIL``
  statuses, as those indicate a problem when running the Suite.

``NOTRUN``:
  Whole suite was not run, but only built. It requires than all Cases within were not run.
  As runnability is decided on a per-Suite basis, only ``NOTRUN`` is applicable for its Cases.

``SKIP``:
  Whole Suite has been skipped at runtime. All Cases need to have ``SKIP`` status as well.
