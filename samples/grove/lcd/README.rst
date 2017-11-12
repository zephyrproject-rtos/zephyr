.. _grove-lcd-sample:

Grove LCD
#########

Overview
********

This sample displays an incrementing counter through the Grove LCD, with
changing backlight.

Requirements
************

To use this sample, the following hardware is required:

* Arduino 101 or Quark D2000 Devboard
* `Grove LCD module`_
* `Grove Base Shield`_ [Optional]

Wiring
******

You will need to connect the Grove LCD via the Grove shield onto a board that
supports Arduino shields.

On some boards you will need to use 2 pull-up resistors (10k Ohm) between the
SCL/SDA lines and 3.3V.

.. note::

   The I2C lines on Quark SE Sensor Subsystem does not have internal pull-up, so
   external one is required.

Take note that even though SDA and SCL are connected to a 3.3V power source, the
Grove LCD VDD line needs to be connected to the 5V power line, otherwise
characters will not be displayed on the LCD (3.3V is enough to power just the
backlight).


Building and Running
********************

This sample should work on any board that has I2C enabled and has an Arduino
shield interface. For example, it can be run on the Quark D2000 DevBoard as
described below:

.. zephyr-app-commands::
   :zephyr-app: samples/grove/lcd
   :board: quark_d2000_crb
   :goals: flash
   :compact:

.. _Grove Base Shield: http://wiki.seeedstudio.com/wiki/Grove_-_Base_Shield
.. _Grove LCD module: http://wiki.seeed.cc/Grove-LCD_RGB_Backlight/
