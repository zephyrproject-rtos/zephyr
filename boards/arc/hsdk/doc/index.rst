.. _hsdk:

DesignWare(R) ARC(R) HS Development Kit
########################################

Overview
********

The DesignWare(R) ARC(R) HS Development Kit is a ready-to-use platform for
rapid software development on the ARC HS3x family of processors. It supports
single- and multi-core ARC HS34, HS36 and HS38 processors and offers a wide
range of interfaces including Ethernet, WiFi, Bluetooth, USB, SDIO, I2C, SPI,
UART, I2S, ADC, PWM and GPIO. A Vivante GPU is also contained in the ARC
Development System SoC. This allows developers to build and debug complex
software on a comprehensive hardware platform

.. image:: ./hsdk.jpg
   :width: 442px
   :align: center
   :alt: DesignWare(R) ARC(R) HS Development Kit (synopsys.com)

For details about the board, see: `ARC HS Development Kit
(IoTDK) <https://www.synopsys.com/dw/ipdir.php?ds=arc-hs-development-kit>`__

Hardware
********

For hardware feature details, refer to :
`Designware HS Development Kit website`_.

Programming and Debugging
*************************

Required Hardware and Software
==============================

To use Zephyr RTOS applications on the HS Development Kit board, a few
additional pieces of hardware are required.

* A micro USB cable provides USB-JTAG debug and USB-UART communication
  to the board

* A universal switching power adaptor (110-240V
  AC to 12V DC), provided in the package, provides power to the board.

* :ref:`The Zephyr SDK <zephyr_sdk>`

* Terminal emulator software for use with the USB-UART. Suggestion:
  `Putty Website`_.

* (optional) A collection of Pmods, Arduino modules, or Mikro modules.
  See `Digilent Pmod Modules`_ or develop your custom interfaces to attach
  to the Pmod connector.

Set up the ARC HS Development Kit
==================================

To run Zephyr application on IoT Development Kit, you need to
set up the board correctly.

* Connect the digilent USB cable from your host to the board.

* Connect the 12V DC power supply to your board

Set up Zephyr Software
======================

Building Sample Applications
==============================

You can try many of the :ref:`sample applications and demos
<samples-and-demos>`.  We'll use :ref:`hello_world`, found in
:zephyr_file:`samples/hello_world` as an example.

Configuring
-----------

You may need to write a prj_arc.conf file if the sample doesn't have one.
Next, you can use the menuconfig rule to configure the target. By specifying
``hsdk`` as the board configuration, you can select the ARC HS Development
Kit board support for Zephyr.

.. zephyr-app-commands::
   :board: hsdk
   :zephyr-app: samples/hello_world
   :goals: menuconfig


Building
--------

You can build an application in the usual way.  Refer to
:ref:`build_an_application` for more details. Here is an example for
:ref:`hello_world`.

.. zephyr-app-commands::
   :board: hsdk
   :zephyr-app: samples/hello_world
   :maybe-skip-config:
   :goals: build


Connecting Serial Output
=========================

In the default configuration, Zephyr's HS Development Kit images support
serial output via the USB-UART on the board.  To enable serial output:

* Open a serial port emulator (i.e. on Linux minicom, putty, screen, etc)

* Specify the tty driver name, for example, on Linux this may be
  :file:`/dev/ttyUSB0`

* Set the communication settings to:


========= =====
Parameter Value
========= =====
Baud:     115200
Data:     8 bits
Parity:    None
Stopbits:  1
========= =====

Debugging
==========

Using the latest version of Zephyr SDK(>=0.10), you can debug and
flash (run) HS Development Kit directly.

One option is to build and debug the application using the usual
Zephyr build system commands.

.. zephyr-app-commands::
   :board: hsdk
   :app: <my app>
   :goals: debug

At this point you can do your normal debug session. Set breakpoints and then
:kbd:`c` to continue into the program.

The other option is to launch a debug server, as follows.

.. zephyr-app-commands::
   :board: hsdk
   :app: <my app>
   :goals: debugserver

Then connect to the debug server at the HS Development Kit from a second
console, from the build directory containing the output :file:`zephyr.elf`.

.. code-block:: console

   $ cd <my app>
   $ $ZEPHYR_SDK_INSTALL_DIR/arc-zephyr-elf/arc-zephyr-elf-gdb zephyr.elf
   (gdb) target remote localhost:3333
   (gdb) load
   (gdb) b main
   (gdb) c

Flashing
========

If you just want to download the application to the HS Development Kit's DDR
and run, you can do so in the usual way.

.. zephyr-app-commands::
   :board: hsdk
   :app: <my app>
   :goals: flash

This command still uses openocd and gdb to load the application elf file to
HS Development Kit, but it will load the application and immediately run. If
power is removed, the application will be lost since it wasn't written to flash.

Most of the time you will not be flashing your program but will instead debug
it using openocd and gdb. The program can be download via the USB cable into
the code and data memories.

The HS Development Kit also supports flashing the Zephyr application
with the U-Boot bootloader, a powerful and flexible tool for loading
an executable from different sources and running it on the target platform.

The U-Boot implementation for the HS Development Kit was further extended with
additional functionality that allows users to better manage the broad
configurability of the HS Development Kit

When you are ready to deploy the program so that it boots up automatically on
reset or power-up, you can follow the steps to place the program on SD card.

For details, see: `Uboot-HSDK-Command-Reference
<https://github.com/foss-for-synopsys-dwc-arc-processors/linux/wiki/Uboot-HSDK-Command-Reference#launching-baremetal-application-on-hsdk>`__


Release Notes
*************

References
**********

.. _embARC website: https://www.embarc.org

.. _Designware HS Development Kit website: <https://www.synopsys.com/dw/ipdir.php?ds=arc_hs_development_kit>`_

.. _Digilent Pmod Modules: http://store.digilentinc.com/pmod-modules

.. _Putty website: http://www.putty.org
