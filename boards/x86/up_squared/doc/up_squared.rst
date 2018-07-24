.. _up_squared:

UP Squared
##########

Overview
********

This board configuration enables kernel support for the `UP Squared`_ board,
along with the following devices:

* High Precision Event Timer (HPET)

* Serial Ports in Polling and Interrupt Driven Modes

* I2C

.. note::
   This board configuration works on all three variants of `UP Squared`_
   boards containing Intel |reg| Pentium |trade| SoC,
   Intel |reg| Celeron |trade| SoC, or Intel |reg| Atom |trade| SoC.

.. note::
   This board configuration works only with the default BIOS settings.
   Enabling/disabling LPSS devices in BIOS (under Advanced -> HAT Configurations)
   will change the MMIO addresses of these devices, and will prevent
   the drivers from communicating with these devices. For drivers that support
   PCI enumeration, :option:`CONFIG_PCI` and :option:`CONFIG_PCI_ENUMERATION`
   will allow these drivers to probe for the correct MMIO addresses.

Hardware
********

General information about the board can be found at the `UP Squared`_ website.

Supported Features
==================

This board supports the following hardware features:

* HPET

* Advanced Programmed Interrupt Controller (APIC)

* Serial Ports in Polling and Interrupt Driven Modes

+-----------+------------+-----------------------+-----------------+
| Interface | Controller | Driver/Component      | PCI Enumeration |
+===========+============+=======================+=================+
| HPET      | on-chip    | system clock          | Not Supported   |
+-----------+------------+-----------------------+-----------------+
| APIC      | on-chip    | interrupt controller  | Not Supported   |
+-----------+------------+-----------------------+-----------------+
| UART      | on-chip    | serial port-polling;  | Supported       |
|           |            | serial port-interrupt |                 |
+-----------+------------+-----------------------+-----------------+
| I2C       | on-chip    | I2C controller        | Supported       |
+-----------+------------+-----------------------+-----------------+

The Zephyr kernel currently does not support other hardware features.

Serial Port Polling Mode Support
--------------------------------

The polling mode serial port allows debug output to be printed.

Serial Port Interrupt Mode Support
----------------------------------

The interrupt mode serial port provides general serial communication
and external communication.

Interrupt Controller
--------------------

This board uses the kernel's static Interrupt Descriptor Table (IDT) to program the
Advanced Programmable Interrupt Controller (APIC) interrupt redirection table.


+-----+---------+--------------------------+
| IRQ | Remarks | Used by Zephyr Kernel    |
+=====+=========+==========================+
| 2   | HPET    | timer driver             |
+-----+---------+--------------------------+
| 4   | UART_0  | serial port when used in |
|     |         | interrupt mode           |
+-----+---------+--------------------------+
| 5   | UART_1  | serial port when used in |
|     |         | interrupt mode           |
+-----+---------+--------------------------+
| 27  | I2C_0   | I2C DW driver            |
+-----+---------+--------------------------+
| 28  | I2C_1   | I2C DW driver            |
+-----+---------+--------------------------+
| 29  | I2C_2   | I2C DW driver            |
+-----+---------+--------------------------+
| 30  | I2C_3   | I2C DW driver            |
+-----+---------+--------------------------+
| 31  | I2C_4   | I2C DW driver            |
+-----+---------+--------------------------+
| 32  | I2C_5   | I2C DW driver            |
+-----+---------+--------------------------+
| 33  | I2C_6   | I2C DW driver            |
+-----+---------+--------------------------+
| 34  | I2C_7   | I2C DW driver            |
+-----+---------+--------------------------+

HPET System Clock Support
-------------------------

The SoC uses HPET timing with legacy-free timer support. The board
configuration uses HPET as a system clock timer.

Connections and IOs
===================

Refer to the `UP Squared`_ website and `UP Squared Pinout`_ website
for connection diagrams.

Memory Mappings
===============

This board configuration uses default hardware memory map
addresses and sizes.

Programming and Debugging
*************************

Use the following procedures for booting an image on a UP Squared board.

