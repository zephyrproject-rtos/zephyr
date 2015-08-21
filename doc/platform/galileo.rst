.. _galileo:

Platform Configuration: galileo
###############################

Overview
********

The galileo platform configuration is used by Zephyr applications
that run on Galileo (Gen 1 or Gen 2) Development Boards.

It provides support for a Quark CPU and the following devices:

* High Precision Event Timer (HPET)

* Peripheral Component Interconnect (PCI) bus query

* Serial Ports in Polling and Interrupt Driven Modes

See `Procedures`_ for using third-party tools to load
a Zephyr application image on the target.

.. note::
   This platform configuration may work with similar boards,
   but they are not officially supported.

Supported Boards
****************

This section either provides links to, or describes the physical
characteristics of the boards that are supported by the galileo
platform configuration. Subsections provide information on pin names,
jumper settings, memory mappings and board component layout.

Pin Names
=========

Refer to page 46 of the Document Number: 329676-001US
Intel® Quark SoC X1000 Datasheet for a component layout
diagram. Click the link to open the `Intel® Galileo Datasheet`_.

.. _Intel® Galileo Datasheet: http://www.intel.com/newsroom/kits/quark/galileo/pdfs/Intel_Galileo_Datasheet.pdf

Also, refer to page 9 of the
`Intel® Galileo Board User Guide`_.

.. _Intel® Galileo Board User Guide: http://download.intel.com/support/galileo/sb/galileo_boarduserguide_330237_001.pdf

Jumpers & Switches
==================

The Zephyr kernel uses the Galileo default jumper settings except
for the IOREF jumper which must be set to match the external
operating voltage of either 3.3 V or 5 V.

The default switch settings for the Galileo are:

+--------------+--------------+
| Jumper       | Setting      |
+==============+==============+
| IOREF        | 3.3V or 5V   |
+--------------+--------------+
| VIN          | 5V  Jumpered |
+--------------+--------------+

For more information, refer to page 14 of the
`Intel® Galileo Board User Guide`_.

.. _Intel® Galileo Board User Guide: http://download.intel.com/support/galileo/sb/galileo_boarduserguide_330237_001.pdf

Memory Mappings
===============

The galileo platform configuration uses default hardware memory map
addresses and sizes.

For a list of memory mapped registers, see page 868 of the
`Intel® Quark SoC X1000 Datasheet`_.

.. _Intel® Quark SoC X1000 Datasheet: https://communities.intel.com/servlet/JiveServlet/previewBody/21828-102-2-25120/329676_QuarkDatasheet.pdf

Component Layout
================

Refer to page 3 of the Intel® Galileo Datasheet for a component layout
diagram. Click the link to open the `Intel® Galileo Datasheet`_.

.. _Intel® Galileo Datasheet: http://www.intel.com/newsroom/kits/quark/galileo/pdfs/Intel_Galileo_Datasheet.pdf

For a block diagram refer to page 38 of the `Intel® Quark SoC X1000 Datasheet`_.

.. _Intel® Quark SoC X1000 Datasheet: https://communities.intel.com/servlet/JiveServlet/previewBody/21828-102-2-25120/329676_QuarkDatasheet.pdf

Supported Features
******************

The galileo platform configuration supports the following
hardware features:

* HPET

* PCI bus

* Advanced Programmed Interrupt Controller (APIC)

* Serial Ports in Polling and Interrupt Driven Modes

+------------------+------------+-----------------------+
| Interface        | Controller | Driver/Component      |
+==================+============+=======================+
| HPET             | on-chip    | system clock          |
+------------------+------------+-----------------------+
| PCI              | on-chip    | PCI library           |
+------------------+------------+-----------------------+
| APIC             | on-chip    | interrupt controller  |
+------------------+------------+-----------------------+
| UART             | on-chip    | serial port-polling;  |
|                  |            | serial port-interrupt |
+------------------+------------+-----------------------+

Other hardware features are not currently supported by the Zephyr kernel.
See the `Intel® Quark Core Hardware Reference Manual`_ for a
complete list of Galileo board hardware features, and the
`Intel® Quark Software Developer Manual for Linux`_

.. _Intel® Quark Core Hardware Reference Manual: http://caxapa.ru/thumbs/497461/Intel_Quark_Core_HWRefMan_001.pdf

.. _Intel® Quark Software Developer Manual for Linux: http://www.intel.com/content/dam/www/public/us/en/documents/manuals/quark-x1000-linux-sw-developers-manual.pdf


PCI
===

The PCI driver initiates a PCI library scan of the PCI bus for any attached devices.
If devices are detected, they are initialized.

.. note::
   The PCI library does not support 64 bit devices. Memory
   address and size storage only require 32 bit integers.

