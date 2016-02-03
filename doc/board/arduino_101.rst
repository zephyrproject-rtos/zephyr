.. _arduino_101:

Board Configuration: arduino 101
################################

Overview
********

The Arduino 101 board is an Arduino product that uses the Intel Quark SE
processor.  As an unsupported configuration, the Zephyr Project is able to be
flashed to an Arduino 101 for experimentation and testing purposes.

As the Quark SE contains both an ARC and an X86 core, a developer will need to
flash an ARC and an X86 image to use both.  Developers can use the
**arduino_101** and **arduino_101_sss** board configurations to build a Zephyr
Kernel that can be flashed and run on the Arduino 101 platform.  The default
configuration for Arduino 101 boards can be found in
:file:`boards/arduino_101/arduino_101_defconfig` for the X86 and
:file:`boards/arduino_101/arduino_101_sss_defconfig` for the ARC.

Board Layout
************

General information for the board can be found at the `Arduino website`_,
which includes both `schematics`_ and BRD files.

Supported Features
******************

The Zephyr kernel supports multiple hardware features on the Arduino 101
through the use of drivers.  Some drivers are functional on the x86 side, some
only on the ARC side, and a few functional on both sides.  The below table
breaks down which drivers and functionality can be found on which architectures:

+-----------+------------+-----+-----+-----------------------+
| Interface | Controller | ARC | x86 | Driver/Component      |
+===========+============+=====+=====+=======================+
| APIC      | on-chip    |  N  |  Y  | interrupt_controller  |
+-----------+------------+-----+-----+-----------------------+
| UART      | on-chip    |  N  |  Y  | serial port-polling;  |
|           |            |     |     | serial port-interrupt |
+-----------+------------+-----+-----+-----------------------+
| SPI       | on-chip    |  Y  |  Y  | spi                   |
+-----------+------------+-----+-----+-----------------------+
| ADC       | on-chip    |  Y  |  N  | adc                   |
+-----------+------------+-----+-----+-----------------------+
| I2C       | on-chip    |  Y  |  Y  | i2c                   |
+-----------+------------+-----+-----+-----------------------+
| GPIO      | on-chip    |  Y  |  Y  | gpio                  |
+-----------+------------+-----+-----+-----------------------+
| PWM       | on-chip    |  Y  |  Y  | pwm                   |
+-----------+------------+-----+-----+-----------------------+
| mailbox   | on-chip    |  Y  |  Y  | ipm                   |
+-----------+------------+-----+-----+-----------------------+

Flashing Arduino 101 for Zephyr
*******************************

For the use of this tutorial, the sample application hello_world will be
used found in :file:`$ZEPHYR_BASE/samples/nanokernel/apps/hello_world`.

Use the following procedures for booting an image on a Arduino 101 board.

.. contents:: Procedures
   :depth: 1
   :local:
   :backlinks: entry

Required Hardware and Software
==============================

Before flashing the Zephyr kernel onto an Arduino 101, a few additional
pieces of hardware are required.

* `FlySwatter2 JTAG debugger`_

* ARM Micro JTAG Connector, Model: `ARM-JTAG-20-10`_

* While using the USB port for power does work, it is recommended that
  the 7V-12V barrel connector be used when working with the JTAG connector.

* :ref:`The Zephyr SDK <zephyr_sdk>`

Connecting JTAG to Arduino 101
==============================

#. Connect the ARM Micro JTAG Connector to the FlySwatter2.

#. Locate the micro JTAG connector on the Arduino 101 board.  It can be found
   adjacent to the SCL and SDA pins in the Arduino headers. Next to the micro
   JTAG header is small white dot that signals the location of pin 0 on the
   header.

#. Connect the FlySwatter2 to the Arduino 101 micro JTAG connector.

#. Ensure that both the cable and header pin 0 locations line up. The cable
   from the ARM Micro JTAG connector uses a red wire on the cable to denote
   which end on the cable has the pin 0.

