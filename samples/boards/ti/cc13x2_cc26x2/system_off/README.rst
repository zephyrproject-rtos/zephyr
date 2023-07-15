.. _ti-cc13x2_cc26x2-system-off-sample:

cc13x2_cc26x2 System Off demo
#############################

Overview
********

This sample can be used for basic power measurement and as an example of
the various sleep modes on TI CC13x2/CC26x2 platforms.  The functional
behavior is:

* Busy-wait for 5 seconds
* Sleep for 2 milliseconds (Idle mode)
* Sleep for 3 seconds (Standby mode)
* Turn the system off after enabling wakeup through a button press
  (Shutdown mode)

A power monitor (e.g. `EnergyTrace <http://www.ti.com/tool/ENERGYTRACE>`_)
can be used to distinguish among these states.

Requirements
************

This application uses the CC13x2/CC26x2 LaunchPad for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/ti/cc13x2_cc26x2/system_off
   :board: cc1352r1_launchxl
   :goals: build flash
   :compact:

After flashing the device, run the code:

1. Unplug the USB cable from the LaunchPad and reconnect it to fully
   power-cycle it.
2. Open UART terminal.
3. Hit the Reset button on the LaunchPad.
4. Device will turn off the external flash, which is on by default, to
   reduce power consumption.
5. Device will demonstrate active, idle and standby modes.
6. Device will explicitly turn itself off by going into shutdown mode.
   Press Button 1 to wake the device and restart the application as if
   it had been powered back on.

Sample Output
=================

cc1352rl_launchxl output
------------------------

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v2.2.0-664-gd774727cc66e  ***

        cc1352r1_launchxl system off demo
        Busy-wait 5 s
        Sleep 2000 us (IDLE)
        Sleep 3 s (STANDBY)
        Entering system off (SHUTDOWN); press BUTTON1 to restart
