.. _qemu_x86:

X86 Emulation (QEMU)
####################

Overview
********

The Zephyr Kernel uses the qemu_x86 board configuration to emulate Pentium-class
systems running on QEMU.
This board configuration provides support for an x86 Minute IA CPU and the
following devices:

* HPET
* Advanced Programmable Interrupt Controller (APIC)
* NS16550 UART


Hardware
********

Supported Features
==================

The qemu_x86 board configuration supports the following hardware features:

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

Devices
=======

HPET System Clock Support
-------------------------

The qemu_x86 board configuration uses an HPET clock frequency of 25 MHz.

Serial Port
-----------

The qemu_x86 board configuration uses a single serial communication channel that
uses the NS16550 serial driver operating in polling mode. To override, enable
the UART_INTERRUPT_DRIVEN Kconfig option, which allows the system to be
interrupt-driven.

Known Problems or Limitations
=============================

The following platform features are unsupported:

* Isolated Memory Regions
* Serial port in Direct Memory Access (DMA) mode
* Serial Peripheral Interface (SPI) flash
* General-Purpose Input/Output (GPIO)
* Inter-Integrated Circuit (I2C)
* Ethernet
* Supervisor Mode Execution Protection (SMEP)
