.. zephyr:board:: bg29_rb4420a

Overview
********

The EFR32BG29 Bluetooth LE +4 dBm DCDC Boost WLCSP Radio Board is a plug-in board for
the Wireless Starter Kit Mainboard (BRD4001A) and the Wireless Pro Kit Mainboard
(BRD4002A). It is a complete reference design for the DCDC Boost mode EFR32xG29 Wireless
SoC, a matching network and a PCB antenna for 4 dBm output power in the 2.4 GHz band.
The EFR32 on the radio board is powered in boost DC-DC configuration from an on-board LDO
for improved efficiency and to demonstrate single-cell operation.
See :ref:`silabs_radio_boards` for more information about the Wireless Mainboard platform.

Hardware
********

- EFR32BG29B220F1024CJ45 SoC
- CPU core: ARM Cortex®-M33 with FPU, DSP and TrustZone
- Memory: 1024 kB Flash, 256 kB RAM
- Transmit power: up to +4 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz)

For more information about the EFR32BG29 SoC and BRD4420A board, refer to these documents:

- `BG29-RB4420A Website`_
- `BG29-RB4420A User Guide`_
- `EFR32BG29 Website`_
- `EFR32BG29 Datasheet`_
- `EFR32BG29 Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The EFR32BG29 SoC is configured to use the HFRCODPLL oscillator at 76.8 MHz as the system
clock, locked to the 38.4 MHz external crystal oscillator on the board.

Serial Port
===========

The EFR32BG29 SoC has two USARTs and two EUSARTs.
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners ::

Connect the BRD4002A mainboard with a mounted BRD4420A radio board to your host
computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: bg29_rb4420a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! bg29_rb4420a

Bluetooth
=========

To use Bluetooth functionality, run the command below to retrieve necessary binary
blobs from the Silicon Labs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: bg29_rb4420a
   :goals: build


.. _BG29-RB4420A Website:
   https://www.silabs.com/development-tools/wireless/bluetooth/bg29-rb4420a-efr32bg29-bluetooth-le-dcdc-boost-wlcsp-radio-board?tab=overview

.. _BG29-RB4420A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug623-efr32bg29-brd4420a-user-guide.pdf

.. _EFR32BG29 Website:
   https://www.silabs.com/wireless/bluetooth/efr32bg29-series-2-socs

.. _EFR32BG29 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/efr32bg29-datasheet.pdf

.. _EFR32BG29 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg29-rm.pdf
