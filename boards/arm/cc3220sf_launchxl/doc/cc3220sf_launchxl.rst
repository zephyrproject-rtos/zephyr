.. _cc3220sf_launchxl:

CC3220SF LaunchXL
#################

Overview
********
The SimpleLink Wi-Fi CC3220SF LaunchPad development kit (CC3220SF-LAUNCHXL)
highlights CC3220SF, a single-chip wireless microcontroller (MCU) with
1MB internal flash, 4MB external serial flash, 256KB of RAM and enhanced
security features.

See the `TI CC3220 Product Page`_ for details.

Features:
=========

* Two separate execution environments: a user application dedicated ARM
  Cortex-M4 MCU and a network processor MCU to run all Wi-Fi and
  internet logical layers
* 40-pin LaunchPad standard leveraging the BoosterPack ecosystem
* On-board accelerometer and temperature sensor
* Two buttons and three LEDs for user interaction
* UART through USB to PC
* BoosterPack plug-in module for adding graphical displays, audio
  codecs, antenna selection, environmental sensing, and more
* Power from USB for the LaunchPad and optional external BoosterPack
* XDS110-based JTAG emulation with serial port for flash programming

Details on the CC3220SF LaunchXL development board can be found in the
`CC3220SF LaunchPad Dev Kit Hardware User's Guide`_.

Hardware
********

The CC3220SF SoC has two MCUs:

#. Applications MCU - an ARM |reg| Cortex |reg|-M4 Core at 80 MHz, with 256Kb RAM,
   and access to external serial 4MB flash with bootloader and peripheral
   drivers in ROM.

#. Network Coprocessor (NWP) - a dedicated ARM MCU, which completely
   offloads Wi-Fi and internet protocols from the application MCU.

Complete details of the CC3220SF SoC can be found in the `CC3220 TRM`_.

Supported Features
==================

Zephyr has been ported to the Applications MCU, with basic peripheral
driver support.

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| UART      | on-chip    | serial port-interrupt |
+-----------+------------+-----------------------+
| GPIO      | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| I2C       | on-chip    | i2c                   |
+-----------+------------+-----------------------+
| SPI_0     | on-chip    | WiFi host driver      |
+-----------+------------+-----------------------+

.. note::

   For consistency with TI SimpleLink SDK and BoosterPack examples,
   the I2C driver defaults to I2C_BITRATE_FAST mode (400 kHz) bus speed
   on bootup.

The accelerometer, temperature sensors, or other peripherals
accessible through the BoosterPack, are not currently supported.

Connections and IOs
====================

Peripherals on the CC3220SF LaunchXL are mapped to the following pins in
the file :file:`boards/arm/cc3220sf_launchxl/pinmux.c`.

+------------+-------+-------+
| Function   | PIN   | GPIO  |
+============+=======+=======+
| UART0_TX   | 55    | N/A   |
+------------+-------+-------+
| UART0_RX   | 57    | N/A   |
+------------+-------+-------+
| LED D7 (R) | 64    | 9     |
+------------+-------+-------+
| LED D6 (O) | 01    | 10    |
+------------+-------+-------+
| LED D5 (G) | 02    | 11    |
+------------+-------+-------+
| Switch SW2 | 15    | 22    |
+------------+-------+-------+
| Switch SW3 | 04    | 13    |
+------------+-------+-------+

The default configuration can be found in the Kconfig file at
:file:`boards/arm/cc3220sf_launchxl/cc3220sf_launchxl_defconfig`.


Programming and Debugging
*************************

TI officially supports development on the CC3220SF using the TI
`CC3220 SDK`_ on Windows and Linux using TI tools: Code Composer
Studio for debugging and `UniFlash`_ for flashing.

For Windows developers, see the `CC3220 Getting Started Guide`_ for
instructions on installation of tools, and how to flash the board using
UniFlash.

Note that zephyr.bin produced by the Zephyr SDK may not load via
UniFlash tool.  If encountering difficulties, use the zephyr.elf
file and openocd instead (see below).

The following instructions are geared towards Linux developers who
prefer command line tools to an IDE.

Before flashing and debugging the board, there are a few one-time board
setup steps to follow.

Prerequisites:
==============

#. Download and install the latest version of `UniFlash`_.
#. Jumper SOP[2..0] (J15) to [010], and connect the USB cable to the PC.

   This should result in a new device "Texas Instruments XDS110 Embed
   with CMSIS-DAP" appearing at /dev/ttyACM1 and /dev/ttyACM0.

#. Update the service pack, and place the board in "Development Mode".

   Setting "Development Mode" enables the JTAG interface, necessary
   for subsequent use of OpenOCD and updating XDS110 firmware.

   Follow the instructions in Section 2.4 "Download the Application",
   in the `CC3220 Getting Started Guide`_, except for steps 5 and 6 in
   Section 2.4.1 which select an MCU image.

