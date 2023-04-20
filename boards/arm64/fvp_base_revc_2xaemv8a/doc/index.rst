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
| SMSC_91C111           | on-chip    | ethernet device      |
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

Environment
===========

First, set the ``ARMFVP_BIN_PATH`` environment variable before building.
Optionally, set ``ARMFVP_EXTRA_FLAGS`` to pass additional arguments to the FVP.

.. code-block:: bash

   export ARMFVP_BIN_PATH=/path/to/fvp/directory

Programming
===========

Use this configuration to build basic Zephyr applications and kernel tests in the
ARM FVP emulated environment, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: fvp_base_revc_2xaemv8a
   :goals: build

This will build an image with the synchronization sample app.
Then you can run it with ``west build -t run``.

Running Zephyr at EL1NS
***********************

In order to run Zephyr as EL1NS with ``CONFIG_ARMV8_A_NS``, you'll need a proper
Trusted Firmware loaded in the FVP model.

The ARM TF-A for FVP can be used to run Zephyr as preloaded BL33 payload.

Checkout and Build the TF-A:

.. code-block:: console

   git clone https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git --depth 1
   cd trusted-firmware-a/
   make PLAT=fvp PRELOADED_BL33_BASE="0x88000000" all fip

then export the ``ARMFVP_BL1_FILE` and ``ARMFVP_FIP_FILE`` environment variables:

.. code-block:: console

   export ARMFVP_BL1_FILE=<path/to/tfa-a/build/fvp/release/bl1.bin>
   export ARMFVP_FIP_FILE=<path/to/tfa-a/build/fvp/release/fip.bin>

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
