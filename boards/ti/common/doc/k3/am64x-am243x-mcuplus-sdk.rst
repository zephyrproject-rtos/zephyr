..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-FileCopyrightText: Copyright 2025 - 2026 Siemens Mobility GmbH
   SPDX-FileCopyrightText: Copyright 2025 Texas Instruments
   SPDX-License-Identifier: Apache-2.0

:orphan:

.. ti-k3-am64x-am243x-mcuplus-sdk


MCU+ SDK
========

Setup
-----

The following steps are from the time this documentation was written and might
change in the future. They target Linux with assumption some basic things (like
python3 and openssl) are installed.

To build these you need to install the TI MCU+ SDK. To do this you need to
follow the steps described in the ``mcupsdk-core`` `Github repository <MCU+ SDK
Github repository_>`_, which includes cloning the repositories with west.  It's
recommended to use another Python venv for this since the MCU+ SDK has own
Python dependencies that could conflict with Zephyr dependencies. You can
replace ``all/dev.yml`` in the ``west init`` command with ``am243x/dev.yml``, if
you want to clone a few less repositories.

You also need to follow the "Downloading And Installing Dependencies" section
but you need to replace all ``am263x`` occurrences in commands with ``am243x``.
You can pass ``--skip_doxygen=true`` and ``--skip_ccs=true`` to the install
script, if you don't want them. You might encounter a error that a script can't
be executed. To fix it you need to mark it as executable with ``chmod +x
<path>`` and run the ``download_components.sh`` again.

.. attention::
   Please also take note of the ``tools`` and ``mcu_plus_sdk`` install path. The
   ``tools`` install path (from ``download_components.sh``) will later be
   referred to as ``$TI_TOOLS`` and the MCU+ SDK path (The ``mcu_plus_sdk``
   directory from where you ran ``west init``) as ``$MCUPSDK``. It is
   recommended to set them as environment variables but it's still necessary to
   replace them accordingly inside files.

.. note::
   Summarized you will most likely want to run the following commands or similar
   versions for setting up the MCU+ SDK:

   .. code-block:: shell

      python3 -m venv .venv
      source .venv/bin/activate
      pip3 install west
      west init -m https://github.com/TexasInstruments/mcupsdk-manifests.git --mr mcupsdk_west --mf am243x/dev.yml
      west update
      ./mcupsdk_setup/am243x/download_components.sh --skip_doxygen=true --skip_ccs=true

After the script finished successfully you want to switch into ``$MCUPSDK`` and
edit the :file:`source/drivers/bootloader/bootloader.c` file to set the
``entryPoint`` to ``0`` inside ``Bootloader_runCpu`` unconditionally. This is
needed due to how Zephyr builds the image currently.

Now you can build the internal libraries with the following commands:

.. code-block:: shell

   make gen-buildfiles DEVICE=am243x PROFILE=release
   make libs DEVICE=am243x PROFILE=release

If you encounter compile errors you have to fix them. For that you might have to
change parameter types, remove missing source files from makefiles or download
missing headers from the TI online reference.


Building MCU+ SDK SBLs
----------------------

The example SBL implementations are found in the
``examples/drivers/boot/<example>/$Z_MCUPSDK_BOARD/r5fss0-0_nortos`` directory.

You can build the example by invoking ``make -C
examples/drivers/boot/<example>/$Z_MCUPSDK_BOARD/r5fss0-0_nortos/ti-arm-clang/
DEVICE=am243x PROFILE=release`` from the ``$MCUPSDK`` directory. While there
exists a ``sbl_prebuilt`` directory it has no contents as of now and is only
populated during the SBL build itself.

Depending on your usage it is recommended to build the following SBLs as necessary:

sbl_null
   Bootloader to load a Zephyr ``.elf`` directly via the Code Composer Studio
   IDE. Recommended to be booted from flash and during development due to ease
   of use.

sbl_uart
   Bootloader to boot a firmware in RPRC format file via UART.
   Recommended to be booted from UART and during development due to not having
   to switch the bootmode between UART and OSPI all the time.

sbl_ospi
   Bootloader to load a firmware in RPRC format from flash.
   Recommended, if persistence is required. It looks for a RPRC image at flash
   offset 0x80000.

sbl_uart_uniflash
   Application that can be booted after the RBL. Usually loaded via UART.
   Receives data via UART and writes them to flash.


You can find more SBLs on the `TI MCU+ SDK bootflow documentation website <TI
bootflow guide_>`_.


Flashing data into external flash with the MCU+ SDK
---------------------------------------------------

To flash data onto external flash you want to build the ``sbl_uart_uniflash``
example, copy the built file called ``sbl_uart_uniflash.release.hs_fs.tiimage``
into your working directory and create a config file, replacing
``$FLASH_OFFSET`` and ``$FLASH_FILE`` with the offset on the flash and file you
want to flash.

.. note::
   ``$FLASH_OFFSET`` should be given as hexadecimal address like ``0x80000``

.. note::
   It is possible to flash multiple files into different locations without
   rebooting. All that is needed to do is copy the last line and andjust the
   file and flash offset.

