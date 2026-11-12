.. zephyr:board:: rak11721

Overview
********

The RAK11721 Breakout Board carries a RAK11720 WisDuo stamp module, which
combines the ultra-low power Apollo3 Blue SoC (AMA3B1KK-KBR-B0) from Ambiq with a
Semtech SX1262 LoRa® transceiver and an integrated Bluetooth Low Energy controller.
The breakout board brings the module pins out to 2.54 mm headers so the module can be
evaluated without designing a carrier PCB.

The same stamp module is carried by the :zephyr:board:`rak11722` WisBlock Core Module,
which plugs into a WisBlock Base Board instead of exposing headers.

Hardware
********

- Apollo3 Blue SoC with up to 96 MHz operating frequency
- ARM® Cortex®-M4F core
- Up to 1 MB of flash memory and 384 KB of RAM
- Integrated Bluetooth 5 Low Energy controller
- Semtech SX1262 LoRa transceiver
- SMA connectors for the LoRa and BLE antennas
- I/O ports:

   - UART
   - I2C
   - SPI
   - SWD

For more information about the stamp module and the board:

- `WisDuo RAK11720 Website`_
- `WisDuo RAK11721 Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The board is shared with the RAK3183 module, so the header silkscreen uses that
module's UART numbering. The headers carry these signals:

+-----------------+------------------+
| Silkscreen      | Devicetree       |
+=================+==================+
| I2C2_SDA/SCL    | ``i2c2``         |
+-----------------+------------------+
| SPI_MOSI/MISO/  | ``spi0``         |
| CLK/CS          |                  |
+-----------------+------------------+
| UART1_TX/RX     | ``uart0``        |
+-----------------+------------------+
| UART2_TX/RX     | ``uart1``        |
+-----------------+------------------+

``uart0`` carries the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Connect a Segger J-Link to the SWD pins on the header to program and debug the
Apollo3 Blue.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11721
   :goals: flash

.. note::
   ``west flash`` requires `SEGGER J-Link software`_ and the `pylink`_ Python module
   to be installed on your host computer.

Open a serial terminal on the ``UART1_TX``/``UART1_RX`` header pins with the following
settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message appears:

.. code-block:: console

   Hello World! rak11721/apollo3_blue

References
**********

.. target-notes::

.. _WisDuo RAK11720 Website:
   https://docs.rakwireless.com/product-categories/wisduo/rak11720-module/overview/

.. _WisDuo RAK11721 Website:
   https://docs.rakwireless.com/product-categories/wisduo/rak11721-breakout-board/overview/

.. _SEGGER J-Link software:
   https://www.segger.com/downloads/jlink

.. _pylink:
   https://github.com/Square/pylink
