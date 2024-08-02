.. _slwrb4104a:

EFR32BG13 2.4 GHz 10 dBm (SLWRB4104A)
#####################################

Overview
********

The EFR32BG13 Blue Gecko Bluetooth® Low Energy Radio Board is one of the two
radio boards delivered with `SLWSTK6020B Bluetooth SoC Starter Kit`_. It
contains a Wireless System-On-Chip from the EFR32BG13 family built on an
ARM Cortex®-M4F processor with excellent low power capabilities.

.. figure:: efr32bg13-slwrb4104a.jpg
   :align: center
   :alt: SLWRB4104A Blue Gecko Bluetooth® Low Energy Radio Board

   SLWRB4104A (image courtesy of Silicon Labs)

The BRD4104A a.k.a. SLWRB4104A radio board plugs into the Wireless Starter Kit
Mainboard BRD4001A and is supported as one of :ref:`silabs_radio_boards`.

Hardware
********

- EFR32BG13P632F512GM48 Blue Gecko SoC
- CPU core: ARM Cortex®-M4 with FPU
- Flash memory: 512 kB
- RAM: 64 kB
- Transmit power: up to +10 dBm
- Operation frequency: 2.4 GHz
- 8Mbit SPI NOR Flash
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz).

For more information about the EFR32BG13 SoC and BRD4104A board, refer to these
documents:

- `EFR32BG13 Website`_
- `EFR32BG13 Datasheet`_
- `EFR32xG13 Reference Manual`_
- `SLWSTK6020B Bluetooth SoC Starter Kit`_
- `BRD4104A User Guide`_
- `BRD4104A Reference Manual`_
- `EFR32BG13-BRD4104A Schematics`_

Supported Features
==================

The board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| MPU       | on-chip    | memory protection unit              |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| COUNTER   | on-chip    | rtcc                                |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| SPI(M)    | on-chip    | spi port-polling                    |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/radio_boards/slwrb4104a/slwrb4104a_defconfig`

Connections and IOs
===================

In the following table, the column **Pin** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Pin   | Function    | Usage                               |
+=======+=============+=====================================+
| PF4   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PF5   | GPIO        | LED1                                |
+-------+-------------+-------------------------------------+
| PF6   | GPIO        | Push Button PB0                     |
+-------+-------------+-------------------------------------+
| PF7   | GPIO        | Push Button PB1                     |
+-------+-------------+-------------------------------------+
| PA5   | GPIO        | Board Controller Enable             |
|       |             | VCOM_ENABLE                         |
+-------+-------------+-------------------------------------+
| PA0   | USART0_TX   | UART Console VCOM_TX US0_TX #0      |
+-------+-------------+-------------------------------------+
| PA1   | USART0_RX   | UART Console VCOM_RX US0_RX #0      |
+-------+-------------+-------------------------------------+
| PC6   | SPI_MOSI    | Flash MOSI US1_TX #11               |
+-------+-------------+-------------------------------------+
| PC7   | SPI_MISO    | Flash MISO US1_RX #11               |
+-------+-------------+-------------------------------------+
| PC8   | SPI_SCLK    | Flash SCLK US1_CLK #11              |
+-------+-------------+-------------------------------------+
| PA4   | SPI_CS      | Flash Chip Select (GPIO)            |
+-------+-------------+-------------------------------------+

System Clock
============

The EFR32BG13P SoC is configured to use the 38.4 MHz external oscillator on the
board.

Serial Port
===========

The EFR32BG13P SoC has three USARTs and one Low Energy UARTs (LEUART).
USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

Flashing
========

Connect the BRD4001A board with a mounted BRD4104A radio module to your host
computer using the USB port.

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: slwrb4104a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! slwrb4161a


.. _EFR32BG13 Website:
   https://www.silabs.com/wireless/bluetooth/efr32bg13-series-1-socs

.. _EFR32BG13 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32bg13-datasheet.pdf

.. _EFR32xG13 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg13-rm.pdf

.. _SLWSTK6020B Bluetooth SoC Starter Kit:
   https://www.silabs.com/products/development-tools/wireless/bluetooth/blue-gecko-bluetooth-low-energy-soc-starter-kit

.. _BRD4104A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug279-brd4104a-user-guide.pdf

.. _BRD4104A Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/brd4104a-rm.pdf

.. _EFR32BG13-BRD4104A Schematics:
   https://www.silabs.com/documents/public/schematic-files/BRD4104A-A00-schematic.pdf