.. code-block::

   --flash-writer=sbl_uart_uniflash.release.hs_fs.tiimage
   --operation=flash-phy-tuning-data
   --file=$FLASH_FILE --operation=flash --flash-offset=$FLASH_OFFSET

After that you need to run ``python3 uart_uniflash.py -p /dev/ttyUSB0
--cfg=<path-to-the-config-file>``. You might have to adjust ``/dev/ttyUSB0``
depending on your connected devices. You can find more documentation on the `TI
MCU+ SDK flashing documentation website <TI flashing guide_>`_.


Creating a SBL bootable RPRC image from a Zephyr ELF
----------------------------------------------------

To convert a Zephyr ELF file into a RPRC file that can be booted by a SBL like
``sbl_uart`` or ``sbl_ospi`` you need to run multiple commands. They are given
in the below code block and under the assumption that the ``zephyr.elf`` file is
in the current working directory. Based the core you want to run the firmware on
you can change ``BOOTIMAGE_CORE_ID``.

.. code-block:: bash

   export BOOTIMAGE_CORE_ID_r5fss0-0=4
   export BOOTIMAGE_CORE_ID_m4=14
   # set CORE_ID as per your target core
   export BOOTIMAGE_CORE_ID=${BOOTIMAGE_CORE_ID_desired-core}
   $TI_TOOLS/sysconfig_1.21.2/nodejs/node $MCUPSDK/tools/boot/out2rprc/elf2rprc.js ./zephyr.elf
   $MCUPSDK/tools/boot/xipGen/xipGen.out -i ./zephyr.rprc -o ./zephyr.rprc_out -x ./zephyr.rprc_out_xip --flash-start-addr 0x60000000
   $TI_TOOLS/sysconfig_1.21.2/nodejs/node $MCUPSDK/tools/boot/multicoreImageGen/multicoreImageGen.js --devID 55 --out ./zephyr.appimage ./zephyr.rprc_out@${BOOTIMAGE_CORE_ID}
   $TI_TOOLS/sysconfig_1.21.2/nodejs/node $MCUPSDK/tools/boot/multicoreImageGen/multicoreImageGen.js --devID 55 --out ./zephyr.appimage_xip ./zephyr.rprc_out_xip@${BOOTIMAGE_CORE_ID}
   python3 $MCUPSDK/source/security/security_common/tools/boot/signing/appimage_x509_cert_gen.py --bin ./zephyr.appimage --authtype 1 --key $MCUPSDK/source/security/security_common/tools/boot/signing/app_degenerateKey.pem --output ./zephyr.appimage.hs_fs

The resulting file will be ``zephyr.appimage.hs_fs`` in the current working
directory. It is in the TI RPRC format that is understood by SBLs. The format is
described on the `TI MCU+ SDK boot tools documentation website <TI tools
boot_>`_.


Full bootflow examples
----------------------

As summary which steps are required for a boot are listed here.

..
   TODO: Add links to the other sections!

.. tabs::

   .. tab:: Debugger booting

      To boot via the debuuger you first should build ``sbl_uart_uniflash`` and
      ``sbl_null`` as described in `Building MCU+ SDK SBLs`_. After that you
      should put your board into the UART boot mode and flash the
      ``sbl_null.release.hs_fs.tiimage`` file to address ``0x0`` as described in
      `Flashing data into external flash with the MCU+ SDK`_.

      After that you can switch into OSPI boot mode and load Zephyr ELF files
      directly via the Code Composer Studio debugger onto the core you want.

   .. tab:: UART boot

      To boot an image via UART you need to build ``sbl_uart`` as described in
      `Building MCU+ SDK SBLs`_ and switch your board into the UART boot mode.

      Then you need to convert the Zephyr ELF file to a RPRC as descrbied in
      `Creating a SBL bootable RPRC image from a Zephyr ELF`_ and run
      ``python3 $MCUPSDK/tools/boot/uart_bootloader.py -p /dev/ttyUSB0 --bootloader=sbl_uart.release.hs_fs.tiimage --file=zephyr.appimage.hs_fs``

   .. tab:: OSPI boot

      To boot an image via OSPI you need to build ``sbl_uart_uniflash`` and
      ``sbl_ospi`` as described in `Building MCU+ SDK SBLs`_. After that you
      need to put your board into UART boot mode and flash the
      ``sbl_ospi.release.hs_fs.tiimage`` file to ``0x0`` as described in
      `Flashing data into external flash with the MCU+ SDK`_. You only have to
      do this once.

      Then for every new Zephyr firmware build you need convert the Zephyr ELF
      Into a RPRC file as described in `Creating a SBL bootable RPRC image from
      a Zephyr ELF`_ and flash it to ``0x80000`` as described in `Flashing data
      into external flash with the MCU+ SDK`_. After that you can switch into
      the OSPI boot mode and run it.

..
   These are all external links to websites. They are kept at the bottom of the
   file here to avoid duplications.

.. _TI bootflow guide:
   https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/BOOTFLOW_GUIDE.html

.. _TI flashing guide:
   https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/TOOLS_FLASH.html

.. _TI tools boot:
   https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/TOOLS_BOOT.html

.. _MCU+ SDK Github repository:
   https://github.com/TexasInstruments/mcupsdk-core
