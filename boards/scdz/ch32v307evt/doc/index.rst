.. zephyr:board:: ch32v307evt

Overview
********

The `SCDZ`_ CH32V307EVT hardware provides support for QingKe V4F 32-bit RISC-V
processor.

The `WCH webpage on CH32V307`_ contains
the processor's information and the datasheet.

Hardware
********

The QingKe V4F 32-bit RISC-V processor of the SCDZ CH32V307EVT is clocked by an external
32 MHz crystal or the internal 8 MHz oscillator and runs with 127 MHz.
The CH32V307 SoC features 8 USART, 5 GPIO banks, 3 SPI, 2 I2C, 2 ADC, RTC,
2 CAN, USB Host/Device, Ethernet and 4 OPA.

Supported Features
==================
- GPIO
- USART

Connections and IOs
===================

LED
---

* LED1 = Unconnected. Connect to an I/O pin (PD0).
* LED2 = Unconnected. Connect to an I/O pin (PD1).

BUTTON
------

* USER0 = Unconnected. Connect to an I/O pinn (PC0).

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Applications for the ``ch32v307evt`` board target can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details); however, an external programmer (like the `WCH LinkE`_) is required since the board
does not have any built-in debug support.

The following pins of the external programmer must be connected to the
following pins on the PCB (best way is to use a ribbon cable):

* VCC = VCC (do not power the board from the USB port at the same time)
* GND = GND
* SWIO = PA13

Flashing
========

You can use ``minichlink`` to flash the board. Once ``minichlink`` has been set
up, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ch32v307evt
   :goals: build flash

Debugging
=========

This board can be debugged via OpenOCD using the WCH openOCD liberated fork, available at https://github.com/jnk0le/openocd-wch.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _WCH webpage on CH32V307: https://www.wch-ic.com/products/CH32V307.html
.. _WCH LinkE: https://www.wch-ic.com/products/WCH-Link.html
