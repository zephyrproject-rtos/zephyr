.. zephyr:board:: m5stack_atom_lite

Overview
********

M5Stack ATOM Lite is an ESP32-based development board from M5Stack.

Hardware
********

It features the following integrated components:

- ESP32-PICO-D4 chip (240MHz dual core, Wi-Fi/BLE 5.0)
- 520KB SRAM
- SK6812 RGB LED
- Infrared LED
- 1x Grove extension port

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

M5Stack ATOM Lite debugging is not supported due to pinout limitations.

Related Documents
*****************

.. target-notes::

.. _`M5Stack ATOM Lite docs`: https://docs.m5stack.com/en/core/ATOM%20Lite
.. _`M5Stack ATOM Lite schematic`: https://static-cdn.m5stack.com/resource/docs/products/core/atom_lite/atom_lite_map_01.webp
.. _`ESP32-PICO-D4 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf
