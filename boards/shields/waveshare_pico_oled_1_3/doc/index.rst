.. _waveshare_pico_oled_1_3:

Waveshare 1.3inch OLED Display Module for Raspberry Pi Pico
###########################################################

Overview
********

The Waveshare 1.3inch OLED Display Module for Raspberry Pi Pico is
a 64x128 vertically long LCD module based on the SinoWealth SH1107 controller.
This module connects via I2C and optionally can use SPI(need soldering).
This display module has two buttons that the user program can use.
It is convenient for implementing user interfaces.

More information about the shield and Arduino adapter can be found at
the `Waveshare Pico OLED 1.3 Module website`_.

Pin Assignments
===============

+-------+-----------+-------------------------------------------+
| Name  | Function  | Usage                                     |
+=======+===========+===========================================+
| GP6   | I2C_SDA   | I2C Data input    (I2C1_SDA)              |
+-------+-----------+-------------------------------------------+
| GP7   | I2C_SCL   | I2C Clock input   (I2C1_SCL)              |
+-------+-----------+-------------------------------------------+
| GP8   | OLED_DC   | Data/Command control pin (optional)       |
+-------+-----------+-------------------------------------------+
| GP9   | OLED_CS   | SPI Chip Select   (SPI1_CSN, optional)    |
+-------+-----------+-------------------------------------------+
| GP10  | OLED_CLK  | SPI Clock input   (SPI1_SCK, optional)    |
+-------+-----------+-------------------------------------------+
| GP11  | OLED_DIN  | SPI Data input    (SPI1_TX, optional)     |
+-------+-----------+-------------------------------------------+
| GP12  | OLED_RST  | Reset                                     |
+-------+-----------+-------------------------------------------+
| GP15  | GPIO      | User Key 0                                |
+-------+-----------+-------------------------------------------+
| GP17  | GPIO      | User Key 1                                |
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

References
**********

.. target-notes::

.. _Waveshare Pico OLED 1.3 Module website:
   https://www.waveshare.com/wiki/Pico-OLED-1.3
