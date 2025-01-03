.. _atm33evk:

Atmosic ATM33/e
###############

Overview
********
The ATM33/e Wireless SoC Series is part of the Atmosic family of extremely low-power Bluetooth® system-on-chip (SoC) solutions. ATM33/e integrates a Bluetooth 5.3 compliant radio, ARM® Cortex® M33F application processor with ARM® TrustZone® enabled security features, and state-of-the-art power management to enable maximum lifetime in battery-operated devices.
For detailed product specifications and features, please refer to https://atmosic.com/products_atm33/

SoCs and EVKs
*****************

.. _board:

==================  =================  =================  ==================  ========  ==========
SoC Part #          EVK Part #         Board List         On-chip             Package   Energy 
                                       <BOARD>            Flash                         Harvesting
==================  =================  =================  ==================  ========  ==========
ATM3330e-5DCAQN     ATMEVK-3330e-QN-7  ATMEVK-3330e-QN-7  512KB               QFN 7x7   x
ATM3330e-5DCAQN     ATMEVK-3330e-QN-6  ATMEVK-3330e-QN-5  512KB               QFN 7x7   x
ATM3330-5DCAQN      ATMEVK-3330-QN-6   ATMEVK-3330-QN-5   512KB               QFN 7x7
ATM3325-5DCAQK      ATMEVK-3325-QK-6   ATMEVK-3325-QK-5   512KB               QFN 5x5
ATM3325-5LCAQK      ATMEVK-3325-LQK-6  ATMEVK-3325-LQK    512KB + 1MB         QFN 5x5
ATM3325-5DCACM      ATMEVK-3325-CM-6   Not Supported      512KB               WLCSP
==================  =================  =================  ==================  ========  ==========

Getting Started
***************

Follow the instructions_ from the official Zephyr documentation on how to get started.

Connecting an ATMEVK on Linux
=============================

Special udev and group permissions are required by OpenOCD, which is the primary
debugger used to interface with Atmosic EVKs, to access the USB FTDI
SWD interface or J-Link OB.  When following Step 4 "Install udev rules, which
allow you ..." for Ubuntu_, add the following line to
`60-openocd.rules`::

 ATTRS{idVendor}=="1366", ATTRS{idProduct}=="1050", MODE="660", GROUP="plugdev", TAG+="uaccess"

.. _Ubuntu: https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#install-the-zephyr-sdk

.. _instructions: https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html

Connecting an ATMEVK on Windows
===============================

The J-Link OB device driver must be replaced with the WinUSB driver to
become available as a USB device and usable by OpenOCD.
This can be done using Zadig.

Windows Administrator privileges are required to replace the driver.

Zadig can be obtained from:

https://github.com/pbatard/libwdi/releases

At the time of this writing, the latest version -- 2.4 -- can be
obtained using the following direct link.

https://github.com/pbatard/libwdi/releases/download/b721/zadig-2.4.exe

To replace the driver:

#. From the "Options" menu of Zadig, click "List all devices".
#. From the drop-down menu, find "BULK interface" corresponding to
   the Atmosic board.  It should show "jlink (v...)" as
   the current driver on the left.
#. Select "WinUSB (v...)" as the replacement on the right.
#. Click "Replace Driver"

Verify the successful installation of WinUSB by going to the Windows
Device Manager and confirm that the "BULK interface" shows
as such rather than the "J-Link driver".  (In Device Manager, expand the category
"Universal Serial Bus devices" and look for "BULK interface".)

Programming and Debugging
*************************

It is recommended to set the environment variables ZEPHYR_TOOLCHAIN_VARIANT to ``zephyr`` and ZEPHYR_SDK_INSTALL_DIR to the directory where Zephyr SDK is installed. For example, assuming the installed SDK version 0.16.8 in the home directory, for reference, it will be like this in a bash shell environment: (use setenv in a C shell environment)::

 export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
 export ZEPHYR_SDK_INSTALL_DIR=<$HOME/zephyr-sdk-0.16.8>

Applications for the Atmosic EVK boards can be built, flashed, and debugged with the familiar `west build` and `west flash`.  Except for the case of debugging, a convenience wrapper script is provided that can invoke all the right `west` commands, as detailed in the `Support Script`_ section. 

The atm33evk boards require at least two images to be built -- the SPE and the application.  SPE is the Secure Processing Environment, and the application typically resides in the non-secure (NSPE) portion.

The Atmosic SPE can be found under ``<WEST_TOPDIR>/openair/samples/spe``.

