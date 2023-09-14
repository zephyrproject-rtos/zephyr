.. _waveshare_pico_oled_1_3:

Waveshare 1.3inch OLED Module for RaspberryPi Pico
##################################################

Overview
********

The Waveshare 1.3" OLED Module for the Raspberry Pi Pico is a 64x128
vertically long LCD module based on the SinoWealth SH1107 controller.
This module connects via I2C and optionally can use SPI(need soldering).
This display module has two buttons that the user program can use.
It is convenient for implementing user interfaces.

More information about the shield and Arduino adapter can be found at
the `Waveshare 1.3inch OLED Module for RaspberryPi Pico 64x128 website`_.

Pin Assignments
===============

+-------+-----------+-------------------------------------------+
| Name  | Function  | Usage                                     |
+=======+===========+===========================================+
| GP0   | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP1   | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP2   | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP3   | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP4   | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP5   | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP6   | I2C1_SDA  | I2C Data input                            |
+-------+-----------+-------------------------------------------+
| GP7   | I2C1_SCL  | I2C Clock input                           |
+-------+-----------+-------------------------------------------+
| GP8   | GPIO      | Data/Command control pin (Need soldering) |
+-------+-----------+-------------------------------------------+
| GP9   | SPI1_CSN  | SPI Chip Select (Need soldering)          |
+-------+-----------+-------------------------------------------+
| GP10  | SPI1_SCK  | SPI Clock input (Need soldering)          |
+-------+-----------+-------------------------------------------+
| GP11  | SPI1_TX   | SPI Data input (Need soldering)           |
+-------+-----------+-------------------------------------------+
| GP12  | RESET     |                                           |
+-------+-----------+-------------------------------------------+
| GP13  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP14  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP15  | GPIO      | User Key 0                                |
+-------+-----------+-------------------------------------------+
| GP16  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP17  | GPIO      | User Key 1                                |
+-------+-----------+-------------------------------------------+
| GP18  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP19  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP20  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP21  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP22  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP23  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP24  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP25  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP26  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP27  | None      |                                           |
+-------+-----------+-------------------------------------------+
| GP28  | None      |                                           |
+-------+-----------+-------------------------------------------+

.. note::
   The SPI interface is not available by default.
   Switch the J1, J2, and J3 jumper by moving 0-ohm registers
   to the SPI side to enable SPI.

Programming
***********

Set ``-DSHIELD=waveshare_pico_oled_1_3`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: rpi_pico
   :shield: waveshare_pico_oled_1_3
   :goals: build

For using buttons, set ``-DSHIELD=waveshare_pico_oled_1_3_keys`` to enable it.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: rpi_pico
   :shield: waveshare_pico_oled_1_3_keys
   :goals: build

References
**********

.. target-notes::

.. _Waveshare 1.3inch OLED Module for RaspberryPi Pico 64x128 website:
   https://www.waveshare.com/pico-oled-1.3.htm
