.. _intel_ehl_crb:

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
      :board: intel_ehl_crb
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

Booting the Elkhart Lake CRB Board using UEFI
=============================================

.. include:: ../../common/efi_boot.rst

Booting the Elkhart Lake CRB Board over network
===============================================

.. include:: ../../common/net_boot.rst

.. note::
   To enable PXE boot for Elkhart Lake CRB board do the following:

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

.. _EHL: https://www.intel.com/content/www/us/en/products/docs/processors/embedded/enhanced-for-iot-platform-brief.html
