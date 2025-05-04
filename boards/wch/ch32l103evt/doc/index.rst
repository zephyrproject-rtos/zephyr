.. zephyr:board:: ch32l103evt

Overview
********

The `WCH`_ CH32L103EVT hardware provides support for QingKe 32-bit RISC-V4C
processor and the following devices:

* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`DMA (Direct Memory Access)`

The board is equipped with two LEDs.
The `WCH webpage on CH32L103`_ contains the processor's manuals.
The `WCH webpage on CH32L103EVT`_ contains the CH32L103EVT's schematic.

Hardware
********

The QingKe 32-bit RISC-V4C processor of the WCH CH32L103EVT is clocked by an external
8 MHz crystal or the internal 8 MHz oscillator and runs up to 96 MHz.
The CH32V208 SoC Features 4 USART, 4 GPIO ports, 2 SPI, 2 I2C, ADC, RTC,
CAN FD, USB 2.0 Host, USB Type-C PD, OPA, and  several timers.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED3/LED4 = Unconnected. Connect to an I/O pin (PA1).

Button
------

* S1 = Reset Button

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``CH32L103EVT`` board target can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details); however, an external programmer is required since the board
does not have any built-in debug support.

The following pins of the external programmer must be connected to the
following pins on the PCB:

* VCC = VCC
* GND = GND
* SWIO = PA13
* SWCLK = PA14

Flashing
========

You can use ``minichlink`` to flash the board. Once ``minichlink`` has been set
up, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ch32l103evt
   :goals: build flash

Debugging
=========

This board can be debugged via ``minichlink``.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _WCH webpage on CH32L103: https://www.wch-ic.com/products/CH32L103.html
.. _WCH webpage on CH32L103EVT: https://www.wch.cn/downloads/CH32L103EVT_ZIP.html
