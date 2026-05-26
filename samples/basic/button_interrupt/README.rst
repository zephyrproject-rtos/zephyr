Button Interrupt
################

Overview
********

This sample demonstrates GPIO interrupt handling on a button.
Press ``sw0`` to toggle ``led0`` and print the LED state over UART.

Requirements
************

- A board with ``sw0`` and ``led0`` devicetree aliases

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button_interrupt
   :board: cy8cproto_062_4343w
   :goals: build flash
   :compact:
