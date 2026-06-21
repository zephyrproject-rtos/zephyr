.. zephyr:board:: rak3272s

Overview
********

RAK3272S Breakout Board is designed to provide easy access to
the board's pins, streamlining development and testing.
Its footprint enables the RAK3172 stamp module pins to be
routed to 2.54 mm headers.

Hardware
********

The breakout board footprint allows RAK3172 stamp module pins to be transferred to 2.54 mm headers.
It is designed for easy access to the pins on the board and simplify the evaluation of the RAK3172
module.

- RAK3172 STM32WLE5CC Module with LPWAN single-core Cortex®-M4 at 48 MHz
- 256-Kbyte Flash memory and 64-Kbyte SRAM
- RF transceiver LoRa® modulations
- Hardware encryption AES256-bit and a True random number generator
- SMA connector for the LoRa antenna
- I/O ports:

   - UART
   - I2C
   - SPI
   - GPIO
   - SWD

For more information about the RAK3272S Breakout Board and RAK3172 stamp module:

- `WisDuo RAK3272S Breakout Board`_
- `WisDuo RAK3172 Website`_
- `STM32WLE5CC on www.st.com`_

Connections and IOs
===================

The RAK3272S exposes the RAK3172 stamp module pins through two 9-pin connectors.

J4 — I2C / UART2 / Debug
--------------------------

+-----+-----------+--------+----------------------------------+---------+
| Pin | Name      | Type   | Description                      | MCU Pin |
+=====+===========+========+==================================+=========+
| 1   | I2C_SDA   | I/O    | I2C data                         | PA11    |
+-----+-----------+--------+----------------------------------+---------+
| 2   | I2C_SCL   | I/O    | I2C clock                        | PA12    |
+-----+-----------+--------+----------------------------------+---------+
| 3   | RST       | Input  | MCU reset (active low)           | NRST    |
+-----+-----------+--------+----------------------------------+---------+
| 4   | GND       | Power  | Ground                           | —       |
+-----+-----------+--------+----------------------------------+---------+
| 5   | SWDIO     | I/O    | SWD debug data                   | PA13    |
+-----+-----------+--------+----------------------------------+---------+
| 6   | SWCLK     | Input  | SWD debug clock                  | PA14    |
+-----+-----------+--------+----------------------------------+---------+
| 7   | UART2_TX  | Output | UART2 transmit / console TX      | PA2     |
+-----+-----------+--------+----------------------------------+---------+
| 8   | UART2_RX  | Input  | UART2 receive / console RX       | PA3     |
+-----+-----------+--------+----------------------------------+---------+
| 9   | 3V3       | Power  | 3.3 V supply                     | —       |
+-----+-----------+--------+----------------------------------+---------+

J5 — SPI / UART1
-----------------

+-----+-----------+--------+----------------------------------+---------+
| Pin | Name      | Type   | Description                      | MCU Pin |
+=====+===========+========+==================================+=========+
| 1   | SPI_MOSI  | I/O    | SPI master-out                   | PA7     |
+-----+-----------+--------+----------------------------------+---------+
| 2   | SPI_MISO  | I/O    | SPI master-in                    | PA6     |
+-----+-----------+--------+----------------------------------+---------+
| 3   | SPI_CLK   | I/O    | SPI clock                        | PA5     |
+-----+-----------+--------+----------------------------------+---------+
| 4   | SPI_CS    | I/O    | SPI chip select                  | PA4     |
+-----+-----------+--------+----------------------------------+---------+
| 5   | UART1_RX  | Input  | UART1 receive                    | PB7     |
+-----+-----------+--------+----------------------------------+---------+
| 6   | UART1_TX  | Output | UART1 transmit                   | PB6     |
+-----+-----------+--------+----------------------------------+---------+
| 7   | GND       | Power  | Ground                           | —       |
+-----+-----------+--------+----------------------------------+---------+
| 8   | BOOT0     | Input  | Boot mode select (active high)   | BOOT0   |
+-----+-----------+--------+----------------------------------+---------+
| 9   | 3V3       | Power  | 3.3 V supply                     | —       |
+-----+-----------+--------+----------------------------------+---------+

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The RAK3272S board can be debugged and flashed with an external debug probe connected
to the SWD pins on J4.
It can also be flashed via `pyOCD`_, but have to install an additional pack to support STM32WL.

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32wl

Flashing an application
=======================

Connect the board to your host computer and build and flash an application.
The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak3272s/stm32wle5xx
   :goals: build flash

Run a serial terminal to connect with your board. By default, ``usart2`` is
accessible via a USB to TTL converter connected to J4 pins 7 (TX) and 8 (RX).

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

.. code-block:: console

   Hello World! rak3272s/stm32wle5xx

References
**********

.. target-notes::

.. _WisDuo RAK3272S Breakout Board:
   https://docs.rakwireless.com/product-categories/wisduo/rak3272s-breakout-board/overview/

.. _WisDuo RAK3172 Website:
   https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Module/Overview/#product-description

.. _STM32WLE5CC on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wle5cc.html

.. _pyOCD:
   https://github.com/pyocd/pyOCD
