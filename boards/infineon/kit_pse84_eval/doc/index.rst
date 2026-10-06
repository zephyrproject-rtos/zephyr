.. zephyr:board:: kit_pse84_eval

Overview
********

The `KIT_PSE84_EVAL`_ is an evaluation kit based on the PSOC™ Edge E84
family, featuring a PSE846GPS2DBZC4A microcontroller with an Arm®
Cortex®-M33 core at 200 MHz, an Arm® Cortex®-M55 core at 400 MHz, and
an Arm® Ethos™-U55 NPU. It is designed for machine learning, wearables,
and IoT applications.

The evaluation kit uses a SODIMM-based detachable SOM connected to a
feature-rich baseboard. The SOM carries the MCU along with 128-Mb QSPI
flash, 1-Gb Octal flash, 128-Mb Octal HYPERRAM™, a PSOC™ 4000T
CAPSENSE™ co-processor, and an onboard AIROC™ CYW55513IUBG Wi-Fi +
Bluetooth® combo module.

The baseboard provides extensive connectivity including USB Host Type-A,
USB Device Type-C, RJ45 Ethernet, M.2 B-key and E-key slots, Arduino,
mikroBUS, and Shield2Go headers, MIPI-DSI display interfaces, analog and
PDM microphones, headphone and speaker outputs, microSD slot, CAPSENSE
buttons and slider, and comprehensive debug headers (ETM/JTAG/SWD).

The board includes an onboard `KitProg3`_ programmer/debugger with USB
Type-C connectivity.

Boot Flow
=========

The Cortex®-M33 is the boot core. The Cortex®-M55 stays in reset until an
application running on the M33 releases it.

.. code-block:: none

   Reset
     -> ROM extended boot
          BOOT SW ON:  CM33 Secure image in external flash (XIP)
          BOOT SW OFF: MCUBoot in RRAM, which validates and starts the CM33 Secure image
     -> CM33 Secure image (Zephyr, or TF-M when using the /ns target)
          -> CM33 application
          -> optional: releases the CM55
               -> CM55 application

The first image run by the ROM must be in MCUboot image format. The build
produces this format automatically; see `Secure Boot`_ for details. Using the
MCUBoot bootloader is optional; see `MCUBoot Bootloader Support`_.

Board Targets
*************

.. list-table::
   :header-rows: 1
   :widths: 35 40 10 15

   * - Build Target
     - What runs
     - Sysbuild
     - Flashed file
   * - ``kit_pse84_eval/pse846gps2dbzc4a/m33``
     - Zephyr on CM33 Secure. The CM55 is not started.
     - No
     - ``zephyr.signed.hex``
   * - ``kit_pse84_eval/pse846gps2dbzc4a/m33/ns``
     - TF-M on CM33 Secure, Zephyr on CM33 Non-Secure.
     - No
     - ``tfm_merged.hex``
   * - ``kit_pse84_eval/pse846gps2dbzc4a/m55``
     - Zephyr on CM55. A minimal CM33 image (``enable_cm55``) is built and flashed with it to
       start the CM55.
     - Yes
     - Both images

There are two ways to run an application on the CM55:

- **Standalone CM55** (``m55`` target with ``--sysbuild``): the CM55 runs the
  application and the CM33 only starts it. This is the usual choice.
- **CM55 with TF-M** (``m33/ns`` and ``m55`` built separately, **without**
  sysbuild): the CM55 uses the TF-M secure services running on the CM33. See
  `TF-M Multicore Support`_.

Hardware
********

- **SoC:** PSOC™ Edge E84 (PSE846GPS2DBZC4A)
- **CPUs:** Arm® Cortex®-M33 at 200 MHz (boot core), Arm® Cortex®-M55 at
  400 MHz
