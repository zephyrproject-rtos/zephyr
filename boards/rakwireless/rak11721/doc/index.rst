.. zephyr:board:: rak11721

Overview
********

RAK11721 Breakout Board provides easy access to the RAK11720 stamp module's
pins via 2.54 mm pitch headers, simplifying development and testing of the
RAK11720.

The RAK11720 module combines an Ambiq Apollo3 Blue SoC with an integrated
Bluetooth 5.0 Low Energy controller and a Semtech SX1262 LoRa® transceiver.
The Apollo3 Blue's ultra-low power design and the SX1262's long-range LoRa
capability make the module suitable for IoT applications such as asset
tracking, environmental monitoring, and sensor networks.

Hardware
********

- Ambiq Apollo3 Blue (AMA3B1KK-KBR-B0) ARM Cortex-M4F at up to 96 MHz
- 16 KB 2-way associative cache
- 1 MB Flash, 384 KB SRAM
- Integrated Bluetooth 5.0 Low Energy controller
- Semtech SX1262 LoRa® transceiver, TX power up to +22 dBm
- LoRaWAN 1.0.3 compliant (Class A/B/C); EU433 / CN470 / IN865 / EU868 /
  AU915 / US915 / KR920 / RU864 / AS923-1/2/3/4
- iPEX connectors for LoRa and BLE antennas
- J4 header: I2C, SWD debug interface, UART1, and reset
- J5 header: SPI, UART0 console, and boot mode select
- Supply voltage: 1.8 V ~ 3.6 V; operating temperature: −40 °C ~ 85 °C

For more information about the RAK11721 Breakout Board and RAK11720 stamp module:

- `WisDuo RAK11721 Breakout Board`_
- `WisDuo RAK11720 Module`_
- `Ambiq Apollo3 Blue SoC`_
- `Semtech SX1262`_

Connections and IOs
===================

The RAK11721 exposes the RAK11720 stamp module pins through two 9-pin connectors.

J4 — I2C / SWD / UART1
------------------------

+-----+-----------+--------+----------------------------------+---------+
| Pin | Name      | Type   | Description                      | MCU Pin |
+=====+===========+========+==================================+=========+
| 1   | I2C_SDA   | I/O    | I2C data                         | GP25    |
+-----+-----------+--------+----------------------------------+---------+
| 2   | I2C_SCL   | I/O    | I2C clock                        | GP27    |
+-----+-----------+--------+----------------------------------+---------+
| 3   | RST       | Input  | MCU reset (active low)           | NRST    |
+-----+-----------+--------+----------------------------------+---------+
| 4   | GND       | Power  | Ground                           | —       |
+-----+-----------+--------+----------------------------------+---------+
| 5   | SWDIO     | I/O    | SWD debug data                   | GP21    |
+-----+-----------+--------+----------------------------------+---------+
| 6   | SWCLK     | Input  | SWD debug clock                  | GP20    |
+-----+-----------+--------+----------------------------------+---------+
| 7   | UART1_TX  | Output | UART1 transmit                   | GP42    |
+-----+-----------+--------+----------------------------------+---------+
| 8   | UART1_RX  | Input  | UART1 receive                    | GP43    |
+-----+-----------+--------+----------------------------------+---------+
| 9   | 3V3       | Power  | 3.3 V supply                     | —       |
+-----+-----------+--------+----------------------------------+---------+

J5 — SPI / UART0 / BOOT
-------------------------

+-----+-----------+--------+----------------------------------+---------+
| Pin | Name      | Type   | Description                      | MCU Pin |
+=====+===========+========+==================================+=========+
| 1   | SPI_MOSI  | Output | SPI master-out                   | GP7     |
+-----+-----------+--------+----------------------------------+---------+
| 2   | SPI_MISO  | Input  | SPI master-in                    | GP6     |
+-----+-----------+--------+----------------------------------+---------+
| 3   | SPI_CLK   | Output | SPI clock                        | GP5     |
+-----+-----------+--------+----------------------------------+---------+
| 4   | SPI_CS    | Output | SPI chip select                  | GP1     |
+-----+-----------+--------+----------------------------------+---------+
| 5   | UART0_RX  | Input  | UART0 receive / console RX       | GP40    |
+-----+-----------+--------+----------------------------------+---------+
| 6   | UART0_TX  | Output | UART0 transmit / console TX      | GP39    |
+-----+-----------+--------+----------------------------------+---------+
| 7   | GND       | Power  | Ground                           | —       |
+-----+-----------+--------+----------------------------------+---------+
| 8   | BOOT0     | Input  | Boot mode select (active high)   | GP41    |
+-----+-----------+--------+----------------------------------+---------+
| 9   | 3V3       | Power  | 3.3 V supply                     | —       |
+-----+-----------+--------+----------------------------------+---------+

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The RAK11721 board can be debugged and flashed with an external debug probe
connected to the SWD pins on J4 (SWDIO pin 5, SWCLK pin 6, GND pin 4,
3V3 pin 9). Both J-Link and pyOCD are supported. pyOCD supports the Apollo3
Blue natively and does not require any additional packs.

Flashing an application
=======================

Connect the board to your host computer and build and flash an application.
The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11721/apollo3_blue
   :goals: build flash

By default the J-Link runner is used. To flash with pyOCD instead:

.. code-block:: console

   west flash --runner pyocd

Run a serial terminal connected to J5 pins 6 (UART0_TX) and 5 (UART0_RX):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

.. code-block:: console

   Hello World! rak11721/apollo3_blue

References
**********

.. target-notes::

.. _WisDuo RAK11721 Breakout Board:
   https://docs.rakwireless.com/product-categories/wisduo/rak11721-breakout-board/overview/

.. _WisDuo RAK11720 Module:
   https://docs.rakwireless.com/product-categories/wisduo/rak11720-module/overview/

.. _Ambiq Apollo3 Blue SoC:
   https://ambiq.com/apollo3-blue/

.. _Semtech SX1262:
   https://www.semtech.com/products/wireless-rf/lora-connect/sx1262
