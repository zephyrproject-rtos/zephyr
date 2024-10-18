.. zephyr:board:: sltb010a

SLTB010A is a development kit based on the EFR32BG22 SoC. Early revisions of
the kit (A00 and A01) use a slightly different PCB (BRD4184A) from later
revisions (BRD4184B).

Hardware
********

- EFR32BG22 Blue Gecko Wireless SoC with upto 76.8 MHz operating frequency
- ARM® Cortex® M33 core with 32 kB RAM and 512 kB Flash
- Macronix ultra low power 8-Mbit SPI flash (MX25R8035F)
- 2.4 GHz ceramic antenna for wireless transmission
- Silicon Labs Si7021 relative humidity and temperature sensor
- Silicon Labs Si1133 UV index and ambient light sensor (EFR32BG22-BRD4184A)
- Vishay VEML6035 ambient light sensor (EFR32BG22-BRD4184B)
- Silicon Labs Si7210 hall effect sensor
- TDK InvenSense ICM-20648 6-axis inertial sensor
- Two Knowles SPK0641HT4H-1 MEMS microphones with PDM output (EFR32BG22-BRD4184B)
- One LED and one push button
- Power enable signals and isolation switches for ultra low power operation
- On-board SEGGER J-Link debugger for easy programming and debugging, which
  includes a USB virtual COM port and Packet Trace Interface (PTI)
- Mini Simplicity connector for access to energy profiling and advanced wireless
  network debugging
- Breakout pads for GPIO access and connection to external hardware
- Reset button
- Automatic switch-over between USB and battery power
- CR2032 coin cell holder and external battery connector

For more information about the EFR32BG SoC and Thunderboard EFR32BG22 board:

- `EFR32BG22 Website`_
- `EFR32BG22 Datasheet`_
- `EFR32xG22 Reference Manual`_
- `Thunderboard EFR32BG22 Website`_
- `EFR32BG22-BRD4184A User Guide`_
- `EFR32BG22-BRD4184B User Guide`_
- `EFR32BG22-BRD4184A Schematics`_
- `EFR32BG22-BRD4184B Schematics`_

Supported Features
==================

The sltb010a board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| MPU       | on-chip    | memory protection unit              |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| COUNTER   | on-chip    | stimer                              |
+-----------+------------+-------------------------------------+
| SPI(M/S)  | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial                              |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| TRNG      | on-chip    | true random number generator        |
+-----------+------------+-------------------------------------+
| I2C(M/S)  | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| RADIO     | on-chip    | bluetooth                           |
+-----------+------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/dev_kits/sltb010a/sltb010a_defconfig`.

Connections and IOs
===================

The EFR32BG22 SoC has four gpio controllers (PORTA, PORTB, PORTC and PORTD).

There are two variants of this board, "A" and "B". Please take a look at your PCB,
to determine which one you have, as the GPIO pin bindings vary between those two.

BRD4184A (SLTB010A revision A00 and A01):

+------+-------------+-----------------------------------+
| Pin  | Function    | Usage                             |
+======+=============+===================================+
| PB0  | GPIO        | LED0 (YELLOW)                     |
+------+-------------+-----------------------------------+
| PB1  | GPIO        | SW0 Push Button PB0               |
+------+-------------+-----------------------------------+
| PA5  | UART_TX     | UART TX Console VCOM_TX US1_TX #1 |
+------+-------------+-----------------------------------+
| PA6  | UART_RX     | UART RX Console VCOM_RX US1_RX #1 |
+------+-------------+-----------------------------------+

BRD4184B (SLTB010A revision A02 and newer):

+------+-------------+-----------------------------------+
| Pin  | Function    | Usage                             |
+======+=============+===================================+
| PA4  | GPIO        | LED0 (YELLOW)                     |
+------+-------------+-----------------------------------+
| PB3  | GPIO        | SW0 Push Button PB0               |
+------+-------------+-----------------------------------+
| PA5  | UART_TX     | UART TX Console VCOM_TX US1_TX #1 |
+------+-------------+-----------------------------------+
| PA6  | UART_RX     | UART RX Console VCOM_RX US1_RX #1 |
+------+-------------+-----------------------------------+

System Clock
============

The EFR32BG22 SoC is configured to use the 38.4 MHz external oscillator on the
board.

Programming and Debugging
=========================

Flashing an application
-----------------------

Connect your device to your host computer using the USB port.
The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

BRD4184A:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sltb010a@0
   :goals: flash

BRD4184B:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sltb010a@2
   :goals: flash

.. note::
   ``west flash`` requires `SEGGER J-Link software`_ to be installed on you host
   computer.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! sltb010a

Bluetooth
=========

To use the BLE function, run the command below to retrieve necessary binary
blobs from the SiLabs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.

BRD4184A:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: sltb010a@0
   :goals: build

BRD4184B:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: sltb010a@2
   :goals: build


.. _Thunderboard EFR32BG22 Website:
   https://www.silabs.com/development-tools/thunderboard/thunderboard-bg22-kit

.. _EFR32BG22-BRD4184A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug415-sltb010a-user-guide.pdf

.. _EFR32BG22-BRD4184B User Guide:
   https://www.silabs.com/documents/public/user-guides/ug464-brd4184b-user-guide.pdf

.. _EFR32BG22-BRD4184A Schematics:
   https://www.silabs.com/documents/public/schematic-files/BRD4184A-A01-schematic.pdf

.. _EFR32BG22-BRD4184B Schematics:
   https://www.silabs.com/documents/public/schematic-files/BRD4184B-A02-schematic.pdf

.. _EFR32BG22 Website:
   https://www.silabs.com/wireless/bluetooth/efr32bg22-series-2-socs

.. _EFR32BG22 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32bg22-datasheet.pdf

.. _EFR32xG22 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg22-rm.pdf

.. _SEGGER J-Link software:
   https://www.segger.com/downloads/jlink
