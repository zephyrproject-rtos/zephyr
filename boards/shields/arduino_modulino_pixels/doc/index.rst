.. _arduino_modulino_pixels:

Arduino Modulino Pixels
#######################

Overview
********

The Arduino Modulino Pixels is a QWIIC compatible module with 8 addressable
LEDs.


.. image:: img/arduino_modulino_pixels.webp
     :align: center
     :alt: Arduino Modulino Pixels

Programming
***********

Set ``--shield arduino_modulino_pixels`` when you invoke ``west build``, the
leds will be available through the LED strip subsystem.

For example,

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: arduino_uno_r4@wifi
   :shield: arduino_modulino_pixels
   :goals: build
