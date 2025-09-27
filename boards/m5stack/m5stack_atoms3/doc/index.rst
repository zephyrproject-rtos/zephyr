.. zephyr:board:: m5stack_atoms3

Overview
********

M5Stack AtomS3 is an ESP32-based development board from M5Stack.

It features the following integrated components:

- ESP32-S3FN8 chip (240MHz dual core, Wi-Fi/BLE 5.0)
- 512KB of SRAM
- 384KB of ROM
- 8MB of Flash
- LCD IPS TFT 0.85", 128x128 px screen (ST7789 compatible)
- 6-axis IMU MPU6886
- Infrared emitter

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your M5Stack AtomS3, please make sure that the board is in good
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

M5Stack AtomS3 debugging is not supported due to pinout limitations.

Related Documents
*****************

.. target-notes::

- _`M5Stack AtomS3 schematic`: https://static-cdn.m5stack.com/resource/docs/products/core/AtomS3/img-b85e925c-adff-445d-994c-45987dc97a44.jpg
- _`ESP32S3 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
