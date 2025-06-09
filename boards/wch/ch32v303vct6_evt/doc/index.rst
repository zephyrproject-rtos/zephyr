.. zephyr:board:: ch32v303vct6_evt

Overview
********

The `WCH`_ CH32V303VCT6-EVT hardware provides support for QingKe V4F 32-bit RISC-V
processor.

The `WCH webpage on CH32V303`_ contains
the processor's information and the datasheet.

Hardware
********

The QingKe V4F 32-bit RISC-V processor of the WCH CH32V303VCT6-EVT is clocked by an external
32 MHz crystal or the internal 8 MHz oscillator and runs up to 144 MHz.
The CH32V303 SoC features 8 USART, 4 GPIO ports, 3 SPI, 2 I2C, 2 ADC, RTC,
CAN, USB Host/Device, and 4 OPA.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Applications for the ``ch32v303vct6_evt`` board target can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details); however, an external programmer (like the `WCH LinkE`_) is required since the board
does not have any built-in debug support.

Flashing
========

You can use ``minichlink`` to flash the board. Once ``minichlink`` has been set
up, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ch32v303vct6_evt
   :goals: build flash

Debugging
=========

This board can be debugged via OpenOCD using the WCH openOCD liberated fork, available at https://github.com/jnk0le/openocd-wch.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _WCH webpage on CH32V303: https://www.wch-ic.com/products/CH32V303.html
.. _WCH LinkE: https://www.wch-ic.com/products/WCH-Link.html
