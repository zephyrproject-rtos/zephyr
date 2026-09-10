.. zephyr:board:: mikroe_stm32_m4_clicker

Overview
********

The Mikroe STM32 M4 Clicker development board contains a STMicroelectronics
Cortex-M4 based STM32F415RG Microcontroller operating at up to 168 MHz with
1 MB of Flash memory and 192 KB of SRAM.

Hardware
********

The STM32 M4 Clicker board contains a USB connector, two LEDs, two push
buttons, and a reset button. It features a mikroBUS socket for interfacing
with external electronics. For more information about the development
board see the `STM32 M4 Clicker website`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building & Flashing
===================

You can build and flash an application in the usual way (See
:ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mikroe_stm32_m4_clicker
   :goals: build flash

Debugging
=========

Debugging also can be done in the usual way.
The following command is debugging the :zephyr:code-sample:`blinky` application.
Also, see the instructions specific to the debug server that you use.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mikroe_stm32_m4_clicker
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _STM32 M4 Clicker website:
	https://www.mikroe.com/clicker-stm32f4
