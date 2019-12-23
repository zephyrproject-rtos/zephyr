.. _gpio-led-sample:

LED GPIO Sample
###############

Overview
********

The sample assumes that the LEDs are connected to GPIO lines.
It is configured to work on boards that have defined the led0 and led1
aliases and id properties for them in their board devicetree description file.
The LED id properties are required for the LED GPIO driver.
The LEDs are controlled, using the
following pattern:

1. Turn on LEDs
#. Turn off LEDs
#. Blink the LEDs
#. Turn off LEDs

Building and Running
********************

This sample can be built and flashed to a board as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/driver/led_gpio
   :board: nrf52840_pca10056
   :goals: build flash
   :compact:

After flashing the image to the board, the user LEDs on the board should act according to the given pattern.
