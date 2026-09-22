.. SPDX-License-Identifier: Apache-2.0
..
.. Copyright (c) 2026 Ezurio LLC

.. note::
   Switch SW1 is used to select I/O compatibility with legacy Bluetooth Xpress firmware.
   It should be set to the C_DEV/SWO position to enable support of the SWO pin and compatibility with
   the board definition files.

System Clock
============

The Lyra |Lyra variant| is configured to use the 38.4 MHz external oscillator on the
board, and can operate at clock speeds of up to 76.8 MHz.

Serial Port
===========

The Lyra |Lyra variant| has two USARTs and one EUART.

* EUART0 defaults to the mikroBUS UART.
* USART0 defaults to the mikroBUS SPI port.
* USART1 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

Applications for the |Board Quoted| board can be built, flashed, and debugged in the usual way.
See :ref:`build_an_application` and :ref:`application_run` for more details on building and running.

.. note::
   Before using the kit, you should update the J-Link firmware
   in `Simplicity Studio`_.

Testing the LED and button
==========================

The :zephyr:code-sample:`blinky` sample can be used to test the DVK LED.

The :zephyr:code-sample:`button` sample can be used to test the DVK button.

Bluetooth Low Energy
====================

To use Bluetooth Low Energy functionality, run the command below to retrieve necessary binary
blobs from the Silicon Labs HAL repository.

.. code-block:: console

   west blobs fetch hal_silabs

Then build the Zephyr kernel and a Bluetooth sample with the following
command. The :zephyr:code-sample:`bluetooth_observer` sample application is used in
this example.