.. _var_assignments:

In the remainder of this document, substitute for ``<WEST_TOPDIR>``, ``<SPE>``, ``<APP>``, ``<MCUBOOT>``, ``<BOARD>``, ``<FLAV>``, and ``<DEVICE_ID>`` appropriately.  For example::

 <ZEPHYR_TOOLCHAIN_VARIANT>: zephyr
 <ZEPHYR_SDK_INSTALL_DIR>: /absolute/path/to/zephyrSDK
 <WEST_TOPDIR>: /absolute/path/to/zephyrproject
 <SPE>: openair/samples/spe
 <APP>: zephyr/samples/hello_world
 <MCUBOOT>: bootloader/mcuboot/boot/zephyr
 <BOARD>: ATMEVK-3330-QN-5
 <FLAV>: PD50LL
 <DEVICE_ID>: 000900028906

Alternatively, use any board from the board_ list as ``<BOARD>``.

Building and Flashing 
=====================

Applications can be built with MCUboot or without the MCUboot option. If a device firmware update (DFU) is not needed, you can choose the option without MCUboot. If you require DFU, then the MCUboot option is required.

There are basically two options:

A. Non-MCUboot Option
---------------------

1. Build the SPE:

::

  west build -p -s <SPE> -b <BOARD> -d build/<BOARD>/<SPE>

2. Build the Application:

Note: ``<BOARD>//ns`` is the non-secure variant of ``<BOARD>``.

Build the app with the non-secure board variant and the SPE configured as follows::

  west build -p -s <APP> -b <BOARD>//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\"

Passing the path to the SPE is for linking in the non-secure-callable veneer file generated in building the SPE.

With this approach, each built image has to be flashed separately.  Optionally, build a single merged image by enabling ``CONFIG_MERGE_SPE_NSPE``, thereby minimizing the flashing steps::

  west build -p -s <APP> -b <BOARD>//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\" -DCONFIG_MERGE_SPE_NSPE=y

3. Flashing the SPE and the Application:

``west flash`` is used to program a device with the necessary images, often only built as described above and sometimes also with a pre-built library provided as an ELF binary.

In this section, substitute ``<DEVICE_ID>`` with the serial number for the Atmosic interface board used.  For an atmevk33 board, this is typically a J-Link serial number, but it can also be an FTDI serial number of the form ``ATRDIXXXX``.  For a J-Link board, pass the ``--jlink`` option to the flash runner as in ``west flash --jlink ...``.

If the application requires Bluetooth (configured with ``CONFIG_BT`` in the prj.conf file) and uses the fixed BLE link controller image option, then the controller image requires programming.  This is typically done before programming the application and resetting (omitting the ``--noreset`` option to ``west flash``).  For example::

  west flash --verify --device=<DEVICE_ID> --jlink --fast_load --skip-rebuild -d build/<BOARD>/<MCUBOOT> --use-elf --elf-file openair/modules/hal_atmosic/ATM33xx-5/drivers/ble/atmwstk_<FLAV>.elf --noreset

where ``<FLAV>`` is one of ``LL`` or ``PD50LL``.

Atmosic provides a mechanism to increase the legacy programming time called FAST LOAD. Apply the option ``--fast_load`` to enable the FAST LOAD.

Flash the SPE and the application separately if ``CONFIG_MERGE_SPE_NSPE`` was not enabled::

  west flash --device=<DEVICE_ID> --jlink --fast_load --verify -d build/<BOARD>/<SPE> --noreset
  west flash --device=<DEVICE_ID> --jlink --fast_load --verify -d build/<BOARD>_ns/<APP>

Alternatively, if ``CONFIG_MERGE_SPE_NSPE`` was enabled in building the application, the first step (programming the SPE) can be skipped.


B. MCUboot Option
-----------------

1. Build the MCUboot and the SPE:

To build with MCUboot, for example, DFU is needed, first build MCUboot::

  west build -p -s <MCUBOOT> -b <BOARD>@mcuboot -d build/<BOARD>/<MCUBOOT> -- -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_DEBUG=n -DCONFIG_BOOT_MAX_IMG_SECTORS=512 -DDTC_OVERLAY_FILE="<WEST_TOPDIR>/zephyr/boards/atmosic/atm33evk/<BOARD>_mcuboot_bl.overlay"

and then the Atmosic SPE::

  west build -p -s <SPE> -b <BOARD>@mcuboot -d build/<BOARD>/<SPE> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n -DDTS_EXTRA_CPPFLAGS=";"

