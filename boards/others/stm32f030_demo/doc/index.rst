.. zephyr:board:: stm32f030_demo

This board has the bare minimum components required to power on
the STM32F030F4P6 MCU. Most of the GPIOs on the STM32 SoC have
been exposed in the external headers with silk screen labels
that match the SoC's pin names.

For practical use, you'll need to add additional components
and circuits using a breadboard, for example.

More information about the board can be found at the `stm32-base.org website`_.

More information about STM32F030F4P6 can be found here:

- `STM32F030 reference manual`_
- `STM32F030 data sheet`_

Hardware
********

- STM32F030F4P6 ARM Cortex-M0 processor, frequency up to 48 MHz
- 16 KiB of flash memory and 4 KiB of RAM
- 8 MHz quartz crystal
- 1 user LED
- One reset button
- 2-way jumper (BOOT0)
- Serial (1x4 male dupont (2.54mm))
- SWD (1x4 male dupont (2.54mm))
- USB port (power only)

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PA9/PA10
- LED : PA4

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``stm32f030_demo`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board can be flashed by using ST-LINKV2 in-circuit debugger and programmer.
This interface is supported by the openocd version included in the Zephyr SDK.

Flashing an application to STM32F030 DEMO BOARD
-----------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f030_demo
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f030_demo
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _stm32-base.org website:
   https://stm32-base.org/boards/STM32F030F4P6-STM32F030-DEMO-BOARD-V1.1

.. _STM32F030 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00091010.pdf

.. _STM32F030 data sheet:
   https://www.st.com/resource/en/datasheet/stm32f030f4.pdf
