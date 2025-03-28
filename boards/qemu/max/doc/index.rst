.. zephyr:board:: qemu_max

Overview
********

This board configuration will use QEMU to emulate a generic MAX hardware
platform.

This configuration provides support for an ARM MAX CPU and these
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
emulated environment, for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_max
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** Booting Zephyr OS build zephyr-v2.0.0-1657-g99d310da48e5 *****
        threadA: Hello World from cpu 0 on qemu_max!
        threadB: Hello World from cpu 0 on qemu_max!
        threadA: Hello World from cpu 0 on qemu_max!
        threadB: Hello World from cpu 0 on qemu_max!
        threadA: Hello World from cpu 0 on qemu_max!
        threadB: Hello World from cpu 0 on qemu_max!
        threadA: Hello World from cpu 0 on qemu_max!
        threadB: Hello World from cpu 0 on qemu_max!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

Networking
==========

The board supports the QEMU built-in Ethernet adapter to connect to the host
system. See :ref:`networking_with_eth_qemu` for details.

It is also possible to use SLIP networking over an emulated serial port.
Although this board only supports a single UART, so subsystems like logging
and shell would need to be disabled, therefore this is not directly supported.

References
**********

.. target-notes::

1. (ID050815) ARM® Cortex®-A Series - Programmer’s Guide for ARMv8-A
2. (ID070919) Arm® Architecture Reference Manual - Armv8, for Armv8-A architecture profile
3. (ARM DAI 0527A) Application Note Bare-metal Boot Code for ARMv8-A Processors
4. AArch64 Exception and Interrupt Handling
5. Fundamentals of ARMv8-A
