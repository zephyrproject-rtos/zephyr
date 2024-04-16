.. _intel_rpl_s_crb:

Raptor Lake S CRB
#################

Overview
********
Raptor Lake Reference Board (RPL CRB) is an example implementation of a
compact single board computer with high performance for IoT edge devices.

This board configuration enables kernel support for the `RPL`_ board.

.. note::
   This board configuration works on the variant of `RPL`_
   boards containing Intel |reg| Core |trade| SoC.

Hardware
********

General information about the board can be found at the `RPL`_ website.

.. include:: ../../../../soc/x86/raptor_lake/doc/supported_features.txt


Connections and IOs
===================

Refer to the `RPL`_ website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image on a RPL CRB board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application on Raptor Lake S CRB:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_rpl_s_crb
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

Booting the Raptor Lake S CRB Board using UEFI
==============================================

.. include:: ../../common/efi_boot.rst

.. _RPL: https://www.intel.com/content/www/us/en/newsroom/resources/13th-gen-core.html#gs.glf2fn
