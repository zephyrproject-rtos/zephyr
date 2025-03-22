.. zephyr:board:: bluepillplus_ch32v203

Overview
********

The `WeActStudio`_ Blue Pill Plus CH32V203 hardware provides support for QingKe 32-bit RISC-V4B
processor and the following devices:

* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`

The board is equipped with two LEDs and two Buttons.
The `WCH webpage on CH32V203`_ contains the processor's manuals.
The `WeActStudio webpage on BPP`_ contains the Blue Pill's schematic.

Hardware
********

The QingKe 32-bit RISC-V4B processor of the Blue Pill Plus CH32V203 is clocked by an external
8 MHz crystal or the internal 8 MHz oscillator and runs up to 144 MHz.
The CH32V203 SoC Features 2-4 USART, 4 GPIO ports, 1-2 SPI, 0-2 I2C, 9-16 ADC, RTC,
CAN, USB Device, USB Host, OPA, and several timers.

Supported Features
==================

The ``bluepillplus_ch32v203`` board target supports the following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| PINCTRL   | on-chip    | pinctrl              |
+-----------+------------+----------------------+
| TIMER     | on-chip    | timer                |
+-----------+------------+----------------------+
| UART      | on-chip    | uart                 |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

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

Applications for the ``bluepillplus_ch32v203`` board target can be built and flashed
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

Testing the LED on the Blue Pill Plus CH32V203
**********************************************

There is 1 sample program that allow you to test that the LED on the board is
working properly with Zephyr:

.. code-block:: console

   samples/basic/blinky

You can build and flash the examples to make sure Zephyr is running
correctly on your board. The button and LED definitions can be found
in :zephyr_file:`boards/weact/bluepillplus_ch32v203/bluepillplus_ch32v203.dts`.

References
**********

.. target-notes::

.. _WeActStudio: https://github.com/WeActStudio
.. _WCH webpage on CH32V203: https://www.wch-ic.com/products/CH32V203.html
.. _WeActStudio webpage on BPP: https://github.com/WeActStudio/WeActStudio.BluePill-Plus-CH32
