.. _st7735r_generic:

Generic ST7735R Display Shield
##############################

Overview
********

This is a generic shield for display shields based on ST7735R display
controller. More information about the controller can be found in
`ST7735R Datasheet`_.

Pins Assignment of the Generic ST7735R Display Shield
=====================================================

+-----------------------+--------------------------------------------+
| Arduino Connector Pin | Function                                   |
+=======================+===============+============================+
| D8                    | ST7735R Reset |                            |
+-----------------------+---------------+----------------------------+
| D9                    | ST7735R DC    | (Data/Command)             |
+-----------------------+---------------+----------------------------+
| D10                   | SPI SS        | (Serial Slave Select)      |
+-----------------------+---------------+----------------------------+
| D11                   | SPI MOSI      | (Serial Data Input)        |
+-----------------------+---------------+----------------------------+
| D12                   | SPI MISO      | (Serial Data Out)          |
+-----------------------+---------------+----------------------------+
| D13                   | SPI SCK       | (Serial Clock Input)       |
+-----------------------+---------------+----------------------------+

Current supported displays
==========================

+----------------------+------------------------------+
| Display              | Shield Designation           |
|                      |                              |
+======================+==============================+
| adafruit             | st7735r_ada_160x128          |
| 160x128 18bit TFT    |                              |
+----------------------+------------------------------+

Requirements
************

This shield can only be used with a board that provides a configuration
for Arduino connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=st7735r_ada_160x128`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: nrf52840dk_nrf52840
   :shield: st7735r_ada_160x128
   :goals: build

References
**********

.. target-notes::

.. _ST7735R Datasheet:
   https://www.crystalfontz.com/controllers/Sitronix/ST7735R/319/
