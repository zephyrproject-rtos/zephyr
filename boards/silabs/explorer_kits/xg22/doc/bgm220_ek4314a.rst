.. zephyr:board:: bgm220_ek4314a

Overview
********

The BGM220 Explorer Kit is a small form factor development and evaluation platform for the BGM220P
Bluetooth Module.

The kit features a USB interface, an on-board SEGGER J-Link debugger, one user-LED and button, and
support for hardware add-on boards via a mikroBus socket and a Qwiic connector.

Hardware
********

* BGM220PC22HNA module

  * EFR32BG22 SoC
  * Crystal for HFXO (38.4 MHz)
  * Crystal for LFXO (32768 Hz)

* CPU core: ARM Cortex®-M33 with FPU
* Flash memory: 512 kB
* RAM: 32 kB
* Transmit power: up to +8 dBm
* Operation frequency: 2.4 GHz

For more information about the BGM220P module and Explorer Kit, refer to these documents:

* `BGM220 Website`_
* `BGM220P Datasheet`_
* `EFR32xG22 Reference Manual`_
* `BGM220 Explorer Kit User's Guide`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The system is configured to use the HFRCODPLL oscillator at 76.8 MHz as the system
clock, locked to the 38.4 MHz crystal oscillator in the module.

Serial Port
===========

The BGM220P module has two USARTs and one EUART.
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
   :board: bgm220_ek4314a
   :goals: build

Connect the board to your host computer using the USB port and you
should see a USB connection.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you'll see the following message on the corresponding serial port
terminal session:

.. code-block:: console

   Hello World! bgm220_ek4314a/bgm220pc22hna

Bluetooth
=========

To use BLE functionality, run the command below to retrieve necessary binary
blobs from the Silicon Labs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/observer
   :board: bgm220_ek4314a
   :goals: build

.. _BGM220 Website:
   https://www.silabs.com/wireless/bluetooth/efr32bg22-series-2-modules

.. _BGM220P Datasheet:
   https://www.silabs.com/documents/public/data-sheets/bgm220p-datasheet.pdf

.. _EFR32xG22 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/efr32xg22-rm.pdf

.. _BGM220 Explorer Kit User's Guide:
   https://www.silabs.com/documents/public/user-guides/ug465-brd4314a.pdf