- **NPU:** Arm® Ethos™-U55
- **SOM Flash:** 128-Mb QSPI flash + 1-Gb Octal flash
- **SOM RAM:** 128-Mb Octal HYPERRAM
- **Wireless:** CYW55513IUBG (on-SOM) + M.2 E-key for external radio
- **Co-processor:** PSOC™ 4000T CAPSENSE co-processor
- **Display:** MIPI-DSI (Raspberry Pi + custom display compatible)
- **Audio:** Analog and PDM microphones, headphone, speaker
- **Sensors:** 6-axis IMU, 3-axis magnetometer
- **USB:** Host Type-A + Device Type-C
- **Ethernet:** RJ45
- **Expansion:** M.2 B-key (memory) + E-key (radio), Shield2Go, mikroBUS, Arduino
- **Storage:** microSD slot
- **User I/O:** CAPSENSE buttons and slider
- **Security:** Arm® TrustZone®-M, secure enclave with crypto accelerators
- **Debug:** Onboard KitProg3 (SWD + UART bridge), ETM/JTAG/SWD headers
- **Voltage Options:** MCU: 2.7 V, 3.3 V, 4.2 V; Peripheral: 1.8 V, 3.3 V

For more information about the PSOC™ Edge E84 and KIT_PSE84_EVAL:

- `PSOC Edge E84 SoC Website`_
- `KIT_PSE84_EVAL Board Website`_

Kit Contents
============

- PSOC™ Edge E84 base board
- PSOC™ Edge E84 SOM module
- 4.3 in. capacitive touch display and USB camera module
- USB Type-C to Type-C cable
- Two proximity sensor wires
- Four stand-offs for Raspberry Pi compatible display
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

+-------+---------------+-----------------------------------+
| Pin   | Function      | Usage                             |
+=======+===============+===================================+
| P6.7  | SCB2 UART TX  | Console TX (``uart2``)            |
+-------+---------------+-----------------------------------+
| P6.5  | SCB2 UART RX  | Console RX (``uart2``)            |
+-------+---------------+-----------------------------------+
| P10.1 | SCB4 UART TX  | BT HCI TX (``uart4``)             |
+-------+---------------+-----------------------------------+
| P10.0 | SCB4 UART RX  | BT HCI RX (``uart4``)             |
+-------+---------------+-----------------------------------+
| P10.3 | SCB4 UART RTS | BT HCI RTS (``uart4``)            |
+-------+---------------+-----------------------------------+
| P10.2 | SCB4 UART CTS | BT HCI CTS (``uart4``)            |
+-------+---------------+-----------------------------------+
| P16.7 | GPIO          | LED0 / Red (active high)          |
+-------+---------------+-----------------------------------+
| P16.6 | GPIO          | LED1 / Green (active high)        |
+-------+---------------+-----------------------------------+
| P16.5 | GPIO          | LED2 / Blue (active high)         |
+-------+---------------+-----------------------------------+
| P8.3  | GPIO          | Button SW0 (active low, pull-up)  |
+-------+---------------+-----------------------------------+

Serial Port
===========

All targets use **SCB2** (``uart2``) for the console and shell. It is routed
through the KitProg3 USB-UART bridge. Default settings are **115200 8N1**.
When both cores run Zephyr, their output shares this UART.

The Bluetooth HCI UART is **SCB4** (``uart4``), with hardware flow control
(RTS/CTS), connected to the on-SOM CYW55513 module.

System Clock
============

The PSOC™ Edge E84 has 14 high-frequency clocks (``CLK_HF0`` to
``CLK_HF13``), configured by the CM33 image. The ones most relevant to
applications are:

- **CLK_HF0:** 200 MHz, CM33 core and system clock
- **CLK_HF1:** 400 MHz, CM55 core

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The `KIT_PSE84_EVAL`_ includes an onboard programmer/debugger (`KitProg3`_)
which can be used to program and debug both PSOC™ Edge E84 cores.

Board Setup
===========

.. note::

   The ``SW6`` (``BOOT SW``) DIP switch selects where the ROM extended boot jumps
   (see `Secure Boot`_):

   - **ON** (default): directly to the application in external flash. Use this
     for all builds without MCUBoot, including the examples below.
   - **OFF**: to the MCUBoot bootloader in RRAM (``0x22011000``). Use this when
     MCUBoot is enabled.

   On some boards the switch is under the attached LCD screen.

Connect a USB cable from your PC to the KitProg3 USB Type-C connector (J8).
Open the KitProg3 serial port with a terminal of your choice (minicom, PuTTY,
etc.) at **115200 8N1**.

