.. _xg27_rb4194a:

EFR32BG27 2.4 GHz 8 dBm (xG27-RB4194A)
#######################################

Overview
********

The EFR32BG27 Blue Gecko Radio Board is one of the two
radio boards delivered with `xG27-PK6017A Website`_. It contains
a Wireless System-On-Chip from the EFR32xG27 family built on an
ARM Cortex®-M33F processor with excellent low power capabilities.

.. figure:: ./efr32xG27-xg27-rb4191a.jpg
   :align: center
   :alt: xG27-RB4194A Blue Gecko Radio Board

   xG27-RB4194A (image courtesy of Silicon Labs)

The BRD4194A a.k.a. xG27-RB4194a radio board plugs into the Wireless Pro Kit
Mainboard BRD4002A and is supported as one of :ref:`silabs_radio_boards`.

Hardware
********

- EFR32BG27C140F768IM40 Blue Gecko SoC
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 768 kB
- RAM: 64 kB
- Transmit power: up to +8 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz).
- Secure Vault High with PUF

For more information about the EFR32BG27 SoC and BRD4191A board, refer to these
documents:

- `EFR32BG27 Website`_
- `EFR32BG27 Datasheet`_
- `EFR32xG27 Reference Manual`_
- `xG27-PK6017A Website`_
- `BRD4194A User Guide`_

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
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| TRNG      | on-chip    | semailbox                           |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+

Other hardware features are currently not supported by the port.

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PB0   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PB1   | GPIO        | LED1                                |
+-------+-------------+-------------------------------------+
| PB0   | GPIO        | Push Button 0                       |
+-------+-------------+-------------------------------------+
| PB1   | GPIO        | Push Button 1                       |
+-------+-------------+-------------------------------------+
| PB4   | GPIO        | Board Controller Enable             |
|       |             | VCOM_ENABLE                         |
+-------+-------------+-------------------------------------+
| PA5   | USART0_TX   | UART Console VCOM_TX US0_TX         |
+-------+-------------+-------------------------------------+
| PA6   | USART0_RX   | UART Console VCOM_RX US0_RX         |
+-------+-------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/radio_boards/xg27_rb4194a/xg27_rb4194a_defconfig`

System Clock
============

The EFR32BG27 SoC is configured to use the 38.4 MHz external oscillator on the
board.

Serial Port
===========

The EFR32BG27 SoC has two USARTs and one EUSART.
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

Flashing
========

Connect the BRD4002A board with a mounted BRD4194A radio module to your host
computer using the USB port.

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xg27_rb4194a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! xg27_rb4194a


.. _xG27-PK6017A Website:
   https://www.silabs.com/development-tools/wireless/efr32xg27-pro-kit-8-dbm?tab=overview

.. _BRD4194A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug551-brd4194a-user-guide.pdf

.. _EFR32BG27 Website:
   https://www.silabs.com/wireless/bluetooth/efr32bg27-series-2-socs/device.efr32bg27c140f768im40?tab=specs

.. _EFR32BG27 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32bg27-datasheet.pdf

.. _EFR32xG27 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg27-rm.pdf
