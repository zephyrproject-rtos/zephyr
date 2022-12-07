.. _up_squared:

UP Squared
##########

Overview
********

UP |sup2| (UP Squared) is an ultra compact single board computer with high
performance and low power consumption. It features the latest Intel |reg| Apollo
Lake Celeron |trade| and Pentium |trade| Processors with only 4W of Scenario Design Power and
a powerful and flexible Intel |reg| FPGA Altera MAX 10 onboard.

.. figure:: img/up_squared.jpg
   :align: center
   :alt: UP Squared

   Up Squared (Credit: https://up-board.org)

This board configuration enables kernel support for the `UP Squared`_ board.

.. note::
   This board configuration works on all three variants of `UP Squared`_
   boards containing Intel |reg| Pentium |trade| SoC,
   Intel |reg| Celeron |trade| SoC, or Intel |reg| Atom |trade| SoC.

Hardware
********

General information about the board can be found at the `UP Squared`_ website.

.. include:: ../../../../soc/x86/apollo_lake/doc/supported_features.txt

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

Programming and Debugging
*************************

Use the following procedures for booting an image on a UP Squared board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application on UP Squared:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: up_squared
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

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

#. Copy the Zephyr EFI image file :file:`zephyr/zephyr.efi` to the USB drive.

Booting the UP Squared Board
============================

Boot the UP Squared board to the EFI shell with USB flash drive connected.

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
   that particular EFI shell.

#. From the EFI shell select Zephyr EFI image to boot.

   .. code-block:: console

      Shell> fs0:zephyr.efi

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
      dhcp-boot=zephyr.efi

   ``zephyr.efi`` is a Zephyr EFI binary created above.

#. Copy the Zephyr EFI image :file:`zephyr/zephyr.efi` to the
   :file:`/srv/tftp` folder.

    .. code-block:: console

       $ sudo cp zephyr/zephyr.efi /srv/tftp


#. TFTP root should be looking like:

   .. code-block:: console

      $ tree /srv/tftp
      /srv/tftp
      └── zephyr.efi

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
      dnsmasq-tftp[5386]: sent /srv/tftp/zephyr.efi to 10.1.1.28

#. When the boot process completes, you have finished booting the
   Zephyr application image.

.. _UP Squared: https://www.up-board.org/upsquared/specifications

.. _UP Squared Pinout: https://wiki.up-community.org/Pinout
