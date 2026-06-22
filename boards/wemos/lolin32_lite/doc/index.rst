.. zephyr:board:: lolin32_lite

Overview
********

WEMOS LOLIN32 Lite is a compact development board based on ESP32-D0WDQ6 or ESP32-D0WD-V3
chips. This board integrates complete Wi-Fi and Bluetooth Low Energy functions with a minimal
footprint design. It features an onboard battery charging circuit and exposes selected
GPIO pins on standard 2.54mm headers.

Hardware
********

This board uses the ESP32 chip with 4MB flash storage (W25Q32). A PH-2 battery connector
with TP4054 charging IC enables portable applications. GPIO22 connects to an onboard LED.
The board includes a CH340C USB-to-serial for programming. A RESET button allows manual
resets. The board comes with either micro-USB or USB Type-C connector. For pin configuration,
see the `WEMOS LOLIN32 Lite Schematic`_.

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

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

.. _`WEMOS LOLIN32 Lite Schematic`: https://done.land/assets/files/lolin32_lite.pdf