Serial Port Polling Mode Support
================================

The polling mode serial port allows debug output to be printed.

For more information, see Intel® Quark SoC X1000 Datasheet, section `18.3.3 FIFO Polled-Mode Operation`_

.. _18.3.3 FIFO Polled-Mode Operation: https://communities.intel.com/servlet/JiveServlet/previewBody/21828-102-2-25120/329676_QuarkDatasheet.pdf


Serial Port Interrupt Mode Support
==================================

The interrupt mode serial port is used to provide general serial communication
and external communication.

For more information, see Intel® Quark SoC X1000 Datasheet, section `21.12.1.4.5 Poll Mode`_

.. _21.12.1.4.5 Poll Mode: https://communities.intel.com/servlet/JiveServlet/previewBody/21828-102-2-25120/329676_QuarkDatasheet.pdf

Interrupt Controller
====================

The galileo platform configuration uses the Zephyr kernel static
Interrupt Descriptor Table (IDT) to program the
Advanced Programmable Interrupt Controller (APIC)
interrupt redirection table.

Interrupts
----------

+-------+-----------+------------------+-------------------------------+
| IRQ   | Name      | Remarks          | Used by Zephyr Kernel         |
+=======+===========+==================+===============================+
| 17    | INTB      |   UART           | serial port, when used in     |
|       |           |                  | interrupt mode                |
+-------+-----------+------------------+-------------------------------+
| 20    | timer     |   HPET           | timer driver                  |
+-------+-----------+------------------+-------------------------------+

.. note::
   The galileo platform configuration does not support
   interrupt sharing, for example, two PCI devices can not use same IRQ.

Configuration Options
=====================

:option:`CONFIG_PCI_DEBUG`
      Set to "y" to enable PCI debugging functions for PCI bus scanning.
      Allows a list of all the PCI devices found to be printed.


HPET System Clock Support
=========================

Galileo uses HPET timing with legacy-free timer support. The galileo platform
configuration uses HPET as a system clock timer.

Procedures
**********

Use the following procedures for booting a project on
a Galileo board.

* `Creating a GRUB2 Boot Loader Image from a Linux Host`_

* `Preparing the Boot Device`_

* `Booting the Galileo Board`_


Creating a GRUB2 Boot Loader Image from a Linux Host
====================================================

Create a GRUB2 boot loader image needed later to load
a project image on to a Galileo board.


Prerequisite
------------

* The Linux host computer is running Ubuntu 12.04.

* The GNU EFI development libraries (version 3.0u) is installed.

* The GRUB 2.0 source code is available.

  .. note:
     Only the specified release of the GRUB2 tarball works with the
     galileo platform configuration.

* The Zephyr application image is available in the project directory.

Steps
-----

1. Install the required development packages on the host computer.

   a. Open a web browser and download the GNU EFI development
      libraries from the following website:
      https://launchpad.net/ubuntu/+source/gnu-efi/3.0u+debian-1ubuntu2~12.04.0/+build/5052631

      The source code is unpacked to the ~/grub-2.00 directory.

   b. In a Linux console, enter the following commands:

      .. code-block:: console

        $ sudo dpkg -i gnu-efi_3.0u+debian-1ubuntu2~12.04.0_i386.deb
        $ sudo apt-get install bison libopts25 libselinux1-dev
          autogen m4 autoconf help2man libopts25-dev flex
          libfont-freetype-perlautomake autotools-dev
          libfreetype6-dev texinfo

   c. Install any additional packages listed in the :file:`INSTALL`
      file included with the GRUB2 source code.

2. Download the GRUB2 source code and unpack it.

   a. In a Linux console, enter the following commands to download GRUB2:

      .. code-block:: console

        $ cd
        $ wget ftp://ftp.gnu.org/gnu/grub/grub-2.00.tar.gz

   b. Enter the following command to unpack GRUB2:

      .. code-block:: console

        $ tar -xzf grub-2.00.tar.gz

      The source code is downloaded and unpacked to
      the :file:`~/grub-2.00` directory.

3. Configure and build the :file:`GRUB2 EFI` image.

   a. In a Linux console, enter the following commands to configure GRUB2:

      .. code-block:: console

        $ cd ~/grub-2.00
        $ ./autogen.sh
        $ CFLAGS="-march=i586" ./configure --with-platform=efi
          --target=i386 --program-prefix=""

   b. Enter the following commands to build the :file:`grub.efi` image:

      .. code-block:: console

        $ make
        $ cd grub-core
        $ ../grub-mkimage -O i386-efi -d . -o grub.efi -p "" part_gpt
          part_msdos ext2 normal chain boot configfile linux multiboot
          help serial terminal elf efi_gop efi_uga terminfo

      The file :file:`grub.efi` is created in the following directory
      :file:`~/grub-2.00/grub-core`.



