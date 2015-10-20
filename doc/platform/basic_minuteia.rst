.. _basic_minuteia:

Platform Configuration: basic_minuteia
######################################

Overview
********

The Zephyr Kernel uses the basic_minuteia platform configuration
to emulate the galileo platform (or something similar) running on QEMU.
It provides support for an x86 Minute IA CPU and the following devices:

* HPET

* Advanced Programmable Interrupt Controller (APIC)

* NS16550 UART

.. note::
   This platform configuration makes no claims about its suitability for use
   with actual galileo hardware, or any other hardware.

Supported Boards
****************

The basic_minuteia platform configuration has been tested on QEMU 2.1.

Supported Features
******************

The basic_minuteia platform configuration supports the following
hardware features:

+--------------+------------+-----------------------+
| Interface    | Controller | Driver/Component      |
+==============+============+=======================+
| HPET         | on-chip    | system clock          |
+--------------+------------+-----------------------+
| APIC         | on-chip    | interrupt controller  |
+--------------+------------+-----------------------+
| NS16550      | on-chip    | serial port           |
| UART         |            |                       |
+--------------+------------+-----------------------+

The kernel currently does not support other hardware features on this platform.

Interrupt Controller
====================

Refer to the :ref:`galileo`.

.. note::
   The basic_minuteia platform configuration does not support PCI.

HPET System Clock Support
=========================

The basic_minuteia platform configuration uses an HPET clock frequency
of 25 MHz.

Serial Port
===========

The basic_minuteia platform configuration uses a single serial
communication channel that uses the NS16550 serial driver
operating in polling mode. To override, enable the UART_INTERRUPT_DRIVEN
Kconfig option, which allows the system to be interrupt-driven.

Known Problems or Limitations
*****************************

The following platform features are unsupported:

* Isolated Memory Regions
* Serial port in Direct Memory Access (DMA) mode
* Serial Peripheral Interface (SPI) flash
* General-Purpose Input/Output (GPIO)
* Inter-Integrated Circuit (I2C)
* Ethernet
* Supervisor Mode Execution Protection (SMEP)
* PCI
