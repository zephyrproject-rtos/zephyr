.. zephyr:board:: adafruit_feather_esp32s3

Overview
********

The Adafruit Feather ESP32-S3 is an ESP32-S3 development board in the Feather
standard layout, sharing peripheral placement with other devices labeled as
Feathers or FeatherWings. The board is equipped with an ESP32-S3 mini module, a
LiPo battery charger, a fuel gauge, a USB-C and Qwiic/STEMMA-QT connector. For
more information, check `Adafruit Feather ESP32-S3`_.

Hardware
********

- ESP32-S3 mini module, featuring the dual core 32-bit Xtensa Microprocessor
  (Tensilica LX7), running at up to 240MHz
- 512KB SRAM and either 8MB flash or 4MB flash + 2MB PSRAM, depending on the
  module variant
- USB-C directly connected to the ESP32-S3 for USB/UART and JTAG debugging
- LiPo connector and built-in battery charging when powered via USB-C
- MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Charging indicator LED, user LED, reset and boot buttons
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Adafruit Feather ESP32-S3 User Guide`_ has detailed information about the
board including `pinouts`_ and the `schematic`_.

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

.. _`Adafruit Feather ESP32-S3`:
   https://www.adafruit.com/product/5323

.. _`OpenOCD`:
   https://github.com/openocd-org/openocd

.. _`JTAG debugging for ESP32-S3`:
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/

.. _Adafruit Feather ESP32-S3 User Guide:
   https://learn.adafruit.com/adafruit-esp32-s3-feather

.. _pinouts:
   https://learn.adafruit.com/adafruit-esp32-s3-feather/pinouts

.. _schematic:
   https://learn.adafruit.com/adafruit-esp32-s3-feather/downloads

.. _ESP32-S3 Datasheet:
   https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf

.. _ESP32 Technical Reference Manual:
   https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
