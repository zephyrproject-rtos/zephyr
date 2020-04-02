.. _nrf-system-off-sample:

nRF5x System Off demo
#####################

Overview
********

This sample can be used for basic power measurement and as an example of
deep sleep on Nordic platforms.  The functional behavior is:

* Busy-wait for 2 seconds
* Sleep for 2 seconds
* Sleep for a duration that would, by policy, cause the system to power
  off if the deep sleep state was not disabled
* Turn the system off after enabling wakeup through a button press

A power monitor will be able to distinguish among these states.

Requirements
************

This application uses nRF51 DK or nRF52 DK board for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf52/system_off
   :board: nrf52dk_nrf52832
   :goals: build flash
   :compact:

Running:

1. Open UART terminal.
2. Power Cycle Device.
3. Device will demonstrate two activity levels which can be measured.
4. Device will demonstrate long sleep at minimal non-off power.
5. Device will turn itself off using deep sleep state 1.  Press Button 1
   to wake the device and restart the application as if it had been
   powered back on.

Sample Output
=================
nRF52 core output
-----------------

.. code-block:: console

   ***** Booting Zephyr OS build v2.1.0-rc1-158-gb642e1a96d17 *****

   nrf52dk_nrf52832 system off demo
   Busy-wait 2 s
   Sleep 2 s
   Sleep 60000 ms (deep sleep minimum)
   Entering system off; press BUTTON1 to restart
