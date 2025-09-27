.. zephyr:board:: ttgo_tbeam

Overview
********

The Lilygo TTGO TBeam, is an ESP32-based development board for LoRa applications.

It's available in two versions supporting two different frequency ranges and features the following integrated components:

- ESP32-PICO-D4 chip (240MHz dual core, 600 DMIPS, 520KB SRAM, Wi-Fi)
- SSD1306, 128x64 px, 0.96" screen (optional)
- SX1278 (433MHz) or SX1276 (868/915/923MHz) LoRa radio frontend (optional, with SMA or IPEX connector)
- NEO-6M or NEO-M8N GNSS module
- X-Powers AXP2101 PMIC
- JST GH 2-pin battery connector
- 18650 Li-Ion battery clip

Some of the ESP32 I/O pins are accessible on the board's pin headers.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your Lilygo TTGO TBeam, please make sure that the board is in good
condition with no obvious signs of damage.

System requirements
*******************

Espressif HAL requires WiFi and Bluetooth binary blobs in order to work. Run the command
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

Lilygo TTGO TBeam debugging is not supported due to pinout limitations.

Sample Applications
*******************

The following sample applications will work out of the box with this board:

* :zephyr:code-sample:`lora-send`
* :zephyr:code-sample:`lora-receive`
* :zephyr:code-sample:`gnss`
* :zephyr:code-sample:`wifi-shell`
* :zephyr:code-sample:`character-frame-buffer`
* :zephyr:code-sample:`blinky`

Related Documents
*****************

.. target-notes::

- _`Lilygo TTGO TBeam schematic`: https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/blob/master/schematic/LilyGo_TBeam_V1.2.pdf
- _`Lilygo TTGO TBeam documentation`: https://www.lilygo.cc/products/t-beam-v1-1-esp32-lora-module
- _`Lilygo github repo`: https://github.com/Xinyuan-LilyGo
- _`ESP32-PICO-D4 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf
- _`ESP32 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- _`ESP32 Hardware Reference`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/index.html
- _`SX127x Datasheet`: https://www.semtech.com/products/wireless-rf/lora-connect/sx1276#documentation
- _`SSD1306 Datasheet`: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
- _`NEO-6M Datasheet`: https://content.u-blox.com/sites/default/files/products/documents/NEO-6_DataSheet_%28GPS.G6-HW-09005%29.pdf
- _`NEO-N8M Datasheet`: https://content.u-blox.com/sites/default/files/NEO-M8-FW3_DataSheet_UBX-15031086.pdf
