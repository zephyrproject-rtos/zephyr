.. zephyr:board:: uedx80480070e_wb_a

Overview
********

The VIEWE UEDX80480070E-WB-A is a 7.0" 800x480 TFT display module with an ESP32-S3 MCU and
capacitive touch. The panel is driven over a 16-bit parallel RGB interface. The module features
Wi-Fi and Bluetooth Low Energy connectivity.

Key features include:

- ESP32-S3 dual-core Xtensa LX7 @ 240MHz
- 16MB Flash, 8MB PSRAM (Octal SPI)
- 7.0" TFT display (800x480, parallel RGB interface wired for RGB565)
- Capacitive touch panel (GT911 controller)
- microSD card slot
- USB-C for power, and USB-C to UART (CH340C) for programming and console
- Boot and Reset buttons

For more information, check the `VIEWE Product Page`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

The display uses most of the GPIOs of the module. GPIO17 and GPIO18 are left free on the side
headers.

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`VIEWE Product Page`: https://viewedisplay.com/product/esp32-7-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl-uart/
