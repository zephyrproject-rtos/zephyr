.. zephyr:board:: segger_trb_stm32f407

Overview
********

The Cortex-M Trace Reference Board V1.2 (SEGGER-TRB-STM32F407 for short)
board is a reference board, based on the ST Microelectronics STM32F407VE
ARM Cortex-M4 CPU, to test hardware tracing with the SEGGER Trace-Pro
debuggers. It is not meant for general prototype development because
it is extremely limited when it comes to IO, and only has 3 LEDs.

Hardware
********

Information about the board can be found at the `SEGGER website`_ .
The `ST STM32F407VE website`_ contains the processor's information
and the datasheet.

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

LED
---

* LED0 (green) = PA8
* LED1 (green) = PA9
* LED2 (green) = PA10

External Connectors
-------------------

JTAG/SWD debug

+-------+--------------+-------+--------------+
| PIN # | Signal Name  | Pin # | Signal Name  |
+=======+==============+=======+==============+
| 1     | VTref        | 2     | SWDIO/TMS    |
+-------+--------------+-------+--------------+
| 3     | GND          | 4     | SWCLK/TCK    |
+-------+--------------+-------+--------------+
| 5     | GND          | 6     | SWO/TDO      |
+-------+--------------+-------+--------------+
| 7     | ---          | 8     | TDI          |
+-------+--------------+-------+--------------+
| 9     | NC           | 10    | nRESET       |
+-------+--------------+-------+--------------+
| 11    | 5V-Supply    | 12    | TRACECLK     |
+-------+--------------+-------+--------------+
| 13    | 5V-Supply    | 14    | TRACEDATA[0] |
+-------+--------------+-------+--------------+
| 15    | GND          | 16    | TRACEDATA[1] |
+-------+--------------+-------+--------------+
| 17    | GND          | 18    | TRACEDATA[2] |
+-------+--------------+-------+--------------+
| 19    | GND          | 20    | TRACEDATA[3] |
+-------+--------------+-------+--------------+


System Clock
============

SEGGER-STM32F407-TRB has one external oscillator. The frequency of
the main clock is 12 MHz. The processor can setup HSE to drive the
master clock, which can be set as high as 168 MHz.

Programming and Debugging
*************************
The SEGGER-TRB-STM32F407 board is specially designed to test the SEGGER
Trace-Pro debuggers, so this example assumes a J-Trace or J-Link is used.

Flashing an application to the SEGGER-TRB-STM32F407
===================================================

Connect the J-Trace/J-Link USB dongle to your host computer and to the JTAG
port of the SEGGER-TRB-STM32F407 board. Then build and flash an application.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: segger_trb_stm32f407
   :goals: build flash

After resetting the board, you should see LED0 blink with a 1 second interval.

Debugging
=========

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: segger_trb_stm32f407
   :maybe-skip-config:
   :goals: debug

.. _SEGGER website:
   https://www.segger.com/products/debug-probes/j-trace/accessories/trace-reference-boards/overview/

.. _ST STM32F407VE website:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f407ve.html
