.. _arduino_101:

Board Configuration: arduino 101
################################

Overview
********

Developers can use the arduino_101 board configuration to build a
Zephyr Kernel that can be flashed and run on the Arduino 101 platform.

This board configuration enables kernel support for the board's Quark SoC.

Board Layout
************

General information for the board can be found at the `Arduino website`_,
which includes both `schematics`_ and EAGLE files.  The site also contains
some details on the board power.

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

* The zflash utility

   1. git clone https://oic-review.01.org/gerrit/a/zflash
   2. cd zflash
   3. sudo make install

.. note::
   The zflash tool, while currently the method to flash devices, may not be
   the final method.  Please do not distribute this tool any further.

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
requires updating via a USB flashing method that is not currently supported by
Zephyr.

Typically Arduino hardware can re-program the Bootloader through connecting
the ICSP header and issuing the "Burn Bootloader" option from the Arduino IDE.
On the Arduino 101, this option is not currently functional.

#. Make sure the Zephyr SDK has been installed on your platform.
   Specifically ensure that openocd is available to you.  This will
   depend upon where you decided to install the SDK; by default the path
   is: :file:`/usr/local/zflash/openocd/bin/openocd`.

#. Open two terminal windows

#. In terminal window 1, change directories to where you wish the binary ROM
   files to be saved.

#. In termminal window 1, enter:

   .. code-block:: console

      $ sudo zflash -b arduino_101 -d

      .. note::

      The zflash tool requires a file to be included when attempting to enable
      the debug server.

   Once started, openocd should stay running in the window.

#. In terminal window 2, enter:

   .. code-block:: console

      $ telnet localhost 4444
      Trying 127.0.0.1...
      Connected to localhost.
      Escape character is '^]'.
      Open On-Chip Debugger

#. Save the boot ROM.  In this case, the variable :var:`$ROM` can be
   replaced with the name of the file you wish to save the image as (e.g.
   rom.bin).

   .. code-block:: console

     > dump_image $ROM 0xffffe000 8192
     dumped 8192 bytes in 0.092147s (86.818 KiB/s)

#. Save the current ARC image.  In this case, the variable
   :var:`$ARC` can be repalced with the name of the file you wish to save the
   image as (e.g. arc.bin)

   .. code-block:: console

     > dump_image $ARC 0x40000000 196608
     dumped 196608 bytes in 2.205578s (87.052 KiB/s)

#. Save the current X86 image.  In this case, the variable
   :var:`$X86` can be repalced with the name of the file you wish to save the
   image as (e.g. x86.bin)

   .. code-block:: console

     > dump_image $X86 0x40030000 196608
     dumped 196608 bytes in 2.205578s (86.836 KiB/s)

#. At this point you have now created a backup for the Arduino 101.

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

   .. code-block:: console

      zflash -r -b arduino_101 $ZFLASH_ROOT/test/quark_se/quark_se_rom.bin

Flashing an ARC Kernel
======================

# Make sure the binary image has been built.

   .. code-block:: console

      $ cd $ZEPHYR_HOME

      $ make -C
      samples/nanonkernel/apps/hello_world BOARD=arduino_101_sss ARCH=arc

.. note::

   When building for the ARC processor, the board type is listed as
   arduino_101_sss, and the ARCH type is set to arc.

   .. code-block:: console

      $ zflash -c -b arduino_101
      samples/nanokernel/apps/hello_world/outdir/zephyr.bin

.. note::

   When flashing an ARC kernel, zflash REQUIRES the use of the
   :option:`-c` flag.

Congratulations you have now flashed the hello_world image to the ARC
processor.

Flashing an x86 Kernel
======================

# Make sure the binary image has been built.

   .. code-block:: console

      $ cd $ZEPHYR_HOME
      $ make -C samples/nanonkernel/apps/hello_world BOARD=arduino_101

.. note::

   When building for the x86 processor, the board type is
   listed as arduino_101, and no ARCH flag needs to be set.

   .. code-block:: console

     $ zflash -b
     arduino_101 samples/nanokernel/apps/hello_world/outdir/zephyr.bin

.. note::

   When flashing an x86 image, zflash does NOT require the :option:`-c`
   flag to be used.

Congratulations you have now flashed the hello_world image to the x86
processor.

Debugging on Arduino 101
========================

The image file used for debugging must be built to the corresponding
architecture that you wish to debug. For example, the binary must be built
for ARCH=x86 if you wish to debug on the x86 core.

1. Build the binary for your application on the architecture you wish to
   debug.  Alternatively, use the instructions above as template for testing.

   When debugging on ARC, you will want to enable the ARC_INIT_DEBUG
   configuration option in your x86 PRJ file.  Details of this flag can be
   found in :file:`arch/x86/soc/quark_se/Kconfig`.  Setting this variable will
   force the ARC processor to halt on bootstrap, giving the debugger a chance
   at connecting and controlling the hardware.

    This can be done by editing the
    :file:`samples/nanokernel/app/hello_world/prj_x86.conf` to include:

   .. code-block:: console

      CONFIG_ARC_INIT=y
      CONFIG_ARC_INIT_DEBUG=y

2. Open two terminal windows

3. In terminal window 1, type:

   .. code-block:: console

     $ sudo zflash -b arduino_101 -d

   Once started, openocd should stay running in the window.

4. Start GDB in terminal window 2

   .. note::

      While debugging on ARC, it will be necessary to use a version of gdb that
      understands ARC binaries.  Currently this resides in the Zephyr SDK at
      :envvar:`$ZEPHYR_SDK_INSTALL_DIR`
      :file:`/sysroots/i686-pokysdk-linux/usr/bin/arc-poky-elf/arc-poky-elf-gdb`.

      It is suggested to create an alias in your shell to run this command,
      such as:

   .. code-block:: console

      alias arc_gdb= "$ZEPHYR_SDK_INSTALL_DIR/sysroots/i686-pokysdk-
      linux/usr/bin/arc-poky-elf/arc-poky-elf-gdb"

   * To debug on x86:

       .. code-block:: console

         $ gdb $ZEPHYR_HOME/samples/nanokernel/apps/hello_world/outdir/zephyr.bin
         gdb$  target remote :3334

   * To debug on ARC will require some extra steps and a third terminal:

   On Terminal 2:

      .. code-block:: console

         $ arc_gdb $ZEPHYR_HOME/samples/nanokernel/apps/hello_world/outdir/zephyr.bin
         gdb$  target remote :3333

   At this point you may set the breakpoint needed in the code/function.

   On Terminal 3 connect to the X86 side:

      .. code-block:: console

         $ gdb
         gdb$  target remote :3334
         gdb$  continue

   The :code`continue` on the X86 side is needed as the ARC_INIT_DEBUG flag has
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

.. _ARM-JTAG-20-10:
   http://www.amazon.com/gp/product/
   B009UEO9ZY/ref=oh_aui_detailpage_o04_s00?ie=UTF8&psc=1
