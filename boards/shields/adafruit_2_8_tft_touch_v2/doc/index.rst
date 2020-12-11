.. _adafruit_2_8_tft_touch_v2:

Adafruit 2.8" TFT Touch Shield v2
#################################

Overview
********

The Adafruit 2.8" TFT Touch Shield v2 with a
resolution of 320x240 pixels, is based on the ILI9341 controller.
This shield comes with a resistive (STMPE610 controller)
or capacitive (FT6206 controller) touchscreen. While the
Zephyr RTOS supports display output to these screens,
touchscreen input is supported only on Capacitive Touch version.
More information about the shield can be found
at the `Adafruit 2.8" TFT Touch Shield v2 website`_.

Pins Assignment of the Adafruit 2.8" TFT Touch Shield v2
========================================================

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
| D4                    | MicroSD SPI CSn                             |
+-----------------------+---------------------------------------------+
| D7                    | Touch Controller IRQ (see note below)       |
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

.. note::
   Touch controller IRQ line is not connected by default. You will need to
   solder the ``ICSP_SI1`` jumper to use it. You will also need to adjust
   driver configuration and its Device Tree entry to make use of it.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``-DSHIELD=adafruit_2_8_tft_touch_v2`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/gui/lvgl
   :board: nrf52840dk_nrf52840
   :shield: adafruit_2_8_tft_touch_v2
   :goals: build

References
**********

.. target-notes::

.. _Adafruit 2.8" TFT Touch Shield v2 website:
   https://learn.adafruit.com/adafruit-2-8-tft-touch-shield-v2
