.. zephyr:board:: esp32s3_matrix

Overview
********

The ESP32-S3-Matrix is an ESP32S3 development board from Waveshare with a 8x8
RGB LED matrix. This board integrates complete Wi-Fi and Bluetooth Low Energy
functions, an accelerometer and gyroscope, a battery charger and GPIO extension
port.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

The board included peripherals:

- 8x8 RGB LED matrix
- Accelerometer/gyroscope

Asymmetric Multiprocessing (AMP)
********************************

ESP32-S3 allows 2 different applications to be executed in ESP32-S3 SoC. Due to its dual-core
architecture, each core can be enabled to execute customized tasks in stand-alone mode
and/or exchanging data over OpenAMP framework. See :zephyr:code-sample-category:`ipc` folder as code reference.

For more information, check the datasheet at `ESP32-S3 Datasheet`_ or the technical reference
manual at `ESP32-S3 Technical Reference Manual`_.

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

.. _ESP32-S3-Matrix Waveshare Wiki: https://www.waveshare.com/wiki/ESP32-S3-Matrix
