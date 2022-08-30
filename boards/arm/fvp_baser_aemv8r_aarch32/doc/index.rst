.. _fvp_baser_aemv8r_aarch32:

Arm FVP BaseR AEMv8-R AArch32
#############################

Overview
********

This board configuration uses Armv8-R AEM FVP [1]_ to emulate a generic
Armv8-R [2]_ 32-bit hardware platform.

Fixed Virtual Platforms (FVP) are complete simulations of an Arm system,
including processor, memory and peripherals. These are set out in a
"programmer's view", which gives you a comprehensive model on which to build
and test your software.

The Armv8-R AEM FVP is a free of charge Armv8-R Fixed Virtual Platform. It
supports the latest Armv8-R feature set. Please refer to FVP documentation
page [3]_ for more details about FVP.

To Run the Fixed Virtual Platform simulation tool you must download "Armv8-R AEM
FVP" from Arm developer [1]_ (This might require the user to register) and
install it on your host PC.

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
| Arm GENERIC TIMER     | on-chip    | system clock         |
+-----------------------+------------+----------------------+

The kernel currently does not support other hardware features on this platform.

When FVP is launched with ``-a, --application FILE`` option, the kernel will be
loaded into DRAM region ``[0x0-0x7FFFFFFF]``. For more information, please refer
to the official Armv8-R AEM FVP memory map document [4]_.

Devices
=======

System Clock
------------

This board configuration uses a system clock frequency of 100 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
UART0.

Programming and Debugging
*************************

Use this configuration to build basic Zephyr applications and kernel tests in the
Arm FVP emulated environment, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: fvp_baser_aemv8r_aarch32
   :goals: build

This will build an image with the synchronization sample app. To run with FVP,
first set environment variable ``ARMFVP_BIN_PATH`` before using it. Then you
can run it with ``west build -t run``.

.. code-block:: bash

   export ARMFVP_BIN_PATH=/path/to/fvp/directory
   west build -t run

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

References
**********

.. [1] https://developer.arm.com/tools-and-software/simulation-models/fixed-virtual-platforms/arm-ecosystem-models
.. [2] Arm Architecture Reference Manual Supplement - Armv8, for Armv8-R AArch32 architecture profile
       https://developer.arm.com/documentation/ddi0568/latest
.. [3] https://developer.arm.com/tools-and-software/simulation-models/fixed-virtual-platforms/docs
.. [4] https://developer.arm.com/documentation/100964/1114/Base-Platform/Base---memory/BaseR-Platform-memory-map