Note that make use of "board revision" to configure our board partitions to work for MCUboot.  On top of the "revisions," MCUboot currently needs an additional overlay that must be provided through the command line to give it the entire SRAM.

2. Build the Application with MCUboot and SPE:

Build the application with MCUboot and SPE as follows::

  west build -p -s <APP> -b <BOARD>@mcuboot//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DDTS_EXTRA_CPPFLAGS=";" -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\"

This is somewhat of a non-standard workflow.  When passing ``-DCONFIG_BOOTLOADER_MCUBOOT=y`` on the application build command line, ``west`` automatically creates a signed, merged image (``zephyr.signed.{bin,hex}``), which is ultimately used by ``west flash`` to program the device.  The original application binaries are renamed with a ``.nspe`` suffixed to the file basename (``zephyr.{bin,hex,elf}`` renamed to ``zephyr.nspe.{bin,hex,elf}``) and are the ones that should be supplied to a debugger.

3. Flashing the MCUboot, SPE, and the Application:

``west flash`` is used to program a device with the necessary images, often only built as described above and sometimes also with a pre-built library provided as an ELF binary.

In this section, substitute ``<DEVICE_ID>`` with the serial number for the Atmosic interface board used.  For an atmevk33 board, this is typically a J-Link serial number, but it can also be an FTDI serial number of the form ``ATRDIXXXX``.  For a J-Link board, pass the ``--jlink`` option to the flash runner as in ``west flash --jlink ...``.

If the application requires Bluetooth (configured with ``CONFIG_BT`` in the prj.conf file), and uses the fixed BLE link controller image option, then the controller image requires programming.  This is typically done before programming the application and resetting (omitting the ``--noreset`` option to ``west flash``).  For example::

  west flash --verify --device=<DEVICE_ID> --jlink --fast_load --skip-rebuild -d build/<BOARD>/<MCUBOOT> --use-elf --elf-file openair/modules/hal_atmosic/ATM33xx-5/drivers/ble/atmwstk_<FLAV>.elf --noreset

where ``<FLAV>`` is one of ``LL`` or ``PD50LL``.

.. _flashing:

Flash MCUboot::

Atmosic provides a mechanism to increase the legacy programming time called FAST LOAD. Apply the option ``--fast_load`` to enable the FAST LOAD.::

   west flash --verify --device=<DEVICE_ID> --jlink --fast_load -d build/<BOARD>/bootloader/mcuboot/boot/zephyr --erase_flash --noreset

Note that ``--erase_flash`` there is an option to erase Flash if needed.

Flash the signed application image (merged with SPE)::

   west flash --verify --device=<DEVICE_ID> --jlink --fast_load -d build/<BOARD>_ns/<APP>

BLE Link Controller Options
---------------------------
When building a Bluetooth application (``CONFIG_BT``), the BLE driver component provides two link controller options. A fixed BLE link controller image and a statically linked BLE link controller library.  The BLE link controller sits at the lowest layer of the Zephyr Bluetooth protocol stack.  Zephyr provides the upper Bluetooth Host stack that can interface with BLE link controllers that conform to the standard Bluetooth Host Controller Interface specification.

To review how the fixed and statically linked controllers are used, please refer to the README.rst in openair/modules/hal_atmosic/ATM33xx-5/drivers/ble/.

If the ATM33 entropy driver is enabled without CONFIG_BT=y (mainly for evaluation), the system still requires a minimal BLE controller stack.  Without choosing a specific stack configuration an appropriate minimal BLE controller will be selected.  This may increase the size of your application.

Note that developers cannot use ``CONFIG_BT_CTLR_*`` `flags`__ with the ATM33 platform, as a custom, hardware-optimized link controller is used instead of Zephyr's link controller software.

.. _CONFIG_BT_CTLR_KCONFIGS: https://docs.zephyrproject.org/latest/kconfig.html#!%5ECONFIG_BT_CTLR
__ CONFIG_BT_CTLR_KCONFIGS_

Support Script
==============

A convenient support script is provided in the Zephyr repository and can be used as follows.  From the ``west topdir`` directory where Zephyr was cloned and ``west`` was initialized, run the following:

Without MCUBoot::

  zephyr/boards/atmosic/atm33evk/support/run.sh -n -e -d [-w <flavor>] [-l <flavor>] -a <application path> -j -s <DEVICE_ID> <BOARD>

With MCUBoot::

  zephyr/boards/atmosic/atm33evk/support/run.sh -e -d [-w <flavor>] [-l <flavor>] -a <application path> -j -s <DEVICE_ID> <BOARD>

