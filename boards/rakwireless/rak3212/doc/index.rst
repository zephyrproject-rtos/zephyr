.. zephyr:board:: rak3212

Overview
********

The RAK3212 Breakout Board carries a RAK3112 WisDuo stamp module, which
combines an Espressif ESP32-S3 MCU with a Semtech SX1262 LoRa transceiver
for LoRa, BLE and Wi-Fi. The breakout board brings every module pin out to
2.54 mm headers so the module can be evaluated without designing a carrier
PCB.

The same stamp module is carried by the :zephyr:board:`rak3312` WisBlock Core
Module, which plugs into a WisBlock Base Board instead of exposing headers.

Hardware
********

The main hardware features are:

- RAK3112 based on Espressif ESP32-S3, dual-core Xtensa® LX7 CPU up to 240 MHz
- Semtech SX1262 for LoRa® modulations
- Integrated 2.4 GHz Wi-Fi (802.11 b/g/n) and Bluetooth® LE 5
- 512 KB of SRAM and 384 KB of ROM on the chip
- IPEX connectors for the antennas
- I/O ports:

   - UART
   - I2C
   - SPI
   - GPIO
   - ADC

.. image:: img/pinout.webp
   :align: center
   :alt: RAK3212 pinout

For more information about the stamp module:

- `WisDuo RAK3112 Website`_
- `Espressif ESP32-S3 Website`_

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

.. _`WisDuo RAK3112 Website`: https://docs.rakwireless.com/product-categories/wisduo/rak3112-module/overview/
.. _`Espressif ESP32-S3 Website`: https://www.espressif.com/en/products/socs/esp32-s3