Infineon OpenOCD Installation
=============================

The `ModusToolbox™ Programming Tools`_ package includes Infineon OpenOCD.
Alternatively, download the `Infineon OpenOCD`_ release for your system and
extract it to a location of your choice.

.. note::

   On Linux, KitProg3 needs device access rights. The ModusToolbox™
   Programming Tools installer sets these up. For a standalone OpenOCD
   installation, run ``openocd/udev_rules/install_rules.sh``.

Tell west where Infineon OpenOCD is installed. On Windows the executable is
``openocd.exe``.

.. code-block:: shell

   west config build.cmake-args -- "-DOPENOCD=path/to/infineon/openocd/bin/openocd"

.. note::

   This replaces any existing ``build.cmake-args`` value. To set the path for a
   single command instead, use ``west flash --openocd <path>``.

Building and Flashing
=====================

CM33:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33
   :goals: build flash

CM33 Non-Secure with TF-M:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33/ns
   :goals: build flash

CM55 (sysbuild builds and flashes the ``enable_cm55`` CM33 image as well):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_eval/pse846gps2dbzc4a/m55
   :west-args: --sysbuild
   :goals: build flash

The console shows the board target that was built, for example:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_pse84_eval/pse846gps2dbzc4a/m33

Use ``west flash --erase`` to erase the device before programming. This erases
the internal RRAM and **all** external flash, including any stored settings.

Debugging
=========

CM33:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33
   :goals: debug

For the CM55, ``west debug`` attaches to the CM55 on GDB port 3334. The CM55 is
started by the CM33, not by a chip reset, so the GDB ``run`` command cannot
restart it. Use the ``restart`` GDB command provided by the board instead.

Secure Boot
***********

The PSOC™ Edge E84 MCU includes an extended boot stage in ROM that, on reset, jumps to the first
application image. The destination is selected by the on-board ``BOOT SW``:

- ``BOOT SW`` **OFF**: the ROM extended boot jumps to the first application located in internal
  RRAM, for example the MCUBoot bootloader.
- ``BOOT SW`` **ON**: the ROM extended boot jumps to the first application located in external
  flash. Zephyr applications for this board are placed here.

In both cases the first application image must be in MCUboot image format, i.e. it must be
preceded by an MCUboot image header (magic number, header size, vector table address, image size)
and followed by the trailer with the hash/signature TLVs. Out of the box, the device is **not**
provisioned for secure boot, so the ROM extended boot only checks the image format and hash; no
cryptographic signature verification is performed against a provisioned key.

The MCUboot image format is produced automatically by the
:file:`soc/infineon/edge/pse84/pse84_metadata.cmake` helper
``pse84_add_extboot_metadata()``, which invokes ``imgtool sign`` with the header address,
header size and slot size derived from the devicetree memory map. By default this helper does not
pass a signing key, which is sufficient for a non-provisioned device.

Enabling Secure Boot
====================

To enable real signature verification by the ROM extended boot, the device must be reprovisioned.
Follow sections **2.2.1**, **2.2.2** and **2.2.3** of the
`PSOC™ Edge Security Getting Started Application Note`_ to:

#. Generate (or import) the OEM signing key pair.
#. Provision the device with the corresponding public key and lifecycle transition.
#. Program the desired security counter / anti-rollback value.

After the device has been reprovisioned, the
``pse84_add_extboot_metadata()`` function in
:file:`soc/infineon/edge/pse84/pse84_metadata.cmake` must be updated so that ``imgtool sign``
also receives the signing key and a security counter. The relevant additions are:

.. code-block:: none

   ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version "0.0.0+0"
     --header-size ${header_size} --erased-val 0xff
     --slot-size ${slot_size} --hex-addr ${hex_addr}
     --key <oem-private-key-file>
     --security-counter <value>
     ${INPUT_FILE} ${OUTPUT_FILE}

Where ``<oem-private-key-file>`` is the path to the OEM private key file (e.g. a ``.pem``
file) matching the public key provisioned into the device, and ``<value>`` is the security
counter assigned during provisioning. Without these additional parameters, images built for a
provisioned device will be rejected by the ROM extended boot.

MCUBoot Bootloader Support
**************************