* replace ``<DEVICE_ID>`` with the appropriate device ID (typically the JLINK serial ID. Ex: ``000900028906``)
* replace ``<BOARD>`` with the targeted board design (Ex: ATMEVK-3325-LQK )
* replace ``<application path>`` with the path to your application (Ex: ``zephyr/samples/bluetooth/peripheral_hr``)
* see below for selecting ``-w``/``-l`` options.

Using -w [flavor] and -l [flavor] Options
-----------------------------------------

See openair/modules/hal_atmosic/ATM33xx-5/drivers/ble/README.rst for an explanation of different BLE controller options. The ``-w`` option selects the use of the fixed BLE controller stack image.  The flavor parameter can be ``LL`` or ``PD50LL``. The ``-l`` option is selected for the statically linked BLE controller library.  The flavor can be PD50.  The ``-l`` flag is mutually exclusive with the ``-w`` option.  When using the ``-l`` option the build will recover memory resources reserved for the fixed image BLE controller and provide them to the NSPE image.  The ``-w`` option should not be used to flash the ATMWSTK when the application has already been built and installed using the ``-l`` option.  Flashing the fixed BLE controller on top of an existing install that uses the static library may corrupt the installed image.

Using the Support Script on Windows
-----------------------------------

This script is written in Bash.  While Bash is readily available on most Linux distributions and macOS, it is not so on Windows.  However, Bash is bundled with Git.  The following single command demonstrates how to build, flash, and run the ``hello_world`` application using Bash in a typical installation of Git executed from the root of the Zephyr workspace::

  C:\zephyrproject>"C:\Program Files\Git\bin\bash.exe" zephyr\boards\atmosic\atm33evk\support\run.sh -e -d -a zephyr\samples\hello_world -j -s <DEVICE_ID> <BOARD>

As an alternative, pass ``-n`` to build without MCUboot.

From this point on out, unless the bootloader has been modified, the source code for the application (in this case ``zephyr\samples\hello_world``) can be modified and then programmed with ``-d`` and ``-e`` omitted::

  C:\zephyrproject>"C:\Program Files\Git\bin\bash.exe" zephyr\boards\atmosic\atm33evk\support\run.sh -a zephyr\samples\hello_world -j -s <DEVICE_ID> <BOARD>

When -d and -a will build not just the app but the dependencies and generat the .atm file as well.

Atmosic In-System Programming (ISP) Tool
========================================

This SDK ships with a tool called Atmosic In-System Programming Tool (ISP) for bundling all three types of binaries -- OTP NVDS, flash NVDS, and flash -- into a single binary archive.

+---------------+-----------------------------------------------------+
|  Binary Type  |  Description                                        |
+===============+=====================================================+
|   .bin        |  binary file contains flash or nvds data only      |
+---------------+-----------------------------------------------------+

The ISP tool, which is also shipped as a stand-alone package, can then be used
to unpack the components of the archive and download them on a device.

Python Requirements
-------------------

Support atm isp archive tool has to install specific python protobuf version 3.20.3 first.

To install with Openair requirement list file::

  pip install -r openair/scripts/requirements.txt

Or install with pip command directly::

  pip install grpcio-tools==1.47.0 protobuf==3.20.3

Note: This install operation will uninstall current python protobuf packages and reinstall python protobuf to version 3.20.3.

West atm_arch commands
----------------------

+-----------------------------------------------------------------------------+------------------------------------------------------------+
| Command Arguments                                                           |  Description                                               |
+=============================================================================+============================================================+
| -atm_isp_path ATM_ISP_PATH, --atm_isp_path ATM_ISP_PATH                     |  specify atm_isp exe path            |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -d, --debug                                                                 |  debug enabled, default false                              |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -s, --show                                                                  |  show archive                                              |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -b, --burn                                                                  |  burn archive                                              |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -a, --append                                                                |  append to input atm file                                  |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -a, --append                                                                |  append to input atm file                                  |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -i INPUT_ATM_FILE, --input_atm_file INPUT_ATM_FILE                          |  input atm file path                                       |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -o OUTPUT_ATM_FILE, --output_atm_file OUTPUT_ATM_FILE                       |  output atm file path                                      |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -p PARTITION_INFO_FILE, --partition_info_file PARTITION_INFO_FILE           |  partition info file path                                  |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -storage_data_file STOAGE_DATA_FILE, --storage_data_file STOAGE_DATA_FILE   |  storage data file path                                    |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -factory_data_file FACTORY_DATA_FILE, --factory_data_file FACTORY_DATA_FILE |  factory data file path                                    |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -spe_file SPE_FILE, --spe_file SPE_FILE                                     |  spe file path                                             |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -app_file APP_FILE, --app_file APP_FILE                                     |  application file path                                     |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -mcuboot_file MCUBOOT_FILE, --mcuboot_file MCUBOOT_FILE                     |  mcuboot file path                                         |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -atmwstk_file ATMWSTK_FILE, --atmwstk_file ATMWSTK_FILE                     |  atmwstk file path (.elf or .bin)                          |
+-----------------------------------------------------------------------------+------------------------------------------------------------+
| -openocd_pkg_root OPENOCD_PKG_ROOT, --openocd_pkg_root OPENOCD_PKG_ROOT     |  Path to directory where openocd and its scripts are found |
+-----------------------------------------------------------------------------+------------------------------------------------------------+


