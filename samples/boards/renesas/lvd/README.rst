.. zephyr:code-sample:: renesas-lvd
   :name: Renesas Low-voltage Detection Sample

   Demonstrates monitoring and reacting to voltage levels.

Overview
********

This sample application shows how to use the Renesas Low-voltage Detection (LVD)
driver in Zephyr to monitor supply voltage and indicate status via LEDs.
The app reads the voltage status, handles LVD interrupts, and updates
LEDs based on whether the voltage is above or below a configured threshold.

Features
********

- Reads voltage status from the LVD peripheral.
- Handles LVD interrupts (if enabled in device tree/config).
- Indicates voltage status using two LEDs:
  - LED_ABOVE: On when voltage is above threshold.
  - LED_BELOW: On when voltage is below threshold.
- Prints status messages to the console.

Hardware Setup
**************

- Ensure that the monitored target pin is supplied with power.

Building and Running
********************

To build and flash the sample on a supported Renesas RX board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/renesas/lvd
   :board: rsk_rx130@512kb
   :goals: build flash
   :compact:

Usage
*****

- On startup, the app prints the configured voltage threshold.
- The app continuously monitors the voltage:
  - If voltage is **above** the threshold, LED_ABOVE is turned on, LED_BELOW is off.
  - If voltage is **below** the threshold, LED_BELOW is turned on, LED_ABOVE is off.
- If LVD interrupt is enabled, voltage changes will trigger an interrupt and update the
  status immediately.

Customization
*************

- You can change the voltage threshold and LVD action via device tree properties.
- LED pins can be reassigned by modifying the ``led0`` and ``led1`` aliases in your
  board's DTS.

Example Output
**************

::

   ===========================================================
   LVD Voltage Monitoring Sample Started
   Threshold Voltage Level : 384 mV
   ===========================================================
   [LVD Init] Voltage is ABOVE threshold (384 mV)
   [LVD] Interrupt: Voltage level crossing detected!
   [WARNING] Voltage instability detected! Check power supply.
   [LVD] Voltage is BELOW threshold (384 mV)
