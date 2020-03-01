.. _qemu_cortex_m4f:

ARM Cortex-M4F Emulation (QEMU)
###############################

Overview
********

This board configuration will use QEMU to emulate the STM32F405 platform.

.. figure:: qemu_cortex_m4f.png
   :width: 600px
   :align: center
   :alt: Qemu

   Qemu (Credit: qemu.org)

This configuration provides support for an ARM Cortex-M4F CPU and these
devices:

* Floating Point Unit
* Memory Protection Unit
* Nested Vectored Interrupt Controller
* System Tick System Clock
* UART

.. note::
   This board configuration makes no claims about its suitability for use
   with an actual STM32F405 hardware system, or any other hardware system.

Hardware
********
Supported Features
==================

The following hardware features are supported:

+--------------+------------+------------------------+
| Interface    | Controller | Driver/Component       |
+==============+============+========================+
| MPU          | on-chip    | memory protection unit |
+--------------+------------+------------------------+
| NVIC         | on-chip    | nested vectored        |
|              |            | interrupt controller   |
+--------------+------------+------------------------+
| SYSTICK      | on-chip    | system clock           |
+--------------+------------+------------------------+
| USART        | on-chip    | serial port            |
+--------------+------------+------------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 3.125 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
on-chip USART1.

Programming and Debugging
*************************

Use this configuration to run basic Zephyr applications and kernel tests in the
QEMU emulated environment, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_cortex_m4f
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        *** Booting Zephyr OS build v2.2.0  ***
        threadA: Hello World from qemu_cortex_m4f!
        threadB: Hello World from qemu_cortex_m4f!
        threadA: Hello World from qemu_cortex_m4f!
        threadB: Hello World from qemu_cortex_m4f!
        threadA: Hello World from qemu_cortex_m4f!
        threadB: Hello World from qemu_cortex_m4f!
        threadA: Hello World from qemu_cortex_m4f!
        threadB: Hello World from qemu_cortex_m4f!
        threadA: Hello World from qemu_cortex_m4f!
        threadB: Hello World from qemu_cortex_m4f!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

References
**********

1. ARMv7-M Architecture Technical Reference Manual (ARM DDI 0403D ID021310)
2. Cortex-M4 Technical Reference Manual (ARM DDI 0439B ID030210)
3. STM32F405/415 Reference Manual (RM0090)
