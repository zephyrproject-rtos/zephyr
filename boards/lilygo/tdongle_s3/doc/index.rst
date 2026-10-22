.. zephyr:board:: tdongle_s3

Overview
********

Lilygo T-Dongle S3 is an IoT mini development board based on the
Espressif ESP32-S3 WiFi/Bluetooth dual-mode chip.

It features the following integrated components:

- ESP32-S3 chip (240MHz dual core, Bluetooth 5, WiFi)
- On-board antenna and IPEX connector
- USB-A connector with integrated TF Card slot
- MX 1.25mm 2-pin battery connector
- APA102 RGB LED
- JST SH 1.0mm 4-pin UART connector
- Transparent plastic case

Hardware
********

This board is based on the ESP32-S3 with 16MB of flash, WiFi and BLE support. It
has an USB-A port for programming and debugging, integrated battery charging
and an on-board antenna. The fitted U.FL external antenna connector can be
enabled by moving a 0-ohm resistor.

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Lilygo T-Dongle S3 schematic`: https://github.com/Xinyuan-LilyGO/T-Dongle-S3/blob/main/shcematic/T-Dongle-S3.pdf
.. _`Lilygo github repo`: https://github.com/Xinyuan-LilyGO/T-Dongle-S3.git
