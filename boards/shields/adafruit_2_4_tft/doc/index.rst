.. _adafruit_2_4_tft_v2.1:

Adafruit 2.8" TFT Shield v2.1
#################################

Overview
********

The Adafruit 2.4" TFT Shield with a
resolution of 320x240 pixels, is based on the ILI9341 controller.
This shield comes with a resistive (STMPE610 controller)
or capacitive (FT6206 controller) touchscreen. While the
Zephyr RTOS supports display output to these screens,
touchscreen input is supported only on Capacitive Touch version.
More information about the shield can be found at the `Adafruit 2.4" TFT
website <https://www.adafruit.com/product/1651>`_.

   Adafruit 2.4" TFT (Credit: Adafruit)

Pins Assignment of the Adafruit 2.4" TFT
========================================================

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
| D4                    | MicroSD SPI CSn                             |
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

TI BoosterHeader connector:
+-----------------------+--------------------------------------------+
| BoosterHeader Pin     | Function                                   |
+=======================+===============+============================+
| P4                    | ST7789V Reset |                            |
+-----------------------+---------------+----------------------------+
| P7                    | SPI SCK       | (Serial Clock Input)       |
+-----------------------+---------------+----------------------------+
| P14                   | SPI MISO      | (Serial Data Out)          |
+-----------------------+---------------+----------------------------+
| P15                   | SPI MOSI      | (Serial Data Input)        |
+-----------------------+---------------+----------------------------+
| P18                   | SPI CS LCD    | (LCD Chip Select)          |
+-----------------------+---------------+----------------------------+
| P30                   | ILI9341 DC    | (Data/Command)             |
+-----------------------+---------------+----------------------------+


.. note::
   Touch controller IRQ line is not connected by default. You will need to
   solder the ``ICSP_SI1`` jumper to use it. You will also need to adjust
   driver configuration and its Device Tree entry to make use of it.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino or Arduino Nano connectors and defines node aliases for SPI and
GPIO interfaces (see :ref:`shields` for more details).

Programming
***********

Set ``--shield adafruit_2_4_tft`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: lp_em_cc35x1/cc3551e
   :shield: adafruit_2_4_tft
   :goals: build

References
**********
