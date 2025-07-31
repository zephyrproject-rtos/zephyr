.. _blinky-sample:

Blinky
######

Overview
********

The Blinky sample is a simple application which blinks an LED forever using the
:ref:`GPIO API <gpio_api>`. The source code shows how to configure GPIO pins as
outputs, then turn them on and off.

This sample has been tested on the LPCXpresso54S018 board which features the
NXP LPC54S018J4MET180E microcontroller with external SPIFI flash.

Requirements
************

The board must have an LED connected via a GPIO pin. The sample code is
configured to work on boards that have defined the ``led0`` devicetree alias.

Building and Running
********************

Build the application for the ``lpcxpresso54s018`` board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: lpcxpresso54s018
   :goals: build
   :compact:

After flashing, the LED starts to blink and messages are printed on the console
showing the current LED state.