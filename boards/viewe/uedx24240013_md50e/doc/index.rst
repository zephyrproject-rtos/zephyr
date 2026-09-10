.. zephyr:board:: uedx24240013_md50e

Overview
********

The VIEWE UEDX24240013-MD50E is a 1.28" 240x240 IPS TFT display module with ESP32-C3 MCU and
a rotary knob. It features Wi-Fi and Bluetooth Low Energy connectivity, making it suitable for
IoT applications with graphical user interfaces.

Key features include:

- ESP32-C3 RiscV @ 160MHz
- 4MB Flash, 400KB SRAM
- Rotary knob and button
- 1.28" IPS TFT display (240x240, GC9A01 driver)

For more information, check the `VIEWE Product Page`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32c3-features.rst
   :start-after: espressif-soc-esp32c3-features

Supported Features
==================

.. zephyr:board-supported-hw::

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

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

+------------+------------------+
| FPC number | ESP32C3 Pin      |
+============+==================+
| 1          | 5V               |
+------------+------------------+
| 2          | GPIO3            |
+------------+------------------+
| 3          | GND              |
+------------+------------------+
| 4          | NC               |
+------------+------------------+
| 5          | NC               |
+------------+------------------+
| 6          | UART0RXD/IO20    |
+------------+------------------+
| 7          | UART0TXD/IO21    |
+------------+------------------+
| 8          | CHIP-EN          |
+------------+------------------+
| 9          | USB-DP/IO19      |
+------------+------------------+
| 10         | USB-DN/IO18      |
+------------+------------------+

References
**********

.. target-notes::

.. _`VIEWE Product Page`: https://viewedisplay.com/product/esp32-1-28-inch-240x240-round-tft-knob-display-gc9a01-arduino-lvgl/
