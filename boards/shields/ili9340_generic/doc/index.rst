.. _ili9340_generic:

Generic ILI9340 TFT Shield
#################################

Overview
********

This is a generic shield for display shields based on ILI9340 display
controller. More information about the controller can be found in
`ILI9340 Datasheet`_.

Pins Assignment of the Generic TFT Shield
========================================================

+-----------------------+---------------------------------------------+
| Arduino Connector Pin | Function                                    |
+=======================+=============================================+
| D4                    | MicroSD SPI CSn                             |
+-----------------------+---------------------------------------------+
| D8                    | STMPE610 SPI CSn (Resistive Touch Version)  |
+-----------------------+---------------------------------------------+
| D9                    | ILI9341 DC       (Data/Command)             |
+-----------------------+---------------------------------------------+
| D10                   | ILI9341 SPI CSn                             |
+-----------------------+---------------------------------------------+
| D11                   | SPI MOSI         (Serial Data Input)        |
+-----------------------+---------------------------------------------+
| D12                   | SPI MISO         (Serial Data Out)          |
+-----------------------+---------------------------------------------+
| D13                   | SPI SCK          (Serial Clock Input)       |
+-----------------------+---------------------------------------------+
| SDA                   | FT6206 SDA       (Capacitive Touch Version) |
+-----------------------+---------------------------------------------+
| SCL                   | FT6206 SCL       (Capacitive Touch Version) |
+-----------------------+---------------------------------------------+

Current supported displays
==========================

+----------------------------+-----------------------------------+
| Display                    | Shield Designation                |
|                            |                                   |
+============================+===================================+
| adafruit_2_8_tft_touch_v2  | ili9340_adafruit_2_8_tft_touch_v2 |
|                            |                                   |
+----------------------------+-----------------------------------+
| ili9340_seeed_tft_v2       | ili9340_ili9340_seeed_tft_v2      |
|                            |                                   |
+----------------------------+-----------------------------------+

.. _ili9340_adafruit_2_8_tft_touch_v2:

The Adafruit 2.8" TFT Touch Shield v2 with a
resolution of 320x240 pixels, is based on the ILI9341 controller.
This shield comes with a resistive (STMPE610 controller)
or capacitive (FT6206 controller) touchscreen. While the
Zephyr RTOS supports display output to these screens,
it currently does not support touchscreen input.
More information about the shield can be found
at the `Adafruit 2.8" TFT Touch Shield v2 website`_.

.. _ili9340_ili9340_seeed_tft_v2:

The Seeed 2.8" TFT Touch Shield v2 with a
resolution of 320x240 pixels, is based on the ILI9341 controller.
This shield comes with a raw resistive touchscreen (no controller)
While the Zephyr RTOS supports display output to these screens,
it currently does not support touchscreen input.
More information about the shield can be found
at the `Seeed 2.8" TFT Touch Shield v2 website`_.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=ili9340_adafruit_2_8_tft_touch_v2`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: nrf52840_pca10056
   :shield: ili9340_adafruit_2_8_tft_touch_v2
   :goals: build

References
**********

.. target-notes::

.. _Adafruit 2.8" TFT Touch Shield v2 website:
   https://learn.adafruit.com/adafruit-2-8-tft-touch-shield-v2

.. _Seeed 2.8" TFT Touch Shield v2 website:
   https://github.com/Seeed-Studio/TFT_Touch_Shield_V2

.. _ILI9340 Datasheet:
   https://cdn-shop.adafruit.com/datasheets/ILI9340.pdf
