.. zephyr:board:: fvp_corstone1000

Arm Corstone-1000-A320 Fixed Virtual Platform
#############################################

Overview
********

This board configuration uses the Arm Corstone-1000-A320 Fixed Virtual Platform
(FVP) to emulate an Arm Corstone-1000 reference design with Cortex-A320 host
processor.

The Corstone-1000 is an Arm reference design for secure IoT systems, featuring:

* Host processor subsystem with Cortex-A320 (ARMv9.2-A)
* Secure Enclave (Cortex-M0+) running TF-M
* Ethos-U85 NPU (Neural Processing Unit)
* CryptoCell-312 hardware accelerator
* System-level security with MCUboot verified boot

This configuration provides support for:

* Cortex-A320 CPU (ARMv9.2-A architecture)
* GICv3 interrupt controller (GIC-600)
* ARM architected (Generic) timer
* PL011 UART controller
* SVE2 (Scalable Vector Extension 2)

.. note::

   The Ethos-U85 NPU is present in the FVP model but is not currently supported
   by this board configuration.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Board Variants
==============

The following board target is available:

* ``fvp_corstone1000/a320`` - ARMv9.2-A with Cortex-A320 CPU

Memory Map
==========

The Corstone-1000-A320 FVP has the following memory layout for the host subsystem:

+------------------+-------------------+--------+
| Region           | Address           | Size   |
+==================+===================+========+
| DRAM             | 0x80000000        | 2 GB   |
+------------------+-------------------+--------+
| GIC Distributor  | 0x1C000000        | 64 KB  |
+------------------+-------------------+--------+
| GIC Redistributor| 0x1C040000        | 2 MB   |
+------------------+-------------------+--------+
| Host UART0       | 0x1A510000        | 4 KB   |
+------------------+-------------------+--------+

Boot Architecture
*****************

The Corstone-1000 uses a multi-stage boot process involving two processors:

.. code-block:: none

   Secure Enclave (M0+):  BL1_1 -> BL1_2 -> BL2/MCUboot -> TF-M SPE -.
                                                                        |
   Host CPU (A320):                                TF-A BL2 -> BL31 -> Zephyr (BL33)

**Secure Enclave (Cortex-M0+):**

1. BL1_1 (ROM) — hardware init, key provisioning
2. BL1_2 (OTP) — verified by BL1_1
3. BL2 (MCUboot) — parses GPT, loads and ECDSA-verifies TF-M and TF-A images
4. TF-M Runtime — initializes ITS/PS/Crypto partitions, powers on host CPU via
   DSU-120T PPU

**Host CPU (Cortex-A320):**

1. TF-A BL2 — loaded by MCUboot, parses FIP, loads BL31 and BL33
2. TF-A BL31 — EL3 runtime (PSCI, GIC init), ERETs to NS-EL1
3. BL33 (Zephyr) — non-secure application at EL1

.. note::

   The FVP provisioning cycle requires two full SE boot iterations before the
   real boot. This is normal — the first boot provisions dummy keys and reboots.

Programming and Debugging
*************************

Prerequisites
=============

1. :ref:`toolchain_zephyr_sdk` (recommended) — provides both ``arm-zephyr-eabi``
   for TF-M and ``aarch64-zephyr-elf`` for Zephyr/TF-A builds, plus ``dtc`` for
   TF-A device tree compilation.

2. **Corstone-1000-A320 FVP** — download from
   `Arm Ecosystem FVPs <https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs>`_.

   Set the ``ARMFVP_BIN_PATH`` environment variable to the directory containing
   the FVP binary (the default installation path is shown below):

   .. code-block:: bash

      export ARMFVP_BIN_PATH=~/FVP_Corstone_1000-A320/models/Linux64_GCC-9.3

3. **sgdisk** — for GPT partition table creation. Install from the ``gdisk``
   package:

   .. code-block:: bash

      # Fedora / RHEL
      sudo dnf install gdisk

      # Ubuntu / Debian
      sudo apt install gdisk

Building
========

This board requires TF-M and TF-A modules with Corstone-1000-A320 support.
After checking out this branch, update the modules:

.. code-block:: bash

   west update

This board requires :ref:`sysbuild` to automatically build TF-M
(Secure Enclave) and TF-A (Host firmware) alongside Zephyr:

.. code-block:: bash

   west build -b fvp_corstone1000/a320 samples/hello_world --sysbuild

.. tip::

   To avoid passing ``--sysbuild`` every time, set it as the default:

   .. code-block:: bash

      west config build.sysbuild true

This produces in ``build/firmware/``:

* ``bl1.bin`` — Secure Enclave ROM (BL1_1 + provisioning bundle)
* ``cs1000.bin`` — 32 MB GPT flash image containing signed TF-M, signed TF-A
  BL2, BL31, and Zephyr in a FIP

Running
=======

.. code-block:: bash

   west build -t run

To set a simulated time limit or other FVP options, set ``ARMFVP_EXTRA_FLAGS``
at configure time:

.. code-block:: bash

   ARMFVP_EXTRA_FLAGS="--simlimit 120" west build -b fvp_corstone1000/a320 \
       samples/hello_world --sysbuild
   west build -t run

Expected output (after provisioning cycle):

.. code-block:: none

   *** Booting Zephyr OS build v4.3.0-... ***
   Hello World! fvp_corstone1000/a320

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

The FVP supports connection via Arm Development Studio or other debuggers
using the Iris debug interface.

.. tip::

   For early boot debugging before the UART is available, semihosting can be used by enabling
   :kconfig:option:`CONFIG_SEMIHOST` and :kconfig:option:`CONFIG_SEMIHOST_CONSOLE`, and disabling
   :kconfig:option:`CONFIG_UART_CONSOLE`. The FVP has
   semihosting enabled by default.

Known Limitations
=================

* **Ethos-U85 NPU** — the NPU is present in the FVP model but is not
  currently supported by Zephyr.

* **mmap overlap warning** — ``mmap_add_region_check() failed. error -22``
  appears in TF-A BL2/BL31. This is a non-fatal Ethos-U85 memory map overlap
  in release mode, also present in the Yocto reference build.


References
**********

- `Arm Corstone-1000 Technical Overview <https://developer.arm.com/documentation/102360/latest/>`_
- `Arm Cortex-A320 <https://developer.arm.com/Processors/Cortex-A320>`_
- `Corstone-1000 Software Documentation <https://corstone1000.docs.arm.com/en/corstone1000-2025.12/index.html>`_
- `Corstone-1000 Armv9-A Edge-AI Ecosystem FVP <https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs>`_