#. Plug the USB Type B cable into the FlySwatter2 and your computer. On
   Linux, you should see something similar to the following in your dmesg:

    .. code-block:: console

      usb 1-2.1.1: new high-speed USB device number 13 using xhci_hcd
      usb 1-2.1.1: New USB device found, idVendor=0403, idProduct=6010
      usb 1-2.1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
      usb 1-2.1.1: Product: Flyswatter2
      usb 1-2.1.1: Manufacturer: TinCanTools
      usb 1-2.1.1: SerialNumber: FS20000
      ftdi_sio 1-2.1.1:1.0: FTDI USB Serial Device converter detected
      usb 1-2.1.1: Detected FT2232H
      usb 1-2.1.1: FTDI USB Serial Device converter now attached to ttyUSB0
      ftdi_sio 1-2.1.1:1.1: FTDI USB Serial Device converter detected
      usb 1-2.1.1: Detected FT2232H
      usb 1-2.1.1: FTDI USB Serial Device converter now attached to ttyUSB1

Making a Backup
===============

Before continuing, it is worth considering the creation of a backup
image of the ROM device as it stands today.  This would be necessary if you
ever decide to run Arduino sketches on the hardware again as the Arduino IDE
requires updating via a USB flashing method that is not currently supported
by Zephyr.

Typically Arduino hardware can re-program the Bootloader through connecting
the ICSP header and issuing the "Burn Bootloader" option from the Arduino
IDE.  On the Arduino 101, this option is not currently functional.

#. Make sure the Zephyr SDK has been installed on your platform.

#. Open a terminal window

#. Source the :file:`zephyr-env.sh` file.

#. Change directories to :file:`$ZEPHYR_BASE`.

#. In the termminal window , enter:

  .. code-block:: console

     $ sudo -E ./boards/arduino_101/support/arduino_101_backup.sh

  .. note::

  This will cause the system to dump two files in your ZEPHYR_BASE:
  A101_BOOT.bin and A101_OS.bin.  These contain copies of the original
  flash that can be used to restore the state to factory conditions.

At this point you have now created a backup for the Arduino 101.

Restoring a Backup
==================

#. Make sure the Zephyr SDK has been installed on your development
   environment.

#. Open a terminal window

#. Source the :file:`zephyr-env.sh` file.

#. Change directories to $ZEPHYR_BASE.

#. In the termminal window , enter:

  .. code-block:: console

     $ sudo -E ./boards/arduino_101/support/arduino_101_restore.sh

  .. note::

  This script expects two files in your :file:`$ZEPHYR_BASE` with titles of
  :file:`A101_OS.bin` and :file:`A101_BOOT.bin`.

Flashing an Application to Arduino 101
======================================

By default, the Arduino 101 comes with an X86 and ARC image ready to run.  Both
images can be replaced by Zephyr OS images following the steps below.  In cases
where only an X86 image is needed or wanted it is important to disable the
ARC processor, as the X86 OS will appear to hang waiting for the ARC processor.

Details on how to disable the ARC can be found in the Debugging on Arduino 101
section.

Flashing the ROM
================

The default boot ROM used by the Arduino 101 requires that any binary to run
be authorized.  Currently the Zephyr project is not supported by this ROM.  To
work around this requirement, an alternative boot ROM has been created that
needs to be flashed just one time.  To flash a Zephyr compatible boot ROM, use
zflash to flash the :file:`quark_se_rom.bin` to the board.

.. note::
    This will cause the Arduino 101 board to no longer run an Arduino sketch
    or work with the Arduino IDE.

#. Source the :file:`zephyr-env.sh` file.

#. Change directories to $ZEPHYR_BASE.

#. The Zephyr Project has included a pre-compiled version of a bootloader for
   general use on the Arduino 101.  Details about how to build your own
   bootloader can be found in the
   :file:`$ZEPHYR_BASE/boards/arduino_101/support/README`

   .. code-block:: console

      $ sudo -E ./boards/arduino_101/support/flash-rom.sh

   This script will flash the boot rom located in
   :file:`$ZEPHYR_BASE/boards/arduino_101/support/quark_se_rom.bin` to the
   Arduino 101 device, overwriting the original shipping ROM.

