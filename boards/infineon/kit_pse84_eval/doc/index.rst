.. zephyr:board:: kit_pse84_eval

Overview
********
The PSOC™ Edge E84 Evaluation Kit (KIT_PSE84_EVAL) enables applications to use the PSOC™ Edge E84 Series
Microcontroller (MCU) together with multiple on-board multimedia, Machine Learning (ML),
and connectivity features including custom MIPI-DSI displays, audio interfaces,
and AIROC™ Wi-Fi and Bluetooth® combo-based connectivity modules.

The PSOC™ Edge E84 MCUs are based on high-performance Arm® Cortex®-M55 including Helium DSP support,
an Ethos™-U55 NPU, and a low-power Arm® Cortex®-M33 paired with Infineon's ultra-low power NNLite
hardware accelerator. They integrate 2.5D graphics accelerators and display interfaces, while
featuring always-on acoustic activity and wake-word detection, efficient HMI operations, and
extended battery life.

The evaluation kit carries a PSOC™ Edge E84 MCU on a SODIMM-based detachable SOM board connected to
the baseboard. The MCU SOM also has 128 MB of QSP| Flash, 1GB of Octal Flash, 128MB of Octal RAM,
PSOC™ 4000T as CAPSENSE™ co-processor, and onboard AIROC™ Wi-Fi and Bluetooth® combo.

Hardware
********
For more information about the PSOC™ Edge E84 MCUs and the PSOC™ Edge E84 Evaluation Kit:

- `PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website`_
- `PSOC™ Edge E84 Evaluation Kit Website`_

Kit Features:
=============

- Cortex®-M55 CPU with Helium™ DSP
- Advanced ML with Arm Ethos™-U55 NPU
- Low-Power Cortex®-M33
- NNLite ultra-low power NPU
- Analog and Digital Microphones
- State-of-the-Art Secured Enclave
- Integrated Programmer/Debugger

Kit Contents:
=============

- PSOC™ Edge E84 base board
- PSOC™ Edge E84 SOM module
- 4.3in capacitive touch display and USB camera module
- USB Type C to Type-C cable
- Two proximity sensor wires
- Four stand-offs for Raspberry Pi compatible display
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Please refer to `kit_pse84_eval User Manual Website`_ for more details.

Programming and Debugging
*************************

.. NOTE::
   `BOOT SW` on the board **MUST** be set to `ON` for any sample applications to work. On some
   boards this switch may be under the attached LCD screen.

.. zephyr:board-supported-runners::

The KIT-PSE84-EVAL includes an onboard programmer/debugger (`KitProg3`_) to provide debugging,
flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and
require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Please refer to the `ModusToolbox™ software installation guide`_ to install Infineon OpenOCD.

Flashing
========

