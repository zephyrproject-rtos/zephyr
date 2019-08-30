.. _ssd1306_128x64_shield:

SSD1306 128x64 pixels generic shield
####################################

Overview
********

Generic shield for OLED displays based on SSD1306 controller with
a resolution of 128x64 pixels. These displays have an I2C interface and
usually only four pins: GND, VCC, SCL and SDA. Display pins can be
connected to the pin header of a board using Jump wires.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines a node alias for I2C interface
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=ssd1306_128x64`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: frdm_k64f
   :shield: ssd1306_128x64
   :goals: build
