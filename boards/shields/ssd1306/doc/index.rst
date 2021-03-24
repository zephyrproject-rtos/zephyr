.. _ssd1306_128_shield:

SSD1306 128x64(/32) pixels generic shield
#########################################

Overview
********

This is a generic shield for 128x64(/32) pixel resolution OLED displays
based on SSD1306 controller. These displays have an I2C interface and
usually only four pins: GND, VCC, SCL and SDA. Display pins can be
connected to the pin header of a board using jumper wires.

Current supported displays
==========================

+---------------------+---------------------+---------------------+
| Display             | Controller /        | Shield Designation  |
|                     | Driver              |                     |
+=====================+=====================+=====================+
| No Name             | SSD1306 /           | ssd1306_128x64      |
| 128x64 pixel        | ssd1306             |                     |
+---------------------+---------------------+---------------------+
| No Name             | SSD1306 /           | ssd1306_128x32      |
| 128x32 pixel        | ssd1306             |                     |
+---------------------+---------------------+---------------------+
| No Name             | SH1106 /            | sh1106_128x64       |
| 128x64 pixel        | ssd1306             |                     |
+---------------------+---------------------+---------------------+

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines a node alias for the I2C interface
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=ssd1306_128x64`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: frdm_k64f
   :shield: ssd1306_128x64
   :goals: build
