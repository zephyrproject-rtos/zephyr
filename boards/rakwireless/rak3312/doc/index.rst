.. zephyr:board:: rak3312

Overview
********

The RAK3312 WisBlock Core Module is a WisBlock Core module for RAK WisBlock based
on the Espressif ESP32-S3 MCU with an integrated Semtech SX1262 LoRa transceiver.
It supports triple wireless connectivity with LoRa, BLE, and WiFi, making it highly
versatile for a wide range of IoT applications including home automation, sensor
networks, and building automation.

- `WisBlock overview`_
- `RAK3312 datasheet`_


Hardware
********

Supported Features
==================

- Based on Espressif ESP32-S3
   - Xtensa® 32-bit LX7 dual-core microprocessor
   - 16 MB Flash
   - 512 kB SRAM
   - 512 kB RAM
   - 8 MB PSRAM
   - 16 kB RTC SRAM
- LoRa transceiver Semtech SX1262
   - LoRa and LoRaWAN support
   - Frequency support from 863 MHz to 928 MHz
- Supported bands: IN865, EU868, AU915, US915, KR920, RU864, and AS923-1/2/3/4
- LoRaWAN Activation by OTAA/ABP
- LoRa Point-to-Point (P2P) communication
- BLE 5.0 support
- WiFi support
- I/O ports: UART/I2C/SPI/ADC/GPIO
- Long-range: greater than 10 km with optimized antenna
- Supply Voltage: 3.0 V~3.6 V
- Temperature range: -40° C~65° C

.. zephyr:board-supported-hw::

Connections and IOs
===================

The RAK3312 features a 40-pin header with various I/O interfaces for the WisBlock ecosystem. The pinout is as follows:

+-----------------------------+----------+-----+-----+----------+-----------------------------+
| Used                        | Name     | Pin | Pin | Name     | Used                        |
+=============================+==========+=====+=====+==========+=============================+
| NC                          | VBAT     | 1   | 2   | VBAT     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 3   | 4   | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | 3V3      | 5   | 6   | 3V3      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO20 / USB_SERIAL         | USB_P    | 7   | 8   | USB_N    | GPIO19 / USB_SERIAL         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | VBUS     | 9   | 10  | SW1      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | TXD0     | 11  | 12  | RXD0     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| RESET                       | RESET    | 13  | 14  | LED1     | GPIO46                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO45                      | LED2     | 15  | 16  | LED3     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | VDD      | 17  | 18  | VDD      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO9 / I2C0_SDA            | I2C1_SDA | 19  | 20  | I2C1_SCL | GPIO40 / I2C0_SCL           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO1 / ADC_VBAT            | AIN0     | 21  | 22  | AIN1     | GPIO2 / ADC                 |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| BOOT                        | BOOT0    | 23  | 24  | IO7      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO12 / SPI3_CS            | SPI_CS   | 25  | 26  | SPI_CLK  | GPIO13 / SPI3_SCK           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO10 / SPI3_MISO          | SPI_MISO | 27  | 28  | SPI_MOSI | GPIO11 / SPI3_MOSI          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO21                      | IO1      | 29  | 30  | IO2      | GPIO14                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO41                      | IO3      | 31  | 32  | IO4      | GPIO42                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO43                      | TXD1     | 33  | 34  | RXD1     | GPIO44                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO17 / I2C1_SDA           | I2C2_SDA | 35  | 36  | I2C2_SCL | GPIO18 / I2C1_SCL           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO38                      | IO5      | 37  | 38  | IO6      | GPIO39                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 39  | 40  | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+

Connecting to a Baseboard
=========================

The RAK3312 can be mounted on a baseboard using the 40-pin header, called WisBlock I/O connector. It is compatible with
the WisBlock ecosystem, allowing for easy integration with various WisBlock modules and sensors.

.. figure:: img/mounting.webp
   :align: center
   :alt: RAK3312 mounting

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building & Flashing
===================

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rak3312/esp32s3/procpu
   :shield: rakwireless_rak19010,rakwireless_rak19012
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak3312/esp32s3/procpu
   :shield: rakwireless_rak19007
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _WisBlock overview:
   https://www.rakwireless.com/en-us/products/wisblock

.. _RAK3312 datasheet:
   https://docs.rakwireless.com/product-categories/wisblock/rak3312/datasheet
