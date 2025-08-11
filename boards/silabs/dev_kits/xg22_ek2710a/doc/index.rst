.. zephyr:board:: xg22_ek2710a

Overview
********

The EFR32xG22E Explorer Kit (xG22-EK2710A) contains
a Wireless System-On-Chip from the EFR32MG22 family built on an
ARM Cortex®-M33 processor with excellent low power capabilities.

Hardware
********

- EFR32MG22E224F512IM40 Mighty Gecko SoC
- CPU core: ARM Cortex®-M33 with FPU
- Flash memory: 512 kB
- RAM: 32 kB
- Transmit power: up to +6 dBm
- Operation frequency: 2.4 GHz
- Crystal for HFXO (38.4 MHz)

For more information about the EFR32MG22 SoC and BRD2710A board, refer to these
documents:

- `EFR32MG22 Website`_
- `EFR32MG22E Datasheet`_
- `EFR32xG22 Reference Manual`_
- `BRD2710A User Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, PA2
means Pin number 2 on PORTA, as used in the board's datasheets and manuals.

+-------+-------------+-------------------------------------+
| Name  | Function    | Usage                               |
+=======+=============+=====================================+
| PA4   | GPIO        | LED0                                |
+-------+-------------+-------------------------------------+
| PC7   | GPIO        | Push Button 0                       |
+-------+-------------+-------------------------------------+
| PA5   | USART1_TX   | UART Console VCOM_TX US0_TX         |
+-------+-------------+-------------------------------------+
| PA6   | USART1_RX   | UART Console VCOM_RX US0_RX         |
+-------+-------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/silabs/dev_kits/xg22_ek2710a/xg22_ek2710a_defconfig`

System Clock
============

The EFR32MG22E SoC is configured to use the 38.4 MHz external oscillator on the
board, and can operate a clock speeds of up to 76.8 MHz.

Serial Port
===========

The EFR32MG22E SoC has two USARTs and one EUART.
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. note::
   Before using the kit the first time, you should update the J-Link firmware
   in Simplicity Studio.

Flashing
========

The sample application :zephyr:code-sample:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xg22_ek2710a
   :goals: build

Connect the xg22_ek2710a to your host computer using the USB port and you
should see a USB connection.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you'll see the following message on the corresponding serial port
terminal session:

.. code-block:: console

   Hello World! xg22_ek2710a/efr32mg22e224f512im40

Bluetooth
=========

To use the BLE function, run the command below to retrieve necessary binary
blobs from the SiLabs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: xg22_ek2710a
   :goals: build

.. _EFR32MG22 Website:
   https://www.silabs.com/wireless/zigbee/efr32mg22-series-2-socs#

.. _EFR32MG22E Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32mg22e-datasheet.pdf

.. _EFR32xG22 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg22-rm.pdf

.. _BRD2710A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug582-brd2710a-user-guide.pdf