Applications for the ``kit_pse84_eval/pse846gps2dbzc4a/m33`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``kit_pse84_eval/pse846gps2dbzc4a/m55``
board target need to be built using sysbuild to include the required application for the other core.

Enter the following command to compile ``hello_world`` for the CM55 core:

.. zephyr-app-commands::
   :app: samples/hello_world
   :board: kit_pse84_eval/pse846gps2dbzc4a/m55
   :goals: build flash
   :west-args: --sysbuild
   :gen-args: -DOPENOCD=path/to/infineon/openocd/bin/openocd


Debugging
=========
The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool
commands. There are multiple ways of doing this. The example below uses a permanent CMake argument
to set the CMake variable ``OPENOCD``.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

.. zephyr-app-commands::
   :app: samples/basic/blinky
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33
   :goals: debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and
perform other standard GDB debugging on the PSOC E84 CM33 core.

MCUBoot Bootloader Support
**************************

The ``kit_pse84_eval`` board supports `MCUBoot`_ for bootloader and
over-the-air (OTA) firmware updates. The PSOC™ Edge E84 extended-boot ROM
validates an MCUBoot-compatible image header at the MCUBoot bootloader location
before handing off to MCUBoot, which then validates and starts the application.

Flash Layout with MCUBoot
=========================

When MCUBoot is enabled, the flash is partitioned as follows:

.. list-table::
   :header-rows: 1

   * - Partition
     - Flash offset
     - Size
     - Description
   * - ``boot_partition``
     - ``0x100000``
     - 256 KB
     - MCUBoot bootloader (CM33 secure, SAHB alias ``0x70100000``)
   * - ``slot0_partition``
     - ``0x140000``
     - 2 MB
     - CM33 primary application (active)
   * - ``slot1_partition``
     - ``0x340000``
     - 2 MB
     - CM33 secondary / update slot
   * - ``slot2_partition``
     - ``0x540000``
     - 2 MB
     - CM55 primary application (active)
   * - ``slot3_partition``
     - ``0x740000``
     - 2 MB
     - CM55 secondary / update slot

Building Images Independently (Standalone)
==========================================

Each image can be built and flashed individually. The board directory provides
DTS overlay files for the MCUBoot partition layout; pass them on the ``west
build`` command line as shown below.

The board directory overlays referenced below are located in
``boards/infineon/kit_pse84_eval/`` relative to ``$ZEPHYR_BASE``.

Step 1 — MCUBoot bootloader
----------------------------

The bootloader must be linked at ``boot_partition``. Pass both the memory map
overlay, the bootloader-specific overlay, and the MCUBoot configuration file.
This configuration file is just an example; it sets overwrite-only mode and
no signature verification by default.

For a **single-image** setup (CM33 only):

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       bootloader/mcuboot/boot/zephyr -d build_mcuboot \
       -- -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_memory_map_mcuboot.dtsi\;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_bl.overlay" \
          -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot.conf"

For a **dual-image** setup (CM33 + CM55), add ``CONFIG_UPDATEABLE_IMAGE_NUMBER=2``
so MCUBoot validates both slot0 and slot2:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       bootloader/mcuboot/boot/zephyr -d build_mcuboot \
       -- -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_memory_map_mcuboot.dtsi\;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_bl.overlay" \
          -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot.conf" \
          -DCONFIG_UPDATEABLE_IMAGE_NUMBER=2

Step 2 — CM33 application (slot0)
----------------------------------

Pass the memory map overlay and the slot0 conf file. The conf file sets
``CONFIG_BOOTLOADER_MCUBOOT=y``, the matching MCUBoot mode, and unsigned
image generation:

For a **single-image** setup:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       <path/to/cm33/app> -d build_cm33 \
       -- -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_memory_map_mcuboot.dtsi" \
          -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf"

For a **dual-image** setup, also enable the CM55 core:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 \
       <path/to/cm33/app> -d build_cm33 \
       -- -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_memory_map_mcuboot.dtsi" \
          -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf" \
          -DCONFIG_SOC_PSE84_M55_ENABLE=y


Step 3 — CM55 application (slot2, dual-image only)
---------------------------------------------------

Pass both the memory map overlay and the slot2 partition override, plus the
slot2 conf file:

.. code-block:: shell

   west build -b kit_pse84_eval/pse846gps2dbzc4a/m55 \
       <path/to/cm55/app> -d build_cm55 \
       -- -DEXTRA_DTC_OVERLAY_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_memory_map_mcuboot.dtsi\;$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_mcuboot_cm55.overlay" \
          -DEXTRA_CONF_FILE="$ZEPHYR_BASE/boards/infineon/kit_pse84_eval/kit_pse84_eval_slot.conf"

Flashing
========

Each image is flashed independently from its build directory.
``west flash`` automatically uses ``zephyr.signed.hex`` for all
MCUBoot-related images.

Flash MCUBoot first (required once; re-flash only when updating the
bootloader):

.. code-block:: shell

   west flash -d build_mcuboot

Flash the CM33 application:

.. code-block:: shell

   west flash -d build_cm33

Flash the CM55 application (dual-image only):

.. code-block:: shell

   west flash -d build_cm55

Iterating on a Single Image
============================

To update only the CM55 firmware without rebuilding the other images:

.. code-block:: shell

   # Rebuild (incremental)
   west build -d build_cm55

   # Re-flash slot2 only
   west flash -d build_cm55

This is the key advantage of the standalone workflow: once MCUBoot and the
CM33 application are stable and flashed, the CM55 image can be developed,
rebuilt, and reflashed independently in seconds.

References
**********

- `PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website`_

.. _PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84#Overview

.. _PSOC™ Edge E84 Evaluation Kit Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-EVAL

.. _kit_pse84_eval User Manual Website:
    https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-eval-qsg-usermanual-en.pdf

.. _ModusToolbox™:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup

.. _ModusToolbox™ software installation guide:
    https://www.Infineon.com/ModusToolboxInstallguide

.. _KitProg3:
    https://github.com/Infineon/KitProg3

.. _MCUBoot:
    https://docs.mcuboot.com/
