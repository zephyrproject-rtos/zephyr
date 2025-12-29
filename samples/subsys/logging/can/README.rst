.. zephyr:code-sample:: logging_can
   :name: CAN Logging
   :relevant-api: can_controller log_api log_ctrl

   Send log messages over CAN bus via the logging subsystem.

Overview
********
This application showcases various features of the CAN logging backend.

Building and Running
********************

This project outputs multiple log message to the console.  It can be built and
executed using the native simulator:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/logging/can
   :host-os: unix
   :board: native_sum
   :goals: run
   :compact:

The sample expects a zcan0 CAN bus.
Under common Linux systems, a virtual can device can be created using:

.. code-block:: console

         ip link add zcan0 type vcan
         ip link set up dev zcan0

If no zcan0 device exists, the example might segfault.

Sample Output
=============

.. code-block:: console

         *** Booting Zephyr OS build v4.3.0-2984-g6eb6ff649346 ***
         [00:00:00.000,000] <inf> sample_main: These log messages are sent under CAN ID 0x7ff (std CAN ID)
         [00:00:00.000,000] <inf> sample_main: Listen to these messages using candump -a or scripts/logging/can/recv_can_log.c
         [00:00:01.010,000] <inf> sample_main: example message 1
         [00:00:02.020,000] <inf> sample_main: example message 2
         [00:00:03.030,000] <inf> sample_main: example message 3
         [00:00:04.040,000] <inf> sample_main: Switching to CAN ID 0x1234abcd (extended CAN ID)
         [00:00:07.050,000] <inf> sample_main: example message 4
         [00:00:08.060,000] <inf> sample_main: example message 5
         [00:00:09.070,000] <inf> sample_main: example message 6
         [00:00:10.080,000] <inf> sample_main: Switching to CAN FD with baud rate switching (same CAN ID)
         [00:00:13.090,000] <inf> sample_main: example message 7
         [00:00:14.100,000] <inf> sample_main: example message 8
         [00:00:15.110,000] <inf> sample_main: example message 9
