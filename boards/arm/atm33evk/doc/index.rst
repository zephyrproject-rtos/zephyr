.. _atm33evk:

Atmosic ATM33xx
###############

Overview
********
The ATM33/ATM33e Wireless SoC Series are part of a family of extreme low-power Bluetooth® 5.3 system-on-chip (SoC) solutions.  This Bluetooth Low Energy SoC integrates a Bluetooth 5.3 compliant radio with ARM® Cortex® M33F application processor, 128 KB Random Access Memory (RAM), 64 KB Read Only Memory (ROM), 512 KB nonvolatile memory (NVM), with ARM® TrustZone® enabled security features, and state-of-the-art power management to enable maximum lifetime in battery-operated devices.

The extremely low power ATM33e/ATM33 Series SoC, with a 0.85 mA radio receiver and a 2.1 mA radio transmitter power consumption, is designed to extend battery life for the Internet-of-Things (IoT) markets. Support for low duty cycle operation allows systems to run for significantly longer periods without battery replacement. In addition, this series of SoCs from Atmosic supports operation from energy harvesting sources, including RF, photovoltaic, TEG (Thermoelectric generator), and motion.  Innovative wake-up mechanisms are supported to provide options for further power consumption reduction.

Hardware Features
*****************
Bluetooth LE
  - Bluetooth Low-Energy 5.3 compliant
  - 2 Mbps, 1 Mbps 500 kbps, and 125 kbps PHY rates
  - Supports Bluetooth Angle-of-Arrival (AoA) and Angle-of-Departure (AoD) direction finding
MCU and Memory
  - 64 MHz ARM® Cortex® M33F MCU
  - 64 KB ROM, 128 KB RAM, 512 KB NVM
  - Retention RAM configuration: 16 KB to 128 KB in 16 KB step sizes
  - 16 MHz / Optional 32.768 kHz Crystal Oscillator
Security
  - ARM® TrustZone®,  HW Root of Trust, Secure Boot, Secure Execution & Debug
  - AES-128/256, SHA-2/HMAC 256 Encryption/Cryptographic Hardware Accelerators
  - True random number generator (TRNG)
Energy Harvesting (ATM33e)
  - On-chip RF Energy Harvesting
  - Supports photovoltaic, thermal, motion and other energy harvesting technologies
  - External Harvesting/Storage Interface
RF and Power Management
  - Fully integrated RF front-end
  - Sensor Hub
  - RF Wakeup Receiver
  - 1.1 V to 4.2 V battery input voltage with integrated Power Management Unit (PMU)
  - Radio power consumption with 3 V battery
    - Rx @ -95 dBm: 0.85 mA
    - Tx @ 0 dBm: 2.1 mA
  - SoC typical power consumption with 3 V battery including PMU
    - Retention @ 32 KB RAM: 1.6 µA
    - Hibernate: 1.1 µA
    - SoC Off: 400 nA
    - SoC Off with Harvesting Enabled: 700 nA
Interfaces
  - I2C (2), I2S, SPI (2), UART (2), PWM (8), GPIOs (15, 21 or 31 depending on the package option)
  - Quad SPI
  - 11-bit application ADC, 4 external, 5 internal channels, up to 2 Msps
  - Two mono or one stereo Digital microphone inputs (PDM)
  - 8 x 20 Keyboard matrix controller (KSM)
  - Quadrature decoder (QDEC)
  - SWD for Interactive Debugging
Package Options
  - ATM3330e: 7x7 mm, 56-pin QFN (up to 31 GPIOs)
  - ATM3330: 7x7 mm, 56-pin QFN (up to 31 GPIOs)
  - ATM3325: 5x5 mm, 40-pin QFN (up to 21 GPIOs)
  - ATM3325: 49L WLCSP (up to 21 GPIOs)
  - ATM3330e: 5x5 mm, 40-pin QFN (up to 15 GPIO)


.. _boards:

Boards
======
* ATMEVK-3325-LQK
* ATMEVK-3325-QK-5
* ATMEVK-3330-QN-5
* ATMEVK-3330e-QN-5
* ATMEVK-3330e-QN-7


Getting Started
***************

Follow the instructions_ from the official Zephyr documentation on how to get started.

.. _instructions: https://docs.zephyrproject.org/3.4.0/develop/getting_started/index.html

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

Applications for the ATMEVK-33xx-xx-5 and ATMEVK-33xxe-xx-5 boards can be built, flashed, and debugged with familiar `west build` and `west flash`.

The atm33evk boards require at least two images to be built -- the SPE and the application.

The Atmosic SPE lives in the HAL module at ``${WEST_TOPDIR}/modules/hal/atmosic/ATM33xx-5/examples/spe``.

.. _var_assignments:

In the remainder of this document, use the following variable assignments/substitutions::

 SPE=modules/hal/atmosic/ATM33xx-5/examples/spe
 APP=zephyr/samples/hello_world
 MCUBOOT=bootloader/mcuboot/boot/zephyr

and::

 BOARD=ATMEVK-3330-QN-5

Alternatively, use any board from the boards_ list as ``$BOARD``.

Building the SPE
================

Non-MCUboot Option
------------------

If device firmware update (DFU) is not needed, the following simple option can be used::

  west build -s $SPE -b $BOARD -d build/$BOARD/$SPE


MCUboot Option
--------------

To build with MCUboot because, for example, DFU is needed, first build MCUboot::

  west build -s $MCUBOOT -b $BOARD@mcuboot -d build/$BOARD/$MCUBOOT -- -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_DEBUG=n -DCONFIG_BOOT_MAX_IMG_SECTORS=512 -DDTC_OVERLAY_FILE="$WEST_TOPDIR/zephyr/boards/arm/atm33evk/${BOARD}_mcuboot_bl.overlay"

and then the Atmosic SPE::

  west build -s $SPE -b $BOARD@mcuboot -d build/$BOARD/$SPE -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n -DDTS_EXTRA_CPPFLAGS=";"

Note that make use of "board revision" to configure our board paritions to work for MCUboot.  On top of the "revisions," MCUboot currently needs an additional overlay that must be provided via the command line to give it the entire SRAM.


Building the Application
========================

Note: ``${BOARD}_ns`` is the non-secure variant of ``$BOARD``.


BLE Link Controller Options
---------------------------
When building a Bluetooth application (``CONFIG_BT``) the BLE driver component provides two link controller options. A fixed BLE link controller image and a statically linked BLE link controller library.  The BLE link controller sits at the lowest layer of the Zephyr Bluetooth protocol stack.  Zephyr provides the upper Bluetooth Host stack that can interface with BLE link controllers that conform to the standard Bluetooth Host Controller Interface specification.

To review how the fixed and statically linked controllers are used, please refer to the README.rst in modules/hal/atmosic/ATM33xx-5/drivers/ble/.

If the ATM33 entropy driver is enabled without CONFIG_BT=y (mainly for evaluation), the system still requires a minimal BLE controller stack.  Without choosing a specific stack configuration an appopriate minimal BLE controller will be selected.  This may increase the size of your application.


Non-MCUboot Option
------------------

Build the app with the non-secure board variant and the SPE (see Non-MCUboot Option build above) configured as follows::

  west build -s $APP -b ${BOARD}_ns -d build/${BOARD}_ns/$APP -- -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/$BOARD/$SPE\"

Passing the path to the SPE is for linking in the non-secure-callable veneer file generated in building the SPE.

With this approach, each built image has to be flashed separately.  Optionally, build a single merged image by enabling ``CONFIG_MERGE_SPE_NSPE``, thereby minimizing the flashing steps::

  west build -s $APP -b ${BOARD}_ns -d build/${BOARD}_ns/$APP -- -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/$BOARD/$SPE\" -DCONFIG_MERGE_SPE_NSPE=y


MCUboot Option
--------------

Build the application with MCUboot and SPE as follows::

  west build -s $APP -b ${BOARD}_ns@mcuboot -d build/${BOARD}_ns/$APP -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DDTS_EXTRA_CPPFLAGS=";" -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/$BOARD/$SPE\"

This is somewhat of a non-standard workflow.  When passing ``-DCONFIG_BOOTLOADER_MCUBOOT=y`` on the application build command line, ``west`` automatically creates a singed, merged image (``zephyr.signed.{bin,hex}``), which is ultimately used by ``west flash`` to program the device.  The original application binaries are renamed with a ``.nspe`` suffixed to the file basename (``zephyr.{bin,hex,elf}`` renamed to ``zephyr.nspe.{bin,hex,elf}``) and are the ones that should be supplied to a debugger.

.. _flashing:

Flashing
========

``west flash`` is used to program a device with the necessary images, often only built as described above and sometimes also with a pre-built library provided as an ELF binary.

In this section, substitute ``$DEVICE_ID`` with the serial for the Atmosic interface board used.  For an atmevk33 board, this is typically a J-Link serial, but it can also be an FTDI serial of the form ``ATRDIXXXX``.

The following subsections describe how to flash a device with and without MCUboot option.  If the application requires Bluetooth (configured with ``CONFIG_BT``), and uses the fixed BLE link controller image option, then the controller image requires programming.  This is typically done prior to programming the application and resetting (omitting the ``--noreset`` option to ``west flash``).  For example::

  west flash --verify --device=$DEVICE_ID --skip-rebuild -d build/$BOARD/$MCUBOOT --use-elf --elf-file modules/hal/atmosic/ATM33xx-5/drivers/ble/atmwstk_LL.elf --noreset

For the non-MCUboot option, substitute ``$MCUBOOT`` with ``$SPE`` in the above command.


Fast-Load Option
----------------
Atmosic provides a mechanism to increase the legacy programming time called FAST LOAD. Apply the option ``--fast_load`` to enable the FAST LOAD. For Example::

  west flash -device=$DEVICE_ID --verify --skip-rebuild --fast_load -d build/${BOARD}_ns/$APP


Non-MCUboot Option
------------------

Flash the SPE and the application separately if ``CONFIG_MERGE_SPE_NSPE`` was not enabled::

  west flash --device=$DEVICE_ID --verify -d build/$BOARD/$SPE --noreset
  west flash --device=$DEVICE_ID --verify -d build/${BOARD}_ns/$APP

Alternatively, if ``CONFIG_MERGE_SPE_NSPE`` was enabled in building the application, the first step (programming the SPE) can be skipped.


MCUboot Option
--------------

First, flash MCUboot::

   west flash --verify --device=$DEVICE_ID -d build/$BOARD/bootloader/mcuboot/boot/zephyr --erase_flash --noreset

Then flash the singed application image (merged with SPE)::

   west flash --verify --device=$DEVICE_ID -d build/${BOARD}_ns/$APP


Support Script
==============

A convenient support script is provided in the Zephyr repository and can be used as follows.  From the ``west topdir`` directory where Zephyr was cloned and ``west`` was initialized, run the following:

Without MCUBoot::

  zephyr/boards/arm/atm33evk/support/run.sh -n -e -d [-w <flavor>] [-l <flavor>] -a <application path> -j -s <DEVICE_ID> <BOARD>

With MCUBoot::

  zephyr/boards/arm/atm33evk/support/run.sh -e -d [-w <flavor>] [-l <flavor>] -a <application path> -j -s <DEVICE_ID> <BOARD>

* replace ``<DEVICE_ID>`` with the appropriate device ID (typically the JLINK serial ID. Ex: ``000900028906``)
* replace ``<BOARD>`` with the targeted board design (Ex: ATMEVK-3325-LQK )
* replace ``<application path>`` with the path to your application (Ex: ``zephyr/samples/bluetooth/peripheral_dis``)
* see below for selecting ``-w``/``-l`` options.

Using -w [flavor] and -l [flavor] Options
-----------------------------------------

The ``-w`` option selects the use of the fixed BLE controller stack image.  The flavor parameter can be ``LL`` or ``PD50LL``. The ``-l`` option selects for the statically linked BLE controller library.  The flavor can be PD50.  The ``-l`` flag is mutually exclusive with the ``-w`` option.  When using the ``-l`` option the build will recover memory resources reserved for the fixed image BLE controller and provide them to the NSPE image.  The ``-w`` option should not be used to flash the ATMWSTK when the application has already been built and installed using the ``-l`` option.  Flashing the fixed BLE controller on top of an existing install that uses the static library may corrupt the installed image.

Using the Support Script on Windows
-----------------------------------

This script is written in Bash.  While Bash is readily available on most Linux distributions and macOS, it is not so on Windows.  However, Bash is bundled with Git.  The following single command demonstrates how to build, flash, and run the ``hello_world`` application using Bash in a typical installation of Git executed from the root of the Zephyr workspace::

  C:\zephyrproject>"C:\Program Files\Git\bin\bash.exe" zephyr\boards\arm\atm33evk\support\run.sh -e -d -a zephyr\samples\hello_world -j -s <DEVICE_ID> <BOARD>

As an alternative, pass ``-n`` to build without MCUboot.

From this point on out, unless the bootloader has been modified, the source code for the application (in this case ``zephyr\samples\hello_world``) can be modified and then programmed with ``-d`` and ``-e`` omitted::

  C:\zephyrproject>"C:\Program Files\Git\bin\bash.exe" zephyr\boards\arm\atm33evk\support\run.sh -a zephyr\samples\hello_world -j -s <DEVICE_ID> <BOARD>


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
|   .elf        |  elf file, a common standard file format, consists  |
|               |  of elf headers and flash data.                     |
+---------------+-----------------------------------------------------+
|   .nvm        |  OTP NVDS file, contains OTP nvds data.             |
+---------------+-----------------------------------------------------+

The ISP tool, which is also shipped as a stand-alone package, can then be used
to unpack the components of the archive and download them on a device.

west atm_arch commands
======================
::

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

Generate atm isp file
=====================
::

  west atm_arch -o ATMEVK-3330-QN-5_beacon.atm \
    -p build/ATMEVK-3330-QN-5_ns/zephyr/samples/bluetooth/beacon/zephyr/partition_info.map \
    --app_file build/ATMEVK-3330-QN-5_ns/zephyr/samples/bluetooth/beacon/zephyr/zephyr.signed.bin \
    --mcuboot_file build/ATMEVK-3330-QN-5/bootloader/mcuboot/boot/zephyr/zephyr/zephyr.bin \
    --atmwstk_file modules/hal/atmosic/ATM33xx-5/drivers/ble/atmwstk_PD50LL.bin \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_isp

Show atm isp file
=================
::

  west atm_arch -i ATMEVK-3330-QN-5_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_isp \
    --show

Flash atm isp file
==================
::

  west atm_arch -i ATMEVK-3330-QN-5_beacon.atm \
    --atm_isp_path modules/hal/atmosic_lib/tools/atm_isp \
    --openocd_pkg_root=modules/hal/atmosic_lib \
    --burn


Viewing the Console Output
**************************

Linux and macOS
===============

For a Linux host, monitor the console output with a favorite terminal
program, such as::

  screen /dev/ttyACM1 115200

On macOS, the serial console will be on USB port (``/dev/tty.usbmodem<12-digit devide ID>[13]``).  Use the following command to find the port for serial console::

  $ ls /dev/tty.usbmodem*
  /dev/tty.usbmodem<DEVICE_ID>1
  /dev/tty.usbmodem<DEVICE_ID>3
  $


Windows
=======

Console output for current Atmosic ATM3330 goes to the JLink CDC UART
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

  west build -p -s ${APP} -b ${BOARD}_ns@mcuboot -d build/${BOARD}_ns/${APP} -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/${BOARD}/${SPE}\" -DDTS_EXTRA_CPPFLAGS=";" -DOVERLAY_CONFIG="overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf"

Building for BLE
----------------

If building smp_svr using RRAM only, then the ``PD50LL`` wireless stack **must** be used. This can be done by using the following variable assignments/substitutions::

  ATMWSTK=PD50LL

If building smp_svr using external flash, either the ``PD50LL`` or the ``LL`` wireless stack can be used. When using the ``LL`` wireless stack, the following variable assignments/substitutions should be used::

  ATMWSTK=LL

When building smp_svr to support DFU over BLE, all images (MCUBoot, SPE, smp_svr) need to be built with ``-DDTS_EXTRA_CPPFLAGS="-DATMWSTK=${ATMWSTK};"`` (when using external flash, the ``-DDFU_IN_FLASH;`` option must also be present).
smp_svr additionally needs to be configured to use the ATMWSTK using ``-DCONFIG_USE_ATMWSTK=y -DCONFIG_ATMWSTK=\"${ATMWSTK}\" -DCONFIG_ATM_SLEEP_ADJ=17`` and use the proper overlay configuration files ``-DEXTRA_CONF_FILE="overlay-bt.conf"`` (If Serial DFU support is also desired, then the overlay files from the serial_dfu_ section)::

  west build -p -s ${MCUBOOT} -b ${BOARD}@mcuboot -d build/${BOARD}/${MCUBOOT} -- -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_BOOT_MAX_IMG_SECTORS=512 -DDTC_OVERLAY_FILE="${WEST_TOPDIR}/zephyr/boards/arm/atm33evk/${BOARD}_mcuboot_bl.overlay" -DDTS_EXTRA_CPPFLAGS="-DATMWSTK=${ATMWSTK};"
  west build -p -s ${SPE} -b ${BOARD}@mcuboot -d build/${BOARD}/${SPE} -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n -DDTS_EXTRA_CPPFLAGS="-DATMWSTK=${ATMWSTK};"
  west build -p -s ${APP} -b ${BOARD}_ns@mcuboot -d build/${BOARD}_ns/${APP} -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/${BOARD}/${SPE}\" -DDTS_EXTRA_CPPFLAGS="-DATMWSTK=${ATMWSTK};" -DCONFIG_USE_ATMWSTK=y -DCONFIG_ATMWSTK=\"${ATMWSTK}\" -DEXTRA_CONF_FILE="overlay-bt.conf" -DCONFIG_ATM_SLEEP_ADJ=17