Flashing an ARC Kernel
======================

# Make sure the binary image has been built.  Change directories to your local
checkout copy of Zephyr, and run:

   .. code-block:: console

      $ source ./zephyr-env.sh
      $ cd $ZEPHYR_BASE/samples/nanokernel/apps/hello_world

      $ make pristine && BOARD=arduino_101_sss ARCH=arc
      $ make BOARD=arduino_101_sss flash

.. note::

   When building for the ARC processor, the board type is listed as
   arduino_101_sss and the ARCH type is set to arc.


Congratulations you have now flashed the hello_world image to the ARC
processor.

Flashing an x86 Kernel
======================

# Make sure the binary image has been built.

   .. code-block:: console

      $ source ./zephyr-env.sh
      $ cd $ZEPHYR_BASE/samples/nanokernel/apps/hello_world
      $ make pristine && BOARD=arduino_101 ARCH=x86
      $ make BOARD=arduino_101 flash

.. note::

   When building for the x86 processor, the board type is listed as
   arduino_101 and the ARCH type is set to x86.

Congratulations you have now flashed the hello_world image to the x86
processor.

Debugging on Arduino 101
========================

The image file used for debugging must be built to the corresponding
architecture that you wish to debug. For example, the binary must be built
for ARCH=x86 if you wish to debug on the x86 core.

1. Build the binary for your application on the architecture you wish to
   debug.  Alternatively, use the instructions above as template for testing.

   When debugging on ARC, you will need to enable the ARC_INIT_DEBUG
   configuration option in your X86 PRJ file.  Details of this flag can be
   found in :file:`arch/x86/soc/quark_se/Kconfig`.  Setting this variable will
   force the ARC processor to halt on bootstrap, giving the debugger a chance
   at connecting and controlling the hardware.

    This can be done by editing the
    :file:`samples/nanokernel/apps/hello_world/prj.conf` to include:

   .. code-block:: console

      CONFIG_ARC_INIT=y
      CONFIG_ARC_INIT_DEBUG=y

   .. note::

       By enabling CONFIG_ARC_INIT, you ::MUST:: flash both an ARC and an X86
       image to the hardware.  If you do not, the X86 image will appear to hang
       at boot while it is waiting for the ARC to finish initialization.

2. Open two terminal windows

3. In terminal window 1, type:

  .. code-block:: console

    $ cd $ZEPHYR_BASE/samples/nanokernel/apps/hello_world
    $ make BOARD=arduino_101 debug

  These commands will start an openocd session that creates a local telnet
  server (on port 4444 for direct openocd commands to be issued), and a
  gdbserver (for gdb access).  The command should not return to a command line
  interface until you are done debugging, at which point you can press Cntl-C
  to shutdown everything.

4. Start GDB in terminal window 2


   * To debug on x86:

       .. code-block:: console

         $ cd $ZEPHYR_BASE/samples/nanokernel/apps/hello_world
         $ gdb outdir/zephyr.elf
         gdb$  target remote :3333

   * To debug on ARC:

     ARC debugging will require some extra steps and a third terminal.  It is
     necessary to use a version of gdb that understands ARC binaries.
     Thankfully one is provided with the Zephyr SDK at
     :envvar:`$ZEPHYR_SDK_INSTALL_DIR`
     :file:`/sysroots/i686-pokysdk-linux/usr/bin/arc-poky-elf/arc-poky-elf-gdb`.

     It is suggested to create an alias in your shell to run this command,
     such as:

     .. code-block:: console

        alias arc_gdb= "$ZEPHYR_SDK_INSTALL_DIR/sysroots/i686-pokysdk-
        linux/usr/bin/arc-poky-elf/arc-poky-elf-gdb"

     a) On Terminal 2:

       .. code-block:: console

          $ cd $ZEPHYR_BASE/samples/nanokernel/apps/hello_world
          $ arc_gdb outdir/zephyr.elf
          gdb$  target remote :3334

     At this point you may set the breakpoint needed in the code/function.

     b) On Terminal 3 connect to the X86 side:

      .. code-block:: console

         $ gdb
         gdb$  target remote :3333
         gdb$  continue

   .. note::
     In previous versions of the SDK, the gdbserver remote ports were reversed.
     The gdb ARC server port was 3333 and the X86 port was 3334.  As of SDK
     v0.7.2, the gdb ARC server port is 3334, and the X86 port is 3333.

   The :code:`continue` on the X86 side is needed as the ARC_INIT_DEBUG flag has
   been set and halts the X86 until the ARC core is ready.  Ready in this case
   is defined as openocd has had a chance to connect, setup registers, and any
   breakpoints.  Unfortunately, there exists no automated method for notifying
   the X86 side that openocd has connected to the ARC at this time.

   Once you've started the X86 side again, and have configured any debug
   stubs on the ARC side, you will need to have gdb issue the continue
   command for the ARC processor to start.


