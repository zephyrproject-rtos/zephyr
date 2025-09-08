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

ESP32-S3 is a low-power MCU-based system on a chip (SoC) with integrated
2.4 GHz Wi-Fi and Bluetooth® Low Energy (Bluetooth LE). It consists of
high-performance dual-core microprocessor (Xtensa® 32-bit LX7), a low power
coprocessor, a Wi-Fi baseband, a Bluetooth LE baseband, RF module, and
numerous peripherals.

Supported Features
==================

.. zephyr:board-supported-hw::

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

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`Adafruit QT Py ESP32S3`: https://www.adafruit.com/product/5426
.. _`Adafruit QT Py ESP32S3 - PSRAM`: https://www.adafruit.com/product/5700
.. _`JTAG debugging for ESP32-S3`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
.. _`SparkFun Qwiic`: https://www.sparkfun.com/qwiic
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
