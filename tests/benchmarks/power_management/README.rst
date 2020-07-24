Title: Generic test application for power management
#####################################

Overview
********

This testcase demonstrates power management features on boards that support SOC PWR MANAGEMENT.
It showcases simple app that allows to enter into light and deep sleep.

Building and Running
********************

The testcase can be built and executed on boards using west.
No pins configurations.


Sample output
=============

.. code-block:: console

   Wake from Light Sleep
   Wake from Deep Sleep
   ResumeBBBAA
   Wake from Light Sleep
   Suspend...
   Wake from Deep Sleep
   ResumeBBBAA

note:: The values shown above might differ.
