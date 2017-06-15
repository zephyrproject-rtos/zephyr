.. _galileo:

Galileo Gen1/Gen2
#################

Overview
********

This board configuration enables kernel support for the board's Intel |reg| Quark |trade| SoC,
along with the following devices:

* High Precision Event Timer (HPET)

* Peripheral Component Interconnect (PCI) bus query

* Serial Ports in Polling and Interrupt Driven Modes

.. note::
   This board configuration may work with similar boards that are not officially
   supported.

Hardware
********

This section provides information about the physical characteristics of the
board.
Subsections contain detailed information on pin names, jumper settings, memory
mappings, and board component layout.

Supported Features
==================

This board supports the following hardware features:

* HPET

* PCI bus

* Advanced Programmed Interrupt Controller (APIC)

* Serial Ports in Polling and Interrupt Driven Modes

* Ethernet in Interrupt Driven Mode

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| HPET      | on-chip    | system clock          |
+-----------+------------+-----------------------+
| PCI       | on-chip    | PCI library           |
+-----------+------------+-----------------------+
| APIC      | on-chip    | interrupt controller  |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+
| Ethernet  | on-chip    | Ethernet              |
+-----------+------------+-----------------------+

The kernel currently does not support other hardware features.
See the `Intel |reg| Quark Core Hardware Reference Manual`_ for a
complete list of Galileo board hardware features, and the
`Intel |reg| Quark Software Developer Manual for Linux`_


PCI
----

PCI drivers assume that IO regions and IRQs for devices are
preconfigured identically by the firmware on all supported devices.
This configuration is specified in the Kconfig file for the Intel
Quark X1000 SoC.  The PCI library supports dynamically enumerating PCI
devices, but that support is disabled by default.

.. note::
   The PCI library does not support 64-bit devices.
   Memory address and size storage only require 32-bit integers.

Serial Port Polling Mode Support
--------------------------------

The polling mode serial port allows debug output to be printed.

For more information, see `Intel |reg| Quark SoC X1000 Datasheet`_,
section 18.3.3 FIFO Polled-Mode Operation


Serial Port Interrupt Mode Support
----------------------------------

The interrupt mode serial port provides general serial communication
and external communication.

For more information, see `Intel |reg| Quark SoC X1000 Datasheet`_, section 21.12.1.4.5 Poll Mode


Interrupt Controller
--------------------

This board uses the kernel's static Interrupt Descriptor Table (IDT) to program the
Advanced Programmable Interrupt Controller (APIC) interrupt redirection table.


+-----+-------+---------+--------------------------+
| IRQ | Name  | Remarks | Used by Zephyr Kernel    |
+=====+=======+=========+==========================+
| 17  | INTB  | UART    | serial port when used in |
|     |       |         | interrupt mode           |
+-----+-------+---------+--------------------------+
| 20  | timer | HPET    | timer driver             |
+-----+-------+---------+--------------------------+

HPET System Clock Support
-------------------------

Galileo uses HPET timing with legacy-free timer support. The Galileo board
configuration uses HPET as a system clock timer.

Ethernet Support
-----------------

The Ethernet driver allocates a Direct Memory Access (DMA)-accessible
pair of receive and transmit buffers and descriptors.  The driver
operates the network interface in store-and-forward mode and enables
the receive interrupt.

For more information, see `Intel |reg| Quark SoC X1000 Datasheet`_,
section 15.0 10/100 Mbps Ethernet

Connections and IOs
===================

For a component layout diagram showing pin names, see page 46 of the
`Intel |reg| Quark SoC X1000 Datasheet`_.

See also the `Intel |reg| Galileo Datasheet`_.

For the Galileo Board Connection Diagram see page 9 of the `Intel |reg| Galileo Board User Guide`_.


Jumpers & Switches
==================

The kernel uses the Galileo default jumper settings except for the IOREF jumper,
which must be set to match the external operating voltage of either 3.3 V or 5 V.

The Galileo default switch settings are:

+--------+--------------+
| Jumper | Setting      |
+========+==============+
| IOREF  | 3.3V or 5V   |
+--------+--------------+
| VIN    | 5V  Jumpered |
+--------+--------------+

For more information, see page 14 of the
`Intel |reg| Galileo Board User Guide`_.


Memory Mappings
===============

This board configuration uses default hardware memory map
addresses and sizes.

For a list of memory mapped registers, see page 868 of the
`Intel |reg| Quark SoC X1000 Datasheet`_.


Component Layout
================

See page 3 of the Intel |reg| Galileo Datasheet for a component layout
diagram. Click the link to open the `Intel |reg| Galileo Datasheet`_.


For a block diagram, see page 38 of the `Intel |reg| Quark SoC X1000 Datasheet`_.


Programming and Debugging
*************************

Use the following procedures for booting an image on a Galileo board.

* `Creating a GRUB2 Boot Loader Image from a Linux Host`_

* `Preparing the Boot Device`_

* `Booting the Galileo Board`_


