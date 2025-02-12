.. zephyr:code-sample:: sx1509b
   :name: SX1509B RGB LED
   :relevant-api: led_interface

   Control an RGB LED connected to an SX1509B driver chip.

Overview
********

This sample controls the intensity of the color LEDs in a Thingy:52 lightwell.
The red, green and blue LED fade up one by one, and this repeats forever.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_sx1509b_intensity
   :board: thingy52/nrf52832
   :goals: build flash
   :compact:

The log is configured to output to RTT. Segger J-Link RTT Viewer can be used to view the log.
