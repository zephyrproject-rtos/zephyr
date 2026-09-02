.. SPDX-FileCopyrightText: Copyright Michael Hope <michaelh@juju.nz>
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: comu

Overview
********

The `Comu`_ board by Icy Electronics is a tiny CH32V203 RISC-V computer that fits inside a USB port. It is user
programmable, fitted with a custom bootloader, has four captive touch buttons, two LEDs, and exposes other pins as test
points.

It features the QingKe V4B 32-bit RISC-V core and provides:

* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`NVIC / PFIC (Programmable Fast Interrupt Controller)`
* :abbr:`UART (Universal Asynchronous Receiver-Transmitter)`
* :abbr:`USB (Universal Serial Bus)`

Hardware
********

The QingKe V4B 32-bit RISC-V processor on the Comu is clocked by the internal 8 MHz HSI oscillator
with PLL up to 144 MHz. The CH32V203 SoC features 64 KB of Flash, 20 KB of SRAM, USB device peripheral,
Touch Key Detection controller, and other common peripherals.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

* LED0 (Left) = PA4 (active low)
* LED1 (Right) = PB11 (active low)

Buttons / Touch Pads
--------------------

DO_NOT_SUBMIT: depends on the TKEY driver

* SW0 (Touch L1) = PB1
* SW1 (Touch L2) = PB0
* SW2 (Touch R2) = PA7
* SW3 (Touch R1) = PA5

Serial Console
--------------

The serial console is mapped to two of the test points:

* USART2 TX = PA2 / TP2
* USART2 RX = PA3 / TP1

Programming and Debugging
*************************

Applications for the ``comu`` board can be built and flashed in the usual way
(see :ref:`build_an_application` and :ref:`application_run` for more details).

Flashing
========

Comu has a built-in bootloader which implements the ``minichlink`` HID protocol. To flash the board, enter the
bootloader by power cycling the board, such as by unplugging and then replugging it, and run:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: comu
   :goals: build flash

The bootloader will automatically time out after 5 seconds and start the application.

Debugging
=========

Due to the limited number of pins, the debug interface is shared with the USB port.

DO_NOT_SUBMIT: document how to recover the board

References
**********

.. target-notes::

.. _Comu: https://github.com/cheyao/comu
.. _WCH webpage on CH32V203: https://www.wch-ic.com/products/CH32V203.html