When ``-DCONFIG_SPE_PATH`` has been specified, the parition_info information will merge from the build_dir of the application and spe to the build_dir of the application and named as parition_info.map.merge.

* When not using SPE, the ``-p`` option should be <build_dir>/zephyr/parition_info.map
* When using SPE, the ``-p`` option should be <build_dir>/zephyr/parition_info.map.merge

When building with wireless stack, the ``-DCONFIG_USE_ATMWSTK=y -DCONFIG_ATMWSTK=\"<ATMWSTK>\"``, the wireless stack elf file should be packed within the isp file as well::

    --atmwstk_file <ATMSWTK_PATH>/atmwstk_<ATMWSTK>.elf

or::

    --atmwstk_file <ATMSWTK_PATH>/atmwstk_<ATMWSTK>.bin

* replace ``<ATMSWTK_PATH>`` the wireless stack file, it should be openair/modules/hal_atmosic/ATM33xx-5/drivers/ble default.
* replace ``<ATMWSTK>`` the wireless stack, PD50LL or LL.

The wireless stack ``elf`` file will be transferred to binary automatically and this requires the environment variables ZEPHYR_TOOLCHAIN_VARIANT to be set. Or to transfer to binary manually by::

    <ZEPHYR_TOOLCHAIN_VARIANT>/arm-zephyr-eabi/bin/arm-zephyr-eabi-objcopy -O binary <ATMSWTK_PATH>/atmwstk_<ATMWSTK>.elf <ATMSWTK_PATH>/atmwstk_<ATMWSTK>.bin

* replace ``<ZEPHYR_TOOLCHAIN_VARIANT>`` the Zephyr toolchain path.

Generate .atm isp file
---------------------

* replace ``<APP_NAME>`` with the application name.

Without SPE::

  west atm_arch -o <BOARD>_<APP_NAME>.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.signed.bin \
    --mcuboot_file build/<BOARD>/<MCUBOOT>/zephyr/zephyr.bin \
    --atmwstk_file openair/modules/hal_atmosic/ATM33xx-5/drivers/ble/atmwstk_PD50LL.elf \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

With SPE::

  west atm_arch -o <BOARD>_<APP_NAME>.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map.merge \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.signed.bin \
    --mcuboot_file build/<BOARD>/<MCUBOOT>/zephyr/zephyr.bin \
    --atmwstk_file openair/modules/hal_atmosic/ATM33xx-5/drivers/ble/atmwstk_PD50LL.elf \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

Without ATMWSTK::

  west atm_arch -o <BOARD>_<APP_NAME>.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map.merge \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.signed.bin \
    --mcuboot_file build/<BOARD>/<MCUBOOT>/zephyr/zephyr.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

Without MCUboot::

  west atm_arch -o <BOARD>_<APP_NAME>.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map.merge \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.bin \
    --spe_file build/<BOARD>/<SPE>/zephyr/zephyr.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

Show and Flash atm isp file
---------------------------

show command::

  west atm_arch -i <BOARD>_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp \
    --show

flash command::

  west atm_arch -i <BOARD>_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp \
    --openocd_pkg_root openair/modules/hal_atmosic \
    --burn

Programming Secure Journal
==========================

The secure journal is a dedicated block of RRAM that has the property of being a write-once, append-only data storage area that replaces traditional OTP memory. This region is located near the end of the RRAM storage array at 0x8F800– 0x8FEEF (1776 bytes).

