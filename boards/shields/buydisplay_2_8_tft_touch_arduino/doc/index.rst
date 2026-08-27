.. _buydisplay_2_8_tft_touch_arduino:

Buydisplay 2.8" TFT Touch Shield with Arduino adapter
#####################################################

Overview
********

The Buydisplay 2.8" TFT Touch Shield has a resolution of 320x240
pixels and is based on the ILI9341 controller. This shield comes with
a capacitive FT6206 controller touchscreen. The Arduino adapter is
required to use this shield.

More information about the shield and Arduino adapter can be found at
the `Buydisplay 2.8" TFT Touch Shield website`_ and
`Arduino adapter website`_.

Pin Assignments
===============

+-----------------------+---------------------------------------------+
| Shield Connector Pin  | Function                                    |
+=======================+=============================================+
| D5                    | Touch Controller IRQ (see note below)       |
+-----------------------+---------------------------------------------+
| D7                    | ILI9341 DC       (Data/Command)             |
+-----------------------+---------------------------------------------+
| D10                   | ILI9341 Reset                               |
+-----------------------+---------------------------------------------+
| D9                    | ILI9341 SPI CSn                             |
+-----------------------+---------------------------------------------+
| D11                   | SPI MOSI         (Serial Data Input)        |
+-----------------------+---------------------------------------------+
| D12                   | SPI MISO         (Serial Data Out)          |
+-----------------------+---------------------------------------------+
| D13                   | SPI SCK          (Serial Clock Input)       |
+-----------------------+---------------------------------------------+
| SDA                   | FT6206 SDA                                  |
+-----------------------+---------------------------------------------+
| SCL                   | FT6206 SCL                                  |
+-----------------------+---------------------------------------------+

.. note::
   Touch controller IRQ line is not connected by default. You will need
   to solder the ``5 INT`` jumper to use it. You will also need to
   adjust driver configuration and its Device Tree entry to make use of
   it.

Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino connectors and defines node aliases for SPI and GPIO interfaces
(see :ref:`shields` for more details).

Programming
***********

Set ``--shield buydisplay_2_8_tft_touch_arduino`` when you invoke
``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: nrf52840dk/nrf52840
   :shield: buydisplay_2_8_tft_touch_arduino
   :goals: build

References
**********

.. target-notes::

.. _Buydisplay 2.8" TFT Touch Shield website:
   https://www.buydisplay.com/2-8-inch-tft-touch-shield-for-arduino-w-capacitive-touch-screen-module

.. _Arduino adapter website:
   https://www.buydisplay.com/arduino-shield-for-tft-lcd-with-ili9341-controller-and-arduino-due-mega-uno
