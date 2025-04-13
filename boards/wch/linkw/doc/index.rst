.. zephyr:board:: linkw

Overview
********

The `WCH`_ LinkW hardware provides support for QingKe 32-bit RISC-V4C
processor and the following devices:

* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`

The board is equipped with two LEDs and two Buttons.
The `WCH webpage on CH32V208`_ contains the processor's manuals.
The `WCH webpage on LinkW`_ contains the LinkW's schematic.

Hardware
********

The QingKe 32-bit RISC-V4C processor of the WCH LinkW is clocked by an external
32 MHz crystal or the internal 8 MHz oscillator and runs up to 144 MHz.
The CH32V208 SoC Features 4 USART, 4 GPIO ports, 2 SPI, 2 I2C, ADC, RTC,
CAN, 2 USB Device, USB Host, OPA, ETH with PHY, several timers, and BLE 5.3.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED0 = Green Mode LED
* LED1 = Blue Activity LED

Button
------

* SW0 = Mode Select Button (Active Low)
* SW1 = Bootstrap Button (Active High)

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``linkw`` board target can be built and flashed
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
   :board: linkw
   :goals: build flash

Debugging
=========

This board can be debugged via OpenOCD or ``minichlink``.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _WCH webpage on CH32V208: https://www.wch-ic.com/products/CH32V208.html
.. _WCH webpage on LinkW: https://www.wch-ic.com/products/WCH-Link.html
