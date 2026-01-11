.. zephyr:board:: adafruit_feather_esp32s3_tft

Overview
********

The Adafruit Feather ESP32-S3 TFT is an ESP32-S3 development board in the
Feather standard layout, sharing peripheral placement with other devices labeled
as Feathers or FeatherWings. The board is equipped with an ESP32-S3 mini module,
a LiPo battery charger, a fuel gauge, a USB-C and Qwiic/STEMMA-QT connector.
Compared to the base model, this TFT variant additionally comes with a 240x135
pixel IPS TFT color display. For more information, check
`Adafruit Feather ESP32-S3 TFT`_.

Hardware
********

- ESP32-S3 mini module, featuring the dual core 32-bit Xtensa Microprocessor
  (Tensilica LX7), running at up to 240MHz
- 512KB SRAM and either 4MB flash + 2MB PSRAM
- USB-C directly connected to the ESP32-S3 for USB/UART and JTAG debugging
- LiPo connector and built-in battery charging when powered via USB-C
- MAX17048 fuel gauge for battery voltage and state-of-charge reporting
- Charging indicator LED, user LED, reset and boot buttons
- Built-in NeoPixel indicator RGB LED
- STEMMA QT connector for I2C devices, with switchable power for low-power mode
- 240x135 pixel IPS TFT color display with 1.14" diagonal and ST7789 chipset

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

The current ``adafruit_feather_esp32s3_tft`` board supports the following
hardware features:

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

The `Adafruit Feather ESP32-S3 TFT User Guide`_ has detailed information about
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

.. _`Adafruit Feather ESP32-S3 TFT`: https://www.adafruit.com/product/5483
.. _`Adafruit Feather ESP32-S3 TFT User Guide`: https://learn.adafruit.com/adafruit-esp32-s3-tft-feather
.. _`pinouts`: https://learn.adafruit.com/adafruit-esp32-s3-tft-feather/pinouts
.. _`schematic`: https://learn.adafruit.com/adafruit-esp32-s3-tft-feather/downloads
