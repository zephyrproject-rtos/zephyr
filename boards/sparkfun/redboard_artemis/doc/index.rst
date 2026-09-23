.. zephyr:board:: sparkfun_redboard_artemis

Overview
********

The SparkFun RedBoard Artemis is an Arduino Uno R3 form-factor board built around the
SparkFun Artemis module, which contains the Ambiq Apollo3 Blue SoC. It has a USB-C
connector with a CH340C USB-to-serial bridge and a Qwiic connector. It is programmed
over USB through the SparkFun Variable Loader (SVL) serial bootloader preinstalled on
the board, without a debug probe.

Hardware
********

- Ambiq Apollo3 Blue SoC, Arm Cortex-M4F at 48 MHz, with a 96 MHz burst mode
- 1 MB flash, of which 960 KB is available to applications (see `Flash layout`_)
- 384 KB RAM
- CH340C USB-to-serial bridge on UART0, with TX/RX activity LEDs
- Qwiic (I2C) connector
- DC barrel jack (7-15 V)
- 32.768 kHz crystal
- Blue user LED on D13
- PDM MEMS microphone
- Reset button

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The board has an Arduino Uno R3 header, exposed through the ``arduino_header``
devicetree node and the ``arduino_i2c``, ``arduino_spi`` and ``arduino_serial``
labels. The Qwiic connector is labeled ``zephyr_i2c``.

.. list-table:: Default peripheral mapping
   :header-rows: 1

   * - Function
     - Header pins
     - SoC pads
     - Notes
   * - UART0 (console)
     - —
     - 48 (TX), 49 (RX)
     - USB-C through the CH340C, 115200 baud
   * - UART1
     - D1 (TX), D0 (RX)
     - 24, 25
     - disabled by default
   * - I2C4
     - D14 (SDA), D15 (SCL)
     - 40, 39
     - shared with the Qwiic connector
   * - SPI0
     - D13 (SCK), D12 (MISO), D11 (MOSI)
     - 5, 6, 7
     - disabled by default; SCK is shared with the LED
   * - LED0 (blue)
     - D13
     - 5
     - active high

Flash layout
============

.. list-table::
   :header-rows: 1

   * - Range
     - Contents
   * - 0x00000 - 0x0BFFF
     - Ambiq secure bootloader
   * - 0x0C000 - 0x0FFFF
     - SparkFun Variable Loader (SVL)
   * - 0x10000 - 0xFFFFF
     - Zephyr application

Both bootloader partitions are marked read-only. The SVL region is not hardware
write-protected: erasing it removes the ability to program the board over USB.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Connect the board over USB-C. The default ``svl`` runner resets the board into SVL
through the CH340C DTR line and uploads ``zephyr.bin`` over the serial port. Pass the
serial port with ``--port``:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sparkfun_redboard_artemis
   :goals: build flash
   :flash-args: --port <port_name>

The upload baud rate can be changed with ``--baud`` (57600 to 921600, default 115200).

Open a serial terminal on the same port with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message:

.. code-block:: console

   Hello World! sparkfun_redboard_artemis/apollo3_blue

Debugging
=========

The board has an unpopulated JTAG footprint (SWDIO on pad 21, SWDCK on pad 20). No
debug runner is configured.

References
**********

.. target-notes::

.. _RedBoard Artemis product page:
   https://www.sparkfun.com/products/15444

.. _RedBoard Artemis hookup guide:
   https://learn.sparkfun.com/tutorials/hookup-guide-for-the-sparkfun-redboard-artemis

.. _RedBoard Artemis schematic:
   https://cdn.sparkfun.com/assets/4/5/a/3/e/RedBoardArtemisSchematic.pdf

.. _Apollo3 Blue datasheet:
   https://ambiq.com/wp-content/uploads/2020/10/Apollo3-Blue-SoC-Datasheet.pdf

.. _SparkFun Variable Loader:
   https://github.com/sparkfun/Apollo3_Uploader_SVL
