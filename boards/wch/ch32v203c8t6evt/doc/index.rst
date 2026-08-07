.. zephyr:board:: ch32v203c8t6evt

Overview
********

The `WCH`_ CH32V203C8T6EVT hardware provides the evaluation kit for the CH32V203 MCU.
A QuingKe V4B 32-bit RISC-V processor.

The board is equipped with two 2 LEDs, but those are not connected by default and a jumper has
to be installed.

The `WCH webpage on CH32V203`_ contains the processor's manuals.
The `openwch github`_ contains the schematics and some more references.

Hardware
********

The QingKe V4B 32-bit RISC-V processor is clocked by an external 8 MHz high speed crystal and a
32768 Hz low speed crystal, both also offering internal oscillators.
Those run the board up to 144 MHz.
The CH32V203 SoC Features 2-4 USART, 4 GPIO ports, 1-2 SPI, 0-2 I2C, 9-16 ADC, RTC,
CAN, USB Device, USB Host, OPA, and several timers.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ch32v203c8t6evt`` board can be built and flashed
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

You can use ``minichlink`` or ``wlink`` to flash the board. Once either has been set up, build and
flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Debugging
=========

This board can be debugged via OpenOCD or ``minichlink``.

References
**********

.. target-notes::

.. _WCH: https://www.wch-ic.com
.. _WCH webpage on CH32V203: https://www.wch-ic.com/products/CH32V203.html
.. _openwch github: https://github.com/openwch/ch32v20x
