.. zephyr:board:: adafruit_feather_esp32

Overview
********

The Adafruit ESP32 Feather is an ESP32-based development board using the
Feather standard layout.

Hardware
********

It features the following integrated components:

- ESP32-PICO-V3-02 chip (240MHz dual core, Wi-Fi + BLE)
- 520KB SRAM
- USB-C port connected to USB to Serial converter
- LiPo battery connector and charger
- Charging indicator LED and user LED
- NeoPixel RGB LED
- Reset and user buttons
- STEMMA QT I2C connector

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

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

Testing
*******

On-board LED
============

Test the functionality of the user LED connected to pin 13 with the blinky
sample program.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

NeoPixel
========

Test the on-board NeoPixel using the led_strip sample program.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

User button
===========

Test the button labeled SW38 using the button input sample program.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

Wi-Fi
=====

Test ESP32 Wi-Fi functionality using the Wi-Fi shell module.

.. note::
   The hal_espressif blobs must be fetched first.

.. zephyr-app-commands::
   :zephyr-app: samples/net/wifi/shell
   :board: adafruit_feather_esp32/esp32/procpu
   :goals: build flash

References
**********

.. target-notes::

.. _`Adafruit ESP32 Feather V2`: https://www.adafruit.com/product/5400
.. _`Adafruit ESP32 Feather V2 Pinouts`: https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts
.. _`Adafruit ESP32 Feather V2 Schematic`: https://learn.adafruit.com/adafruit-esp32-feather-v2/downloads#schematic-and-fab-print-3112284
.. _`ESP32-PICO-MINI-02 Datasheet`: https://cdn-learn.adafruit.com/assets/assets/000/109/588/original/esp32-pico-mini-02_datasheet_en.pdf?1646852017
.. _`STEMMA QT`: https://learn.adafruit.com/introducing-adafruit-stemma-qt
