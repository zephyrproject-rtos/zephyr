.. _ehl_crb:

Elkhart Lake CRB
################

Overview
********
Elkhart Lake Reference Board (EHL CRB) is an example implementation of a
compact single board computer with high performance for IoT edge devices.

This board configuration enables kernel support for the `EHL`_ board.

.. note::
   This board configuration works on the variant of `EHL`_
   boards containing Intel |reg| Atom |trade| SoC.

Hardware
********

General information about the board can be found at the `EHL`_ website.

.. include:: ../../../../soc/x86/elkhart_lake/doc/supported_features.txt


Connections and IOs
===================

Refer to the `EHL`_ website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image on a EHL CRB board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application on Elkhart Lake CRB:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: ehl_crb
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

Booting the Elkhart Lake CRB Board using UEFI
=============================================

.. include:: ../../common/efi_boot.rst

Booting the Elkhart Lake CRB Board over network
===============================================

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

       $ cp zephyr/zephyr.efi /srv/tftp/

#. Restart ``dnsmasq`` service:

   .. code-block:: console

      $ sudo systemctl restart dnsmasq.service

Prepare Elkhart Lake CRB board for network boot
-----------------------------------------------

#. Enable boot from PXE. Go to EFI shell and make sure that the first boot
   option is ``UEFI PXEv4``.

   .. code-block:: console

      Shell> bcfg boot dump
      Option: 00. Variable: Boot0007
        Desc    - UEFI PXEv4 (MAC:6805CABC1997)
        DevPath - PciRoot(0x0)/Pci(0x1C,0x0)/Pci(0x0,0x0)/MAC(6805CABC1997,0x0)/IPv4(0.0.0.0)
        Optional- Y
      ...

#. If UEFI PXEv4 is not the first boot option use ``bcfg boot mv`` command to
   change boot order

   .. code-block:: console

      Shell> bcfg boot mv 7 0

Booting Elkhart Lake CRB
------------------------

#. Connect the board to the host system using the serial cable and
   configure your host system to watch for serial data.

#. Power on the Elkhart Lake CRB board.

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

.. _EHL: https://www.intel.com/content/www/us/en/products/docs/processors/embedded/enhanced-for-iot-platform-brief.html