The ``kit_pse84_eval`` board supports `MCUBoot`_ for bootloader and
over-the-air (OTA) firmware updates. The PSOC™ Edge E84 extended-boot ROM
validates an MCUBoot-compatible image header at the MCUBoot bootloader location
in RRAM before handing off to MCUBoot, which then validates and starts the
application(s).

.. IMPORTANT::
   When using MCUBoot, the ``SW6`` (``BOOT SW``) DIP switch **MUST** be set to
   the **OFF** position. This causes the extended boot to jump to the MCUBoot
   bootloader in RRAM at ``0x22011000``. If the switch is left ON, the extended
   boot will attempt to jump directly to the external flash XIP address,
   bypassing MCUBoot entirely.

The base memory map (``kit_pse84_eval_memory_map.dtsi``) defines every flash
region under a role label describing what it *is* (for example ``m33s_xip``,
``m33_xip``, ``m55_xip``). The MCUBoot image-numbering labels
(``slotN_partition``) are layered on top of those role labels. The absolute,
shared slots ``slot2_partition`` .. ``slot5_partition`` are consumed only by
the MCUBoot bootloader, so they are attached to those role nodes in the
bootloader overlay
(``boot/zephyr/boards/kit_pse84_eval_pse846gps2dbzc4a_m33.overlay`` in the
MCUBoot repository), which also selects
``boot_partition`` and RRAM as the bootloader flash device. The
``slot0_partition`` / ``slot1_partition`` labels are relative to the image
being built and are bound by the per-core board files
(``kit_pse84_eval_m33.dts`` / ``kit_pse84_eval_m33_ns.dts``). This means no
extra DTS overlay is needed for application images.

Flash Layout with MCUBoot
=========================

When MCUBoot is enabled, the flash is partitioned as follows:

.. list-table::
   :header-rows: 1

   * - Partition
     - Location
     - Offset
     - Size
     - Description
   * - ``boot_partition``
     - RRAM
     - ``0x11000``
     - 256 KB
     - MCUBoot bootloader (address ``0x22011000``)
   * - ``slot0_partition``
     - flash0_s
     - ``0x100000``
     - 2.25 MB
     - CM33S primary application (active)
   * - ``slot1_partition``
     - flash0_s
     - ``0x700000``
     - 2.25 MB
     - CM33S secondary / update slot
   * - ``slot2_partition``
     - flash0
     - ``0x340000``
     - 1.75 MB
     - CM33NS primary application (active)
   * - ``slot3_partition``
     - flash0
     - ``0x940000``
     - 1.75 MB
     - CM33NS secondary / update slot
   * - ``slot4_partition``
     - flash0_sahb
     - ``0x580000``
     - 1.5 MB
     - CM55 primary application (active)
   * - ``slot5_partition``
     - flash0_sahb
     - ``0xB00000``
     - 1.5 MB
     - CM55 secondary / update slot

The primary slot partitions share the same DTS node as the XIP code partitions
(``m33s_xip`` / ``m33_xip`` / ``m55_xip``), so the same ``zephyr,code-partition``
setting works for both MCUBoot and non-MCUBoot builds. The 0x400-byte image
header is accounted for by ``CONFIG_ROM_START_OFFSET``.

Building Images Independently (Standalone)
==========================================

Each image can be built and flashed individually. Application images bind
``slot0_partition`` through their per-core board devicetree, so no extra DTS
overlays are needed for application images.

The application-level ``kit_pse84_eval_slot.conf`` referenced below is located
in ``boards/infineon/kit_pse84_eval/`` relative to ``$ZEPHYR_BASE``.

Step 1 — MCUBoot bootloader
----------------------------

The bootloader must be linked at ``boot_partition`` in RRAM. The required
overlay and configuration live in the MCUBoot repository and are applied
automatically by Zephyr's board-specific overlay/conf mechanism whenever
MCUBoot is built for this board:
``boot/zephyr/boards/kit_pse84_eval_pse846gps2dbzc4a_m33.overlay`` and
``boot/zephyr/boards/kit_pse84_eval_pse846gps2dbzc4a_m33.conf``. No
``EXTRA_DTC_OVERLAY_FILE`` / ``EXTRA_CONF_FILE`` argument is needed.

