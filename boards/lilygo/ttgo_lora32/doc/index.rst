.. zephyr:board:: ttgo_lora32

Overview
********

The Lilygo TTGO LoRa32 is a development board for LoRa applications based on the ESP32-PICO-D4.

It's available in two versions supporting two different frequency ranges and features the following integrated components:

- ESP32-PICO-D4 chip (240MHz dual core, 600 DMIPS, 520KB SRAM, Wi-Fi)
- SSD1306, 128x64 px, 0.96" screen
- SX1278 (433MHz) or SX1276 (868/915/923MHz) LoRa radio frontend
- JST GH 2-pin battery connector
- TF card slot

Some of the ESP32 I/O pins are accessible on the board's pin headers.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

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

Lilygo TTGO LoRa32 debugging is not supported due to pinout limitations.

Sample Applications
*******************

The following sample applications will work out of the box with this board:

* :zephyr:code-sample:`lora-send`
* :zephyr:code-sample:`lora-receive`
* :zephyr:code-sample:`fs`
* :zephyr:code-sample:`character-frame-buffer`

Related Documents
*****************

.. target-notes::

.. _`Lilygo TTGO LoRa32 schematic`: https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/schematic/T3_V1.6.1.pdf
.. _`Lilygo TTGO LoRa32 documentation`: https://www.lilygo.cc/products/lora3
.. _`Lilygo github repo`: https://github.com/Xinyuan-LilyGo
.. _`ESP32-PICO-D4 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf
.. _`ESP32 Hardware Reference`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/index.html
.. _`SX127x Datasheet`: https://www.semtech.com/products/wireless-rf/lora-connect/sx1276#documentation
.. _`SSD1306 Datasheet`: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
