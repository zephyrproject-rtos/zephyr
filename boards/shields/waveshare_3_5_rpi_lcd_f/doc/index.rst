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
| LCD_BL             | 3.3V                                     |
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

LP-MSPM33C321A
--------------

The shield connects via the LP-MSPM33C321A BoosterPack headers.
SPI uses UNICOMM2 (UC2), I2C uses UNICOMM15_0.
Jumpers J16/J17 must be installed (default) for I2C pull-ups on PA0/PA1.

+--------------------+------------------------------------------+
| Signal             | Pin                                      |
+====================+==========================================+
| VCC                | 5.0V                                     |
+--------------------+------------------------------------------+
| GND                | GND                                      |
+--------------------+------------------------------------------+
| LCD_BL             | 3.3V                                     |
+--------------------+------------------------------------------+
| SPI MOSI (UC2)     | PB17 (UC2 PICO)                          |
+--------------------+------------------------------------------+
| SPI CLK  (UC2)     | PB18 (UC2 SCLK)                          |
+--------------------+------------------------------------------+
| SPI MISO (opt.)    | PB19 (UC2 POCI)                          |
+--------------------+------------------------------------------+
| SPI CS   (GPIO)    | PA2  (GPIO, active-low)                  |
+--------------------+------------------------------------------+
| ST7796S D/C        | PA25 (GPIO, active-high)                 |
+--------------------+------------------------------------------+
| ST7796S RST        | PB26 (GPIO, active-low)                  |
+--------------------+------------------------------------------+
| GT911 SDA (I2C0)   | PA0  (pull-up via jumper J16)            |
+--------------------+------------------------------------------+
| GT911 SCL (I2C0)   | PA1  (pull-up via jumper J17)            |
+--------------------+------------------------------------------+
| GT911 INT          | PB20 (GPIO, active-high)                 |
+--------------------+------------------------------------------+
| GT911 RST          | PB21 (GPIO, active-low)                  |
+--------------------+------------------------------------------+

LP-AM13E230
-----------

The shield connects via the LP-AM13E230 BoosterPack headers.
Jumpers J17 and J18 must be installed (default) for I2C pull-ups.

+--------------------+--------+--------+--------------------------------+
| Signal             | RPi pin| Header | AM13E23019 pin                 |
+====================+========+========+================================+
| VCC (5V)           | 4      | J3-21  | 5V rail                        |
+--------------------+--------+--------+--------------------------------+
| GND                | 6      | J3-22  | GND                            |
+--------------------+--------+--------+--------------------------------+
| SPI MOSI (UC3)     | 19     | J2-15  | PB10 (UC3 TX/PICO)             |
+--------------------+--------+--------+--------------------------------+
| SPI SCLK (UC3)     | 23     | J1-7   | PB5  (UC3 SCLK/RX)             |
+--------------------+--------+--------+--------------------------------+
| SPI CS (GPIO)      | 24     | J2-19  | PA21 (GPIO, active-low)        |
+--------------------+--------+--------+--------------------------------+
| SPI MISO (opt.)    | 21     | J2-14  | PA20 (UC3 RTS/POCI)            |
+--------------------+--------+--------+--------------------------------+
| ST7796S D/C        | 15     | J4-35  | PA25 (GPIO, active-high)       |
+--------------------+--------+--------+--------------------------------+
| ST7796S RST        | 13     | J3-26  | PA17 (GPIO, active-low)        |
+--------------------+--------+--------+--------------------------------+
| Backlight EN       | 12     | J3-24  | PA18 (GPIO, active-high)       |
+--------------------+--------+--------+--------------------------------+
| GT911 SDA (UC2)    | 3      | J1-10  | PA22 (UC2 I2C SDA, J18 pull-up)|
+--------------------+--------+--------+--------------------------------+
| GT911 SCL (UC2)    | 5      | J1-9   | PA23 (UC2 I2C SCL, J17 pull-up)|
+--------------------+--------+--------+--------------------------------+
| GT911 INT          | 7      | J2-11  | PB11 (GPIO, active-high)       |
+--------------------+--------+--------+--------------------------------+
| GT911 RST          | 11     | J3-23  | PA16 (GPIO, active-low)        |
+--------------------+--------+--------+--------------------------------+

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

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: lp_am13e230
   :shield: waveshare_3_5_rpi_lcd_f
   :goals: build

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: lp_mspm33c321a
   :shield: waveshare_3_5_rpi_lcd_f
   :goals: build

References
**********

.. target-notes::

.. _Waveshare 3.5inch RPi LCD (F) website:
   https://www.waveshare.com/3.5inch-rpi-lcd-f.htm