That ``kit_pse84_eval_pse846gps2dbzc4a_m33.conf`` defaults to
``CONFIG_UPDATEABLE_IMAGE_NUMBER=2`` so that MCUBoot validates two
independent images out of the box (slot0/slot1 for image 0 and
slot2/slot3 for image 1). This matches the TF-M flow (signed SPE
at slot0 + signed NSPE at slot2).

For a **two-image** build (TF-M SPE + NSPE):

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       bootloader/mcuboot/boot/zephyr -d build_mcuboot

For a **single-image** setup (CM33S only), override the image count to 1:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       bootloader/mcuboot/boot/zephyr -d build_mcuboot \
       -- -DCONFIG_UPDATEABLE_IMAGE_NUMBER=1

Step 2 — CM33S application (slot0)
-----------------------------------

No extra DTS overlay is needed. The conf file sets
``CONFIG_BOOTLOADER_MCUBOOT=y``, the matching MCUBoot mode, and unsigned
image generation:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       <path/to/cm33s/app> -d build_cm33s \
       -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf"

Step 3 — Flashing
-----------------

Each image is flashed independently from its build directory.
``west flash`` automatically selects the correct hex file.

Flash MCUBoot first (required once; re-flash only when updating the
bootloader):

.. code-block:: shell

   west flash -d build_mcuboot

Flash the CM33S application:

.. code-block:: shell

   west flash -d build_cm33s

Upgrade Mode: Swap-Using-Move
=============================

The board configuration files use MCUBoot **swap-using-move** as the default
upgrade mode:

- ``kit_pse84_eval_pse846gps2dbzc4a_m33.conf`` (in the MCUBoot repository)
  sets ``CONFIG_BOOT_SWAP_USING_MOVE=y`` and
  ``CONFIG_MCUBOOT_BOOT_MAX_ALIGN=16`` on the bootloader.
- ``kit_pse84_eval_slot.conf`` sets
  ``CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_USING_MOVE=y`` on the application.

Both files must agree on the mode: the application compiles in its own trailer
geometry (``BOOT_MAX_ALIGN``) and imgtool alignment, and these have to match the
bootloader or the image-confirm offsets will not line up.

Swap is possible on this board because the external QSPI flash
(``s25fs128s``) has a **16-byte** ``write-block-size`` — the size of the
device's Automatic-ECC block — which is within MCUBoot's ``BOOT_MAX_ALIGN``
limit. Every trailer field and swap-status entry is written once per 16-byte
ECC block, so ECC is preserved. ``swap-using-move`` needs no scratch partition
and requires the primary and secondary slots to be equal-sized with uniform
sectors.

In swap mode imgtool signs each image with ``--align 16`` and writes a trailer,
as described below.

Signing and staging a secondary (upgrade) image
-----------------------------------------------

The application build places its signed image (``zephyr.signed.hex``) at the
**primary** slot (for example ``slot0`` / ``m33s_xip``).
To exercise an upgrade you must stage a second image in the **secondary** slot
(continuing the example this would be ``slot1`` / ``m33s_upgrade``)
with a valid trailer so MCUBoot detects the pending update. ``west flash`` on
the same build would only re-program the primary slot, so the secondary image
has to be re-signed with a trailer and relocated to the secondary address.

Relevant addresses (secure SAHB programming view for cm33s, base ``0x70000000``):

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 40

   * - Slot
     - Partition
     - Program address
     - Purpose
   * - ``slot0``
     - ``m33s_xip``
     - ``0x70100000``
     - Primary (active) image
   * - ``slot1``
     - ``m33s_upgrade``
     - ``0x70700000``
     - Secondary (upgrade) image

Sign the upgrade application's raw binary with imgtool, adding a padded trailer
and placing it at the secondary slot's program address. The ``--slot-size``
(``0x240000``) and ``--align 16`` values match the slot geometry and
``write-block-size``; ``--header-size`` matches ``CONFIG_ROM_START_OFFSET``:

