.. _waveshare_pico_lcd_1_14:

Waveshare 1.14inch LCD Display Module for Raspberry Pi Pico
###########################################################

Overview
********

The Waveshare 1.14inch LCD Display Module for Raspberry Pi Pico is
a 240x135/65K color IPS LCD module based on the ST7789V controller.
This module connects via SPI.
This display module has two buttons and joystick that the user program can use.
It is convenient for implementing user interfaces.

More information about the shield and Arduino adapter can be found at
the `Waveshare Pico-LCD-1.14 Module website`_.

Pin Assignments
===============

+-------+-----------+-------------------------------------------+
| Name  | Function  | Usage                                     |
+=======+===========+===========================================+
| GP2   | UP        | Joystick Up                               |
+-------+-----------+-------------------------------------------+
| GP3   | CTRL      | Joystick Center                           |
+-------+-----------+-------------------------------------------+
| GP8   | LCD_DC    | Data/Command control pin                  |
+-------+-----------+-------------------------------------------+
| GP9   | LCD_CS    | SPI Chip Select   (SPI1_CSN)              |
+-------+-----------+-------------------------------------------+
| GP10  | LCD_CLK   | SPI Clock input   (SPI1_SCK)              |
+-------+-----------+-------------------------------------------+
| GP11  | LCD_DIN   | SPI Data input    (SPI1_TX)               |
+-------+-----------+-------------------------------------------+
| GP12  | LCD_RST   | Reset                                     |
+-------+-----------+-------------------------------------------+
| GP13  | LCD_BL    | Backlight                                 |
+-------+-----------+-------------------------------------------+
| GP15  | GPIO      | User Key A                                |
+-------+-----------+-------------------------------------------+
| GP17  | GPIO      | User Key 1                                |
+-------+-----------+-------------------------------------------+
| GP18  | DOWN      | Joystick Down                             |
+-------+-----------+-------------------------------------------+
| GP20  | RIGHT     | Joystick Right                            |
+-------+-----------+-------------------------------------------+

Programming
***********

Set ``-DSHIELD=waveshare_pico_lcd_1_14`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: rpi_pico
   :shield: waveshare_pico_oled_1_14
   :goals: build

References
**********

.. target-notes::

.. _Waveshare Pico-LCD-1.14 Module website:
   https://www.waveshare.com/wiki/Pico-LCD-1.14