Preparing the Boot Device
=========================

Prepare either an SD-micro card or USB flash drive to boot the
Zephyr application image on a Galileo board. The
following instructions apply to both devices.


Prerequisites
-------------

* There is access to a Windows host.

* The stripped project image and the GRUB2 image have been copied
  from your Linux host to your Windows host.

* There is a serial port for communication.


Steps
-----

1. Insert the boot device into the Windows host computer,
   and make note of the Drive letter assigned to the device.

2. In the :guilabel:`Windows Computer` folder, right click the boot
   device and select :guilabel:`Format`.

3. Format the boot device with the FAT file system.
   This is typically the default file system type on Windows.

4. Double click the formatted device to open it.

5. Create the following directory tree on the device::

     `-- F:
         |-- efi
         |   |-- boot
         ‘-- kernel

6. Copy the images to the directory tree.

   a. For a microkernel image, copy the file :file:`microkernel.strip`
      to the kernel directory.

   b. Alternatively, for a nanokernel image, copy the file
      :file:`nanokernel.strip` to the kernel directory.

   c. Copy the file :file:`grub.efi` to the boot directory.

7. Create a :file:`GRUB2` configuration file.

   a. In the boot directory, create a text file :file:`grub.cfg`
      that contains the following:

      .. code-block:: console

        set default=0
        set timeout=10
        menuentry "Zephyr Microkernel" {
             multiboot /kernel/microkernel.strip
        }

   b. Alternatively, if you want to use a nanokernel image,
      add the following:

      .. code-block:: console

        menuentry "Zephyr Nanokernel" {
            multiboot /kernel/nanokernel.strip
        }

   The device is ready to use to boot the board.


Booting the Galileo Board
=========================

Boot the Galileo board from the boot device using GRUB2
with the boot loader present in the on-board flash.

.. note::
   A stripped project image file is automatically created when the
   project is built. The stripped image has removed debug
   information from the :file:`ELF` file.

Prerequisites
-------------

* The automatically created stripped Zephyr application image is
  in the project directory.

* There is a serial port for communication.

  .. note::
     For details on how to connect and configure the serial port,
     see the Getting Started guide that you received with the board.

Steps
-----

1. Insert the prepared boot device (micro-SD card or USB flash
   drive) into the board and start the board.

   The boot process begins and displays a large amount of output.

2. When the following output appears, press :kbd:`F7`:

   .. code-block:: console

     [Bds]BdsWait ...Zzzzzzzzzzzz...
     [Bds]BdsWait(5)..Zzzz...
     [Bds]BdsWait(4)..Zzzz...
     [Bds]Press [Enter] to directly boot.
     [Bds]Press [F7]    to show boot menu options.

3. From the menu that appears, select :guilabel:`UEFI Internal Shell`.

4. At the shell prompt enter:

   .. code-block:: console

     grub.efi

   GRUB2 starts, and a menu shows entries for the items you added
   to the :file:`file grub.cfg`.

5. Select the image you want to boot and press :guilabel:`Enter`.

   When the boot process finishes, you have finished booting the
   Zephyr application image.

Known Problems and Limitations
******************************

There is no support for the following:

* Isolated Memory Regions
* Serial port in Direct Memory Access (DMA) mode
* Serial Peripheral Interface (SPI) flash
* General-Purpose Input/Output (GPIO)
* Inter-Integrated Circuit (I2C)
* Ethernet
* Supervisor Mode Execution Protection (SMEP)

Bibliography
************

1. `Intel® Galileo Datasheet`_.

.. _Intel® Galileo Datasheet: http://www.intel.com/newsroom/kits/quark/galileo/pdfs/Intel_Galileo_Datasheet.pdf

2. `Intel® Galileo Board User Guide`_.

.. _Intel® Galileo Board User Guide: http://download.intel.com/support/galileo/sb/galileo_boarduserguide_330237_001.pdf

3. `Intel® Quark SoC X1000 Datasheet`_.

.. _Intel® Quark SoC X1000 Datasheet: https://communities.intel.com/servlet/JiveServlet/previewBody/21828-102-2-25120/329676_QuarkDatasheet.pdf

4. `Intel® Quark Core Hardware Reference Manual`_.

.. _Intel® Quark Core Hardware Reference Manual: http://caxapa.ru/thumbs/497461/Intel_Quark_Core_HWRefMan_001.pdf

5. `Intel® Quark Software Developer Manual for Linux`_.

.. _Intel® Quark Software Developer Manual for Linux: http://www.intel.com/content/dam/www/public/us/en/documents/manuals/quark-x1000-linux-sw-developers-manual.pdf

