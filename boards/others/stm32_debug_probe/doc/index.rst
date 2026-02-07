.. zephyr:board:: stm32_debug_probe

Overview
********

The STM32 Debug Probe, is a inexpensive board for the `STM32F103x8`_ MCU.

Pin Mapping
===========

The pinout diagram of STM32 Debug Probe can be seen:

.. figure:: img/stm32_debug_probe_pinout.jpg
     :align: center
     :alt: Pinout for STM32 Debug Probe

     Pinout for STM32 Debug Probe


STLinkV2 connection:
====================

The board can be flashed by using STLinkV2 with the following connections.

+--------+---------------+
| Pin    | STLINKv2      |
+========+===============+
| SWCLK  | Clock         |
+--------+---------------+
| GND    | GND           |
+--------+---------------+
| SWDIO  | SW IO         |
+--------+---------------+


Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
------------

The on-board 8Mhz crystal is used to produce a 72Mhz system clock with PLL.

Serial Port
-----------

The Zephyr console output is assigned to UART_2. Default settings are 115200 8N1.

On-Board LEDs
-------------

The board has one on-board LED that is connected to PB12.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``stm32_debug_probe`` board configuration can be
built and flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32_debug_probe
   :goals: build flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32_debug_probe
   :maybe-skip-config:
   :goals: debug

.. _STM32F103x8:
        https://www.st.com/resource/en/datasheet/stm32f103c8.pdf
