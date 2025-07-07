.. zephyr:board:: qemu_kvm_arm64

Overview
********

This board configuration will use QEMU to run a KVM guest on an AArch64
host.

This configuration provides support for an AArch64 Cortex-A CPU and these
devices:

* GICv3 interrupt controller
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

This board configuration uses the host system clock frequency.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART0.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Refer to the qemu_cortex_a53 board instructions for this part.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