Arduino 101 Pinout
******************

When using the Zephyr kernel, the pinout mapping for the Arduino 101 becomes a
little more complicated.  The table below details which pins in Zephyr map to
those on the Arduino 101 board for control.  Full details of the pinmux
implementation, what valid options can be configured, and where things map can
be found in the :file:`boards/arduino_101/pinmux.c`.


+-------------+----------+------------+
| Arduino Pin | Function | Zephyr Pin |
+=============+==========+============+
| IO-0        | UART1-RX |     17     |
+-------------+----------+------------+
| IO-1        | UART1-TX |     16     |
+-------------+----------+------------+
| IO-2        | GPIO     |     52     |
+-------------+----------+------------+
| IO-3        | GPIO     |     51     |
|             |          |     63     |
+-------------+----------+------------+
| IO-4        | GPIO     |     53     |
+-------------+----------+------------+
| IO-5        | GPIO     |     49     |
|             |          |     64     |
+-------------+----------+------------+
| IO-6        | PWM2     |     65     |
+-------------+----------+------------+
| IO-7        | GPIO     |     54     |
+-------------+----------+------------+
| IO-8        | GPIO     |     50     |
+-------------+----------+------------+
| IO-9        | PWM3     |     66     |
+-------------+----------+------------+
| IO-10       | AIN0     |     0      |
+-------------+----------+------------+
| IO-11       | AIN3     |     3      |
+-------------+----------+------------+
| IO-12       | AIN1     |     1      |
+-------------+----------+------------+
| IO-13       | AIN2     |     2      |
+-------------+----------+------------+
| ADC0        | GPIO SS  |     10     |
+-------------+----------+------------+
| ADC1        | GPIO SS  |     11     |
+-------------+----------+------------+
| ADC2        | GPIO SS  |     12     |
+-------------+----------+------------+
| ADC3        | GPIO SS  |     13     |
+-------------+----------+------------+
| ADC4        | AIN14    |     14     |
+-------------+----------+------------+
| ADC5        | AIN9     |     9      |
+-------------+----------+------------+

.. note::
  IO3 and IO5 require both pins to be set for functionality changes.

Release Notes
*************

When debugging on ARC, it is important that the x86 core be started and
running BEFORE attempting to debug on ARC.  This is because the IPM console
calls will hang waiting for the x86 core to clear the communication.

Bibliography
************

.. _Arduino Website: https://www.arduino.cc/en/Main/ArduinoBoard101

.. _schematics: https://www.arduino.cc/en/uploads/Main/Arduino101Schematic.pdf

.. _FlySwatter2 JTAG debugger:
   http://www.tincantools.com/JTAG/Flyswatter2.html

.. _Intel Datasheet:
   http://www.intel.com/content/www/us/en/embedded/products/quark/mcu/se-soc/overview.html

.. _ARM-JTAG-20-10:
   http://www.amazon.com/gp/product/
   B009UEO9ZY/ref=oh_aui_detailpage_o04_s00?ie=UTF8&psc=1
