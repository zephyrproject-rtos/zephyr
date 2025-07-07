.. zephyr:board:: candlelight

Overview
********

The candleLight is an open-hardware USB to CAN 2.0B adapter board available from a number of
sources.

Hardware
********

The candleLight board is equipped with a STM32F072CB microcontroller and features an USB connector,
a DB-9M connector for the CAN bus, and two user LEDs. Schematics and component placement drawings
are available in the `candleLight GitHub repository`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The STM32F072CB PLL is driven by the internal RC oscillator (HSI) running at 8 MHz and
configured to provide a system clock of 48 MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

If flashing via USB DFU, short resistor ``R203`` when applying power to the candleLight in order to
enter the built-in DFU mode.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: candlelight
   :goals: flash

.. _candleLight GitHub repository:
   https://github.com/HubertD/candleLight
