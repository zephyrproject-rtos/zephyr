.. zephyr:board:: am62l_evm

Overview
********

The AM62L EVM board configuration is used by Zephyr applications that run on
the TI AM62L platform. The board configuration provides support for:

- ARM Cortex-A53 core and the following features:

   - General Interrupt Controller (GIC)
   - ARM Generic Timer (arch_timer)
   - On-chip SRAM (oc_sram)
   - UART interfaces (uart0 to uart6)

The board configuration also enables support for the semihosting debugging console.

See the `TI AM62L Product Page`_ for details.

Hardware
********
The AM62L EVM features the AM62L SoC, which is composed of a dual Cortex-A53
cluster. The following listed hardware specifications are used:

- High-performance ARM Cortex-A53
- Memory

   - 160KB of SRAM
   - 2GB of DDR4

- Debug

   - XDS110 based JTAG

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 1250 MHz.

DDR RAM
-------

The board has 2GB of DDR RAM available.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
MAIN domain UART (main_uart0).

Building and Booting
*********************

Dual-Stage Boot Architecture
=============================

The AM62L SoC uses a dual-stage boot architecture:

**Stage 1: tiboot3.bin **
   - Loaded by boot ROM from boot media (SD/eMMC/OSPI)
   - Contains SPL/BL1 (first-stage bootloader) from TF-A
   - Contains TIFS (TI Foundation Security firmware)
   - Contains board configuration (security, power, resource management)
   - Initializes SRAM, validates TIFS, loads Stage 2 into DDR

**Stage 2: tispl.bin **
   - Loaded by ROM into DDR memory
   - Contains TF-A BL31 (ARM Trusted Firmware - secure monitor)
   - Contains Zephyr application binary

Building Boot Images
====================

Prerequisites
-------------

For BL1 boot and SCMI support, the TI downstream ARM Trusted Firmware is required.
The upstream TF-A does not include the necessary Zephyr module integration.

Apply the following patch to ``west.yml`` to use TI's downstream TF-A with the
required Zephyr module support:

.. code-block:: diff

   diff --git a/west.yml b/west.yml
   index 1234567..abcdefg 100644
   --- a/west.yml
   +++ b/west.yml
   @@ -408,6 +408,7 @@
        - name: trusted-firmware-a
   +      url: https://github.com/TexasInstruments/arm-trusted-firmware.git
   -      revision: 44bcc378b6ec4af8693d008f43983e488f1f5740
   +      revision: 17d2997c0e7d4549720a5d176916f5ea0f63b009
          path: modules/tee/tf-a/trusted-firmware-a
          groups:
            - tee

After applying this patch, run ``west update`` to fetch the TI downstream TF-A with
SCMI and BL1 boot support.

Building
--------

By default, building generates ``tispl.bin`` automatically:

.. zephyr-app-commands::
   :tool: west
   :app: samples/hello_world
   :board: am62l_evm/am62l3/a53
   :goals: build
   :compact:

This generates ``build/zephyr/tispl.bin`` (Stage 2: TF-A BL31 + TIFS+ Board Configuration + Zephyr application).

To also generate ``tiboot3.bin`` (Stage 1 bootloader), enable :kconfig:option:`CONFIG_TI_K3_BUILD_TIBOOT3`:

.. zephyr-app-commands::
   :tool: west
   :app: samples/hello_world
   :board: am62l_evm/am62l3/a53
   :goals: build
   :gen-args: -DCONFIG_TI_K3_BUILD_TIBOOT3=y
   :compact:

The build system will automatically:

1. Build ARM Trusted Firmware-A (BL31, and BL1 if generating tiboot3.bin)
2. Generate board configuration blobs (if generating tiboot3.bin or tispl.bin)
3. Create tispl.bin (Stage 2 with TF-A BL31 + Zephyr app) - **always generated**
4. Create tiboot3.bin (Stage 1 bootloader) - **only if CONFIG_TI_K3_BUILD_TIBOOT3=y**

Generated Boot Files
====================

After a default build, the following files are generated:

.. code-block:: console

   build/zephyr/tispl.bin
   build/zephyr/bl31.bin

When :kconfig:option:`CONFIG_TI_K3_BUILD_TIBOOT3` is enabled, additional files are generated:

.. code-block:: console

   build/zephyr/tiboot3.bin
   build/zephyr/bl1.bin