#. Ensure the XDS-110 emulation firmware is updated.

   Download and install the latest `XDS-110 emulation package`_.

   Follow the directions here to update the firmware:
   http://processors.wiki.ti.com/index.php/XDS110#Updating_the_XDS110_Firmware

   Note that the emulation package install may place the xdsdfu utility
   in <install_dir>/ccs_base/common/uscif/xds110/.

#. Switch Jumper SOP[2..0] (J15) back to [001].

   Remove power from the board (disconnect USB cable) before switching jumpers.

#. Install TI OpenOCD

   Clone the TI OpenOCD git repository from: http://git.ti.com/sdo-emu/openocd.
   Follow the instructions in the Release Notes in that repository to build
   and install.

   Since the default TI OpenOCD installation is /usr/local/bin/,
   and /usr/local/share/, you may want to backup any current openocd
   installations there.
   If you decide to change the default installation location, also update
   the OPENOCD path variable in :file:`boards/arm/cc3220sf_launchxl/board.cmake`.

#. Ensure CONFIG_XIP=y (default) is set.

   This locates the program into flash, and sets CONFIG_CC3220SF_DEBUG=y,
   which prepends a debug header enabling the flash to persist over
   subsequent reboots, bypassing the bootloader flash signature
   verification.

   See Section 21.10 "Debugging Flash User Application Using JTAG" of the
   `CC3220 TRM`_ for details on the secure flash boot process.


Once the above prerequisites are met, applications for the ``_cc3220sf_launchxl``
board can be built, flashed, and debugged with openocd and gdb per the Zephyr
Application Development Primer (see :ref:`build_an_application` and
:ref:`application_run`).

Flashing
========

To build and flash an application, execute the following commands for <my_app>:

.. zephyr-app-commands::
   :zephyr-app: <my_app>
   :board: cc3220sf_launchxl
   :goals: flash

This will load the image into flash.

To see program output from UART0, connect a separate terminal window:

.. code-block:: console

  % screen /dev/ttyACM0 115200 8N1

Then press the reset button (SW1) on the board to run the program.

Debugging
=========

To debug a previously flashed image, after resetting the board, use the 'debug'
build target:

.. zephyr-app-commands::
   :zephyr-app: <my_app>
   :board: cc3220sf_launchxl
   :maybe-skip-config:
   :goals: debug


WiFi Support
************

The SimpleLink Host Driver, imported from the SimpleLink SDK, has been ported
to Zephyr, and communicates over a dedicated SPI to the network co-processor.
It is available as a Zephyr WiFi device driver in
:file:`drivers/wifi/simplelink`.

Currently only the Zephyr WiFi management operations are exported.

Usage:
======

Set :option:`CONFIG_WIFI_SIMPLELINK` to ``y`` to enable WiFi.
See :file:`samples/net/wifi/boards/cc3220sf_launchxl.conf`.

Provisioning:
=============

SimpleLink provides a few rather sophisticated WiFi provisioning methods.
To keep it simple for Zephyr development and demos, the SimpleLink
"Fast Connect" policy is enabled, with one-shot scanning.
This enables the cc3220sf_launchxl to automatically reconnect to the last
good known access point (AP), without having to restart a scan, and
re-specify the SSID and password.

To connect to an AP, first run the Zephyr WiFi shell sample application,
and connect to a known AP with SSID and password.

See :ref:`wifi_sample`

Once the connection succeeds, the network co-processor keeps the AP identity in
its persistent memory.  Newly loaded WiFi applications then need not explicitly
execute any WiFi scan or connect operations, until the need to change to a new AP.

References
**********

CC32xx Wiki:
    http://processors.wiki.ti.com/index.php/CC31xx_%26_CC32xx

.. _TI CC3220 Product Page:
    http://www.ti.com/product/cc3220

.. _CC3220 TRM:
   http://www.ti.com/lit/pdf/swru465

.. _CC3220 Programmer's Guide:
   http://www.ti.com/lit/pdf/swru464

.. _CC3220 Getting Started Guide:
   http://www.ti.com/lit/pdf/swru461

.. _UniFlash:
   http://processors.wiki.ti.com/index.php/Category:CCS_UniFlash

.. _CC3220 SDK:
   http://www.ti.com/tool/download/SIMPLELINK-CC3220-SDK

.. _CC3220SF LaunchPad Dev Kit Hardware User's Guide:
   http://www.ti.com/lit/pdf/swru463

..  _XDS-110 emulation package:
   http://processors.wiki.ti.com/index.php/XDS_Emulation_Software_Package#XDS_Emulation_Software_.28emupack.29_Download
