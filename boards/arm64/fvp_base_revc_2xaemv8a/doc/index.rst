.. _fvp_base_revc_2xaemv8a:

ARM BASE RevC AEMv8A Fixed Virtual Platforms
############################################

Overview
********

This board configuration will use ARM Fixed Virtual Platforms(FVP) to emulate
a generic Armv8-A 64-bit hardware platform.

This configuration provides support for a generic Armv8-A 64-bit CPU and
these devices:

* GICv3 interrupt controller
* ARM architected (Generic) timer
* PL011 UART controller

Hardware
********

Supported Features
==================

The following hardware features are supported:

+-----------------------+------------+----------------------+
| Interface             | Controller | Driver/Component     |
+=======================+============+======================+
| GICv3                 | on-chip    | interrupt controller |
+-----------------------+------------+----------------------+
| PL011 UART            | on-chip    | serial port          |
+-----------------------+------------+----------------------+
| ARM GENERIC TIMER     | on-chip    | system clock         |
+-----------------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

Devices
========

System Clock
------------

This board configuration uses a system clock frequency of 100 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
UART0.

Known Problems or Limitations
==============================

Programming and Debugging
*************************

Use this configuration to build basic Zephyr applications and kernel tests in the
ARM FVP emulated environment, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: fvp_base_revc_2xaemv8a
   :goals: build

This will build an image with the synchronization sample app.

To run with FVP, ARMFVP_BIN_PATH must be set before running:

e.g. export ARMFVP_BIN_PATH=<path/to/fvp/dir>

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

Networking
==========

References
**********

1. (ID070919) Arm® Architecture Reference Manual - Armv8, for Armv8-A architecture profile
2. AArch64 Exception and Interrupt Handling
3. https://developer.arm.com/tools-and-software/simulation-models/fixed-virtual-platforms
