.. zephyr:code-sample:: renesas_lvd
   :name: Renesas Low-voltage Detection Sample using Comparator

   Demonstrates monitoring and reacting to voltage levels.

Overview
********

This sample application shows how to use Comparator to monitor voltage levels
with Renesas Low-voltage Detection (LVD).

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

The comparator configures trigger is rising edge. When the voltage on the monitored pin
crosses the threshold (Vref) - defined in the board's device tree, an interrupt is generated
and turns the LED on.
