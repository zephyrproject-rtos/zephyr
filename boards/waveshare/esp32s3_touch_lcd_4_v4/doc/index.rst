.. zephyr:board:: esp32s3_touch_lcd_4_v4

Overview
********

The ESP32-S3-Touch-LCD-4_v4 (version 4) is an ESP32S3 development board from Waveshare with a 480x480 pixel LCD and touch.
It further more provided connector for CAN, RS485 and I2C devices.
This board integrates complete Wi-Fi and Bluetooth
Low Energy functions, an accelerometer and gyroscope and a battery charger

Notice that the board might also still be available in older version. One of the differences is an io-expander in V4 being replaced by a preprogrammed RISC processor also providing an ADC input allowing monitoring the battery voltage

Hardware
********

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

.. _`ESP32-S3-Touch-LCD-4-V4 Waveshare Wiki`: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4
