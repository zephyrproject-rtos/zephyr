.. zephyr:board:: uedx32480035e_wb_a

Overview
********

The VIEWE UEDX32480035E-WB-A is a 3.5" 320x480 IPS TFT display module with ESP32-S3 MCU and
capacitive touch. It features Wi-Fi and Bluetooth Low Energy connectivity, making it suitable for
IoT applications with graphical user interfaces.

Key features include:

- ESP32-S3 dual-core Xtensa LX7 @ 240MHz
- 16MB Flash, 8MB PSRAM (Octal SPI)
- 3.5" IPS TFT display (320x480)
- Capacitive touch panel (CHSC6540 controller)
- USB-C for power and programming
- Boot and Reset buttons

For more information, check the `VIEWE Product Page`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

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

References
**********

.. target-notes::

.. _`VIEWE Product Page`: https://viewedisplay.com/product/esp32-3-5-inch-320x480-mcu-ips-tft-display-touch-screen-arduino-lvgl-wifi-ble-uart-smart-module/
