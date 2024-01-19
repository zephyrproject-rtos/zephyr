.. zephyr:code-sample:: device-pm
   :name: Device Power Management
   :relevant-api: subsys_pm_device subsys_pm_device_runtime

   Resume and suspend devices using device runtime power management API

Overview
********

This sample application uses a dummy driver implementation to demonstrate using the
:ref:`Device Runtime Power Management API<device-runtime-pm-apis>` to resume and suspend devices.
As parent and child devices are resumed/suspended, the power management action is output to the
console.

Building and Running
********************

Build and flash the sample as follows, changing ``msp_exp432p401r_launchxl`` to your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pm/device_pm
   :board: msp_exp432p401r_launchxl
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   parent suspending..
   child suspending..
   *** Booting Zephyr OS build zephyr-v3.5.0-4510-g18c23dec5f60 ***
   Device PM sample app start
   parent resuming..
   child resuming..
   Dummy device resumed
   child suspending..
   parent suspending..
   Device PM sample app complete