.. code-block:: shell

   python bootloader/mcuboot/scripts/imgtool.py sign \
       --version 0.0.0+0 --header-size 0x400 --slot-size 0x240000 --align 16 \
       --pad --hex-addr 0x70700000 \
       build_cm33s_upgrade/zephyr/zephyr.bin \
       build_cm33s_upgrade/zephyr/upgrade_slot1.hex

Then flash the relocated hex into the secondary slot without rebuilding, so the
primary slot is left untouched:

.. code-block:: shell

   west flash -d build_cm33s_upgrade --no-rebuild \
       --hex-file build_cm33s_upgrade/zephyr/upgrade_slot1.hex

On the next reset MCUBoot swaps the secondary image into the primary slot and
boots it.

.. NOTE::
   ``--pad`` alone requests a **test** swap: the upgrade runs once, and unless
   the application confirms it (by calling ``boot_write_img_confirmed()`` or
   flashing an image signed with ``--pad --confirm``), MCUBoot **reverts** to
   the previous image on the following reset. Use ``--pad --confirm`` to make
   the upgrade permanent immediately.

TF-M with MCUBoot (Multi-Image)
================================

When TF-M is used together with MCUBoot, the TF-M Secure Processing
Environment (SPE) is placed in ``slot0_partition`` (image 0) and the
Non-Secure application (NSPE) in ``slot2_partition`` (image 1). MCUBoot
runs in multi-image mode (``CONFIG_UPDATEABLE_IMAGE_NUMBER=2``, the
board default) and validates the SPE and NSPE independently. TF-M itself
still handles the run-time transition from secure to non-secure state.

Signing is done by the helper in
:file:`soc/infineon/edge/pse84/pse84_tfm_signing.cmake`, a thin wrapper around
the default ``cmake/mcuboot.cmake`` flow. It:

- Includes the stock ``cmake/mcuboot.cmake`` to sign the NSPE. Because the
  Non-Secure board devicetree binds ``slot0_partition`` to the NS slot, the
  stock flow already sizes the image against the correct slot (the NS view of
  slot2) with ``imgtool sign --overwrite-only --align 1``.