The secure journal data updates are controlled by a secure counter (address ratchet). The counter determines the next writable location at an offset from the start of the journal. An offset greater than the counter value is writable while any offset below or equal to the counter is locked from updates. The counter can only increment monotonically and cannot be rolled back. This provides the immutability of OTP as well as the flexibility to append new data items or override past items using a find the latest TLV search.

The west extension command `secjrnl` is provided by the Atmosic HAL to allow for easy access and management of the secure journal on supported platforms.

The tool provides a help command that describes all available operations through::

 west secjrnl --help

Dumping Secure Journal
----------------------

To dump the secure journal, run the command::

 west secjrnl dump --device <DEVICE_ID>

This will dump all the TLV tags located in the secure journal.

Appending a tag to the Secure Journal
-------------------------------------

To append a new tag to the secure journal::

 west secjrnl append --device <DEVICE_ID> --tag=<TAG_ID> --data=<TAG_DATA>

* replace ``<TAG_ID>`` with the appropriate tag ID (Ex: ``0xde``)
* replace ``<TAG_DATA>`` with the data for the tag. This is passed as a string. To pass raw byte values format it like so: '\xde\xad\xbe\xef'. As such, ``--data="data"`` will result in the same output as ``--data="\x64\x61\x74\x61``.

The secure journal uses a find latest search algorithm to allow overrides. If the passed tag should NOT be overridden in the future, add the flag ``--locked`` to the append command. See the following section for more information regarding locking a tag.


NOTE: The ``append`` command does NOT increment the ratchet. The newly appended tag is still unprotected from erasing.

Locking down a tag
------------------

The secure journal provides a secure method of storing data while still providing options to update the data if needed. However, there are key data entries that should never be updated across the life of the device (e.g. UUID).
This support is provided by software and can be enabled for a tag by passing the ``--locked`` to the command when appending a new tag.

It is important to understand, that once a tag is **locked** (and ratcheted), the specific tag can never be updated in the future - Appending a new tag of the same value will be ignored.


Erasing non-ratcheted data from the Secure Journal
--------------------------------------------------

Appended tags are not ratcheted down. This allows for prototyping with the secure journal before needing to lock down the TLVs. To support prototyping, you can erase non-ratcheted data easily through::

 west secjrnl erase --device <DEVICE_ID>



Ratcheting Secure Journal
-------------------------

To ratchet data, run the command::

 west secjrnl ratchet_jrnl --device <DEVICE_ID>

This will list the non-ratcheted tags and confirm that you want to ratchet the tags. Confirm by typing 'yes'.

NOTE: This process is non-reversible. Once ratcheted, that region of the secure journal cannot be modified.

Viewing the Console Output
**************************

Linux and macOS
===============

For a Linux host, monitor the console output with a favorite terminal
program, such as::

  screen /dev/ttyACM1 115200

On macOS, the serial console will be on a USB port (``/dev/tty.usbmodem<12-digit device ID>[13]``).  Use the following command to find the port for the serial console::

  $ ls /dev/tty.usbmodem*
  /dev/tty.usbmodem<DEVICE_ID>1
  /dev/tty.usbmodem<DEVICE_ID>3
  $


Windows
=======

