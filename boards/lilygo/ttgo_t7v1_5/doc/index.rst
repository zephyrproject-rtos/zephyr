.. zephyr:board:: ttgo_t7v1_5

Overview
********

LILYGO® TTGO T7 Mini32 V1.5 ia an IoT mini development board
based on the Espressif ESP32-WROVER-E module.

It features the following integrated components:
- ESP32 chip (240MHz dual core, 520KB SRAM, Wi-Fi, Bluetooth)
- on board antenna
- Micro-USB connector for power and communication
- JST GH 2-pin battery connector
- LED

Hardware
********

This board is based on the ESP32-WROVER-E module with 4MB of flash (there
are models 16MB as well), WiFi and BLE support. It has a Micro-USB port for
programming and debugging, integrated battery charging and an on-board antenna.

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

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

Sample Applications
*******************

The following samples will run out of the box on the TTGO T7 V1.5 board.

To build the blinky sample:

.. zephyr-app-commands::
   :tool: west
   :app: samples/basic/blinky
   :board: ttgo_t7v1_5/esp32/procpu
   :goals: build

To build the bluetooth beacon sample:

.. zephyr-app-commands::
   :tool: west
   :app: samples/bluetooth/beacon
   :board: ttgo_t7v1_5/esp32/procpu
   :goals: build


References
**********

.. target-notes::

.. _`Lilygo TTGO T7-V1.5 schematic`: https://github.com/LilyGO/TTGO-T7-Demo/blob/master/t7_v1.5.pdf
.. _`Lilygo github repo`: https://github.com/LilyGO/TTGO-T7-Demo/tree/master
.. _`Espressif ESP32-WROVER-E datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-wrover-e_esp32-wrover-ie_datasheet_en.pdf
