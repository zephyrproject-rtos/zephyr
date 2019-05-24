.. _up_squared:

UP Squared
##########

Overview
********

UP |sup2| (UP Squared) is an ultra compact single board computer with high
performance and low power consumption. It features the latest Intel |reg| Apollo
Lake Celeron |trade| and Pentium |trade| Processors with only 4W of Scenario Design Power and
a powerful and flexible Intel |reg| FPGA Altera MAX 10 onboard.

.. figure:: img/up_squared.png
   :width: 800px
   :align: center
   :alt: UP Squared

   Up Squared (Credit: https://up-board.org)

This board configuration enables kernel support for the `UP Squared`_ board,
along with the following devices:

* High Precision Event Timer (HPET)

* Serial Ports in Polling and Interrupt Driven Modes

* GPIO

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

* Serial Ports in Polling and Interrupt Driven Modes, High-Speed

* GPIO

* I2C

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
| GPIO      | on-chip    | GPIO controller       | Not Supported   |
+-----------+------------+-----------------------+-----------------+
| I2C       | on-chip    | I2C controller        | Supported       |
+-----------+------------+-----------------------+-----------------+

The Zephyr kernel currently does not support other hardware features.

Serial Port Support
-------------------

Serial port I/O is supported in both polling and interrupt-driven modes.

Baud rates beyond 115.2kbps (up to 3.6864Mbps) are supported, with additional
configuration. The UARTs are fed a master clock which is fed into a PLL which
in turn outputs the baud master clock. The PLL is controlled by a per-UART
32-bit register called ``PRV_CLOCK_PARAMS`` (aka the ``PCP``), the format of
which is:

+--------+---------+--------+--------+
| [31]   | [30:16] | [15:1] | [0]    |
+========+=========+========+========+
| enable | ``m``   | ``n``  | toggle |
+--------+---------+--------+--------+

The resulting baud master clock frequency is ``(n/m)`` * master.

On the UP^2, the master clock is 100MHz, and the firmware by default sets
the ``PCP`` to ``0x3d090240``, i.e., ``n = 288``, ``m =  15625``, which
results in the de-facto standard 1.8432MHz master clock and a max baud rate
of 115.2k.  Higher baud rates are enabled by changing the PCP and telling
Zephyr what the resulting master clock is.

Use devicetree to set the value of the ``PRV_CLOCK_PARAMS`` register in
the UART block of interest. Typically an overlay ``up_squared.overlay``
would be present in the application directory, and would look something
like this:

   .. code-block:: console

    / {
        soc {
            uart@0 {
                pcp = <0x3d090900>;
                clock-frequency = <7372800>;
                current-speed = <230400>;
            };
        };
    };

The relevant variables are ``pcp`` (the value to use for ``PRV_CLOCK_PARAMS``),
and ``clock-frequency`` (the resulting baud master clock). The meaning of
``current-speed`` is unchanged, and as usual indicates the initial baud rate.

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
| 14  | GPIO    | GPIO APL driver          |
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

GPIO
----

GPIOs are exposed through the HAT header, and can be referred using
predefined macros such as ``UP2_HAT_PIN3``. The physical pins are
connected to the on-board FPGA acting as level shifter. Therefore,
to actually utilize these GPIO pins, the function of the pins and
directions (input/output) must be set in the BIOS. This can be
accomplished in BIOS, under menu ``Advanced``, and option
``HAT Configurations``. When a corresponding pin is set to act as
GPIO, there is an option to set the direction of the pin. This needs
to be set accordingly for the GPIO to function properly.

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

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Creating a GRUB2 Boot Loader Image from a Linux Host
====================================================

If you are having problems running an application using the preinstalled
copy of GRUB, follow these steps to test on supported boards using a custom GRUB.

#. Install the requirements to build GRUB on your host machine.

   On Ubuntu, type:

   .. code-block:: console

      $ sudo apt-get install bison autoconf libopts25-dev flex automake \
      pkg-config gettext autopoint

   On Fedora, type:

   .. code-block:: console

      $ sudo dnf install gnu-efi bison m4 autoconf help2man flex \
      automake texinfo gettext-devel

#. Clone and build the GRUB repository using the script in Zephyr tree, type:

   .. code-block:: console

      $ cd $ZEPHYR_BASE
      $ ./boards/x86/common/scripts/build_grub.sh x86_64

#. Find the binary at
   :file:`$ZEPHYR_BASE/boards/x86/common/scripts/grub/bin/grub_x86_64.efi`.

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application on UP Squared:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :tool: all
      :board: up_squared
      :goals: build

   .. note::

      A stripped project image file named :file:`zephyr.strip` is automatically
      created in the build directory after the application is built. This image
      has removed debug information from the :file:`zephyr.elf` file.

Preparing the Boot Device
=========================

Prepare a USB flash drive to boot the Zephyr application image on
a UP Squared board.

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

      $ mkfs.vfat -F 32 <device-node>

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

#. Insert the prepared boot device (USB flash drive) into the UP Squared board.

#. Connect the board to the host system using the serial cable and
   configure your host system to watch for serial data.  See
   https://wiki.up-community.org/Serial_console.

   .. note::
      On Windows, PuTTY has an option to set up configuration for
      serial data.  Use a baud rate of 115200.

#. Power on the UP Squared board.

#. When the following output appears, press :kbd:`F7`:

   .. code-block:: console

      Press <DEL> or <ESC> to enter setup.

#. From the menu that appears, select the menu entry that describes
   that particular type of USB flash drive.

   GRUB2 starts and a menu shows entries for the items you added
   to the file :file:`grub.cfg`.

#. Select the image you want to boot and press :guilabel:`Enter`.

   When the boot process completes, you have finished booting the
   Zephyr application image.

   .. note::
      You can safely ignore this message if it appears:

      .. code-block:: console

         WARNING: no console will be available to OS


Booting the UP Squared Board over network
=========================================

Build Zephyr image
------------------

#. Follow `Build Zephyr application`_ steps to build Zephyr image.

Prepare Linux host
------------------

#. Follow `Creating a GRUB2 Boot Loader Image from a Linux Host`_ steps
   to create grub binary.

#. Install DHCP, TFTP servers. For example ``dnsmasq``

   .. code-block:: console

      $ sudo apt-get install dnsmasq

#. Configure DHCP server. Configuration for ``dnsmasq`` is below:

   .. code-block:: console

      # Only listen to this interface
      interface=eno2
      dhcp-range=10.1.1.20,10.1.1.30,12h

#. Configure TFTP server.

   .. code-block:: console

      # tftp
      enable-tftp
      tftp-root=/srv/tftp
      dhcp-boot=grub_x86_64.efi

   ``grub_x86_64.efi`` is a grub binary created above.

#. Create the following directories inside TFTP root :file:`/srv/tftp`

    .. code-block:: console

       $ sudo mkdir -p /srv/tftp/EFI/BOOT
       $ sudo mkdir -p /srv/tftp/kernel

#. Copy the Zephyr image :file:`zephyr/zephyr.strip` to the
   :file:`/srv/tftp/kernel` folder.

    .. code-block:: console

       $ sudo cp zephyr/zephyr.strip /srv/tftp/kernel

#. Copy your built version of GRUB to :file:`/srv/tftp/grub_x86_64.efi`

#. Create :file:`/srv/tftp/EFI/BOOT/grub.cfg` containing the following:

   .. code-block:: console

      set default=0
      set timeout=10

      menuentry "Zephyr Kernel" {
         multiboot /kernel/zephyr.strip
      }

#. TFTP root should be looking like:

   .. code-block:: console

      $ tree /srv/tftp
      /srv/tftp
      ├── EFI
      │   └── BOOT
      │       └── grub.cfg
      ├── grub_x86_64.efi
      └── kernel
          └── zephyr.strip

#. Restart ``dnsmasq`` service:

   .. code-block:: console

      $ sudo systemctl restart dnsmasq.service

Prepare UP Squared board for network boot
-----------------------------------------

#. Enable PXE network from BIOS settings.

   .. code-block:: console

      Advanced -> Network Stack Configuration -> Enable Network Stack -> Enable Ipv4 PXE Support

#. Make network boot as the first boot option.

   .. code-block:: console

      Boot -> Boot Option #1 : [Network]

Booting UP Squared
------------------

#. Connect the board to the host system using the serial cable and
   configure your host system to watch for serial data.  See
   https://wiki.up-community.org/Serial_console.

#. Power on the UP Squared board.

#. Verify that the board got an IP address:

   .. code-block:: console

      $ journalctl -f -u dnsmasq
      dnsmasq-dhcp[5386]: DHCPDISCOVER(eno2) 00:07:32:52:25:88
      dnsmasq-dhcp[5386]: DHCPOFFER(eno2) 10.1.1.28 00:07:32:52:25:88
      dnsmasq-dhcp[5386]: DHCPREQUEST(eno2) 10.1.1.28 00:07:32:52:25:88
      dnsmasq-dhcp[5386]: DHCPACK(eno2) 10.1.1.28 00:07:32:52:25:88

#. Verify that network booting is started:

   .. code-block:: console

      $ journalctl -f -u dnsmasq
      dnsmasq-tftp[5386]: sent /srv/tftp/grub_x86_64.efi to 10.1.1.28
      dnsmasq-tftp[5386]: sent /srv/tftp/EFI/BOOT/grub.cfg to 10.1.1.28
      dnsmasq-tftp[5386]: sent /srv/tftp/kernel/zephyr.strip to 10.1.1.28

#. When the boot process completes, you have finished booting the
   Zephyr application image.

.. _UP Squared: https://www.up-board.org/upsquared/specifications

.. _UP Squared Pinout: https://wiki.up-community.org/Pinout
