.. zephyr:board:: ch32v00x_core

Overview
********

The `WeAct`_ CH32V00xCoreBoard is an evaluation board for the RISC-V
based `WCH`_ CH32V006F8U6 SOC.

The board is equipped with a LED, button, and USB port for power. The
`WeAct Github page`_ includes details, schematics, and links out to
the CH32V006 user manual.

Hardware
********

- QingKe V2C 32-bit RISC-V processor at up to 48 MHz
- 8 KiB of RAM and 62 KiB of flash
- Internal 24 MHz high speed oscillator
- USB port for power
- 2 UARTs, 1 I2C, 1 SPI, and 31 GPIOs
- 3 timers with PWM

See the datasheet for more.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

* LED1 on PC4
* User button on PD7
* USART1 TX on PC0 and RX on PC1

PD7 is also the RST pin and should be reconfigured from RST to GPIO mode,
such as by running ``minichlink -D``.

PD3, PD4, and PD5 are connected to the to the USB data lines and should be
left floating. The only usable USART2 pinctrl configuration uses PD3 so,
if USART2 is needed, remove SB4 or power the board externally.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ch32v00x_core`` board can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details); however, an external programmer is required since the board
does not have any built-in debug support.

Connect the programmer to the following pins on the PCB:

* VCC = VCC (do not power the board from the USB port at the same time)
* GND = GND
* SWIO = PD1

Flashing
========

You can use minichlink_ to flash the board. Once ``minichlink`` has been set
up, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ch32v00x_core
   :goals: build flash

Debugging
=========

This board can be debugged via OpenOCD or ``minichlink``.

Testing the LED
***************

The ``blinky`` sample can be used to test that the LEDs on the board are working
properly with Zephyr:

* :zephyr:code-sample:`blinky`

You can build and flash the examples to make sure Zephyr is running
correctly on your board. The LED definitions can be found in
:zephyr_file:`boards/weact/ch32v00x_core/ch32v00x_core.dts`.

References
**********

.. target-notes::

.. _WeAct: https://weactstudioone.aliexpress.com/
.. _WeAct Github page: https://github.com/WeActStudio/WeActStudio.CH32V00xCoreBoard
.. _WCH: http://www.wch-ic.com
.. _minichlink: https://github.com/cnlohr/ch32fun/tree/master/minichlink
