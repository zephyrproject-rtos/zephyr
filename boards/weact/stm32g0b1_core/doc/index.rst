.. zephyr:board:: weact_stm32g0b1_core

Overview
********

The WeAct Studio STM32G0B1 Core Board is breakout board for the STM32G0B1CBT6

Hardware
********

The board is equipped with a STM32G0B1CBT6 microcontroller and features a USB-C connector,
a debug header, one LED, three buttons (reset, boot, user), and two 24-pin headers.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The STM32G0B1CBT6 PLL is driven by an external crystal oscillator (HSE) running at 8 MHz and
configured to provide a system clock of 64 MHz.

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX: PA9/PA10


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board can be debugged by installing the included header,
and attaching an SWD debugger to the 3V3 (3.3V), G (GND), SCK, and DIO
pins on that header.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: weact_stm32g0b1_core
   :goals: flash

.. _WeAct STM32G0B1 Core Board website:
   https://github.com/WeActStudio/WeActStudio.STM32G0B1CoreBoard