* `Creating a GRUB2 Boot Loader Image from a Linux Host`_

* `Preparing the Boot Device`_

* `Booting the UP Squared Board`_

Creating a GRUB2 Boot Loader Image from a Linux Host
====================================================

If you are having problems running an application using the preinstalled
copy of GRUB, follow these steps to test on supported boards using a custom GRUB.

#. Install the requirements to build GRUB on your host machine.

   On Ubuntu, type:

   .. code-block:: console

      $ sudo apt-get install bison autoconf libopts25-dev flex automake

   On Fedora, type:

   .. code-block:: console

     $ sudo dnf install gnu-efi bison m4 autoconf help2man flex \
        automake texinfo

#. Clone and build the GRUB repository using the script in Zephyr tree, type:

   .. code-block:: console

     $ cd $ZEPHYR_BASE
     $ ./boards/x86/common/scripts/build_grub.sh x86_64

#. Find the binary at
   :file:`$ZEPHYR_BASE/boards/x86/common/scripts/grub/bin/grub_x86_64.efi`.

Preparing the Boot Device
=========================

Prepare a USB flash drive to boot the Zephyr application image on
a UP Squared board.

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application on UP Squared:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: up_squared
      :goals: build

   .. note::

      A stripped project image file named :file:`zephyr.strip` is automatically
      created in the build directory after the application is built. This image
      has removed debug information from the :file:`zephyr.elf` file.

#. Refer to the `UP Squared Serial Console Wiki page
   <https://wiki.up-community.org/Serial_console>`_ for instructions on how to
   connect for serial console.

#. Format the USB flash drive as FAT32.

   On Windows, open ``File Explorer``, and right-click on the USB flash drive.
   Select ``Format...``. Make sure in ``File System``, ``FAT32`` is selected.
   Click on the ``Format`` button and wait for it to finish.

   On Linux, graphical utilities such as ``gparted`` can be used to format
   the USB flash drive as FAT32. Alternatively, under terminal, find out
   the corresponding device node for the USB flash drive (for example,
   ``/dev/sdd``). Execute the following command:

   .. code-block:: console

      mkfs.vfat -F 32 <device-node>

   .. important::
      Make sure the device node is the actual device node for
      the USB flash drive. Or else you may erase other storage devices
      on your system, and will render the system unusable afterwards.

#. Create the following directories

   :file:`efi`

   :file:`efi/boot`

   :file:`kernel`

#. Copy the kernel file :file:`zephyr/zephyr.strip` to the :file:`$USB/kernel` folder.

#. Copy your built version of GRUB to :file:`$USB/efi/boot/bootx64.efi`

#. Create :file:`$USB/efi/boot/grub.cfg` containing the following:

   .. code-block:: console

      set default=0
      set timeout=10

      menuentry "Zephyr Kernel" {
         multiboot /kernel/zephyr.strip
      }

Booting the UP Squared Board
============================

Boot the UP Squared board from the boot device using GRUB2 via USB flash drive.

Steps
-----

1. Insert the prepared boot device (USB flash drive) into the UP Squared board.

2. Connect the board to the host system using the serial cable and
   configure your host system to watch for serial data.  See
   https://wiki.up-community.org/Serial_console.

   .. note::
      On Windows, PuTTY has an option to set up configuration for
      serial data.  Use a baud rate of 115200.

3. Power on the UP Squared board.

4. When the following output appears, press :kbd:`F7`:

   .. code-block:: console

     Press <DEL> or <ESC> to enter setup.

5. From the menu that appears, select the menu entry that describes
   that particular type of USB flash drive.

   GRUB2 starts and a menu shows entries for the items you added
   to the file :file:`grub.cfg`.

6. Select the image you want to boot and press :guilabel:`Enter`.

   When the boot process completes, you have finished booting the
   Zephyr application image.

   .. note::
      You can safely ignore this message if it appears:

      .. code-block:: console

         WARNING: no console will be available to OS


.. _UP Squared: http://www.up-board.org/upsquared/

.. _UP Squared Pinout: https://wiki.up-community.org/Pinout
