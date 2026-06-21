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

- Based on STM32WLE5CCU6
- I/O ports: UART/I2C/GPIO
- Temperature range: -70° C to +85° C
- Supply voltage: 2.0 ~ 3.6 V
- Low-Power Wireless Systems with 7.8 kHz to 500 kHz Bandwidth
- Ultra-Low Power Consumption of less than 2.0 uA in sleep mode
- LoRa PA Boost mode with 22 dBm output power
- Serial Wire Debug (SWD) interface
- Module size: 20 mm x 30 mm

.. zephyr:board-supported-hw::

Connections and IOs
===================

The RAK3372 features a 40-pin header with various I/O interfaces for the WisBlock ecosystem. The pinout is as follows:

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
| PA3 / UART2_TX              | TXD0     | 11  | 12  | RXD0     | PA2 / UART2_RX              |
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
| PB12                        | IO3      | 31  | 32  | IO4      | PA10                        |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB6 / UART1_TX              | TXD1     | 33  | 34  | RXD1     | PB7 / UART1_RX              |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | I2C2_SDA | 35  | 36  | I2C2_SCL | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA15                        | IO5      | 37  | 38  | IO6      | PA9                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 39  | 40  | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+

Connecting to a Baseboard
=========================

The RAK3372 can be mounted on a baseboard using the 40-pin header, called WisBlock I/O connector. It is compatible with
the WisBlock ecosystem, allowing for easy integration with various WisBlock modules and sensors.

.. figure:: img/mounting.webp
   :align: center
   :alt: RAK3372 mounting

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building & Flashing
===================

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rak3372/stm32wle5xx
   :shield: rakwireless_rak19010,rakwireless_rak19012
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak3372/stm32wle5xx
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
