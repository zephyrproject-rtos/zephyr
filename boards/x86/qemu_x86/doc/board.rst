.. _qemu_x86:

X86 Emulation (QEMU)
####################

Overview
********

The X86 QEMU board configuration is used to emulate the X86 architecture. This board
configuration provides support for an x86 Minute IA (Lakemont) CPU and the
following devices:

* HPET
* Advanced Programmable Interrupt Controller (APIC)
* NS16550 UART


Hardware
********

Supported Features
==================

This configuration supports the following hardware features:

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

The configuration uses an HPET clock frequency of 25 MHz.

Serial Port
-----------

The board configuration uses a single serial communication channel that
uses the NS16550 serial driver operating in polling mode. To override, enable
the UART_INTERRUPT_DRIVEN Kconfig option, which allows the system to be
interrupt-driven.

If SLIP networking is enabled (see below), an additional serial port will be
used for it.

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

Programming and Debugging
*************************

Use this configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment, for example, with the :ref:`synchronization_sample`:

.. code-block:: console

        $ make -C samples/synchronization BOARD=qemu_x86 run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 27 2017 13:09:26 *****
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!
        threadA: Hello World from x86!
        threadB: Hello World from x86!

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

Networking
==========

The board supports SLIP networking over an emulated serial port
(``CONFIG_NET_SLIP_TAP=y``). The detailed setup is described in
:ref:`networking_with_qemu`.
