.. SPDX-License-Identifier: Apache-2.0
..
.. Copyright (c) 2026 Ezurio LLC

.. note::
   Qwiic SCL is multipexed to the same I/O as MikroBUS SCL. MikroBUS I2C based boards can be used
   in conjunction with Qwiic boards by closing solder bridges SB5 and SB6.

   Qwiic SDA is multipexed to the same I/O as MikroBUS SDA. MikroBUS I2C based boards can be used
   in conjunction with Qwiic boards by closing solder bridges SB9 and SB10.

   Refer to the |User Guide Ref| and |Schematics Ref| for further details.

System Clock
============

The Lyra 24 |Lyra 24 variant| is configured to use the 39 MHz external oscillator on the
board, and can operate at clock speeds of up to 78 MHz.

Serial Port
===========

The Lyra 24 |Lyra 24 variant| has two EUSARTs and one USART.

* EUSART0 defaults to the mikroBUS UART.
* EUSART1 defaults to the mikroBUS SPI port.
* USART0 is connected to the board controller and is used for the console.

Programming and Debugging
*************************

Applications for the |Board Quoted| board can be built, flashed, and debugged in the usual way.
See :ref:`build_an_application` and :ref:`application_run` for more details on building and running.

.. note::
   Before using the kit you should update the J-Link firmware
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
