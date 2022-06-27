.. _mimxrt1060_evk-system-off-sample:

RT1060 System Off demo
######################

Overview
********

This sample can be used for basic power measurement and as an example of
soft off on NXP i.MX RT platforms. The functional behavior is:

* Busy-wait for 2 seconds
* Turn the system off after enabling wakeup through a button press, and
  additionally set an alarm 10 seconds in the future to wake up the processor

A power monitor will be able to distinguish among these states.

Requirements
************

This application uses MIMXRT1060_EVK or MIMXRT1060_EVKB for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/mimxrt1060_evk/system_off
   :board: mimxrt1060_evk
   :goals: build flash
   :compact:

Running:

1. Open UART terminal.
2. Power Cycle Device.
3. Device will turn on and idle for 2 seconds
4. Device will turn itself off using deep sleep state 1. Press SW 5
   to wake the device and restart the application as if it had been
   powered back on. Alternatively, wait 10 seconds for the SNVS RTC
   alarm to fire and wake the device up automatically.

Sample Output
=================
MIMXRT1060_EVK core output
--------------------------

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.0.0-2733-ged206aca47cc  ***

   mimxrt1060_evkb system off demo
   Busy-wait 2 s
   Sleep 2 s
   RTC Alarm set for 10 seconds to wake from soft-off.
   Entering system off; press GPIO_5 to restart sooner
