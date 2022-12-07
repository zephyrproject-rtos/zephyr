.. _qemu_cortex_a55:

ARM Cortex-A55 Emulation (QEMU)
###############################

Overview
********

This board configuration will use QEMU to emulate a generic Cortex-A55 hardware
platform.

This configuration provides support for an ARM Cortex-A55 CPU and these
devices:

* GIC-400 interrupt controller
* ARM architected timer
* PL011 UART controller

Hardware
********
Supported Features
==================

The following hardware features are supported:

+--------------+------------+----------------------+
| Interface    | Controller | Driver/Component     |
+==============+============+======================+
| GIC          | on-chip    | interrupt controller |
+--------------+------------+----------------------+
| PL011 UART   | on-chip    | serial port          |
+--------------+------------+----------------------+
| ARM TIMER    | on-chip    | system clock         |
+--------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 62.5 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART0.

Known Problems or Limitations
==============================

The following platform features are unsupported:

* Writing to the hardware's flash memory


Programming and Debugging
*************************

Use this configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_cortex_a55
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

	[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: cortex-a55
	*** Booting Zephyr OS build zephyr-v3.2.0-2393-g0aef1450f031 ***
	thread_a: Hello World from cpu 0 on qemu_cortex_a55!
	thread_b: Hello World from cpu 0 on qemu_cortex_a55!
	thread_a: Hello World from cpu 0 on qemu_cortex_a55!
	thread_b: Hello World from cpu 0 on qemu_cortex_a55!
	thread_a: Hello World from cpu 0 on qemu_cortex_a55!
	thread_b: Hello World from cpu 0 on qemu_cortex_a55!

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

Networking
==========

References
**********

1. (ID050815) ARM® Cortex®-A Series - Programmer’s Guide for ARMv8-A
2. (ID070919) Arm® Architecture Reference Manual - Armv8, for Armv8-A architecture profile
3. (ARM DAI 0527A) Application Note Bare-metal Boot Code for ARMv8-A Processors
4. AArch64 Exception and Interrupt Handling
5. Fundamentals of ARMv8-A
