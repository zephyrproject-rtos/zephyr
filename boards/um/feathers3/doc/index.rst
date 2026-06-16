.. zephyr:board:: um_feathers3

Overview
********

The Unexpected Maker FeatherS3 is an ESP32-S3 development board in the Feather
standard layout. The board features a bare ESP32-S3 chip with 16MB flash and 8MB
PSRAM, dual 3.3V LDOs, LiPo battery charging, USB-C with native USB, a WS2812B
NeoPixel LED, two STEMMA QT/Qwiic I2C connectors on separate buses, and 28
GPIOs including castellated headers. For more information, check
`Unexpected Maker FeatherS3`_.

Hardware
********

- ESP32-S3 dual core 32-bit Xtensa Microprocessor (Tensilica LX7), running at
  up to 240MHz
- 512KB SRAM, 16MB QSPI flash, and 8MB QSPI PSRAM
- USB-C directly connected to the ESP32-S3 for USB/UART and JTAG debugging
- LiPo connector and built-in battery charging when powered via USB-C
- 2x 700mA 3.3V LDO regulators (LDO2 is user-controlled and auto shuts down
  in deep sleep)
- Built-in WS2812B NeoPixel RGB LED (powered by LDO2)
- Blue status LED
- Boot and reset buttons
- 2x STEMMA QT/Qwiic I2C connectors on separate I2C buses (one on each LDO)
- Ambient light sensor (ALS-PT19)
- Battery voltage and USB power sensing

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Unexpected Maker FeatherS3`_ product page has detailed information about
the board including pinouts and the schematic.

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

.. _`Unexpected Maker FeatherS3`: https://unexpectedmaker.com/feathers3
