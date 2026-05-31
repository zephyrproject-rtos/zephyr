.. zephyr:board:: m5stack_sticks3

Overview
********

M5Stack StickS3 is an ESP32-S3 based development board from M5Stack in a small stick form factor.

Hardware
********

The board is built around the ESP32-S3-PICO-1-N8R8 module and provides:

- ESP32-S3 dual-core MCU, 8 MB flash, 8 MB PSRAM
- USB-C with native USB Serial/JTAG
- Two user buttons (G11, G12)
- 6-axis IMU (Bosch BMI270) on I2C
- 1.14" IPS TFT LCD (ST7789 compatible)
- ES8311 audio codec with microphone and speaker amplifier
- Infrared transmitter and receiver
- M5PM1 power-management companion (battery charging, switchable rails)

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

Related Documents
*****************

.. target-notes::

.. _`M5Stack StickS3 product page`: https://docs.m5stack.com/en/core/M5StickS3
