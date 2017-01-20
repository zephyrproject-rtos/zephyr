.. _qemu_cortex_m3:

ARM Cortex-M3 Emulation (QEMU)
##############################

Overview
********

The Zephyr kernel uses the qemu_cortex_m3 board configuration to emulate the TI
LM3S6965 platform running on QEMU. It provides support for an ARM Cortex-M3 CPU
and the following devices:

* Nested Vectored Interrupt Controller
* System Tick System Clock
* Stellaris UART

.. note::
   This board configuration makes no claims about its suitability for use
   with an actual ti_lm3s6965 hardware system, or any other hardware system.

Hardware
********
Supported Features
==================

The qemu_cortex_m3 board configuration supports the following hardware features:

+--------------+------------+----------------------+
| Interface    | Controller | Driver/Component     |
+==============+============+======================+
| NVIC         | on-chip    | nested vectored      |
|              |            | interrupt controller |
+--------------+------------+----------------------+
| Stellaris    | on-chip    | serial port          |
| UART         |            |                      |
+--------------+------------+----------------------+
| SYSTICK      | on-chip    | system clock         |
+--------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========
System Clock
------------

The qemu_cortex_m3 board configuration uses a system clock frequency of 12 MHz.

Serial Port
-----------

The qemu_cortex_m3 board configuration uses a single serial communication
channel with the CPU's UART0.

Known Problems or Limitations
==============================

The following platform features are unsupported:

* Memory protection through optional MPU.  However, using a XIP kernel
  effectively provides TEXT/RODATA write protection in ROM.
* SRAM at addresses 0x1FFF0000-0x1FFFFFFF
* Writing to the hardware's flash memory

References
**********

1. The Definitive Guide to the ARM Cortex-M3, Second Edition by Joseph Yiu (ISBN
   978-0-12-382090-7)
2. ARMv7-M Architecture Technical Reference Manual (ARM DDI 0403D ID021310)
3. Procedure Call Standard for the ARM Architecture (ARM IHI 0042E, current
   through ABI release 2.09, 2012/11/30)
4. Cortex-M3 Revision r2p1 Technical Reference Manual (ARM DDI 0337I ID072410)
5. Cortex-M3 Devices Generic User Guide (ARM DUI 0052A ID121610)
