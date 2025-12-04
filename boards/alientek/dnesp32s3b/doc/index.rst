.. zephyr:board:: dnesp32s3b

Overview
********

DNESP32S3B is a development board from Alientek, featuring a compatible
ESP32-S3-WROOM-1-N16R8 module and rich set of on-board peripherals, providing an
out-of-the-box development experience for AIoT applications.

It includes the following integrated components:

- ATK-MWS3S module, compatible with ESP32-S3-WROOM-1-N16R8 (240MHz dual-core, Bluetooth LE, Wi-Fi, 16MB Flash and 8MB PSRAM)
- 2KB EEPROM
- 1 blue power LED (on when powered)
- 1 red-blue dual-color user LED
- 3 user buttons
- 1 USB-A host port
- 1 USB-C JTAG/programming port
- 1 USB-C port connected to external CH343P for programming
- 1 microSD card slot
- 2 PH-2.0 UART connectors
- 1 3x2 2.54mm GPIO header
- 2.4-inch 320x240 I8080 LCD display with optional touch support
- Buzzer
- Ambient light sensor
- QMI8658A 6-axis IMU
- NS4168 audio codec with speaker and microphone

Hardware
********

The board is based on the ESP32-S3 with 16MB of flash, 8MB of PSRAM, WiFi and BLE
support. It has 3 USB ports (1 USB-A host, 1 USB-C JTAG/programming, and 1 USB-C
connected to external CH343P for programming).

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

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

Sample Applications
*******************

The following code samples run out of the box on the DNESP32S3B board:

* :zephyr:code-sample:`blinky`
* :zephyr:code-sample:`button`

References
**********

- `DNESP32S3B web page <https://www.alientek.com/Product_Details/118.html>`_

.. target-notes::