Booting from SD Card
********************

The AM62L EVM supports two boot methods: direct boot (no U-Boot) and U-Boot-based boot.

Option 1: Direct Boot (No U-Boot)
==================================

This method boots Zephyr directly using only the generated boot images.

**Build Requirements:**

To use direct boot, both ``tiboot3.bin`` and ``tispl.bin`` are required. Build with:

.. code-block:: console

   west build -b am62l_evm/am62l3/a53 samples/hello_world -- \
     -DCONFIG_TI_K3_BUILD_TIBOOT3=y

This generates both boot images (tispl.bin is enabled by default, tiboot3.bin requires the flag).

**Preparation:**

1. Prepare an SD card with a FAT32 boot partition
2. Copy the generated boot images to the SD card:

   .. code-block:: console

      cp build/zephyr/tiboot3.bin /media/boot/
      cp build/zephyr/tispl.bin /media/boot/

3. Insert the SD card and power on the board

**Boot Sequence:**

.. code-block:: text

   Boot ROM → tiboot3.bin (SPL + TIFS) → tispl.bin (TF-A + Zephyr)
                                                           ↓
                                                    Zephyr Running

The boot ROM loads ``tiboot3.bin``, which initializes the hardware and loads
``tispl.bin`` into DDR. The TF-A BL31 then starts the Zephyr application at
0x82000000. No U-Boot is involved.

Option 2: U-Boot-Based Boot
============================

Download TI's official `WIC`_ and flash the WIC file with an etching software
onto an SD-card.

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and start zephyr.bin:

.. code-block:: console

   fatload mmc 1:1 0x82000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x82000000

The Zephyr application should start running on the A53 core.


Debugging
*********

The board is equipped with an XDS110 JTAG debugger. To debug a binary, utilize the ``debug`` build target:

.. zephyr-app-commands::
   :app: <my_app>
   :board: am62l_evm/am62l3/a53
   :maybe-skip-config:
   :goals: debug

.. hint::
   To utilize this feature, you'll need OpenOCD version 0.12 or higher. Due to the possibility of
   older versions being available in package feeds, it's advisable to `build OpenOCD from source`_.


Custom Board Configuration
***************

Board configurations are defined in YAML files under ``boards/ti/am62l_evm/support/``:

- ``board-cfg.yaml`` - General board configuration
- ``sec-cfg.yaml`` - Security configuration
- ``pm-cfg.yaml`` - Power management (not required for AM62L)
- ``rm-cfg.yaml`` - Resource management (not required for AM62L)

To customize, edit these files and rebuild. The build system will automatically
regenerate the board configuration binary blob.

Custom Signing Keys
===================

For production, replace the development signing key:

.. code-block:: kconfig

   CONFIG_TI_K3_CERT_KEY_PATH="/path/to/production_key.pem"

The key must be RSA 2048-bit in PEM format.

Boot Image Structure
=====================

**tiboot3.bin structure:**

.. code-block:: text

   +------------------+
   | X.509 Certificate|  (Signed with RSA key)
   +------------------+
   | SPL/BL1 Binary   |  (TF-A first stage)
   +------------------+
   | TIFS Firmware    |  (System firmware)
   +------------------+
   | TIFS Inner Cert  |  (Firmware certificate)
   +------------------+
   | Board Config     |  (Combined YAML configs)
   +------------------+

**tispl.bin structure:**

.. code-block:: text

   +------------------+
   | X.509 Certificate|  (Signed with RSA key)
   +------------------+
   | TF-A BL31        |  (ARM Trusted Firmware)
   +------------------+
   | TIFS Firmware    |  (System firmware)
   +------------------+
   | Board Config     |  (Combined YAML configs)
   +------------------+
   | Zephyr Binary    |  (Application code)
   +------------------+

References
**********

.. _AM62L EVM TRM:
   https://www.ti.com/lit/pdf/sprujb4

.. _TI AM62L Product Page:
   https://www.ti.com/product/AM62L

.. _WIC:
   https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-YjEeNKJJjt/11.02.08.02/tisdk-default-image-am62lxx-evm-11.02.08.02.rootfs.wic.xz

.. _EVM User's Guide:
   https://www.ti.com/lit/pdf/SPRUJG8

.. _build OpenOCD from source:
   https://docs.u-boot.org/en/latest/board/ti/k3.html#building-openocd-from-source
