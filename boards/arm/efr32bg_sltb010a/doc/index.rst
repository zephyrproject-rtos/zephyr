.. _efr32bg_sltb010a:

EFR32BG-SLTB010A
################

Overview
********

The EFR32™ Blue Gecko Starter Kit EFR32BG-SLTB010A (a.k.a Thunderboard EFR32BG22)
contains a MCU from the EFR32BG family built on ARM® Cortex®-M33F
processor with low power capabilities.

.. image:: ./efr32bg_sltb010a.jpg
   :align: center
   :alt: EFR32BG-SLTB010A

Hardware
********

- EFR32BG22 Blue Gecko Wireless SoC with upto 76.8 MHz operating frequency
- ARM® Cortex® M33 core with 32 kB RAM and 512 kB Flash
- Macronix ultra low power 8-Mbit SPI flash (MX25R8035F)
- 2.4 GHz ceramic antenna for wireless transmission
- Silicon Labs Si7021 relative humidity and temperature sensor
- Silicon Labs Si1133 UV index and ambient light sensor
- Silicon Labs Si7210 hall effect sensor
- TDK InvenSense ICM-20648 6-axis inertial sensor
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

For more information about the EFR32BG SoC and Thunderboard EFR32BG22
(EFR32BG-SLTB010A) board:

- `EFR32BG22 Website`_
- `EFR32BG22 Datasheet`_
- `EFR32xG22 Reference Manual`_
- `EFR32BG22-SLTB010A Website`_
- `EFR32BG22-SLTB010A User Guide`_
- `EFR32BG22-SLTB010A Schematics`_

Supported Features
==================

The efr32bg_sltb010a board configuration supports the following hardware features:

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

The default configuration can be found in the defconfig file:
``boards/arm/efr32bg_sltb010a/efr32bg_sltb010a_defconfig``.

Other hardware features are currently not supported by the port.

Connections and IOs
===================

The EFR32BG SoC has six gpio controllers (PORTA, PORTB, PORTC, PORTD,
PORTE and PORTF).

In the following table, the column Name contains Pin names. For example, PE2
means Pin number 2 on PORTE and #27 represents the location bitfield , as used
in the board's and microcontroller's datasheets and manuals.

+------+-------------+-----------------------------------+
| Name | Function    | Usage                             |
+======+=============+===================================+
| PB0  | GPIO        | LED0 (RED)                        |
+------+-------------+-----------------------------------+
| PB1  | GPIO        | SW0 Push Button PB0               |
+------+-------------+-----------------------------------+
| PA5  | UART_TX     | UART TX Console VCOM_TX US1_TX #1 |
+------+-------------+-----------------------------------+
| PA6  | UART_RX     | UART RX Console VCOM_RX US1_RX #1 |
+------+-------------+-----------------------------------+

System Clock
============

The EFR32BG SoC is configured to use the 38.4 MHz external oscillator on the
board.

Serial Port
===========

The EFR32BG22 SoC has two USARTs.
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   from `J-Link-Downloads`_

Flashing
========

The EFR32BG-SLTB010A includes an `J-Link`_ serial and debug adaptor built into the
board. The adaptor provides:

- A USB connection to the host computer, which exposes a Mass Storage and a
  USB Serial Port.
- A Serial Flash device, which implements the USB flash disk file storage.
- A physical UART connection which is relayed over interface USB Serial port.

Flashing an application to EFR32BG-SLTB010A
-------------------------------------------

The sample application :ref:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: efr32bg_sltb010a
   :goals: build

Connect the EFR32BG-SLTB010A to your host computer using the USB port and you
should see a USB connection.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! efr32bg_sltb010a


.. _EFR32BG22-SLTB010A Website:
   https://www.silabs.com/development-tools/thunderboard/thunderboard-bg22-kit

.. _EFR32BG22-SLTB010A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug415-sltb010a-user-guide.pdf

.. _EFR32BG22-SLTB010A Schematics:
   https://www.silabs.com/documents/public/schematic-files/BRD4184A-A01-schematic.pdf

.. _EFR32BG22 Website:
   https://www.silabs.com/wireless/bluetooth/efr32bg22-series-2-socs

.. _EFR32BG22 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32bg22-datasheet.pdf

.. _EFR32xG22 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg22-rm.pdf

.. _J-Link:
   https://www.segger.com/jlink-debug-probes.html

.. _J-Link-Downloads:
   https://www.segger.com/downloads/jlink
