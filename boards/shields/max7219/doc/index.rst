.. _max7219_shield:

MAX7219 LED display driver shield
#################################

Overview
********

This is a generic shield for LED matrix based on MAX7219.
These displays have an SPI interface and usually
only five pins: VCC, GND, DIN, CS, and CLK.

Current supported displays
==========================

+---------------------+---------------------+---------------------+
| Display             | Controller          | Shield Designation  |
|                     |                     |                     |
+=====================+=====================+=====================+
| No Name             | MAX7219             | max7219_8x8         |
| 8x8 pixel           |                     |                     |
+---------------------+---------------------+---------------------+

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines a node alias for the SPI interface
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=max7219_8x8`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display/
   :board: nrf52840dk_nrf52840
   :shield: max7219_8x8
   :goals: build