Console output for the current Atmosic ATM3330 goes to the JLink CDC UART
serial port.  That is Interface 2 of J-Link OB USB on the Atmosic
board.  To view the console output, use a serial terminal
program such as PuTTY (available from
https://www.chiark.greenend.org.uk/~sgtatham/putty) to connect to
JLink CDC UART port generated by interface 2 of J-Link OB USB
with the baud rate set to 115200.

If using PuTTY, open a session with the following three parameters:

#. Serial line: <COM port> (see next paragraph)
#. Speed: 115200
#. Connection type: Serial

A common way to determine <COM port> for parameter #1 above is to use
the Windows Device Manager as follows.

#. Under the "View" menu, choose "Devices by container"
#. Under the container "J-Link", find "JLink CDC UART Port (COM<N>)", where <N> is some COM port sequence number

Then use "COM<N>" for the serial line parameter in PuTTY.


Zephyr DFU
==========

The steps for building and flashing will mostly remain the same as documented in the above sections.
Any differences will be noted here.

For this section, use the following updated variable assignments/substitutions along with the ones provided `above`__::

  APP=zephyr/samples/subsys/mgmt/mcumgr/smp_svr

__ var_assignments_

In Zephyr, DFU is possible using the ``mcumgr`` subsystem. This makes use of some of the features from MCUBoot to facilitate image uploading and swapping.
To test this subsystem, Zephyr provides an SMP server sample that makes use of the subsystem to test performing Serial DFU and BLE OTA firmware updates.
To perform the DFU, the ``mcumgr`` program can be used. Currently, this supports UART on all platforms and BLE on macOS and Linux (only Linux is tested currently for BLE).
More information about the smp_svr sample and how to use the mcumgr utility can be found `here. <https://docs.zephyrproject.org/latest/samples/subsys/mgmt/mcumgr/smp_svr/README.html>`_

A new overlay file has been provided named ``overlay-disable-stats.conf`` that saves around 3 kB by disabling ``taskstat`` and the stats subsystems if those features are not needed.

To flash smp_svr follow the MCUBoot instructions from flashing_.
When using BLE remember that the wireless stack must also be flashed.

.. _serial_dfu:

Building for Serial (UART)
--------------------------

On Atmosic EVKs, only UART0 can be used to perform DFU, as UART1 RX is not connected by default.
However, UART1 should be usable on a custom board design if it is connected.
Special care will need to be made for BENIGN_BOOT if the default pins are used.

By default, the UART0 peripheral is not enabled, which will cause a build error.
To enable UART0, please modify the board's DTS file and add ``status = "okay";`` to the ``&uart0`` block.

When building smp_svr to support DFU over serial, the only change from a standard MCUBoot build is to make sure that the proper overlay configurations are applied ``-DOVERLAY_CONFIG="overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf"``::

  west build -p -s <APP> -b <BOARD>@mcuboot//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\" -DDTS_EXTRA_CPPFLAGS=";" -DOVERLAY_CONFIG="overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf"

Building for BLE
----------------

If building smp_svr using RRAM only, then the ``PD50LL`` wireless stack **must** be used. This can be done by using the following variable assignments/substitutions::

  ATMWSTK=PD50LL

If building smp_svr using an external flash, either the ``PD50LL`` or the ``LL`` wireless stack can be used. When using the ``LL`` wireless stack, the following variable assignments/substitutions should be used::

  ATMWSTK=LL

When building smp_svr to support DFU over BLE, all images (MCUBoot, SPE, smp_svr) need to be built with ``-DDTS_EXTRA_CPPFLAGS="-DATMWSTK=<ATMWSTK>;"`` (when using an external flash, the ``-DDFU_IN_FLASH;`` option must also be present).
smp_svr additionally needs to be configured to use the ATMWSTK by using ``-DCONFIG_USE_ATMWSTK=y -DCONFIG_ATMWSTK=\"<ATMWSTK>\" -DCONFIG_ATM_SLEEP_ADJ=17`` and use the proper overlay configuration files ``-DEXTRA_CONF_FILE="overlay-bt.conf"`` (If Serial DFU support is also desired, then the overlay files from the serial_dfu_ section)::

  west build -p -s <MCUBOOT> -b <BOARD>@mcuboot -d build/<BOARD>/<MCUBOOT> -- -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_BOOT_MAX_IMG_SECTORS=512 -DDTC_OVERLAY_FILE="<WEST_TOPDIR>/zephyr/boards/atmosic/atm33evk/<BOARD>_mcuboot_bl.overlay" -DDTS_EXTRA_CPPFLAGS="-DATMWSTK=<ATMWSTK>;"
  west build -p -s <SPE> -b <BOARD>@mcuboot -d build/<BOARD>/<SPE> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n -DDTS_EXTRA_CPPFLAGS="-DATMWSTK=<ATMWSTK>;"
  west build -p -s <APP> -b <BOARD>@mcuboot//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\" -DDTS_EXTRA_CPPFLAGS="-DATMWSTK=<ATMWSTK>;" -DCONFIG_USE_ATMWSTK=y -DCONFIG_ATMWSTK=\"<ATMWSTK>\" -DEXTRA_CONF_FILE="overlay-bt.conf" -DCONFIG_ATM_SLEEP_ADJ=17

Building with Secure Debug
--------------------------

Secure Debug is a collection of hardware and software features to limit access to the debug port for devices in production. It is not intended to be used in development because once security measures are enabled many steps in the normal development flow will no longer function.

Managing the OTP bits
^^^^^^^^^^^^^^^^^^^^^

At a hardware level, the debug security state at power-on is defined by two OTP bits (ATM_OTP_MASK_SEC_DBG_DEBUG_SECURED and ATM_OTP_MASK_SEC_DBG_DEBUG_DISABLED).
Hardware applies the debug security state prior to the CPU booting.  No intervention is required by software to enforce the security state.  When secure debug is either SECURED or DISABLED, access through SWD is disallowed even if benign boot is enabled. When the port is SECURED (rather than DISABLED), the state can be cleared by software after a software challenge to prove the identity of the debug access requester.  The authenticator is implemented in the MCUboot image that monitors a UART console port.

To check the state of the OTP bits, users can use the atmotp west extension by issuing the following command::

 west atmotp get --board <BOARD> --device <DEVICE_ID> --otp SEC_DBG_CONFIG.DEBUG_DISABLED

or::

 west atmotp get --board <BOARD> --device <DEVICE_ID> --otp SEC_DBG_CONFIG.DEBUG_SECURED

To completely disable secure debug, users can issue the following command (this is irreversible)::

 west atmotp burn --board <BOARD> --device <DEVICE_ID> --otp SEC_DBG_CONFIG.DEBUG_DISABLED

To enable secure debug, users can issue the following command::

 west atmotp get --board <BOARD> --device <DEVICE_ID> --otp SEC_DBG_CONFIG.DEBUG_SECURED

The authenticator software component runs during the boot sequence of MCUboot. Secure debug is not accessible in Non-MCUboot builds. If no authentication occurs, the software will sticky lock the debug port until reset.  A Python script is provided to demonstrate communications with the MCUboot authenticator to unlock the debug port.  The challenge/authentication process must be performed on each boot.  The challenge consists of a unique hash of per-device data stored in the secure journal.  This is computed by the MCUboot image and provided as a base64 encoded text output on the UART console port.  The hash will be unique for each manufactured device.  The challenge must be signed with the private ECDSA key and the resulting signature provided back to the authenticator to verify it using its local public ECDSA key.   The signature is unique for the device and can be used for every challenge response.

Compiling MCUboot with secure debug
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To build with secure debug, add the following additional flags::

  -DCONFIG_ATM_MCUBOOT_SECURE_DEBUG=y -DDTS_EXTRA_CPPFLAGS="-DUSE_ATM_SECURE_DEBUG"

NOTE: if building with DFU_IN_FLASH, then your flags will look like this::

  -DCONFIG_ATM_MCUBOOT_SECURE_DEBUG=y -DDTS_EXTRA_CPPFLAGS="-DDFU_IN_FLASH;-DUSE_ATM_SECURE_DEBUG"

The DTS option ``-DUSE_ATM_SECURE_DEBUG`` will enable UART0 as a bi-directional console port for authentication use.

The MCUboot extension for secure debug will use a default private ECC-P256 key to generate the public ECC-P256 key stored in the image.  This is a widely distributed key and should not be used in production.

At this time the authenticator implements a 500ms default timeout through ``CONFIG_ATM_MCUBOOT_UART_DEBUG_AUTH_TIMEOUT_MS`` while monitoring the console port for characters.  You can adjust as needed ``-DCONFIG_ATM_MCUBOOT_UART_DEBUG_AUTH_TIMEOUT_MS=<milliseconds>`` to extend the timeout. A future update will support monitoring the UART RX pin for a logic high state to detect the presence of a host UART connection.

Using the debug unlock script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A debug unlock Python script is provided in ``openair/tools/scripts/sec_debug_unlock.py``. This tool requires PySerial. ::
  
  python sec_debug_unlock.py -v -k <private ECC-P256 key in .pem format> -p <console port>

To unlock using the default private key in ``openair/lib/atm_debug_auth/`` ::

  python sec_debug_unlock.py -v -k openair/lib/atm_debug_auth/root-debug-ec-p256.pem -p <console port>

The unlocking script using the ``-v`` option will verbosely output::

Sending: b'DBG REQUEST\n'
Received: b'Static Challenge: o9H3wvgqOfAi/mvTV/qvvdNjBqzGILIai3G4OBURjhE=\n'
Unlock Static Challenge
Sending: b'DBG STATIC_RESPONSE sMdx+QFewpAt3Dnqy9BrjSLNxgtObtu3IKhSvpuvbG7J9IClpt/zJL4XRlo9rt7KCCw6orjUIyBdaWWM657aRw==\n'
Received: b'Debug unlocked\n'

The SWD port will be unlocked and MCUboot will remain in a benign state with the processor halted at a WFI instruction (Wait For Interrupt).  The developer can freely attach a debugger such as GDB and inspect the target (read memory, set breakpoints).  If the debugger allows the CPU to continue then MCUboot will continue its boot from the point at which WFI was entered.

