.. _efr32mg24_dk2601b:

xG24-DK2601B
###########################

Overview
********

The EFR32MG24 Mighty Gecko Board dev kit contains
a Wireless System-On-Chip from the EFR32MG24 family built on an
ARM Cortex®-M33F processor with excellent low power capabilities.

.. figure:: ./img/efr32xg24_dk2601b.jpg
   :height: 260px
   :align: center
   :alt: SLWRB4180A Mighty Gecko Radio Board

   xG24-DK2601B (image courtesy of Silicon Labs)

Hardware
********

- EFR32MG24B310F1536IM48-B Mighty Gecko SoC
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 1536 kB
- RAM: 256 kB
- Transmit power: up to +20 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz).

For more information about the EFR32MG24 SoC and BRD2601B board, refer to these
documents:

- `EFR32MG24 Website`_
- `EFR32MG24 Datasheet`_
- `EFR32xG24 Reference Manual`_
- `BRD2601B User Guide`_

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
| COUNTER   | on-chip    | stimer                              |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial                              |
+-----------+------------+-------------------------------------+
| TRNG      | on-chip    | semailbox                           |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| I2C(M/S)  | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+

Other hardware features are currently not supported by the port.

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PA4   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PB0   | GPIO        | LED1                                |
+-------+-------------+-------------------------------------+
| PB2   | GPIO        | Push Button 0                       |
+-------+-------------+-------------------------------------+
| PB3   | GPIO        | Push Button 1                       |
+-------+-------------+-------------------------------------+
| PA5   | USART0_TX   | UART Console EFM_BC_TX US0_TX       |
+-------+-------------+-------------------------------------+
| PA6   | USART0_RX   | UART Console EFM_BC_RX US0_RX       |
+-------+-------------+-------------------------------------+

The default configuration can be found in the defconfig file:

	``boards/arm/efr32_radio/efr32_radio_brd4186a_defconfig``

System Clock
============

The EFR32MG24 SoC is configured to use the 39 MHz external oscillator on the
board.

Serial Port
===========

The EFR32MG24 SoC has one USART and two EUSARTs.
USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   from `J-Link-Downloads`_

Flashing
========

The sample application :ref:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: efr32xg24_dk2601b
   :goals: build

Connect the efr32xg24_dk2601b to your host computer using the USB port and you
should see a USB connection.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you'll see the following message on the corresponding serial port
terminal session:

.. code-block:: console

   Hello World! efr32xg24_dk2601b

.. _EFR32MG24 Website:
   https://www.silabs.com/wireless/zigbee/efr32mg24-series-2-socs#

.. _EFR32MG24 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32mg24-datasheet.pdf

.. _EFR32xG24 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg24-rm.pdf

.. _BRD2601B User Guide:
   https://www.silabs.com/documents/public/user-guides/ug524-brd2601b-user-guide.pdf

.. _J-Link-Downloads:
   https://www.segger.com/downloads/jlink
