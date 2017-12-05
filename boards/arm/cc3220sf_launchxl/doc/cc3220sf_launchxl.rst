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
`CC3220SF LaunchXL User's Guide`_.

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
file and dslite.sh instead.

The following instructions are geared towards Linux developers who
prefer command line tools to an IDE.

Flashing
========

The TI UniFlash tool can be used to download a program into flash, which
will persist over subsequent reboots.

Prerequisites:
--------------

#. Python 2.7 (the DSLite tool does not work with Python v 3.x).
#. Download and install `UniFlash`_ version 4.1 for Linux.
#. Jumper SOP[2..0] (J15) to 010, and connect the USB cable to the PC.

   This should result in a new device "Texas Instruments XDS110 Embed
   with CMSIS-DAP" appearing at /dev/ttyACM1 and /dev/ttyACM0.

#. Update the service pack, and place board in "Development Mode".

   Follow the instructions in Section 3.4 "Download the Application",
   in the `CC3220 Getting Started Guide`_, except for steps 5 and 6 which
   select an MCU image.

#. Ensure the XDS-110 emulation firmware is updated.

   Download and install the latest `XDS-110 emulation package`_.
   Follow the directions here to update the firmware:
   http://processors.wiki.ti.com/index.php/XDS110#Updating_the_XDS110_Firmware

#. Ensure CONFIG_XIP=y is set.

   Add a 'CONFIG_XIP=y' line to the project's prj.conf file.

   This locates the program into flash, and sets CONFIG_CC3220SF_DEBUG=y,
   which prepends a debug header enabling the flash to persist over
   subsequent reboots, bypassing the bootloader flash signature
   verification.

   See Section of the 21.10 of the `CC3220 TRM`_ for details on the
   secure flash boot process.

Flashing Command:
-----------------

Once the above prerequisites are met, use the UniFlash command line tool
to flash the Zephyr image:

.. code-block:: console

  % dslite.sh -c $ZEPHYR_BASE/boards/arm/cc3220sf_launchxl/support/CC3220SF.ccxml \
    -e -f zephyr.elf

The CC3220SF.ccxml is a configuration file written by TI's Code Composer
Studio IDE, and required for the dslite.sh tool.

To see program output from UART0, one can execute in a separate terminal
window:

.. code-block:: console

  % screen /dev/ttyACM0 115200 8N1

Debugging
=========

It is possible to enable loading and debugging of an application via
openocd and gdb, by linking and locating the program completely in SRAM.

Prerequisites:
--------------

Follow the same prerequisites as in Flashing above, in addition:

#. Ensure OpenOCD v0.9+ is configured/built with CMSIS-DAP support.
#. Power off the board, jumper SOP[2..0] (J15) to 001, and reconnect
   the USB cable to the PC.
#. Set CONFIG_XIP=n and build the Zephyr elf file.

The necessary OpenOCD CFG and sample gdbinit scripts can be found in
:file:`boards/arm/cc3220sf_launchxl/support/`.

Debugging Command
-----------------

.. code-block:: console

  % arm-none-eabi-gdb -x $ZEPHYR_BASE/boards/arm/cc3220sf_launchxl/support/gdbinit_xds110 \
    zephyr.elf

References
**********

CC32xx Wiki:
    http://processors.wiki.ti.com/index.php/CC31xx_%26_CC32xx

.. _TI CC3220 Product Page:
    http://www.ti.com/product/cc3220

.. _CC3220 TRM:
   http://www.ti.com/lit/ug/swru465/swru465.pdf

.. _CC3220 Programmer's Guide:
   http://www.ti.com/lit/ug/swru464/swru464.pdf

.. _CC3220 Getting Started Guide:
   http://www.ti.com/lit/ug/swru461/swru461.pdf

.. _UniFlash:
   http://processors.wiki.ti.com/index.php/Category:CCS_UniFlash

.. _CC3220 SDK:
   http://www.ti.com/tool/download/SIMPLELINK-CC3220-SDK

.. _CC3220SF LaunchXL User's Guide:
   http://www.ti.com/lit/ug/swru463/swru463.pdf

..  _XDS-110 emulation package:
   http://processors.wiki.ti.com/index.php/XDS_Emulation_Software_Package#XDS110_Reset_Download
