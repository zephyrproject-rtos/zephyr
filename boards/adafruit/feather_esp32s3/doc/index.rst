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

Asymmetric Multiprocessing (AMP)
================================

The ESP32-S3 SoC allows 2 different applications to be executed in asymmetric
multiprocessing. Due to its dual-core architecture, each core can be enabled to
execute customized tasks in stand-alone mode and/or exchanging data over OpenAMP
framework. See :zephyr:code-sample-category:`ipc` folder as code reference.

For more information, check the datasheet at `ESP32-S3 Datasheet`_.

Supported Features
==================

The current ``adafruit_feather_esp32s3`` board supports the following hardware
features:

+------------+------------+-------------------------------------+
| Interface  | Controller | Driver/Component                    |
+============+============+=====================================+
| UART       | on-chip    | serial port                         |
+------------+------------+-------------------------------------+
| GPIO       | on-chip    | gpio                                |
+------------+------------+-------------------------------------+
| PINMUX     | on-chip    | pinmux                              |
+------------+------------+-------------------------------------+
| USB-JTAG   | on-chip    | hardware interface                  |
+------------+------------+-------------------------------------+
| SPI Master | on-chip    | spi                                 |
+------------+------------+-------------------------------------+
| TWAI/CAN   | on-chip    | can                                 |
+------------+------------+-------------------------------------+
| ADC        | on-chip    | adc                                 |
+------------+------------+-------------------------------------+
| Timers     | on-chip    | counter                             |
+------------+------------+-------------------------------------+
| Watchdog   | on-chip    | watchdog                            |
+------------+------------+-------------------------------------+
| TRNG       | on-chip    | entropy                             |
+------------+------------+-------------------------------------+
| LEDC       | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| MCPWM      | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| PCNT       | on-chip    | qdec                                |
+------------+------------+-------------------------------------+
| GDMA       | on-chip    | dma                                 |
+------------+------------+-------------------------------------+
| USB-CDC    | on-chip    | serial                              |
+------------+------------+-------------------------------------+
| Wi-Fi      | on-chip    |                                     |
+------------+------------+-------------------------------------+
| Bluetooth  | on-chip    |                                     |
+------------+------------+-------------------------------------+

Connections and IOs
===================

The `Adafruit Feather ESP32-S3 User Guide`_ has detailed information about the
board including `pinouts`_ and the `schematic`_.

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
