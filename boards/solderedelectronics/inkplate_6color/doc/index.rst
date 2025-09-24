.. zephyr:board:: inkplate_6color

Overview
********

The Inkplate 6Color is an ESP32-based development board featuring a 5.85-inch
7-color e-paper display, designed and manufactured by `Soldered Electronics`_.
It targets low-power, always-on visual applications such as dashboards,
signage, information panels, and IoT displays.

The board combines an ESP32-WROVER-E module with a high-resolution color
e-paper panel and on-board peripherals including an RTC, microSD storage,
battery monitoring, and a GPIO expander. The e-paper display retains its image
without power between refreshes, enabling extremely low average power
consumption in battery-operated deployments.

For more information, check `Inkplate 6Color`_.

Hardware
********

The Inkplate 6Color is built around the ESP32-WROVER-E module (4 MB flash,
8 MB PSRAM) and includes the following additional on-board components:

- 5.85-inch 7-color e-paper display (600 × 448 pixels)
- PCAL6416A 16-bit I2C GPIO expander
- PCF85063A real-time clock with battery backup
- MicroSD card slot (SPI)
- Battery voltage monitor (ADC)
- Wake-up button (active low)
- easyC I2C expansion connector
- USB-C connector for power and programming
- JST battery connector

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Connections and IOs
===================

**Internal I2C Bus (Bus 0):**

+---------------+---------+------------------------------+
| Device        | Address | Function                     |
+===============+=========+==============================+
| PCAL6416A     | 0x20    | 16-bit GPIO expander         |
+---------------+---------+------------------------------+
| PCF85063A     | 0x51    | Real-time clock              |
+---------------+---------+------------------------------+

**GPIO Assignments:**

+--------+------------------+----------------------------------+
| GPIO   | Function         | Usage                            |
+========+==================+==================================+
| GPIO1  | UART0_TX         | Console UART transmit            |
+--------+------------------+----------------------------------+
| GPIO3  | UART0_RX         | Console UART receive             |
+--------+------------------+----------------------------------+
| GPIO12 | SPI2_MISO        | SD card SPI interface            |
+--------+------------------+----------------------------------+
| GPIO13 | SPI2_MOSI        | SD card SPI interface            |
+--------+------------------+----------------------------------+
| GPIO14 | SPI2_SCLK        | SD card SPI clock                |
+--------+------------------+----------------------------------+
| GPIO15 | SPI2_CS          | SD card chip select              |
+--------+------------------+----------------------------------+
| GPIO21 | I2C0_SDA         | External I2C data (easyC)        |
+--------+------------------+----------------------------------+
| GPIO22 | I2C0_SCL         | External I2C clock (easyC)       |
+--------+------------------+----------------------------------+
| GPIO26 | I2C1_SDA         | Internal I2C data                |
+--------+------------------+----------------------------------+
| GPIO27 | SPI3_MOSI        | E-paper display interface        |
+--------+------------------+----------------------------------+
| GPIO32 | I2C1_SCL         | Internal I2C clock               |
+--------+------------------+----------------------------------+
| GPIO36 | Wake Button      | User wake-up button (active low) |
+--------+------------------+----------------------------------+
| GPIO39 | RTC Interrupt    | PCF85063A interrupt output       |
+--------+------------------+----------------------------------+

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

.. _`Inkplate 6Color`: https://soldered.com/product/inkplate-6color/
.. _`Soldered Electronics`: https://soldered.com/
