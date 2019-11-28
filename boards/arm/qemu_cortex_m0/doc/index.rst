.. _qemu_cortex_m0:

ARM Cortex-M0 Emulation (QEMU)
##############################

Overview
********

This board configuration will use QEMU to emulate the
BBC Microbit (Nordic nRF51822) platform.

.. figure:: qemu_cortex_m0.png
   :width: 600px
   :align: center
   :alt: Qemu

   Qemu (Credit: qemu.org)

This configuration provides support for an ARM Cortex-M0 CPU and these devices:

* Nested Vectored Interrupt Controller
* TIMER (nRF TIMER System Clock)

.. note::
   This board configuration makes no claims about its suitability for use
   with an actual nRF51 Microbit hardware system, or any other hardware system.

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
| nRF          | on-chip    | serial port          |
| UART         |            |                      |
+--------------+------------+----------------------+
| nRF TIMER    | on-chip    | system clock         |
+--------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 1 MHz.

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
   :board: qemu_cortex_m0
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

References
**********

1. The Definitive Guide to the ARM Cortex-M0, Second Edition by Joseph Yiu (ISBN
   978-0-12-803278-7)
2. ARMv6-M Architecture Technical Reference Manual (ARM DDI 0419D 0403D ID051917)
3. Procedure Call Standard for the ARM Architecture (ARM IHI 0042E, current
   through ABI release 2.09, 2012/11/30)
4. Cortex-M0 Revision r2p1 Technical Reference Manual (ARM DDI 0432C ID113009)
5. Cortex-M0 Devices Generic User Guide (ARM DUI 0497A ID112109)
