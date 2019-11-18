.. _nrf5x-power-mgr-sample:

nRF5x Power management demo
###########################

Overview
********

This sample demonstrates a power manager app that uses the Zephyr system
power management infrastructure to manage multiple low-power states that
are conditional on the duration of sleep.  The power mode selection is
done automatically by residency-based policy implemented by the Zephyr
Power Management Subsystem.

After all combinations have been completed the system falls of the end
of main() resulting in the system being turned off.  Pressing button 1
will wake the system.

Note that these sleep states are simulated by placing the CPU into idle:
there is no reduction in microcontroller power consumption between
(non-deep) sleep states.  Disabling/enabling peripherals based on selected
power state is simulated by controlling LED state.

However, a power monitor will be able to distinguish the system active
(busy-wait), sleep, and system off (deep sleep) states.

Requirements
************

This application uses nRF51 DK or nRF52 DK board for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf52/power_mgr
   :board: nrf52_pca10040
   :goals: build flash
   :compact:

Running:

1. Open UART terminal.
2. Power Cycle Device.
3. Device will simulate the effect of power mode states on sleeps of
   various durations.
4. Device will turn itself off using deep sleep state 1.  Press Button 1
   to wake the device.

Sample Output
=================
nRF52 core output
-----------------

.. code-block:: console

   *** Power Management Demo on nrf52_pca10040 ***
   Simulate the effect of supporting system sleep states:
   * Sleep for durations less than 1500 ms (S0) leaves LEDs on
   * Sleep for durations between 1500 and 3500 ms (S1) turns off LED 1
   * Sleep for durations above 3500 ms (S2) turns off LED 2
   * Sleep above 60000 ms enters system off.
   3 power states are supported
   [  0.038]: Sys states for 03: +S1 +S2
   [  0.041]: Busy-wait 1 s
   [  1.058]: Sleep 1499 ms
   [  2.559]: Sleep 2500 ms
   [  2.561]:  --> Entering to SYS_POWER_STATE_SLEEP_1 state.
   [  5.061]:  --> Exited from SYS_POWER_STATE_SLEEP_1 state.
   [  5.066]: Sleep 3500 ms
   [  5.069]:  --> Entering to SYS_POWER_STATE_SLEEP_2 state.
   [  8.569]:  --> Exited from SYS_POWER_STATE_SLEEP_2 state.
   [  8.574]: Sys states for 02: -S1 +S2
   [  8.577]: Busy-wait 1 s
   [  9.582]: Sleep 1499 ms
   [ 11.084]: Sleep 2500 ms
   [ 13.586]: Sleep 3500 ms
   [ 13.588]:  --> Entering to SYS_POWER_STATE_SLEEP_2 state.
   [ 17.088]:  --> Exited from SYS_POWER_STATE_SLEEP_2 state.
   [ 17.094]: Sys states for 01: +S1 -S2
   [ 17.097]: Busy-wait 1 s
   [ 18.102]: Sleep 1499 ms
   [ 19.604]: Sleep 2500 ms
   [ 19.606]:  --> Entering to SYS_POWER_STATE_SLEEP_1 state.
   [ 22.106]:  --> Exited from SYS_POWER_STATE_SLEEP_1 state.
   [ 22.111]: Sleep 3500 ms
   [ 22.114]:  --> Entering to SYS_POWER_STATE_SLEEP_1 state.
   [ 25.614]:  --> Exited from SYS_POWER_STATE_SLEEP_1 state.
   [ 25.619]: Sys states for 00: -S1 -S2
   [ 25.622]: Busy-wait 1 s
   [ 26.626]: Sleep 1499 ms
   [ 28.128]: Sleep 2500 ms
   [ 30.630]: Sleep 3500 ms
   [ 34.132]: Completed cycle through sleep power state combinations
   [ 34.138]: Falling off main(); press BUTTON1 to restart 3000c
   [ 34.144]:  --> Entering to SYS_POWER_STATE_DEEP_SLEEP_1 state.
