.. zephyr:board:: esp32s3_geek

Overview
********

The `ESP32-S3-GEEK`_ is a geek development board designed by Waveshare. It is
based on the Espressif ESP32-S3R2 chip. It has an USB A connector, a 1.14inch
LCD screen, a microSD card slot, and several expansion connectors.

Hardware
********

- ESP32-S3R2 microcontroller with 2MB of on-chip PSRAM
- 16MB of on-board flash
- USB type A connector
- BOOT button
- 135x240 pixels 1.14 inch color LCD screen
- microSD card slot
- 4-pin I2C connector (Quiic / Stemma QT compatible)
- 3-pin UART connector
- 3-pin GPIO connector

See also `ESP32-S3-GEEK wiki`_ and `schematic`_.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

System requirements
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

.. _ESP32-S3-GEEK:
    https://www.waveshare.com/esp32-s3-geek.htm

.. _ESP32-S3-GEEK wiki:
    https://www.waveshare.com/wiki/ESP32-S3-GEEK

.. _schematic:
    https://files.waveshare.com/wiki/ESP32-S3-GEEK/ESP32-S3-GEEK-Schematic1.pdf
