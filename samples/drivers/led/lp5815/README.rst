.. zephyr:code-sample:: lp5815
   :name: LP5815 RGB LED
   :relevant-api: led_interface

   Control an RGB LED using the LP5815 driver.

Overview
********

This sample demonstrates the LP5815 3-channel RGB LED driver. It cycles
through several colors using ``led_set_color()``, starts a hardware blink
using ``led_blink()``, and then returns to manual control with
``led_set_color()``.

Requirements
************

A board with a TI LP5815 LED controller connected via I2C, with an RGB LED
wired to the three output channels.

Building and Running
********************

This sample requires a board-specific overlay that defines the LP5815 node
on the I2C bus. Build and flash as usual:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/lp5815
   :goals: build flash
