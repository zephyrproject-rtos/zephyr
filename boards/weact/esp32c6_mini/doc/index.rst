.. zephyr:board:: weact_esp32c6_mini

Overview
********

WeAct ESP32-C6 Mini is a compact development board based on ESP32-C6FH4 chip with integrated
4 MB flash. This board integrates complete Wi-Fi, Bluetooth LE, Zigbee, and Thread functions.

For more information, check `WeAct ESP32-C6 Mini`_.

Hardware
********

The WeAct ESP32-C6 Mini is a compact board with the ESP32-C6FH4 chip directly mounted, featuring
a 4 MB SPI flash. The board includes a USB Type-C connector, boot and reset buttons, and an RGB LED.

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

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`WeAct ESP32-C6 Mini`: https://github.com/WeActStudio/WeActStudio.ESP32C6-MINI
