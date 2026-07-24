.. zephyr:code-sample:: task-wdt
   :name: Task watchdog
   :relevant-api: task_wdt_api

   Monitor a thread using a task watchdog.

Overview
********

This sample allows to test the :ref:`task watchdog <task_wdt_api>` subsystem.

Building and Running
********************

It should be possible to build and run the task watchdog sample on almost any
board. If a hardware watchdog is defined in the devicetree, it is used as a
fallback. Otherwise the task watchdog will run independently.

Building and Running for ST Nucleo L073RZ
=========================================
The sample can be built and executed for the
:zephyr:board:`nucleo_l073rz` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/task_wdt
   :board: nucleo_l073rz
   :goals: build flash
   :compact:

For other boards just replace the board name.

Sample output
=============

The following output is printed:

.. code-block:: console

   Task watchdog sample application.
   Main thread still alive...
   Control thread started.
   Main thread still alive...
   Main thread still alive...
   Main thread still alive...
   Control thread getting stuck...
   Main thread still alive...
   Task watchdog channel 1 callback, thread: control
   Resetting device...

The intentional watchdog reset is demonstrated only once per power-on:
after the device has been reset by the task watchdog, the control thread
keeps feeding its watchdog channel so that the board stays alive. This
avoids an endless reset loop, which could otherwise interfere with
flashing the next application (e.g. in automated device testing). Power
cycle the board to run the demonstration again.
