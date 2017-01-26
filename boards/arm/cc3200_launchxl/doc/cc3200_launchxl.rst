.. _cc3200_launchxl:

CC3200 LaunchXL
###############

Overview
********
The SimpleLink CC3200 LaunchXL is a development board for the CC3200
wireless microcontroller (MCU), the industry’s first single-chip
programmable MCU with built-in Wi-Fi connectivity.

Features:
=========

* SimpleLink Wi-Fi, internet-on-a-chip solution with integrated MCU
* 40-pin LaunchPad standard that leverages the BoosterPack ecosystem
* FTDI based JTAG emulation with serial port for Flash programming
* On-board accelerometer and temperature sensor
* Two buttons and three LEDs for user interaction
* UART through USB to PC
* Power from USB for the LaunchPad and optional external BoosterPack
* GNU Debugger (GDB) support over Open On chip debugger (OpenOCD)

Details on the CC3200 LaunchXL development board can be found in the
`CC3200 LaunchXL User's Guide`_.

Hardware
********

The CC3200 SoC has two MCUs:

#. Applications MCU - an ARM® Cortex®-M4 Core at 80 MHz, with 256Kb RAM,
and access to external serial 1Mb flash with bootloader and peripheral
drivers in ROM.

#. Network Coprocessor (NWP) - a dedicated ARM MCU, which completely
offloads Wi-Fi and internet protocols from the application MCU.

Complete details of the CC3200 SoC can be found in the `CC3200 TRM`_.

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

The accelerometer, temperature sensors, WiFi, or other perhipherals
accessible through the BoosterPack, are not currently supported by
this Zephyr port.

Connections and IOs
====================

Peripherals on the CC3200 LaunchXL are mapped to the following pins in
the file :file:`boards/arm/cc3200_launchxl/pinmux.c`.

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
:file:`boards/arm/cc3200_launchxl/cc3200_launchxl_defconfig`.


Programming and Debugging
*************************

Build
=====

Follow the :ref:`getting_started` instructions for Zephyr application
development.

To build for the CC3200 LaunchXL:

.. code-block:: console

   % make BOARD=cc3200_launchxl

This will produce both a zephyr.elf  (for GDB debugging) and a
zephyr.bin file (for flashing) in the outdir/cc3200_launchxl/
subdirectory.

FTDI USB Setup
==============

The CC3200 LaunchXL has onboard FTDI based JTAG emulation with serial
port for Flash programming.

Under Linux, plugging the CC3200 USB cable into the PC should result in
a new USB device showing up at /dev/ttyUSB1.  If not, it may be
necessary to create a udev rule to load the FTDI Linux driver for the
TI vendor and product ID.  For example:

.. code-block:: console

   % cat /etc/udev/rules.d/98-usbftdi.rules
   SUBSYSTEM=="usb", ATTRS{idVendor}=="0451", ATTRS{idProduct}=="c32a", MODE="0660", GROUP="dialout", RUN+="/sbin/modprobe ftdi-sio" RUN+="/bin/sh -c '/bin/echo 0451 c32a > /sys/bus/usb-serial/drivers/ftdi_sio/new_id'"

To ensure access to the usb ftdi device without having to be root, add
yourself to the dialout group:

.. code-block:: console

   % sudo usermod -a -G dialout <username>

Once the USB cable is connected to your host PC running Linux, one
should see something like:

.. code-block:: console

    % dmesg -t
    usb 1-2: new full-speed USB device number 32 using ohci-pci
    usb 1-2: New USB device found, idVendor=0451, idProduct=c32a
    usb 1-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
    usb 1-2: Product: USB <-> JTAG/SWD
    usb 1-2: Manufacturer: FTDI
    usb 1-2: SerialNumber: cc3101
    ftdi_sio 1-2:1.0: FTDI USB Serial Device converter detected
    usb 1-2: Detected FT2232C
    usb 1-2: Number of endpoints 2
    usb 1-2: Endpoint 1 MaxPacketSize 64
    usb 1-2: Endpoint 2 MaxPacketSize 64
    usb 1-2: Setting MaxPacketSize 64
    usb 1-2: FTDI USB Serial Device converter now attached to ttyUSB0
    ftdi_sio 1-2:1.1: FTDI USB Serial Device converter detected
    usb 1-2: Detected FT2232C
    usb 1-2: Number of endpoints 2
    usb 1-2: Endpoint 1 MaxPacketSize 64
    usb 1-2: Endpoint 2 MaxPacketSize 64
    usb 1-2: Setting MaxPacketSize 64
    usb 1-2: FTDI USB Serial Device converter now attached to ttyUSB1
    ftdi_sio ttyUSB0: failed to get modem status: -110

    % ls -l /dev/ttyUSB1
    crw-rw---- 1 root dialout  /dev/ttyUSB1

.. note::
   The ttyUSB1 device is used for UART0 output.  ttyUSB0 is not used.


Flashing
========

The CC3200 has no integrated internal flash, but has 1Mb external serial
flash for storing program images and other files.  Upon reset, the TI
bootloader copies the program from serial flash into RAM, and then
transfers control to the program.

The `CC3200 Programmer's Guide`_ provides instructions for development
using the `CC3200 SDK`_ with Windows platforms, including how to flash
the zephyr.bin binary onto the board.

For Windows:
------------

See Section 5.4 of the `CC3200 Programmer's Guide`_; or, follow the
directions per the `UniFlash Quick Start Guide`_. Please be sure to use
UniFlash version 3.4.1.

For Linux:
----------

An option for flashing the CC3200 LaunchXL on Linux is the
`cc3200tool`_.  See the README there for build/install/usage.

The following command has been known to work:

.. code-block:: console

   % cc3200tool -p /dev/ttyUSB1 --reset prompt write_file zephyr.bin
   /sys/mcuimg.bin

.. note:: You will need to manually insert a jumper on SOP2 (J15) for
   flashing and remove the jumper for execution.

Debugging
=========

The `CC3200 SDK`_ supports debugging using GDB (for ARM) over OpenOCD,
and includes the necessary OpenOCD CFG and sample gdbinit scripts.

See Section 5.3.3.5 of the `CC3200 Programmer's Guide`_.

To see program output from UART0, one can execute in a separate terminal
window:

.. code-block:: console

  % screen /dev/ttyUSB1 115200 8N1

.. note:: The bootloader takes the first 16Kb of the 256Kb RAM, so the
   Zephyr application starts at 0x20004000.  The Zephyr CC3200
   configuration thus sets the max SRAM size to 240Kb.


References
**********

CC32xx Wiki:
    http://processors.wiki.ti.com/index.php/CC31xx_%26_CC32xx

TI CC3200 Product Page:
    http://www.ti.com/product/cc3200

.. _CC3200 TRM:
   http://www.ti.com/lit/pdf/swru367

.. _CC3200 Programmer's Guide:
   http://www.ti.com/lit/pdf/swru369

.. _UniFlash Quick Start Guide:
   http://processors.wiki.ti.com/index.php/CC31xx_%26_CC32xx_UniFlash_Quick_Start_Guide

.. _cc3200tool:
   https://github.com/ALLTERCO/cc3200tool

.. _CC3200 SDK:
   http://www.ti.com/tool/cc3200sdk

.. _CC3200 LaunchXL User's Guide:
   http://www.ti.com/lit/pdf/swru372
