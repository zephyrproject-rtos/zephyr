.. zephyr:board:: adafruit_feather_esp32s3_tft_reverse

Overview
********

The Adafruit ESP32-S3 Reverse TFT Feather is a development board in the
Feather standard layout, sharing peripheral placement with other devices
labeled as Feathers or FeatherWings. The board is equipped with an
ESP32-S3 mini module, a fuel gauge, a USB-C and Qwiic/STEMMA-QT connector.
This variant additionally comes with a 240x135 pixel IPS TFT color display
on the backside of the boards and with 3 buttons.

For more information, check
`Adafruit ESP32-S3 Reverse TFT Feather`_.

Hardware
********

- ESP32-S3 mini module, featuring the dual core 32-bit Xtensa Microprocessor
  (Tensilica LX7), running at up to 240MHz
- 512KB SRAM and either 4MB flash + 2MB PSRAM
- USB-C directly connected to the ESP32-S3 for USB/UART and JTAG debugging
- LiPo connector and built-in battery charging when powered via USB-C
- MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Charging indicator LED, user LED, reset buttons
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode
- 240x135 pixel IPS TFT color display with 1.14" diagonal and ST7789 chipset
- Three User Tactile buttons - D0, D1, and D2. D0/BOOT0 is also used for entering ROM
  bootloader mode if necessary.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Adafruit ESP32-S3 Reverse TFT Feather User Guide`_ has detailed information about
the board including `pinouts`_ and the `schematic`_.

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

.. _`Adafruit ESP32-S3 Reverse TFT Feather`: https://www.adafruit.com/product/5691
.. _`Adafruit ESP32-S3 Reverse TFT Feather User Guide`: https://learn.adafruit.com/esp32-s3-reverse-tft-feather
.. _`pinouts`: https://learn.adafruit.com/esp32-s3-reverse-tft-feather/pinouts
.. _`schematic`: https://learn.adafruit.com/esp32-s3-reverse-tft-feather/downloads