- Merges the signed SPE (produced by TF-M's own build) with the freshly
  signed NSPE into ``tfm_merged.hex`` and points ``west flash`` at it.

Boot flow::

    Extended Boot (ROM) → MCUBoot (RRAM, 0x22011000)
        → validates image 0 (TF-M SPE  @ slot0 / 0x18100000)
        → validates image 1 (NSPE     @ slot2 / 0x08340000)
        → jumps to TF-M SPE
            → TF-M configures TrustZone / MPC / SAU
            → TF-M launches NSPE (@ 0x08340000)

Step 1 — MCUBoot bootloader
---------------------------

Same as the standalone two-image case above — the board default already
validates slot0 and slot2, and the bootloader overlay/conf are applied
automatically from the MCUBoot repository:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       bootloader/mcuboot/boot/zephyr -d build_mcuboot

Step 2 — TF-M NS application (SPE + NSPE)
-----------------------------------------

Building the ``m33/ns`` target with ``CONFIG_BUILD_WITH_TFM=y`` (enabled by
default for the ``/ns`` variant) automatically builds TF-M as the SPE,
signs it and the NSPE independently, and produces ``tfm_merged.hex``
containing both signed images ready to be validated by MCUBoot:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns \
       <path/to/ns/app> -d build_tfm_ns \
       -- -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf"

Step 3 — Flashing
------------------

Flash MCUBoot (once):

.. code-block:: shell

   west flash -d build_mcuboot

Flash the TF-M + NS application. ``west flash`` automatically uses
``tfm_merged.hex`` which contains the signed SPE (slot0) and signed NSPE
(slot2):

.. code-block:: shell

   west flash -d build_tfm_ns

.. NOTE::
   Because SPE and NSPE are signed and validated as two independent
   MCUBoot images, OTA updates can target either one without re-signing
   the other: a new SPE is staged in slot1 and a new NSPE in slot3.

TF-M Multicore Support
**********************

In this configuration TF-M runs on the CM33 Secure side and serves PSA requests from both the
CM33 Non-Secure application and the CM55 application. The CM55 reaches TF-M through a
mailbox-based relay. It is enabled with ``CONFIG_PSOC_EDGE_M55_SRF_SUPPORT``, which must be set
on **both** images.

.. note::

   Unlike the standalone CM55 target, this configuration does **not** use sysbuild. The two
   images are built as separate Zephyr applications and flashed separately.

Building
========

The CM55 build uses PSA manifest headers generated by the CM33 Non-Secure (TF-M) build, so the
CM33 image **must be built first**. The CM55 build is pointed at it with the
``PSE84_CM33_BUILD_DIR`` CMake variable, which must be the CM33 build directory (the one passed
to ``west build -d``). Use an absolute path.

#. Build the CM33 Non-Secure image:

   .. code-block:: shell

      west build -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns \
                 -d build_multicore_33 samples/hello_world \
                 -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y

#. Build the CM55 image:

   .. code-block:: shell

      west build -b kit_pse84_eval/pse846gps2dbzc4a/m55 \
                 -d build_multicore_55 samples/basic/blinky \
                 -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
                    -DPSE84_CM33_BUILD_DIR=<absolute-path-to>/build_multicore_33

If ``PSE84_CM33_BUILD_DIR`` is not given, it defaults to ``${ZEPHYR_BASE}/build``. If the
directory does not contain ``tfm/generated/interface/include``, the CM55 build stops with a
``FATAL_ERROR``.

Flashing
========

Flash the CM55 image first. On reset the CM33 Non-Secure image starts the CM55, and it faults if
no valid CM55 image is present.

.. code-block:: shell

   west flash -d build_multicore_55
   west flash -d build_multicore_33

Multicore with MCUBoot (Three Images)
=====================================

The TF-M multicore setup can also run behind MCUBoot. In this configuration
MCUBoot validates three independent images before hand-off:

- image 0 — TF-M SPE at ``slot0_partition`` (``0x100000``)
- image 1 — CM33 Non-Secure at ``slot2_partition`` (``0x340000``)
- image 2 — CM55 application at ``slot4_partition`` (``0x580000``)

Compared to the standalone multicore build above, the only additions are:
build MCUBoot with ``CONFIG_UPDATEABLE_IMAGE_NUMBER=3`` and pass the
``kit_pse84_eval_slot.conf`` (which enables ``CONFIG_BOOTLOADER_MCUBOOT``)
to both application builds. ``CONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y`` still
enables the CM55 core and must be set on both images.

.. note::

   ``SW6`` (``BOOT SW``) must be in the **OFF** position so the extended
   boot jumps to MCUBoot in RRAM.

Step 1 — MCUBoot bootloader (three images)
------------------------------------------

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       bootloader/mcuboot/boot/zephyr -d build_mcuboot3 \
       -- -DCONFIG_UPDATEABLE_IMAGE_NUMBER=3

Step 2 — CM33-NS image (SPE + NSPE)
-----------------------------------

Build the CM33-NS image first; the CM55 build consumes its generated PSA
manifest headers.

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns \
       -d build_multicore_33 samples/hello_world \
       -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
          -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf"

Step 3 — CM55 image
-------------------

Point the CM55 build at the CM33-NS build directory via
``PSE84_CM33_BUILD_DIR``.

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m55 \
       -d build_multicore_55 samples/basic/blinky \
       -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
          -DPSE84_CM33_BUILD_DIR=<absolute-path-to>/build_multicore_33 \
          -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf"

Step 4 — Flashing
-----------------

Flash MCUBoot once, then the CM55 image before the CM33-NS image (the
CM33-NS image boots the CM55 on reset):

.. code-block:: shell

   west flash -d build_mcuboot3
   west flash -d build_multicore_55
   west flash -d build_multicore_33

References
**********

.. _KIT_PSE84_EVAL:
    https://www.infineon.com/evaluation-board/KIT-PSE84-EVAL

.. _PSOC Edge E84 SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84

.. _KIT_PSE84_EVAL Board Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-EVAL

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _PSOC™ Edge Security Getting Started Application Note:
    https://www.infineon.com/assets/row/public/documents/30/42/infineon-an237849-getting-started-psoc-edge-security-applicationnotes-en.pdf

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3

.. _MCUBoot:
    https://docs.mcuboot.com/
