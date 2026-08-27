.. zephyr:board:: doit_esp32_devkit_v1

Overview
********

DOIT ESP32 DevKit V1 is an entry-level development board based on the ESP32-WROOM-32 module.
This board integrates complete Wi-Fi and Bluetooth Low Energy functions and exposes all GPIO pins
on standard 2.54mm headers for easy prototyping.

Hardware
********

This board uses ESP32 chip revision 1.1. A blue LED is connected to GPIO2, and a BOOT
button is connected to GPIO0. The board has a micro-USB connector for power supply and programming/debugging.

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

.. _`DOIT ESP32 DevKit V1 Schematic`: https://embedded-systems-design.github.io/overview-of-the-esp32-devkit-doit-v1/SchematicsforESP32.pdf
