.. zephyr:board:: slwrb4182a

Overview
********

The `EFR32xG22 Wireless Gecko 2.4 GHz +6 dBm Radio Board`_ is available standalone and as part of
the `EFR32xG22 Wireless Gecko Starter Kit`_. It is a complete reference design for the EFR32xG22
Wireless SoC, with matching network and a PCB antenna for 6 dBm output power in the 2.4 GHz band.

See :ref:`silabs_radio_boards` for more information about the Wireless Mainboard platform.

.. _EFR32xG22 Wireless Gecko 2.4 GHz +6 dBm Radio Board:
   https://www.silabs.com/development-tools/wireless/slwrb4182a-efr32xg22-wireless-gecko-radio-board

.. _EFR32xG22 Wireless Gecko Starter Kit:
   https://www.silabs.com/development-tools/wireless/efr32xg22-wireless-starter-kit

Hardware
********

- EFR32MG22C224F512IM40 SoC
- CPU core: ARM Cortex®-M33 with FPU, DSP and TrustZone
- Memory: 512 kB Flash, 32 kB RAM
- Transmit power: up to +6 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz)
- 8 Mbit SPI NOR Flash

For more information about the EFR32MG22 SoC and BRD4182A board, refer to these documents:

- `SLWRB4182A User Guide <https://www.silabs.com/documents/public/reference-manuals/brd4182a-rm.pdf>`__
- `EFR32xG22 Reference Manual <https://www.silabs.com/documents/public/reference-manuals/efr32xg22-rm.pdf>`__
- `EFR32MG22 Datasheet <https://www.silabs.com/documents/public/data-sheets/efr32mg22-datasheet.pdf>`__

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The EFR32MG22 SoC is configured to use the HFRCODPLL oscillator at 76.8 MHz as the system
clock, locked to the 38.4 MHz external crystal oscillator on the board.

Serial Port
===========

The EFR32MG22 SoC has two USARTs and one EUART.
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners ::

Connect the BRD4001A mainboard with a mounted BRD4182A radio board to your host
computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: slwrb4182a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! slwrb4182a

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
   :board: slwrb4182a
   :goals: build
