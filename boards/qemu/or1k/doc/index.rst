.. _qemu_or1k:

OpenRISC 1000 Emulation (QEMU)
##############################

Overview
********

This board configuration will use QEMU to emulate the OpenRISC 1000 platform.

This configuration provides support for an or1k CPU core and these devices:

* OpenRISC Interrupt Controller
* OpenRISC Tick Timer
* NS16550 UART


.. note::
   This board configuration makes no claims about its suitability for use
   with an actual OpenRISC 1000 hardware system, or any other hardware system.

Hardware
********

Supported Features
==================

The following hardware features are supported:

+--------------------------------+------------+----------------------+
| Interface                      | Controller | Driver/Component     |
+================================+============+======================+
| OpenRISC Interrupt Controller  | on-chip    | interrupt controller |
+--------------------------------+------------+----------------------+
| OpenRISC Tick Timer            | on-chip    | system clock         |
+--------------------------------+------------+----------------------+
| NS16550 UART                   | FPGA       | serial port          |
+--------------------------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========
System Clock
------------

Qemu Tick Timer timer uses a clock frequency of 20 MHz,
see hw/openrisc/cputimer.c in Qemu source tree for details.

Serial Port
-----------

This board configuration uses a single serial communication channel
with UART3.

Programming and Debugging
*************************

Use this configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment, for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_or1k
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v3.5.0-3843-g5a1358a9ef ***
        thread_a: Hello World from cpu 0 on qemu_or1k!
        thread_b: Hello World from cpu 0 on qemu_or1k!
        thread_a: Hello World from cpu 0 on qemu_or1k!
        thread_b: Hello World from cpu 0 on qemu_or1k!
        thread_a: Hello World from cpu 0 on qemu_or1k!
        thread_b: Hello World from cpu 0 on qemu_or1k!
        thread_a: Hello World from cpu 0 on qemu_or1k!
        thread_b: Hello World from cpu 0 on qemu_or1k!
        thread_a: Hello World from cpu 0 on qemu_or1k!
        thread_b: Hello World from cpu 0 on qemu_or1k!


Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.


References
**********

https://www.qemu.org/
