.. zephyr:board:: ucan

Overview
********

The FYSETC UCAN is an open-source USB to CAN 2.0B adapter board. More information can be found on
the `UCAN website`_ and in the `UCAN wiki`_.

Hardware
********

The UCAN board is equipped with a STM32F072CB microcontroller and features an USB-C connector, a
terminal block for connecting to the CAN bus, and two user LEDs. Schematics and component placement
drawings are available in the `UCAN GitHub repository`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The STM32F072CB PLL is driven by an external crystal oscillator (HSE) running at 8 MHz and
configured to provide a system clock of 48 MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

If flashing via USB DFU, short pins ``B0`` and ``3V3`` when applying power to the UCAN board in
order to enter the built-in DFU mode.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ucan
   :goals: flash

.. _UCAN website:
   https://www.fysetc.com/products/fysetc-ucan-board

.. _UCAN wiki:
   https://wiki.fysetc.com/UCAN/

.. _UCAN GitHub repository:
   https://github.com/FYSETC/UCAN/
