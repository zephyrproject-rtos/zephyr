.. zephyr:board:: slwrb4311a

Overview
********

The `BGM220PC22 Bluetooth Module 2.4 GHz +8 dBm Radio Board`_ is available standalone and as part of
the `BGM220 Bluetooth Module Wireless Starter Kit`_. It is a complete reference design for the BGM220
Wireless Module.

See :ref:`silabs_radio_boards` for more information about the Wireless Mainboard platform.

.. _BGM220PC22 Bluetooth Module 2.4 GHz +8 dBm Radio Board:
   https://www.silabs.com/development-tools/wireless/bluetooth/slwrb4311a-bgm220pc22-bluetooth-module-radio-board

.. _BGM220 Bluetooth Module Wireless Starter Kit:
   https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-wireless-starter-kit

Hardware
********

- BGM220PC22HNA module based on EFR32BG22 SoC
- CPU core: ARM Cortex®-M33 with FPU, DSP and TrustZone
- Memory: 512 kB Flash, 32 kB RAM
- Transmit power: up to +8 dBm
- Operation frequency: 2.4 GHz
- Crystals for LFXO (32.768 kHz) and HFXO (38.4 MHz)
- 8 Mbit SPI NOR Flash

For more information about the BGM220 module and BRD4311A board, refer to these documents:

- `SLWRB4311A User Guide <https://www.silabs.com/documents/public/user-guides/ug432-brd4311a-user-guide.pdf>`__
- `EFR32xG22 Reference Manual <https://www.silabs.com/documents/public/reference-manuals/efr32xg22-rm.pdf>`__
- `BGM220P Datasheet <https://www.silabs.com/documents/public/data-sheets/bgm220p-datasheet.pdf>`__

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The BGM220 module is configured to use the HFRCODPLL oscillator at 76.8 MHz as the system
clock, locked to the 38.4 MHz external crystal oscillator on the board.

Serial Port
===========

The BGM220 module has two USARTs and one EUART.
USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners ::

Connect the BRD4001A mainboard with a mounted BRD4311A radio board to your host
computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: slwrb4311a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! slwrb4311a

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
   :board: slwrb4311a
   :goals: build
