.. _basic_cortex_m3:

Platform Configuration: basic_cortex_m3
#######################################

Overview
********

The basic_cortex_m3 platform configuration is used by Zephyr applications
to emulate the TI LM3S6965 platform running on QEMU. It provides support
for an ARM Cortex-M3 CPU and the following devices:

* Nested Vectored Interrupt Controller

* System Tick System Clock

* Stellaris UART

.. note::
   This platform configuration makes no claims about its suitability for use
   with an actual ti_lm3s6965 hardware system, or any other hardware system.

Supported Boards
****************

The basic_cortex_m3 platform configuration has been tested on
QEMU 2.1 patched with Zephyr's
:file:`0001-armv7m-support-basepri-primask-interrupt-locking.patch`.

Supported Features
******************

The basic_cortex_m3 platform configuration supports the following
hardware features:

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

Other hardware features are not currently supported by Zephyr applications.

Interrupt Controller
====================

.. _fsl_frdm_k64f's platform documention: fsl_frdm_k64f.html

Refer to the `fsl_frdm_k64f's platform documention`_.

.. note::
   Unlike the fsl_frdm_k64 platform configuration, the basic_cortex_m3
   platform configuration sets option :option:`NUM_IRQ_PRIO_BITS` to '3'.

System Clock
============
The basic_cortex_m3 platform configuration uses a system
clock frequency of 12 MHz.

Serial Port
===========

The basic_cortex_m3 platform configuration uses a single
serial communication channel with the CPU's UART0.

Known Problems or Limitations
*****************************

There is no support for the following:

* Memory protection through optional MPU.
  However, using a XIP kernel effectively provides
  TEXT/RODATA write protection in ROM.

* SRAM at addresses 0x1FFF0000-0x1FFFFFFF

* Writing to the hardware's flash memory

Bibliography
************

1. The Definitive Guide to the ARM Cortex-M3,
   Second Edition by Joseph Yiu (ISBN?978-0-12-382090-7)
2. ARMv7-M Architecture Technical Reference Manual
   (ARM DDI 0403D ID021310)
3. Procedure Call Standard for the ARM Architecture
   (ARM IHI 0042E, current through ABI release 2.09,
   2012/11/30)
4. Cortex-M3 Revision r2p1 Technical Reference Manual
   (ARM DDI 0337I ID072410)
5. Cortex-M3 Devices Generic User Guide
   (ARM DUI 0052A ID121610)
