.. zephyr:board:: rak11310

Overview
********

The RAK11310 WisBlock Core Module carries a RAK11300 WisDuo stamp module, which
pairs a Raspberry Pi RP2040 with a Semtech SX1262 LoRa transceiver, on a board
compatible with WisBlock base boards.

- `WisBlock overview`_
- `RAK11310 datasheet`_

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The RAK11310 features a 40-pin header with various I/O interfaces for the
WisBlock ecosystem. The column beside each signal names the RP2040 pin behind
it, which the WisBlock numbering does not follow: the connector's UART0 is the
RP2040's UART1 and its I2C2 is the RP2040's I2C0. The pinout is as follows:

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
| GPIO4 / UART1_TX            | TXD0     | 11  | 12  | RXD0     | GPIO5 / UART1_RX            |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| RESET                       | RESET    | 13  | 14  | LED1     | GPIO23                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO24                      | LED2     | 15  | 16  | LED3     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | VDD      | 17  | 18  | VDD      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO2 / I2C1_SDA            | I2C1_SDA | 19  | 20  | I2C1_SCL | GPIO3 / I2C1_SCL            |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO26 / ADC0               | AIN0     | 21  | 22  | AIN1     | GPIO27 / ADC1               |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| BOOT                        | BOOT0    | 23  | 24  | IO7      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO17 / SPI0_CS            | SPI_CS   | 25  | 26  | SPI_CLK  | GPIO18 / SPI0_SCK           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO16 / SPI0_MISO          | SPI_MISO | 27  | 28  | SPI_MOSI | GPIO19 / SPI0_MOSI          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO6                       | IO1      | 29  | 30  | IO2      | GPIO22                      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO7                       | IO3      | 31  | 32  | IO4      | GPIO28 / ADC2               |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO0 / UART0_TX            | TXD1     | 33  | 34  | RXD1     | GPIO1 / UART0_RX            |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO20 / I2C0_SDA           | I2C2_SDA | 35  | 36  | I2C2_SCL | GPIO21 / I2C0_SCL           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO9                       | IO5      | 37  | 38  | IO6      | GPIO8                       |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 39  | 40  | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+

Three connector signals are absent on this module. SW1, LED3 and IO7 have no
RP2040 pin behind them, so a module that asks for one fails to build.

IO1 and IO2 share RP2040 PWM slice 3A, so only one of the two can carry a PWM
output. IO1 is the one the board maps; IO2 stays a plain GPIO.

Connecting to a Baseboard
=========================

RAK11310 is a WisBlock Core Module and must be mounted on a WisBlock base board
(for example the ``rakwireless_rak19007`` shield) through the 40-pin WisBlock I/O
connector to expose the IO. See the `WisBlock overview`_ for physical
mounting instructions.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building & Flashing
===================

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11310/rp2040
   :shield: rakwireless_rak19007
   :goals: build flash

You should see ``Hello World! rak11310/rp2040`` on the module's USB console.

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
