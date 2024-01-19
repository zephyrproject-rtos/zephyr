.. zephyr:code-sample:: pm-latency
   :name: Power Management Latency
   :relevant-api: subsys_pm_sys subsys_pm_sys_policy

   Add and remove latency constraints using the System Power Management Policy API

Overview
********

This sample application demonstrates using the
:ref:`System Power Management Policy API<system-pm-apis>` to subscribe to latency changes, add
latency constraints, remove latency constraints, and unsubscribe from latency changes. Each time
the latency constraint is changed, the application sleeps in order to test that the correct power
management states are entered.

Building and Running
********************

Build and flash the sample as follows, changing ``msp_exp432p401r_launchxl`` to your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pm/latency
   :board: msp_exp432p401r_launchxl
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.5.0-4510-g18c23dec5f60 ***
   [00:00:00.000,000] <inf> app: Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE
   [00:00:01.100,000] <inf> app: Sleeping for 1.2 seconds, we should enter SUSPEND_TO_IDLE
   [00:00:02.300,000] <inf> app: Sleeping for 1.3 seconds, we should enter STANDBY
   [00:00:03.600,000] <inf> app: Setting latency constraint: 30ms
   [00:00:03.600,000] <inf> app: Latency constraint changed: 30ms
   [00:00:03.600,000] <inf> app: Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE
   [00:00:04.700,000] <inf> app: Sleeping for 1.2 seconds, we should enter SUSPEND_TO_IDLE
   [00:00:05.900,000] <inf> app: Sleeping for 1.3 seconds, we should enter SUSPEND_TO_IDLE
   [00:00:07.200,000] <inf> app: Opening test device
   [00:00:07.200,000] <inf> dev_test: Adding latency constraint: 20ms
   [00:00:07.200,000] <inf> app: Latency constraint changed: 20ms
   [00:00:07.200,000] <inf> app: Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE
   [00:00:08.301,000] <inf> app: Sleeping for 1.2 seconds, we should enter RUNTIME_IDLE
   [00:00:09.501,000] <inf> app: Sleeping for 1.3 seconds, we should enter RUNTIME_IDLE
   [00:00:10.801,000] <inf> app: Updating latency constraint: 10ms
   [00:00:10.801,000] <inf> app: Latency constraint changed: 10ms
   [00:00:10.801,000] <inf> dev_test: Latency constraint changed: 10ms
   [00:00:10.801,000] <inf> app: Sleeping for 1.1 seconds, we should stay ACTIVE
   [00:00:11.901,000] <inf> app: Sleeping for 1.2 seconds, we should stay ACTIVE
   [00:00:13.101,000] <inf> app: Sleeping for 1.3 seconds, we should stay ACTIVE
   [00:00:14.401,000] <inf> app: Updating latency constraint: 30ms
   [00:00:14.401,000] <inf> app: Latency constraint changed: 20ms
   [00:00:14.401,000] <inf> dev_test: Latency constraint changed: 20ms
   [00:00:14.401,000] <inf> app: Closing test device
   [00:00:14.401,000] <inf> dev_test: Removing latency constraint
   [00:00:14.401,000] <inf> app: Latency constraint changed: 30ms
   [00:00:14.401,000] <inf> dev_test: Latency constraint changed: 30ms
   [00:00:14.401,000] <inf> app: Sleeping for 1.1 seconds, we should enter RUNTIME_IDLE
   [00:00:15.501,000] <inf> app: Sleeping for 1.2 seconds, we should enter SUSPEND_TO_IDLE
   [00:00:16.701,000] <inf> app: Sleeping for 1.3 seconds, we should enter SUSPEND_TO_IDLE
   [00:00:18.002,000] <inf> app: Removing latency constraints
   [00:00:18.002,000] <inf> app: Latency constraint changed: none
   [00:00:18.002,000] <inf> app: Finished, we should now enter STANDBY
