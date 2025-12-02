.. zephyr:board:: weact_esp32s3_mini

Overview
********

The WeAct Studio ESP32-S3-MINI is a compact development board featuring the ESP32-S3-MINI-1-N4R2
module with integrated 4MB flash and 2MB PSRAM. This board provides complete Wi-Fi and Bluetooth
Low Energy functionality with an onboard RGB WS2812 LED and BOOT button. For more information,
check `WeAct Studio ESP32-S3-MINI`_.

Hardware
********


WeAct Studio ESP32-S3-MINI includes the following features:

- 4MB integrated Flash memory
- 2MB integrated PSRAM
- Onboard RGB WS2812 LED (GPIO48)
- BOOT button (GPIO0)
- USB Type-C connector

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

.. _`WeAct Studio ESP32-S3-MINI`: https://github.com/WeActStudio/WeActStudio.ESP32S3-MINI/
