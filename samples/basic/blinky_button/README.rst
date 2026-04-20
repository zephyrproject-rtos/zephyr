.. zephyr:code-sample:: blinky-button
   :name: Button-controlled Blinky
   :relevant-api: gpio_interface

   Control an LED with a push button, supporting multiple blink modes.

Overview
********

This sample demonstrates using a push button to control LED behavior with
multiple modes:

- **Short press**: Toggle LED on/off (when in OFF mode)
- **Long press (>1 second)**: Cycle through blink speeds

Blink Modes
===========

1. **OFF**: LED is static, controlled by short press
2. **SLOW**: LED blinks with 1 second period
3. **MEDIUM**: LED blinks with 500ms period
4. **FAST**: LED blinks with 100ms period

Requirements
************

The board must have:

- An LED connected to a GPIO pin with alias ``led0``
- A push button connected to a GPIO pin with alias ``sw0``

Building and Running
********************

For RTS5912 EVB:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky_button
   :board: rts5912_evb
   :goals: build flash
   :compact:

Hardware Setup for RTS5912 EVB
==============================

- **LED0**: GPIO125 (active high) - connect LED between GPIO125 and GND
- **Button**: GPIO126 (active low with pull-up) - connect button between GPIO126 and GND

Sample Output
=============

.. code-block:: console

   Button-controlled Blinky Sample
   ================================
   Short press: Toggle LED (when in OFF mode)
   Long press (>1s): Cycle blink modes

   Starting in SLOW (1s) mode
   Blink mode changed to: MEDIUM (500ms)
   Blink mode changed to: FAST (100ms)
   Blink mode changed to: OFF
   LED toggled ON
   LED toggled OFF
