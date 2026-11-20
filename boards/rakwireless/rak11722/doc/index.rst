.. zephyr:board:: rak11722

Overview
********

The RAK11722 WisBlock Core Module carries a RAK11720 WisDuo stamp module, which
combines the ultra-low power Apollo3 Blue SoC (AMA3B1KK-KBR-B0) from Ambiq with a
Semtech SX1262 LoRa® transceiver and an integrated Bluetooth Low Energy controller.
The module ends in the 40-pin WisBlock connector.

The same stamp module is carried by the :zephyr:board:`rak11721` Breakout Board,
which brings the module pins out to headers instead.

Hardware
********

The RAK11722 plugs into a WisBlock Base Board, such as the RAK19007, which
provides the power supply and the programming and debug interface.

- Apollo3 Blue SoC with up to 96 MHz operating frequency
- ARM® Cortex®-M4F core
- 16 kB 2-way Associative/Direct-Mapped Cache per core
- Up to 1 MB of flash memory for code/data
- Up to 384 KB of low leakage / low power RAM for code/data
- Integrated Bluetooth 5 Low-energy controller
- Semtech SX1262 low power high range LoRa transceiver
- iPEX connectors for the LORA antenna and BLE antenna.
- 2 user LEDs on RAK19007 WisBlock Base board
- Powered by either Micro USB, 3.7V rechargeable battery or a 5V Solar Panel Port

For more information about the stamp module and the board:

- `WisDuo RAK11720 Website`_
- `WisBlock RAK11722 Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The RAK11722 mounts on a WisBlock Base Board through the 40-pin WisBlock
connector. The pinout is as follows:

+-----------------------------+----------+-----+-----+----------+-----------------------------+
| Used                        | Name     | Pin | Pin | Name     | Used                        |
+=============================+==========+=====+=====+==========+=============================+
| NC                          | VBAT     | 1   | 2   | VBAT     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 3   | 4   | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | 3V3      | 5   | 6   | 3V3      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | USB_P    | 7   | 8   | USB_N    | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | VBUS     | 9   | 10  | SW1      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP39 / UART0_TX             | TXD0     | 11  | 12  | RXD0     | GP40 / UART0_RX             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| RESET                       | RESET    | 13  | 14  | LED1     | GP44                        |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP45                        | LED2     | 15  | 16  | IO8      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | VDD      | 17  | 18  | VDD      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP25 / I2C2_SDA             | I2C1_SDA | 19  | 20  | I2C1_SCL | GP27 / I2C2_SCL             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP13 / ADC8                 | AIN0     | 21  | 22  | AIN1     | GP33 / ADC5                 |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| BOOT                        | BOOT0    | 23  | 24  | IO7      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP1 / SPI0_NSS              | SPI_CS   | 25  | 26  | SPI_CLK  | GP5 / SPI0_CLK              |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP6 / SPI0_MISO             | SPI_MISO | 27  | 28  | SPI_MOSI | GP7 / SPI0_MOSI             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP38                        | IO1      | 29  | 30  | IO2      | GP4                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP37                        | IO3      | 31  | 32  | IO4      | GP31 / ADC3                 |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP42 / UART1_TX             | TXD1     | 33  | 34  | RXD1     | GP43 / UART1_RX             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP32                        | I2C2_SDA | 35  | 36  | I2C2_SCL | GP36                        |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GP12 / ADC9                 | IO5      | 37  | 38  | IO6      | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 39  | 40  | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+

The module does not route IO6, IO7, IO8 or SW1.

LED1 and LED2 drive LEDs on the base board rather than on the module, so the
board defines no ``led0`` alias. An application that needs one, such as
:zephyr:code-sample:`blinky`, gets it from an overlay naming the LEDs of the
base board in use.

WisBlock I2C2 lands on GP32 and GP36, neither of which has an I2C function on
the Apollo3 Blue, so those two pins are usable as GPIO only.

Of the CTIMER instances that reach a WisBlock pin, only the one behind IO4 is
free: the others drive the counters the board enables.

The console is ``uart0``, reaching the host through the USB-to-serial converter
on the base board.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The RAK11722 board shall be connected to a Segger Embedded Debugger Unit
`J-Link OB <https://www.segger.com/jlink-ob.html>`_. This provides a debug
interface to the Apollo3 Blue chip. You can use JLink to communicate with
the Apollo3 Blue.

Connect your device to your host computer using the JLINK USB port.
The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11722
   :goals: flash

.. note::
   ``west flash`` requires `SEGGER J-Link software`_ and `pylink`_ Python module
   to be installed on your host computer.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! rak11722/apollo3_blue

References
**********

.. target-notes::

.. _WisDuo RAK11720 Website:
   https://docs.rakwireless.com/product-categories/wisduo/rak11720-module/overview/

.. _WisBlock RAK11722 Website:
   https://docs.rakwireless.com/product-categories/wisblock/rak11722/overview/

.. _SEGGER J-Link software:
   https://www.segger.com/downloads/jlink

.. _pylink:
   https://github.com/Square/pylink
