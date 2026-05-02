.. zephyr:board:: esp32s3_rlcd_4_2

Overview
********

The `ESP32-S3-RLCD-4.2`_ is an ESP32-S3 development board from Waveshare with a 4.2-inch reflective LCD,
touch input, RTC, microSD slot, and battery measurement support.

The schematic and pinout can be found in the `ESP32-S3-RLCD-4.2 Schematic`_ reference.

Hardware
********

- ESP32-S3 module (16 MB flash, 8 MB PSRAM)
- 4.2-inch reflective LCD panel driven over SPI (ST7306 controller)
- RTC (PCF85063A)
- Temperature and humidity sensor (SHTC3)
- microSD card slot (SDMMC)
- BOOT and user buttons

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

.. _`ESP32-S3-RLCD-4.2`: https://www.waveshare.com/wiki/ESP32-S3-RLCD-4.2
.. _`ESP32-S3-RLCD-4.2 Schematic`: https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf
