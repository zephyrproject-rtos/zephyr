.. _atm34evk:

Atmosic ATM34xx
###############

Overview
********
The ATM34 and ATM34e are members of the new ATM34/e series that supports both Bluetooth LE and/or 802.15.4 for highly robust command/control or home automation type protocols.
The SoC family integrates the radio along with an ARM® Cortex® M33F application processor, up to 256 KB Random Access Memory (RAM), 64 KB Read-Only Memory (ROM), 512-1536 KB of nonvolatile memory (NVM). The family also supports the ARM® TrustZone® enabled security features, and state-of-the-art power management to enable maximum lifetime in battery-operated devices and integrated energy harvesting capabilities.


The extremely low power ATM34 and ATM34e series SoCs, with a 0.95 mA radio receiver and a 2.1 mA radio transmitter power consumption, is designed to extend battery life for the Internet-of-Things (IoT) markets. Support for low duty cycle operation allows systems to run for significantly longer periods without battery replacement. In addition, this series of SoCs from Atmosic supports operation from energy harvesting sources, including RF, photovoltaic, TEG (Thermoelectric generator), and motion.  Innovative wake-up mechanisms are supported to provide options for further power consumption reduction.


Hardware Features
*****************
Bluetooth LE
  - Bluetooth Low-Energy 5.4 compliant
  - 2 Mbps, 1 Mbps 500 kbps, and 125 kbps Coded PHY rates
  - 802.15.4 Support for Thread, Matter and Zigbee
MCU and Memory
  - 64 MHz ARM® Cortex® M33F MCU with DSP
  - 64 KB ROM, 256 KB RAM, 512-1536 KB NVM
  - Retention RAM configuration: 16 KB to 256 KB in 16 KB step sizes
  - 16 MHz / Optional 32.768 kHz Crystal Oscillator
Security
  - ARM® TrustZone®,  HW Root of Trust, Secure Boot, Secure Execution & Debug
  - AES-128/256, SHA-2/HMAC 256 Encryption/Cryptographic Hardware Accelerators
  - True random number generator (TRNG)
Energy Harvesting (ATM34e)
  - On-chip RF Energy Harvesting
  - Supports photovoltaic, thermal, motion and other energy harvesting technologies
  - External Harvesting/Storage Interface
RF and Power Management
  - Fully integrated RF front-end
  - Sensor Hub
  - RF Wakeup Receiver
  - 1.1 V to 4.2 V battery input voltage with integrated Power Management Unit (PMU)
  - Radio power consumption with 3 V battery
    - Rx @ -97 dBm: 0.95 mA
    - Tx @ 0 dBm: 2.5 mA
  - SoC typical power consumption with 3 V battery including PMU
    - Retention @ 32 KB RAM: 1.9 µA
    - Hibernate: 1.3 µA
    - SoC Off: 500 nA
    - SoC Off with Harvesting Enabled: 800 nA
Interfaces
  - I2C (2), I2S, SPI (2), UART (2), PWM (8), GPIOs (21 or 31 depending on the package option)
  - Quad SPI
  - 16-bit application ADC, 4 external, 5 internal channels, up to 2 Msps
  - Two mono or one stereo Digital microphone inputs (PDM)
  - 8 x 20 Keyboard matrix controller (KSM)
  - Quadrature decoder (QDEC)
  - SWD for Interactive Debugging
Package Options
  - ATM3430e: 7x7 mm, 56-pin QFN (up to 31 GPIOs)
  - ATM3430: 7x7 mm, 56-pin QFN (up to 31 GPIOs)
  - ATM3425: 5x5 mm, 40-pin QFN (up to 21 GPIOs)
  - ATM3405: 5x5 mm, 40-pin QFN (up to 21 GPIOs) (BLE only; No 15.4)


.. _boards:

Boards
======
* ATMEVK-3405-PQK-2
* ATMEVK-3425-PQK-2
* ATMEVK-3430e-WQN-2


Getting Started
***************

Follow the instructions_ from the official Zephyr documentation on how to get started.



Connecting an ATMEVK on Linux
=============================

Special udev and group permissions are required by OpenOCD, which is the primary
debugger used to interface with Atmosic EVKs, in order to access the USB FTDI
SWD interface or J-Link OB.  When following Step 4 "Install udev rules, which
allow you ..." for Ubuntu_, add the following line to
`60-openocd.rules`::

 ATTRS{idVendor}=="1366", ATTRS{idProduct}=="1050", MODE="660", GROUP="plugdev", TAG+="uaccess"

.. _Ubuntu: https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#install-the-zephyr-sdk

.. _instructions: https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html

Connecting an ATMEVK on Windows
===============================

The J-Link OB device driver must be replaced with WinUSB in order for it to
become available as a USB device and usable by OpenOCD.
This can be done using Zadig.

Windows Administrator privileges are required for replacing a driver.

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
Device Manager and confirming that the "BULK interface" shows
as such rather than "J-Link driver".  (In Device Manager, expand category
"Universal Serial Bus devices" and look for "BULK interface".)


Programming and Debugging
*************************

Applications for the ATMEVK-34xx-xxx-2 and ATMEVK-3430e-xxx-2 boards can be built, flashed, and debugged with the familiar `west build` and `west flash`.

The atm34evk boards require at least two images to be built -- the SPE and the application.

The Atmosic SPE can be found under ``<WEST_TOPDIR>/openair/samples/spe``.

.. _var_assignments:

In the remainder of this document, substitute for ``<SPE>``, ``<APP>``, ``<MCUBOOT>``, and ``<BOARD>`` appropriately.  For example::

 <WEST_TOPDIR>: /absolute/path/to/zephyrproject
 <SPE>: openair/samples/spe
 <APP>: zephyr/samples/hello_world
 <MCUBOOT>: bootloader/mcuboot/boot/zephyr
 <BOARD>: ATMEVK-3425-PQK-2

Alternatively, use any board from the boards_ list as ``<BOARD>``.

Building the SPE
================

Non-MCUboot Option
------------------

If device firmware update (DFU) is not needed, the following simple option can be used::

  west build -p -s <SPE> -b <BOARD> -d build/<BOARD>/<SPE>


MCUboot Option
--------------

To build with MCUboot because, for example, DFU is needed, first build MCUboot::

  west build -p -s <MCUBOOT> -b <BOARD>@mcuboot -d build/<BOARD>/<MCUBOOT> -- -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_DEBUG=n -DCONFIG_BOOT_MAX_IMG_SECTORS=512 -DDTC_OVERLAY_FILE="<WEST_TOPDIR>/zephyr/boards/atmosic/atm34evk/<BOARD>_mcuboot_bl.overlay"

and then the Atmosic SPE::

  west build -p -s <SPE> -b <BOARD>@mcuboot -d build/<BOARD>/<SPE> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n -DDTS_EXTRA_CPPFLAGS=";"

Note that make use of "board revision" to configure our board partitions to work for MCUboot.  On top of the "revisions," MCUboot currently needs an additional overlay that must be provided via the command line to give it the entire SRAM.


Building the Application
========================

Note: ``<BOARD>//ns`` is the non-secure variant of ``<BOARD>``.


BLE Link Controller Options
---------------------------
When building a Bluetooth application (``CONFIG_BT``) the BLE driver component provides a statically linked BLE link controller library.  The BLE link controller sits at the lowest layer of the Zephyr Bluetooth protocol stack.  Zephyr provides the upper Bluetooth Host stack that can interface with BLE link controllers that conform to the standard Bluetooth Host Controller Interface specification.

To review how the statically linked controller library is used, please refer to the README.rst in modules/hal/atmosic/ATM34xx-5/drivers/ble/.

If the ATM34 entropy driver is enabled without CONFIG_BT=y (mainly for evaluation), the system still requires a minimal BLE controller stack.  Without choosing a specific stack configuration an appropriate minimal BLE controller will be selected.  This may increase the size of your application.

Note that developers cannot use ``CONFIG_BT_CTLR_*`` `flags`__ with the ATM34 platform, as a custom, hardware-optimized link controller is used instead of Zephyr's link controller software.

.. _CONFIG_BT_CTLR_KCONFIGS: https://docs.zephyrproject.org/latest/kconfig.html#!%5ECONFIG_BT_CTLR
__ CONFIG_BT_CTLR_KCONFIGS_


Enabling A Random BD Address
----------------------------

Non-production ATM34 EVKs in the field have no BD address programmed in the secure journal.  On such boards, upon loading a BLE application, an assert error occurs with a message appearing on the console similar to the one below::

  ASSERT ERR(0) at <zephyrproject-root>/atmosic-private/modules/hal_atmosic/ATM34xx/drivers/eui/eui.c:129

To avoid this error, the BLE application must be built with an option to allocate a random BD address.  This can be done by adding ``-DCONFIG_ATM_EUI_ALLOW_RANDOM=y`` to the build options.


Non-MCUboot Option
------------------

Build the app with the non-secure board variant and the SPE (see Non-MCUboot Option build above) configured as follows::

  west build -p -s <APP> -b <BOARD>//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\"

Passing the path to the SPE is for linking in the non-secure-callable veneer file generated in building the SPE.

With this approach, each built image has to be flashed separately.  Optionally, build a single merged image by enabling ``CONFIG_MERGE_SPE_NSPE``, thereby minimizing the flashing steps::

  west build -p -s <APP> -b <BOARD>//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\" -DCONFIG_MERGE_SPE_NSPE=y


MCUboot Option
--------------

Build the application with MCUboot and SPE as follows::

  west build -p -s <APP> -b <BOARD>@mcuboot//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DDTS_EXTRA_CPPFLAGS=";" -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\"

This is somewhat of a non-standard workflow.  When passing ``-DCONFIG_BOOTLOADER_MCUBOOT=y`` on the application build command line, ``west`` automatically creates a singed, merged image (``zephyr.signed.{bin,hex}``), which is ultimately used by ``west flash`` to program the device.  The original application binaries are renamed with a ``.nspe`` suffixed to the file basename (``zephyr.{bin,hex,elf}`` renamed to ``zephyr.nspe.{bin,hex,elf}``) and are the ones that should be supplied to a debugger.

.. _flashing:

Flashing
========

``west flash`` is used to program a device with the necessary images, often only built as described above and sometimes also with a pre-built library provided as an ELF binary.

In this section, substitute ``<DEVICE_ID>`` with the serial for the Atmosic interface board used.  For an atmevk34 board, this is typically a J-Link serial, but it can also be an FTDI serial of the form ``ATRDIXXXX``.  For a J-Link board, pass the ``--jlink`` option to the flash runner as in ``west flash --jlink ...``.

The following subsections describe how to flash a device with and without MCUboot option.


Fast-Load Option
----------------
Atmosic provides a mechanism to increase the legacy programming time called FAST LOAD. Apply the option ``--fast_load`` to enable the FAST LOAD. For Example::

  west flash --device=<DEVICE_ID> --jlink --verify --skip-rebuild --fast_load -d build/<BOARD>_ns/<APP>


Non-MCUboot Option
------------------

Flash the SPE and the application separately if ``CONFIG_MERGE_SPE_NSPE`` was not enabled::

  west flash --device=<DEVICE_ID> --jlink --verify -d build/<BOARD>/<SPE> --noreset
  west flash --device=<DEVICE_ID> --jlink --verify -d build/<BOARD>_ns/<APP>

Alternatively, if ``CONFIG_MERGE_SPE_NSPE`` was enabled in building the application, the first step (programming the SPE) can be skipped.


MCUboot Option
--------------

First, flash MCUboot::

   west flash --verify --device=<DEVICE_ID> --jlink -d build/<BOARD>/bootloader/mcuboot/boot/zephyr --erase_flash --noreset

Then flash the singed application image (merged with SPE)::

   west flash --verify --device=<DEVICE_ID> --jlink -d build/<BOARD>_ns/<APP>


Atmosic In-System Programming (ISP) Tool
****************************************

This SDK ships with a tool called Atmosic In-System Programming Tool
(ISP) for bundling all three types of binaries -- OTP NVDS, flash NVDS, and
flash -- into a single binary archive.

+---------------+-----------------------------------------------------+
|  Binary Type  |  Description                                        |
+---------------+-----------------------------------------------------+
|   .bin        |  binary file, contains flash or nvds data only.     |
+---------------+-----------------------------------------------------+

The ISP tool, which is also shipped as a stand-alone package, can then be used
to unpack the components of the archive and download them on a device.

west atm_arch commands
======================

atm isp archive tool
  -atm_isp_path ATM_ISP_PATH, --atm_isp_path ATM_ISP_PATH
                        specify atm_isp exe path path
  -d, --debug           debug enabled, default false
  -s, --show            show archive
  -b, --burn            burn archive
  -a, --append          append to input atm file
  -i INPUT_ATM_FILE, --input_atm_file INPUT_ATM_FILE
                        input atm file path
  -o OUTPUT_ATM_FILE, --output_atm_file OUTPUT_ATM_FILE
                        output atm file path
  -p PARTITION_INFO_FILE, --partition_info_file PARTITION_INFO_FILE
                        partition info file path
  -nvds_file NVDS_FILE, --nvds_file NVDS_FILE
                        nvds file path
  -spe_file SPE_FILE, --spe_file SPE_FILE
                        spe file path
  -app_file APP_FILE, --app_file APP_FILE
                        application file path
  -mcuboot_file MCUBOOT_FILE, --mcuboot_file MCUBOOT_FILE
                        mcuboot file path
  -atmwstk_file ATMWSTK_FILE, --atmwstk_file ATMWSTK_FILE
                        atmwstk file path
  -openocd_pkg_root OPENOCD_PKG_ROOT, --openocd_pkg_root OPENOCD_PKG_ROOT
                        Path to directory where openocd and its scripts are found

Support Linux and Windows currently. The ``--atm_isp_path`` option should be specifiec accordingly.

On Linux::
  the ``--atm_isp_path`` option should be modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

On Windows::
  the ``--atm_isp_path`` option should be modules/hal/atmosic_lib/tools/atm_arch/bin/Windows_NT/atm_isp.exe

When ``-DCONFIG_SPE_PATH`` has been sepcified, the parition_info infomation will merge from build_dir of application and spe to build_dir of application and named as parition_info.map.merge.

When not use SPE::
  the ``-p`` option should be <build_dir>/zehpyr/parition_info.map
When use SPE::
  the ``-p`` option should be <build_dir>/zehpyr/parition_info.map.merge


Generate atm isp file
---------------------

Without SPE::
  west atm_arch -o <BOARD>_beacon.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.signed.bin \
    --mcuboot_file build/<BOARD>/<MCUBOOT>/zephyr/zephyr.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

With SPE::
  west atm_arch -o <BOARD>_beacon.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map.merge \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.signed.bin \
    --mcuboot_file build/<BOARD>/<MCUBOOT>/zephyr/zephyr.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

Without MCUBOOT::
  west atm_arch -o <BOARD>_beacon.atm \
    -p build/<BOARD>_ns/<APP>/zephyr/partition_info.map.merge \
    --app_file build/<BOARD>_ns/<APP>/zephyr/zephyr.bin \
    --spe_file build/<BOARD>/<SPE>/zephyr/zephyr.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp

Show atm isp file
-----------------

  west atm_arch -i <BOARD>_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp \
    --show

Flash atm isp file
------------------

  west atm_arch -i <BOARD>_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_arch/bin/Linux/atm_isp \
    --openocd_pkg_root atmosic-private/modules/hal_atmosic/ATM34xx \
    --burn

Programming Secure Journal
=========================

The secure journal is a dedicated block of RRAM that has the property of being a write once, append-only data storage area that replaces traditional OTP memory. This region is located near the end of the RRAM storage array at 0x8F800– 0x8FEEF (1776 bytes).

The secure journal data updates are controlled by a secure counter (address ratchet). The counter determines the next writable location at an offset from the start of the journal. An offset greater than the counter value is writable while any offset below or equal to the counter is locked from updates. The counter can only increment monotonically and cannot be rolled back. This provides the immutability of OTP as well as the flexibility to append new data items or overriding past items using a find latest TLV search.

The west extension command `secjrnl` is provided by the Atmosic HAL to allow for easy access and management of the secure journal on supported platforms.

The tool provides a help command that describes all available operations via::

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

The secure journal uses a find latest search algorithm to allow overrides. If the passed tag should NOT be overridden in the future, add the flag ``--locked`` to the append command. See following section for more information regarding locking a tag.


NOTE: The ``append`` command  does NOT increment the ratchet. The newly appended tag is still unprotected from erasing.

Locking down a tag
------------------

The secure journal provides a secure method of storing data while still providing options to update the data if needed. However, there are key data entries that should never be updated across the life of the device (e.g. UUID).
This support is provided by software and can be enabled for a tag by passing ``--locked`` to the command when appending a new tag.

It is important to understand, once a tag is **locked** (and ratcheted), the specific tag can never be updated in the future - Appending a new tag of the same value will be ignored.


Erasing non-ratcheted data from the Secure Journal
--------------------------------------------------

Appended tags are not ratcheted down. this allows for prototyping with the secure journal before needing to lock down the TLVs. To support prototyping, you can erase non-ratcheted data easily via::

 west secjrnl erase --device <DEVICE_ID>



Ratcheting Secure Journal
-------------------------

To ratchet data, run the command::

 west secjrnl ratchet_jrnl --device <DEVICE_ID>

This will list the non-ratcheted tags and confirm that you want to ratchet the tags. Confirm by typing 'yes'.

NOTE: This process is non reversible. Once ratcheted, that region of the secure journal cannot be modified.

Viewing the Console Output
**************************

Linux and macOS
===============

For a Linux host, monitor the console output with a favorite terminal
program, such as::

  screen /dev/ttyACM1 115200

On macOS, the serial console will be on USB port (``/dev/tty.usbmodem<12-digit device ID>[13]``).  Use the following command to find the port for serial console::

  $ ls /dev/tty.usbmodem*
  /dev/tty.usbmodem<DEVICE_ID>1
  /dev/tty.usbmodem<DEVICE_ID>3
  $


Windows
=======

Console output for current Atmosic ATM34xx goes to the JLink CDC UART
serial port.  That is Interface 2 of J-Link OB USB on the Atmosic
board.  In order to view the console output, use a serial terminal
program such as PuTTY (available from
https://www.chiark.greenend.org.uk/~sgtatham/putty) to connect to
JLink CDC UART port generated by the interface 2 of J-Link OB USB
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

In Zephyr, DFU is possible using the ``mcumgr`` subsystem. This makes use of some of the features from MCUBoot in order to facilitate image uploading and swapping.
In order to test this subsystem, Zephyr provides an SMP server sample that makes use of the subsystem to test performing Serial DFU and BLE OTA firmware updates.
To actually perform the DFU, the ``mcumgr`` program can be used. Currently, this supports UART on all platforms and BLE on macOS and Linux (only Linux is tested currently for BLE).
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

By default the UART0 peripheral is not enabled, which will cause a build error.
In order to enable UART0, please modify the boards DTS file and add ``status = "okay";`` to the ``&uart0`` block.

When building smp_svr to support DFU over serial, the only change from a standard MCUBoot build is to make sure that the proper overlay configurations are applied ``-DOVERLAY_CONFIG="overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf"``::

  west build -p -s <APP> -b <BOARD>@mcuboot//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\" -DDTS_EXTRA_CPPFLAGS=";" -DOVERLAY_CONFIG="overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf"

Building for BLE
----------------

If building smp_svr using external flash, either the ``PD50LL`` or the ``CPD200`` or the ``LL`` wireless stack can be used. When using the ``PD50`` wireless stack, the following variable assignments/substitutions should be used::

  ATMWSTKLIB=PD50

When building smp_svr to support DFU over BLE, all images (MCUBoot, SPE, smp_svr) need to be built with ``-DDTS_EXTRA_CPPFLAGS="-DATMWSTKLIB=<ATMWSTK>;"`` (when using external flash, the ``-DDFU_IN_FLASH;`` option must also be present).
smp_svr additionally needs to be configured to use the ATMWSTK ``-DCONFIG_ATM_SLEEP_ADJ=17`` and use the proper overlay configuration files ``-DEXTRA_CONF_FILE="overlay-bt.conf"`` (If Serial DFU support is also desired, then the overlay files from the serial_dfu_ section)::

  west build -p -s <MCUBOOT> -b <BOARD>@mcuboot -d build/<BOARD>/<MCUBOOT> -- -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_BOOT_MAX_IMG_SECTORS=512 -DDTC_OVERLAY_FILE="<WEST_TOPDIR>/zephyr/boards/atmosic/atm34evk/<BOARD>_mcuboot_bl.overlay" -DDTS_EXTRA_CPPFLAGS="-DATMWSTKLIB=<ATMWSTKLIB>;"
  west build -p -s <SPE> -b <BOARD>@mcuboot -d build/<BOARD>/<SPE> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n -DDTS_EXTRA_CPPFLAGS="-DATMWSTKLIB=<ATMWSTKLIB>;"
  west build -p -s <APP> -b <BOARD>@mcuboot//ns -d build/<BOARD>_ns/<APP> -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DCONFIG_SPE_PATH=\"<WEST_TOPDIR>/build/<BOARD>/<SPE>\" -DDTS_EXTRA_CPPFLAGS="-DATMWSTKLIB=<ATMWSTKLIB>;"  -DEXTRA_CONF_FILE="overlay-bt.conf" -DCONFIG_ATM_SLEEP_ADJ=17
