.. zephyr:board:: adafruit_qt_py_esp32s3

Overview
********

An Adafruit based Xiao compatible board based on the ESP32-S3, which is great
for IoT projects and prototyping with new sensors.

For more details see the `Adafruit QT Py ESP32S3`_ product page.

Hardware
********

This board comes in 2 variants, both based on the ESP32-S3 with WiFi and BLE
support. The default variant supporting 8MB of flash with no PSRAM, while the
``psram`` variant supporting 4MB of flash with 2MB of PSRAM. Both boards have a
USB-C port for programming and debugging and is based on a standard XIAO 14
pin pinout.

In addition to the Xiao compatible pinout, it also has a RGB NeoPixel for
status and debugging, a reset button, and a button for entering the ROM
bootloader or user input. Like many other Adafruit boards, it has a
`SparkFun Qwiic`_-compatible `STEMMA QT`_ connector for the I2C bus so you
don't even need to solder.

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

.. _`Adafruit QT Py ESP32S3`: https://www.adafruit.com/product/5426
.. _`Adafruit QT Py ESP32S3 - PSRAM`: https://www.adafruit.com/product/5700
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
