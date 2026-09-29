.. zephyr:board:: rak3372

Overview
********

The RAK3372 WisBlock Core module is a RAK3172 LoRa module with
an expansion PCB and connectors compatible with the WisBlock Base.
It allows an easy way to access the pins of the RAK3172 module,
simplifying development and testing processes.

The module itself comprises a RAK3172 as its main component. The RAK3372
is based on the STM32WLE5CCU6 LoRa SoC transceiver chip. It features
ultra-low power consumption of less than 2.0 uA during sleep mode with
configurable high LoRa output RF power up to 22 dBm during transmission mode.

- `WisBlock overview`_
- `RAK3372 datasheet`_



Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The RAK3372 features a 40-pin header with various I/O interfaces for the
WisBlock ecosystem. The pinout is as follows:

+-----------------------------+----------+-----+-----+----------+-----------------------------+
| Used                        | Name     | Pin | Pin | Name     | Used                        |
+=============================+==========+=====+=====+==========+=============================+
| NC                          | VBAT     | 1   | 2   | VBAT     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 3   | 4   | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | 3V3      | 5   | 6   | 3V3      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| CH340_P / UART2             | USB_P    | 7   | 8   | USB_N    | CH340_N / UART2             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | VBUS     | 9   | 10  | SW1      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC (R10 unpopulated)        | TXD0     | 11  | 12  | RXD0     | NC (R9 unpopulated)         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| RESET                       | RESET    | 13  | 14  | LED1     | PA0                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA1                         | LED2     | 15  | 16  | LED3     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | VDD      | 17  | 18  | VDD      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA11 / I2C2_SDA             | I2C1_SDA | 19  | 20  | I2C1_SCL | PA12 / I2C2_SCL             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB3 / ADC_VBAT              | AIN0     | 21  | 22  | AIN1     | PB4 / ADC                   |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| BOOT                        | BOOT0    | 23  | 24  | IO7      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA4 / SPI1_CS               | SPI_CS   | 25  | 26  | SPI_CLK  | PA5 / SPI1_SCK              |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA6 / SPI1_MISO             | SPI_MISO | 27  | 28  | SPI_MOSI | PA7 / SPI1_MOSI             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB5                         | IO1      | 29  | 30  | IO2      | PA8                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB12                        | IO3      | 31  | 32  | IO4      | PB2                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB6 / UART1_TX              | TXD1     | 33  | 34  | RXD1     | PB7 / UART1_RX              |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA10                        | I2C2_SDA | 35  | 36  | I2C2_SCL | PA9                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA15                        | IO5      | 37  | 38  | IO6      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 39  | 40  | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+

The module does not route IO6, IO7, TXD0 or RXD0: R3, R5, R9 and R10 are left
unpopulated, and the SoM pins behind IO6 and IO7 carry I2C2 instead. A sensor
slot that needs IO6, such as slot D on a base board, is therefore unavailable.

The console is ``usart2``, wired on the module through R7 and R8 to the onboard
CH340E USB-to-serial converter. The converter's USB lines leave on WisBlock
connector pins 7 and 8, and the module carries no USB jack of its own, so a
WisBlock Base Board is needed to reach the console.

Connecting to a Baseboard
=========================

The module mounts on a WisBlock Base Board through the 40-pin WisBlock
connector. See the `RAK3372 datasheet`_ for the mounting orientation.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building & Flashing
===================

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rak3372
   :shield: rakwireless_rak19007
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak3372
   :shield: rakwireless_rak19007
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _WisBlock overview:
   https://www.rakwireless.com/en-us/products/wisblock

.. _RAK3372 datasheet:
   https://docs.rakwireless.com/product-categories/wisblock/rak3372/datasheet
