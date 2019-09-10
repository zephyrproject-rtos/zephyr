.. _ssd1306_128x32_shield:

SSD1306 128x32 pixels generic shield
####################################

Overview
********

This is a generic shield for 128x32 pixel resolution OLED displays
based on SSD1306 controller. These displays have an I2C interface and
usually only four pins: GND, VCC, SCL and SDA. Display pins can be
connected to the pin header of a board using jumper wires.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines a node alias for the I2C interface
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=ssd1306_128x32`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: frdm_k64f
   :shield: ssd1306_128x32
   :goals: build
