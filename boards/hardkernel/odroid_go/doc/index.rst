.. zephyr:board:: odroid_go

Overview
********

ODROID-GO Game Kit is a "Do it yourself" ("DIY") portable game console by
HardKernel. It features a custom ESP32-WROVER with 16 MB flash and it operates
from 80 MHz - 240 MHz. More details can be found in `ODROID-GO pages`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

The board peripherals:

- 2.4 inch 320x240 TFT LCD
- Speaker
- Micro SD card slot
- Micro USB port (battery charging and USB_UART data communication
- Input Buttons (Menu, Volume, Select, Start, A, B, Direction Pad)
- Expansion port (I2C, GPIO, SPI)

Supported Features
==================

.. zephyr:board-supported-hw::

External Connector
==================

+-------+------------------+-------------------------+
| PIN # | Signal Name      | ESP32-WROVER Functions  |
+=======+==================+=========================+
| 1     | GND              | GND                     |
+-------+------------------+-------------------------+
| 2     | VSPI.SCK (IO18)  | GPIO18, VSPICLK         |
+-------+------------------+-------------------------+
| 3     | IO12             | GPIO12                  |
+-------+------------------+-------------------------+
| 4     | IO15             | GPIO15, ADC2_CH3        |
+-------+------------------+-------------------------+
| 5     | IO4              | GPIO4, ADC2_CH0         |
+-------+------------------+-------------------------+
| 6     | P3V3             | 3.3 V                   |
+-------+------------------+-------------------------+
| 7     | VSPI.MISO (IO19) | GPIO19, VSPIQ           |
+-------+------------------+-------------------------+
| 8     | VSPI.MOSI (IO23) | GPIO23, VSPID           |
+-------+------------------+-------------------------+
| 9     | N.C              | N/A                     |
+-------+------------------+-------------------------+
| 10    | VBUS             | USB VBUS (5V)           |
+-------+------------------+-------------------------+

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ODROID-GO pages`: https://wiki.odroid.com/odroid_go/odroid_go
