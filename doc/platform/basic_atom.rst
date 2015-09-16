.. _basic_atom:

Platform Configuration: basic_atom
##################################

Overview
********

The basic_atom platform configuration is used by Zephyr applications
that run on QEMU emulating the Atom N28xx platform.  This platform
configuration provides support for an x86 Atom CPU and the following devices:

* HPET

* Advanced Programmed Interrupt Controller (APIC)

* NS16550 UART

.. note::
   This platform configuration makes no claims about its suitability for use
   with an actual Atom N28xx hardware system, or any other hardware system.

Supported Boards
****************

The basic_atom platform configuration has been tested on QEMU 2.1.

Supported Features
******************

The basic_atom platform configuration supports the following
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
   The basic_atom platform configuration does not support PCI.

HPET System Clock Support
=========================

The basic_atom platform configuration uses a system
clock frequency of 25 MHz.

.. note::
   The LOAPIC timer may be used instead of the HPET. To do so,
   set SYS_CLOCK_HW_CYCLES_PER_SEC to a custom value that is tied to the host system speed.

Serial Port
===========

The basic_atom platform configuration uses a single serial communication channel
with a NS16550 serial driver that operates in polling mode.
For an interrupt-driven driver, enable the UART_INTERRUPT_DRIVEN kernel configuration option.

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
