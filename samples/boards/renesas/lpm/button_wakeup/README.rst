.. zephyr:code-sample:: renesas_button_wakeup
   :name: Renesas Button Wakeup Sample

   Demonstrates waking the board from PM_STATE_SOFT_OFF using a button

Overview
********

This sample shows how to use the Renesas WUC wakeup controller to
enable a physical button as a wakeup source and enter soft-off power
state (``PM_STATE_SOFT_OFF``). When ``CONFIG_POWEROFF`` is enabled, the
application uses ``sys_poweroff()`` to enter the state. Otherwise it uses
``pm_state_force()``. On wake, the application reports the wakeup source
via the console.

Hardware Setup
**************

- Use the provided board overlay in boards/ for pin mapping.
- Connect a momentary push button to the configured wakeup pin.

Building and Running
********************

To build and flash the sample on a supported Renesas EK board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/renesas/lpm/button_wakeup
   :board: ek_ra8p1/r7ka8p1kflcac/cm85
   :goals: build flash
   :compact:

When the board enters soft-off the console prints ``Entering Entering
Deep Software Standby mode via...``.
Pressing the configured wakeup button will wake the device and print
``Wakeup source triggered``.
Pressing the reset button will also wake the device, but the console
will not print ``Wakeup source triggered``.
