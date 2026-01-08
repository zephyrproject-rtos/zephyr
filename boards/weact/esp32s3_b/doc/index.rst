.. zephyr:board:: weact_esp32s3_b

Overview
********

The WeAct Studio ESP32-S3-B is a compact development board equipped with ESP32-S3R8
chip and 16MB external flash memory. This board integrates complete Wi-Fi
and Bluetooth Low Energy functions with additional features including an onboard RGB WS2812 LED
and user button. For more information, check `WeAct Studio ESP32-S3-B`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

WeAct Studio ESP32-S3-B includes the following features:

- Onboard RGB WS2812 LED (GPIO48)
- User button (GPIO45)
- BOOT button (GPIO0)

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

.. _`WeAct Studio ESP32-S3-B`: https://github.com/WeActStudio/WeActStudio.ESP32S3-AorB
