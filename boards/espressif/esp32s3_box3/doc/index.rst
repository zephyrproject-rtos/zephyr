.. zephyr:board:: esp32s3_box3

Overview
********

The ESP32-S3-BOX-3 is a next-generation development tool designed for AIoT, Edge AI, and IIoT applications.
It is based on the powerful ESP32-S3 SoC and features a compact and beautiful enclosure with rich assemblies,
empowering developers to easily customize and expand its functionality.

The ESP32-S3-BOX-3 is equipped with ESP32-S3-WROOM-1 module, which offers 2.4 GHz Wi-Fi + Bluetooth 5 (LE)
wireless capability as well as AI acceleration capabilities. On top of 512 KB SRAM provided by the ESP32-S3 SoC,
the module comes with additional 16 MB Quad flash and 16 MB Octal PSRAM.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Key Features
============

- ESP32-S3-WROOM-1 module with 16MB Flash + 16MB PSRAM
- 2.4-inch ILI9341 LCD display (320x240 resolution)
- Capacitive touch screen (TT21100 controller)
- Dual microphone with ES7210 codec
- Speaker with ES8311 audio codec
- Temperature and humidity sensor (AHT30)
- Two Pmod™-compatible headers for hardware extensibility
- Type-C USB connector for power, programming, and debugging
- Built-in buttons and LED indicators

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The ESP32-S3-BOX-3 uses the following GPIO pin assignments:

- GPIO0: Boot button
- GPIO1: Mute button / Speaker mute status
- GPIO2: I2S MCLK
- GPIO3: Touch screen interrupt (TT21100)
- GPIO4: Display DC pin (SPI)
- GPIO5: Display CS pin (SPI)
- GPIO6: Display SDA/MOSI (SPI)
- GPIO7: Display SCK (SPI)
- GPIO17: I2S SCLK
- GPIO40: I2C SCL (Temperature & Humidity sensor)
- GPIO41: I2C SDA (Temperature & Humidity sensor)
- GPIO45: I2S LRCK
- GPIO46: PA Control
- GPIO47: LCD Backlight control
- GPIO48: LCD Reset pin

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

.. _`ESP32-S3-BOX-3`: https://www.espressif.com/en/dev-board/esp32-s3-box-3-en
