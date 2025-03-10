.. _intel_btl_s:

Bartlett Lake S
###############

Overview
********
Bartlett Lake processor is a 64-bit multi-core processor built on Intel 7 process
Technology. Bartlett Lake is based on a Hybrid architecture, utilizing
P-cores for performance and E-Cores for efficiency.

The S-Processor line is a 2-Chip Platform that includes the Processor Die and
Platform Controller Hub (PCH-S) Die in the Package.

For more information about Raptor Lake Processor lines, P-cores, and E-cores
please refer to `BTL`_.

This board configuration enables kernel support for the Bartlett Lake S boards.

Hardware
********

General information about the board can be found at the `BTL`_ website.

Connections and IOs
===================

Refer to the `BTL`_ website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image for an Bartlett Lake S CRB board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for Bartlett Lake S CRB:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_btl_s_crb
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

.. include:: ../../../intel/common/efi_boot.rst
   :start-after: start_include_here

.. _BTL: https://www.intel.com/content/www/us/en/secure/content-details/839635/bartlett-lake-s-processor-external-design-specification-eds-for-edge-platforms.html?DocID=839635
