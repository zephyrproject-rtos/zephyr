.. zephyr:board:: m5stack_paper_color

Overview
********

`M5Stack PaperColor`_ is an ESP32-S3R8 based development board with a 4-inch Spectra 6 full-color
e-paper display.

Hardware
********

The board includes the following features:

- ESP32-S3R8 with 16MB flash and 8MB PSRAM
- 400 x 600 4-inch Spectra 6 e-paper display
- `M5PM1`_ power management IC
- SHT40 temperature and humidity sensor
- RX8130CE real-time clock
- microSD card slot
- Three user buttons
- Two WS2812 RGB LEDs
- Infrared transmitter
- Grove connector
- 1250 mAh battery

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`M5Stack PaperColor`:
   https://docs.m5stack.com/en/core/PaperColor

.. _`M5PM1`:
   https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1207/M5PM1_Datasheet_EN.pdf
