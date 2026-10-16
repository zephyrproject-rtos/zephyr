.. _waveshare_3_5_rpi_lcd_f:

Waveshare 3.5" RPi LCD (F)
##########################

Overview
********

The Waveshare 3.5" RPi LCD (F) has a resolution of 320x480 pixels and
is based on the ST7796S display controller. The capacitive touch panel
uses a GT911 touch controller.

More information about the shield can be found at the
`Waveshare 3.5inch RPi LCD (F) website`_.

Pin Assignments
===============

LP-MSPM0G3519
-------------

+--------------------+------------------------------------------+
| Signal             | Pin                                      |
+====================+==========================================+
| VCC                | 5.0V                                     |
+--------------------+------------------------------------------+
| GND                | GND                                      |
+--------------------+------------------------------------------+
| LCD_BL             | 3.3V.                                    |
+--------------------+------------------------------------------+
| SPI MOSI (SPI1)    | PB22 (SPI1 PICO)                         |
+--------------------+------------------------------------------+
| SPI CLK  (SPI1)    | PB23 (SPI1 SCLK)                         |
+--------------------+------------------------------------------+
| SPI CS   (SPI1)    | PA18 (GPIO, active-low)                  |
+--------------------+------------------------------------------+
| SPI MISO (opt.)    | PB7  (SPI1 POCI)                         |
+--------------------+------------------------------------------+
| ST7796S D/C        | PB1  (GPIO, active-high)                 |
+--------------------+------------------------------------------+
| ST7796S RST        | PB18 (GPIO, active-low)                  |
+--------------------+------------------------------------------+
| GT911 SDA (I2C0)   | PA0  (pull-up via jumper J16)            |
+--------------------+------------------------------------------+
| GT911 SCL (I2C0)   | PA1  (pull-up via jumper J17)            |
+--------------------+------------------------------------------+
| GT911 INT          | PB28 (GPIO, active-high)                 |
+--------------------+------------------------------------------+
| GT911 RST          | PB20 (GPIO, active-low)                  |
+--------------------+------------------------------------------+

Requirements
************

This shield can only be used with a board that provides a configuration
for SPI and I2C interfaces (see :ref:`shields` for more details).

Programming
***********

Set ``--shield waveshare_3_5_rpi_lcd_f`` when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: lp_mspm0g3519
   :shield: waveshare_3_5_rpi_lcd_f
   :goals: build

References
**********

.. target-notes::

.. _Waveshare 3.5inch RPi LCD (F) website:
   https://www.waveshare.com/3.5inch-rpi-lcd-f.htm
