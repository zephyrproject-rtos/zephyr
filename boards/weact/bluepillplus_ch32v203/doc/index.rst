.. zephyr:board:: bluepillplus_ch32v203

Overview
********

The `WeActStudio`_ BluePill Plus CH32V203 hardware provides support for QingKe V4B 32-bit RISC-V
processor and the following devices:

* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`UART (Universal Asynchronous Receiver-Transmitter)`

The board is equipped with two LEDs and three Buttons.
User can use one of the LEDs and one of the buttons.
The `WCH webpage on CH32V203`_ contains the processor's manuals.
The `WeActStudio webpage on BPP`_ contains the BluePill's schematic.

Hardware
********

The QingKe V4B 32-bit RISC-V processor of the BluePill Plus CH32V203 is clocked by an external
8 MHz crystal or the internal 8 MHz oscillator and runs up to 144 MHz.
The CH32V203 SoC Features 2-4 USART, 4 GPIO ports, 1-2 SPI, 0-2 I2C, 9-16 ADC, RTC,
CAN, USB Device, USB Host, OPA, and several timers.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED0 = Blue User LED

Button
------

* SW0 = User Button

Programming and Debugging
*************************

Applications for the ``bluepillplus_ch32v203`` board can be built and flashed
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
   :board: bluepillplus_ch32v203
   :goals: build flash

Debugging
=========

This board can be debugged via OpenOCD or ``minichlink``.

References
**********

.. target-notes::

.. _WeActStudio: https://github.com/WeActStudio
.. _WCH webpage on CH32V203: https://www.wch-ic.com/products/CH32V203.html
.. _WeActStudio webpage on BPP: https://github.com/WeActStudio/WeActStudio.BluePill-Plus-CH32
