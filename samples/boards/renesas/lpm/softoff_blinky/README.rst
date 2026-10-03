.. zephyr:code-sample:: renesas_softoff_blinky
   :name: Renesas Softoff Blinky Sample

   Demonstrates LED state restore after wake from PM_STATE_SOFT_OFF

Overview
********

This sample toggles an LED and demonstrates restoring LED state after
waking from soft-off. It shows integration between WUC and a wakeup
counter to periodically wake the device and toggle the LED.

Hardware Setup
**************

- Use the provided board overlay in boards/ for pin mapping.
- Ensure the LED0 alias is present on the target board.

Building and Running
********************

To build and flash the sample on a supported Renesas EK board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/renesas/lpm/softoff_blinky
   :board: ek_ra8p1/r7ka8p1kflcac/cm85
   :goals: build flash
   :compact:

The sample prints the restored LED state on wake (``LED: ON`` or ``LED: OFF``)
and will enter soft-off (prints ``Entering PM_STATE_SOFT_OFF``) when setup
is complete.