Creating a GRUB2 Boot Loader Image from a Linux Host
====================================================

If you are having problems running an application using the default GRUB
of the hardware, follow these steps to test on Galileo2 boards using a custom
GRUB.

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
     $ ./scripts/build_grub.sh

#. Find the binary at :file:`$ZEPHYR_BASE/scripts/grub/bin/grub.efi`.



Preparing the Boot Device
=========================

Prepare either an SD-micro card or USB flash drive to boot the Zephyr
application image on a Galileo board. The following instructions apply to both
devices.


#. Set the board configuration to Galileo by changing the
   :command:`make` command that is executed in the app directory
   (e.g. :file:`$ZEPHYR_BASE/samples/hello_world`) to:

   .. code-block:: console

      $ make BOARD=galileo

   .. note::
      A stripped project image file named :file:`zephyr.strip` is
      automatically created when the project is built. This image has
      removed debug information from the :file:`zephyr.elf` file.

#. Use one of these cables for serial output:

   `<http://www.ftdichip.com/Products/Cables/USBTTLSerial.htm>`_

#. Format a microSD as FAT

#. Create the following directories

   :file:`efi`

   :file:`efi/boot`

   :file:`kernel`

#. Copy the kernel file :file:`outdir/galileo/zephyr.strip` to the :file:`$SDCARD/kernel` folder.

#. Copy your built version of GRUB to :file:`$SDCARD/efi/boot/bootia32.efi`

#. Create :file:`$SDCARD/efi/boot/grub.cfg` containing the following:

   .. code-block:: console

      set default=0
      set timeout=10

      menuentry "Zephyr Kernel" {
         multiboot /kernel/zephyr.strip
      }

Booting the Galileo Board
=========================

Boot the Galileo board from the boot device using GRUB2
with the firmware present in the on-board flash.


Steps
-----

1. Insert the prepared boot device (micro-SD card or USB flash
   drive) into the Galileo board.

2. Connect the board to the host system using the serial cable and
   configure your host system to watch for serial data.  See
   `<https://software.intel.com/en-us/articles/intel-galileo-gen-2-board-assembly-using-eclipse-and-intel-xdk-iot-edition>`_
   for the gen. 2 board,
   `<https://software.intel.com/en-us/articles/intel-galileo-gen-1-board-assembly-using-eclipse-and-intel-xdk-iot-edition>`_
   for the gen. 1 board, or the Getting Started guide that you
   received with the board.

   .. note::
      On Windows, PuTTY has an option to set up configuration for
      serial data.  Use a baud rate of 115200 and the SCO keyboard
      mode.  The keyboard mode option is in a submenu of the Terminal
      menu on the left side of the screen.

3. Power on the Galileo board.

4. When the following output appears, press :kbd:`F7`:

   .. code-block:: console

     Press [Enter] to directly boot.
     Press [F7]    to show boot menu options.

5. From the menu that appears, select :guilabel:`UEFI Misc Device` to
   boot from a micro-SD card.  To boot from a USB flash drive, select
   the menu entry that desribes that particular type of USB flash
   drive.

   GRUB2 starts and a menu shows entries for the items you added
   to the file :file:`grub.cfg`.

6. Select the image you want to boot and press :guilabel:`Enter`.

   When the boot process completes, you have finished booting the
   Zephyr application image.

   .. note::
      If the following messages appear during boot, they can be safely
      ignored.

      .. code-block:: console

         WARNING: no console will be available to OS
         error: no suitable video mode found.

Known Problems and Limitations
******************************

At this time, the kernel does not support the following:

* Isolated Memory Regions
* Serial port in Direct Memory Access (DMA) mode
* Supervisor Mode Execution Protection (SMEP)

Bibliography
************

1. `Intel |reg| Galileo Datasheet`_, Order Number: 329681-003US

.. _Intel |reg| Galileo Datasheet:
   https://www.intel.com/content/dam/support/us/en/documents/galileo/sb/galileo_datasheet_329681_003.pdf

2. `Intel |reg| Galileo Board User Guide`_.

.. _Intel |reg| Galileo Board User Guide:
   http://download.intel.com/support/galileo/sb/galileo_boarduserguide_330237_001.pdf

3. `Intel |reg| Quark SoC X1000 Datasheet`_, Order Number: 329676-001US

.. _Intel |reg| Quark SoC X1000 Datasheet:
   https://communities.intel.com/servlet/JiveServlet/previewBody/21828-102-2-25120/329676_QuarkDatasheet.pdf

4. `Intel |reg| Quark Core Hardware Reference Manual`_.

.. _Intel |reg| Quark Core Hardware Reference Manual:
   http://caxapa.ru/thumbs/497461/Intel_Quark_Core_HWRefMan_001.pdf

5. `Intel |reg| Quark Software Developer Manual for Linux`_.

.. _Intel |reg| Quark Software Developer Manual for Linux:
   http://www.intel.com/content/dam/www/public/us/en/documents/manuals/quark-x1000-linux-sw-developers-manual.pdf
