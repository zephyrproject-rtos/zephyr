.. zephyr:board:: ch32v103evt

Overview
********

The `WCH`_ CH32V103EVT is an evaluation board for the RISC-V based CH32V103C8T6
SOC.

The board is equipped with a power LED, reset button, USB port for power, and
two user LEDs. The `WCH webpage on CH32V103`_ contains the processor's
information and the datasheet.

Hardware
********

The QingKe V3A 32-bit RISC-V processor of the WCH CH32V103EVT is clocked by an
external crystal and runs at 72 MHz.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED1 = Unconnected. Connect to an I/O pin (PD0).
* LED2 = Unconnected. Connect to an I/O pin (PC0).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ch32v103evt`` board can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details); however, an external programmer is required since the board
does not have any built-in debug support.

Connect the programmer to the following pins on the PCB:

* VCC = VCC (do not power the board from the USB port at the same time)
* GND = GND
* SWIO = PA13
* SWCLK = PA14

Flashing
========

You can use minichlink_ to flash the board. Once ``minichlink`` has been set
up, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ch32v103evt
   :goals: build flash

Debugging
=========

This board can be debugged via OpenOCD or ``minichlink``.

Testing the LED on the WCH CH32V103EVT
**************************************

The ``blinky`` sample can be used to test that the LEDs on the board are working
properly with Zephyr:

* :zephyr:code-sample:`blinky`

You can build and flash the examples to make sure Zephyr is running
correctly on your board. The LED definitions can be found in
:zephyr_file:`boards/wch/ch32v103evt/ch32v103evt.dts`.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _WCH webpage on CH32V103: https://www.wch-ic.com/downloads/CH32V103DS0_PDF.html
.. _minichlink: https://github.com/cnlohr/ch32fun/tree/master/minichlink
