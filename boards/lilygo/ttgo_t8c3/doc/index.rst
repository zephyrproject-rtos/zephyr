.. zephyr:board:: ttgo_t8c3

Overview
********

Lilygo TTGO T8-C3 is an IoT mini development board based on the
Espressif ESP32-C3 WiFi/Bluetooth dual-mode chip.

It features the following integrated components:

- ESP32-C3 chip (160MHz single core, 400KB SRAM, Wi-Fi)
- on board antenna and IPEX connector
- USB-C connector for power and communication
- JST GH 2-pin battery connector
- LED

Hardware
********

This board is based on the ESP32-C3 with 4MB of flash, WiFi and BLE support. It
has an USB-C port for programming and debugging, integrated battery charging
and an on-board antenna. The fitted U.FL external antenna connector can be
enabled by moving a 0-ohm resistor.

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Lilygo TTGO T8-C3 schematic`: https://github.com/Xinyuan-LilyGO/T8-C3/blob/main/Schematic/T8-C3_V1.1.pdf
.. _`Lilygo github repo`: https://github.com/Xinyuan-LilyGo
