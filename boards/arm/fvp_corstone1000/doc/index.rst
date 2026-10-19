.. zephyr:board:: fvp_corstone1000

Arm Corstone-1000 Fixed Virtual Platform
########################################

Overview
********

This board configuration uses the Arm Corstone-1000 Fixed Virtual Platform
(FVP) to emulate an Arm Corstone-1000 reference design with Cortex-A320 or
Cortex-A35 host as the processor.

The Corstone-1000 is an Arm reference design for secure IoT systems, featuring:

* Host processor subsystem (Cortex-A320 or Cortex-A35)
* Secure Enclave (Cortex-M0+) running TF-M
* Ethos-U85 NPU (Neural Processing Unit) or External System (Cortex-M3) subsystem
* CryptoCell-312 hardware accelerator
* System-level security with MCUboot verified boot

Zephyr board options
====================

.. tabs::

  .. tab:: Corstone-1000-A320 (FVP)

   Zephyr runs on the Cortex-A320 host processor of the Corstone-1000-A320 FVP.

   The Cortex-A320 configuration provides support for:

   * Cortex-A320 CPU (Armv9.2-A architecture)
   * GICv3 interrupt controller (GIC-600)
   * Arm Generic Timer
   * PL011 UART controller
   * SVE2 (Scalable Vector Extension 2)

   Boot Architecture:
    - The Cortex-A320 host uses a multi-stage boot process involving two processors:

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

      .. note::

         The Ethos-U85 NPU is present in the Corstone-1000-A320 FVP but is not
         currently supported by this board configuration.

  .. tab:: Corstone-1000-A35 (FVP)

   Zephyr runs on a single Cortex-A35 host processor of the standard Corstone-1000 FVP.

   The Cortex-A35 configuration provides support for:

   * Cortex-A35 CPU (Armv8-A architecture, AArch64 execution state)
   * GICv2 interrupt controller
   * Arm Generic Timer
   * PL011 UART controller
   * PSCI CPU power management through TF-A

   Boot Architecture:
    - The Cortex-A35 host uses the Secure Enclave and TF-A boot chain:

      .. code-block:: none

         Secure Enclave (M0+):  BL1_1 -> BL1_2 -> BL2/MCUboot -> TF-M SPE -.
                                                                              |
         Host CPU (A35):                                 TF-A BL2 -> BL31 -> Zephyr (BL33)

      The Secure Enclave verifies and loads the firmware, then powers on the host CPU.
      TF-A BL2 loads BL31 and Zephyr from the FIP. BL31 provides the EL3 runtime
      and starts Zephyr as the non-secure BL33 application at EL1.

      .. note::

         Initial provisioning reboots the Secure Enclave before normal boot, as with
         the Cortex-A320 target.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Board Variants
==============

The following board targets are available:

* ``fvp_corstone1000/a320`` - single-core ARMv9.2-A with Cortex-A320 CPU
* ``fvp_corstone1000/a320/smp`` - quad-core SMP with all 4 Cortex-A320 CPUs
* ``fvp_corstone1000/corstone1000/a35`` - single-core Armv8-A with Cortex-A35 CPU

Programming and Debugging
*************************

Prerequisites
=============

1. :ref:`toolchain_zephyr_sdk` (recommended) — provides both ``arm-zephyr-eabi``
   for TF-M and ``aarch64-zephyr-elf`` for Zephyr/TF-A builds, plus ``dtc`` for
   TF-A device tree compilation.

2. Install the FVP corresponding to the selected board target from
   `Arm Ecosystem FVPs <https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs>`_:

   * ``fvp_corstone1000/a320`` and ``fvp_corstone1000/a320/smp`` require
     ``FVP_Corstone-1000-A320``
   * ``fvp_corstone1000/corstone1000/a35`` requires ``FVP_Corstone-1000``

   Set the ``ARMFVP_BIN_PATH`` environment variable to the directory containing
   the selected FVP binary. For the Cortex-A320 targets, for example:

   .. code-block:: bash

      export ARMFVP_BIN_PATH=~/FVP_Corstone_1000-A320/models/Linux64_GCC-9.3

   For the Cortex-A35 target, point ``ARMFVP_BIN_PATH`` to
   the directory containing ``FVP_Corstone-1000`` instead.

3. The Cortex-A320 and Cortex-A35 sysbuilds require ``sgdisk`` to create the GPT partition
   table. Install it from the ``gdisk`` package:

   .. code-block:: bash

      # Fedora / RHEL
      sudo dnf install gdisk

      # Ubuntu / Debian
      sudo apt install gdisk

Building
========

The Cortex-A320 and Cortex-A35 targets require TF-M and TF-A modules
with support for the selected Corstone-1000 variant. After cloning the
repository, update the modules:

.. code-block:: bash

   west update

Both host variants require :ref:`sysbuild` to automatically build TF-M
(Secure Enclave) and TF-A (Host firmware) alongside Zephyr.

For Cortex-A320:

.. code-block:: bash

   west build -b fvp_corstone1000/a320 samples/hello_world --sysbuild

Use ``fvp_corstone1000/a320/smp`` to build for all four Cortex-A320 CPUs.

For Cortex-A35:

.. code-block:: bash

   west build -b fvp_corstone1000/corstone1000/a35 samples/hello_world --sysbuild

.. tip::

   To avoid passing ``--sysbuild`` every time, set it as the default:

   .. code-block:: bash

      west config build.sysbuild true

For either host variant, the build creates the following files in ``build/firmware/``:

* ``bl1.bin`` — Secure Enclave ROM (BL1_1 + provisioning bundle)
* ``cs1000.bin`` — 32 MB GPT flash image containing signed TF-M, signed TF-A
  BL2, BL31, and Zephyr in a FIP

Running
=======

After building any target, start its corresponding FVP with:

.. code-block:: bash

   west build -t run

To set a simulated time limit or other FVP options, set ``ARMFVP_EXTRA_FLAGS``
at configure time:

.. code-block:: bash

   ARMFVP_EXTRA_FLAGS="--simlimit 120" west build -b fvp_corstone1000/a320 \
       samples/hello_world --sysbuild
   west build -t run

Expected output (after the provisioning cycle for Cortex-A320 and Cortex-A35 targets):

.. code-block:: none

   *** Booting Zephyr OS build v4.3.0-... ***
   Hello World! fvp_corstone1000/<a320,a320/smp,corstone1000/a35>

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

* **Linux/Unix host only** - the sysbuild integration uses POSIX shell
  utilities (``sh``, ``dd``, ``sgdisk``) to assemble the Secure Enclave
  flash image and is not currently supported on Windows hosts.

* **Ethos-U85 NPU** — the NPU is present in the Corstone-1000-A320 FVP model
  but is not currently supported by Zephyr.

* **mmap overlap warning** — ``mmap_add_region_check() failed. error -22``
  appears in TF-A BL2/BL31. This is a non-fatal Ethos-U85 memory map overlap
  in release mode, also present in the Yocto reference build.


References
**********

- `Arm Corstone-1000 Technical Overview <https://developer.arm.com/documentation/102360/latest/>`_
- `Arm Cortex-A320 <https://developer.arm.com/Processors/Cortex-A320>`_
- `Arm Cortex-A35 <https://developer.arm.com/Processors/Cortex-A35>`_
- `Corstone-1000 Software Documentation <https://corstone1000.docs.arm.com/en/corstone1000-2025.12/index.html>`_
- `Corstone-1000 Armv9-A Edge-AI Ecosystem FVP <https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs>`_
