.. zephyr:board:: esp32c6_lcd_1_47

Overview
********

The Waveshare ESP32-C6-LCD-1.47 is a compact development board based on the
ESP32-C6-WROOM-1U-N4 module with a 4 MB SPI flash. It features a 1.47" LCD
display and integrates Wi-Fi 6, Bluetooth LE, Zigbee, and Thread connectivity.
For more information, check `Waveshare ESP32-C6-LCD-1.47`_.

Hardware
********

The Waveshare ESP32-C6-LCD-1.47 board is based on the ESP32-C6 SoC and
includes the following features:

- 1.47" LCD display (172x320, ST7789 driver)
- On-board RGB LED (WS2812, GPIO8 via SPI)
- USB Type-C connector for power and programming
- 4 MB SPI flash

.. include:: ../../../espressif/common/soc-esp32c6-features.rst
   :start-after: espressif-soc-esp32c6-features

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Waveshare ESP32-C6-LCD-1.47`: https://www.waveshare.com/esp32-c6-lcd-1.47.htm
