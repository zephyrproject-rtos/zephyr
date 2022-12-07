.. _qemu_cortex_m3:

ARM Cortex-M3 Emulation (QEMU)
##############################

Overview
********

This board configuration will use QEMU to emulate the TI LM3S6965 platform.

This configuration provides support for an ARM Cortex-M3 CPU and these devices:

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

The following hardware features are supported:

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

This board configuration uses a system clock frequency of 12 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART0.

If SLIP networking is enabled (see below), an additional serial port will be
used for it.

Known Problems or Limitations
==============================

The following platform features are unsupported:

* Memory protection through optional MPU.  However, using a XIP kernel
  effectively provides TEXT/RODATA write protection in ROM.
* SRAM at addresses 0x1FFF0000-0x1FFFFFFF
* Writing to the hardware's flash memory


Programming and Debugging
*************************

Use this configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_cortex_m3
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!
        threadA: Hello World from arm!
        threadB: Hello World from arm!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

Networking
==========

The board supports SLIP networking over an emulated serial port
(``CONFIG_NET_SLIP_TAP=y``). The detailed setup is described in
:ref:`networking_with_qemu`.

It is also possible to use the QEMU built-in Ethernet adapter to connect
to the host system. This is faster than using SLIP and is also the preferred
way. See :ref:`networking_with_eth_qemu` for details.

References
**********

1. The Definitive Guide to the ARM Cortex-M3, Second Edition by Joseph Yiu (ISBN
   978-0-12-382090-7)
2. ARMv7-M Architecture Technical Reference Manual (ARM DDI 0403D ID021310)
3. Procedure Call Standard for the ARM Architecture (ARM IHI 0042E, current
   through ABI release 2.09, 2012/11/30)
4. Cortex-M3 Revision r2p1 Technical Reference Manual (ARM DDI 0337I ID072410)
5. Cortex-M3 Devices Generic User Guide (ARM DUI 0052A ID121610)
