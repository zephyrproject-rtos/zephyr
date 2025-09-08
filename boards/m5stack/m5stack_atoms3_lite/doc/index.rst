.. zephyr:board:: m5stack_atoms3_lite

Overview
********

M5Stack AtomS3 Lite is an ESP32-based development board from M5Stack.

It features the following integrated components:

- ESP32-S3FN8 chip (240MHz dual core, Wi-Fi/BLE 5.0)
- 512KB of SRAM
- 384KB of ROM
- 8MB of Flash
- RGB Status-LED

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your M5Stack AtomS3 Lite, please make sure that the board is in good
condition with no obvious signs of damage.

System requirements
*******************

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

M5Stack AtomS3 Lite debugging is not supported due to pinout limitations.

References
**********

.. target-notes::

- _`M5Stack AtomS3 Lite schematic`: https://static-cdn.m5stack.com/resource/docs/products/core/AtomS3%20Lite/img-4061fdd4-6954-4709-a7e7-b0f50e5ba52e.webp
- _`ESP32S3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
