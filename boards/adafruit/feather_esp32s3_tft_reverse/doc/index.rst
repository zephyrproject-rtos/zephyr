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


Asymmetric Multiprocessing (AMP)
================================

The ESP32-S3 SoC allows 2 different applications to be executed in asymmetric
multiprocessing. Due to its dual-core architecture, each core can be enabled to
execute customized tasks in stand-alone mode and/or exchanging data over OpenAMP
framework. See :zephyr:code-sample-category:`ipc` folder as code reference.

For more information, check the datasheet at `ESP32-S3 Datasheet`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Adafruit ESP32-S3 Reverse TFT Feather User Guide`_ has detailed information about
the board including `pinouts`_ and the `schematic`_.

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

.. _`Adafruit ESP32-S3 Reverse TFT Feather`:
   https://www.adafruit.com/product/5691

.. _`OpenOCD`:
   https://github.com/openocd-org/openocd

.. _`JTAG debugging for ESP32-S3`:
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/

.. _Adafruit ESP32-S3 Reverse TFT Feather User Guide:
   https://learn.adafruit.com/esp32-s3-reverse-tft-feather

.. _pinouts:
   https://learn.adafruit.com/esp32-s3-reverse-tft-feather/pinouts

.. _schematic:
   https://learn.adafruit.com/esp32-s3-reverse-tft-feather/downloads

.. _ESP32-S3 Datasheet:
   https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf

.. _ESP32 Technical Reference Manual:
   https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
