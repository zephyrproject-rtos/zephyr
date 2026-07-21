.. zephyr:board:: candlelightfd

Overview
********

The candleLight FD is an open-hardware USB to CAN FD adapter board available from Linux Automation GmBH.
Find more information about the board at the `Linux Automation website`_.

Hardware
********

The candleLight FD board is equipped with a STM32G0B1CBT6 microcontroller and features an USB-C connector,
a DB-9M connector for the CAN bus, and two user LEDs. Schematics and component placement drawings
are available in the `candleLight FD GitHub repository`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The STM32G0B1CBT6 PLL is driven by an external crystal oscillator (HSE) running at 8 MHz and
configured to provide a system clock of 60 MHz. This allows generating a FDCAN1 and FDCAN2 core
clock of 80 MHz.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

If flashing via USB DFU, short jumper ``BOOT`` when applying power to the candleLight FD in order to
enter the built-in DFU mode.

Variants
========

The candleLight FD is can be retrofitted with a second transceiver, making it a dual CAN FD device:

- ``candlelightfd``: The default variant.
- ``candlelightfd_stm32g0b1xx_dual``: Variant for the dual CAN FD.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: candlelightfd
   :goals: flash

.. _Linux Automation website:
   https://linux-automation.com/en/products/candlelight-fd.html

.. _candleLight FD GitHub repository:
   https://github.com/linux-automation/candleLightFD
