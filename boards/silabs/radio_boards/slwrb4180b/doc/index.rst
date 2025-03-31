.. zephyr:board:: slwrb4180b

Overview
********

The EFR32MG21 Mighty Gecko Radio Board is one of the two
radio boards delivered with `EFR32-SLWSTK6006A Website`_. It features a
Wireless System-On-Chip (SoC) from the EFR32MG21 family, built on an
ARM Cortex®-M33F processor, offering exceptional low-power performance.

The SLWRB4180B radio board is designed to connect seamlessly with
the Wireless Starter Kit Mainboards BRD4001A and BRD4002A

Hardware
********

- EFR32MG21A020F1024IM32 Mighty Gecko SoC
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 1024 kB
- RAM: 96 kB
- Transmit power: up to +20 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz).

For more information about the EFR32MG21 SoC and BRD4180B board, refer to these
documents:

- `EFR32MG21 Website`_
- `EFR32MG21 Datasheet`_
- `EFR32xG21 Reference Manual`_
- `EFR32-SLWSTK6006A Website`_
- `BRD4180B User Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PD2
means Pin number 2 on PORTD, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PD2   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PD3   | GPIO        | LED1                                |
+-------+-------------+-------------------------------------+
| PB0   | GPIO        | Push Button PB0                     |
+-------+-------------+-------------------------------------+
| PB1   | GPIO        | Push Button PB1                     |
+-------+-------------+-------------------------------------+
| PD4   | GPIO        | Board Controller Enable             |
|       |             | EFM_BC_EN                           |
+-------+-------------+-------------------------------------+
| PA5   | USART1_TX   | UART Console EFM_BC_TX US1_TX       |
+-------+-------------+-------------------------------------+
| PA6   | USART1_RX   | UART Console EFM_BC_RX US1_RX       |
+-------+-------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/radio_boards/slwrb4180b/slwrb4180b_defconfig`

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Connect the BRD4001A or BRD4002A mainboard, with the BRD4180B radio module mounted,
to your host computer via the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: slwrb4180b
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! slwrb4180b


.. _EFR32-SLWSTK6006A Website:
   https://www.silabs.com/products/development-tools/wireless/efr32xg21-wireless-starter-kit

.. _BRD4180B User Guide:
   https://www.silabs.com/documents/public/user-guides/ug427-brd4180b-user-guide.pdf

.. _EFR32MG21 Website:
   https://www.silabs.com/products/wireless/mesh-networking/efr32mg21-series-2-socs

.. _EFR32MG21 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32mg21-datasheet.pdf

.. _EFR32xG21 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg21-rm.pdf
