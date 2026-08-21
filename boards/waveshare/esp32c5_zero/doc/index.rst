.. zephyr:board:: esp32c5_zero

Overview
********

ESP32-C5-Zero (without header) and ESP32-C5-Zero-M (With pre-soldered header)
are ultra-compact multi-protocol development boards based on the ESP32-C5-HF4,
integrating wireless communication, USB interface, and onboard peripherals,
suitable for embedded and IoT applications.
The ESP32-C5-HF4 chip is a variant of ESP32-C5 which provides integrated 4MB
flash stacked on-package. There is no SPI-PSRAM on this board, so only the base
384kB SRAM is available for use
For more information, check `ESP32-C5-Zero`_.

Hardware
********

ESP32-C5-Zero ships with:

- 240MHz ESP32-C5-HF4 SoC with 4 MB Flash stacked in the package
- USB-C connector wired to native USB OTG
- Dual-band 2.4GHz and 5GHz Wi-Fi 6, Bluetooth 5 and Zigbee 3
- On-board antenna-switching chip allowing dynamic switching
between onboard or external antenna
- RGB WS2812 LED
- 19 accessible GPIOs (15 on castellated pin-headers, 4 on underside pads)

Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing.
Developers can either connect peripherals with jumper wires, mount ESP32-C5-Zero on
a breadboard, or solder to a PCB as a castellated module.

.. include:: ../../../espressif/common/soc-esp32c5-features.rst
   :start-after: espressif-soc-esp32c5-features

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

.. _`ESP32-C5-Zero`: https://docs.waveshare.com/ESP32-C5-Zero
