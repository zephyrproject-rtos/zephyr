.. zephyr:board:: rak11310

Overview
********

RAK11310 is a WisBlock Core module for RAK hardware IoT platform WisBlock. It is
powered by an RP2040 ARM microcontroller developed by the Raspberry Pi Foundation,
combined with the SX1262 LoRa transceiver from Semtech.

The RAK11310 WisBlock Core module has a RAK11300 LoRa stamp module in it together
with a high-quality connector that is compatible with WisBlock Base boards. It allows
an easy way to access the pins of the RAK11310 module in order to simplify
the development of IoT devices.

The RF communication characteristic of the LoRa module makes it suitable for a
variety of applications in the IoT field such as home automation, sensor networks,
building automation, personal area networks applications (health/fitness sensors,
and monitors, etc.).

- `WisBlock overview`_
- `RAK11310 datasheet`_

Hardware
********

Supported Features
==================

- Based on RAK11300
- Uses the RP2040 as the main processor
- Semtech SX1262 low power high range LoRa transceiver
- I/O ports: UART/I2C/GPIO/USB
- Serial Wire Debug (SWD) interface
- Module Size: 20 x 30 mm
- Supply Voltage: 2.0 V ~ 3.6 V
- Temperature Range: -20 °C ~ 70 °C
- Chipset: Raspberry Pi Foundation RP2040, Semtech SX1262

.. zephyr:board-supported-hw::

Connections and IOs
===================

The RAK11310 features a 40-pin header with various I/O interfaces for the WisBlock ecosystem. The pinout is as follows:

+-----------------------------+----------+-----+-----+----------+-----------------------------+
| Used                        | Name     | Pin | Pin | Name     | Used                        |
+=============================+==========+=====+=====+==========+=============================+
| NC                          | VBAT     | 1   | 2   | VBAT     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 3   | 4   | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | 3V3      | 5   | 6   | 3V3      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| USB_P                       | USB_P    | 7   | 8   | USB_N    | USB_N                       |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | VBUS     | 9   | 10  | SW1      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO4 / UART0_TX            | TXD0     | 11  | 12  | RXD0     | GPIO5 / UART0_RX            |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| RESET                       | RESET    | 13  | 14  | LED1     | GPIO23                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO24                      | LED2     | 15  | 16  | LED3     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | VDD      | 17  | 18  | VDD      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO2 / I2C2_SDA            | I2C1_SDA | 19  | 20  | I2C1_SCL | GPIO3 / I2C2_SCL            |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO26 / ADC_VBAT           | AIN0     | 21  | 22  | AIN1     | GPIO27 / ADC                |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| BOOT                        | BOOT0    | 23  | 24  | IO7      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO17 / SPI0_CS            | SPI_CS   | 25  | 26  | SPI_CLK  | GPIO18 / SPI0_SCK           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO16 / SPI0_MISO          | SPI_MISO | 27  | 28  | SPI_MOSI | GPIO19 / SPI0_MOSI          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO6                       | IO1      | 29  | 30  | IO2      | GPIO22                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO7                       | IO3      | 31  | 32  | IO4      | GPIO28                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO0 / UART1_TX            | TXD1     | 33  | 34  | RXD1     | GPIO1 / UART1_RX            |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO20 / I2C0_SDA           | I2C2_SDA | 35  | 36  | I2C2_SCL | GPIO21 / I2C0_SCL           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO9                       | IO5      | 37  | 38  | IO6      | GPIO8                       |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 39  | 40  | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+

Connecting to a Baseboard
=========================

The RAK11310 can be mounted on a baseboard using the 40-pin header, called WisBlock I/O connector. It is compatible with
the WisBlock ecosystem, allowing for easy integration with various WisBlock modules and sensors.

.. figure:: img/mounting.webp
   :align: center
   :alt: RAK11310 mounting

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building & Flashing
===================

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rak11310/rp2040
   :shield: rakwireless_rak19010,rakwireless_rak19012
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11310/rp2040
   :shield: rakwireless_rak19007
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _WisBlock overview:
   https://www.rakwireless.com/en-us/products/wisblock

.. _RAK11310 datasheet:
   https://docs.rakwireless.com/product-categories/wisblock/rak11310/datasheet
