.. zephyr:board:: fvp_base_revc_2xaem

Arm BASE RevC 2xAEM Fixed Virtual Platforms
###########################################

Overview
********

This board configuration will use Arm Fixed Virtual Platforms(FVP) to emulate
a generic AEM (Architectural Envelope Model) hardware platform supporting both
ARMv8-A and ARMv9-A architectures.

This configuration provides support for generic AEM CPUs and these devices:

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

Board Variants
==============

The following board targets are available:

* ``fvp_base_revc_2xaem/v8a`` - ARMv8-A (64-bit) with Cortex-A53 cores
* ``fvp_base_revc_2xaem/v8a/smp`` - ARMv8-A SMP (4 cores)
* ``fvp_base_revc_2xaem/v8a/smp/ns`` - ARMv8-A SMP Non-Secure
* ``fvp_base_revc_2xaem/v9a`` - ARMv9-A (64-bit) with Cortex-A510 cores
* ``fvp_base_revc_2xaem/v9a/smp`` - ARMv9-A SMP (4 cores)
* ``fvp_base_revc_2xaem/v9a/smp/ns`` - ARMv9-A SMP Non-Secure
* ``fvp_base_revc_2xaem/a320`` - ARMv9.2-A with Cortex-A320 configuration

**Cortex-A320 Variant:**

The ``fvp_base_revc_2xaem/a320`` variant provides Cortex-A320 specific FVP
configuration with:

* ARMv9.2-A architecture compliance
* Enhanced cryptographic extensions (SHA3, SHA512, SM3, SM4)
* Advanced memory tagging (MTE Level 3)
* QARMA3 Pointer Authentication
* Optimized cache configuration for Cortex-A320
* Performance monitoring unit with SVE-specific events

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
Arm FVP emulated environment, for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: fvp_base_revc_2xaem/v8a
   :goals: build

This will build an image with the synchronization sample app for ARMv8-A.
Then you can run it with ``west build -t run``.

For ARMv9-A variants:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: fvp_base_revc_2xaem/v9a
   :goals: build

For Cortex-A320 variants:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: fvp_base_revc_2xaem/a320
   :goals: build

For SMP variants:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: fvp_base_revc_2xaem/v8a/smp
   :goals: build

For SMP Non-Secure variants with TF-A:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: fvp_base_revc_2xaem/v8a/smp/ns
   :goals: build

Running Zephyr at EL1NS
***********************

In order to run Zephyr as EL1NS with ``CONFIG_ARMV8_A_NS``, you'll need a proper
Trusted Firmware loaded in the FVP model.

The Arm TF-A for FVP can be used to run Zephyr as preloaded BL33 payload.

Checkout and Build the TF-A:

.. code-block:: console

   git clone https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git --depth 1
   cd trusted-firmware-a/
   make PLAT=fvp PRELOADED_BL33_BASE="0x88000000" all fip

then export the :envvar:`ARMFVP_BL1_FILE` and :envvar:`ARMFVP_FIP_FILE` environment variables:

.. code-block:: console

   export ARMFVP_BL1_FILE=<path/to/tfa-a/build/fvp/release/bl1.bin>
   export ARMFVP_FIP_FILE=<path/to/tfa-a/build/fvp/release/fip.bin>

Migration from Legacy Board Names
*********************************

The legacy board name ``fvp_base_revc_2xaemv8a`` has been replaced with the
unified ``fvp_base_revc_2xaem/v8a`` naming. Update your build commands:

* Old: ``west build -b fvp_base_revc_2xaemv8a``
* New: ``west build -b fvp_base_revc_2xaem/v8a``

The legacy board name remains supported for backward compatibility.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

References
**********

- `Arm Architecture Reference Manual - Armv8 <https://developer.arm.com/documentation/ddi0487/latest>`_
- `Fixed Virtual Platforms <https://developer.arm.com/tools-and-software/simulation-models/fixed-virtual-platforms>`_
