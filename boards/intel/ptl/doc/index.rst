.. zephyr:board:: intel_ptl_h_crb

Overview
********
Panther Lake H processor is a 64-bit multi-core processor built on the advanced
Intel 18A(1.8nm) process node. It is based on a Hybrid architecture, utilizing
P-cores for performance and E-Cores for efficiency.

For more information about Panther Lake Processor lines, P-cores, and E-cores
please refer to `INTEL_PTL`_.

This board configuration enables kernel support for the Panther Lake H boards.

Hardware
********

.. zephyr:board-supported-hw::

General information about the board can be found at the `INTEL_PTL`_ website.

Connections and IOs
===================

Refer to the `INTEL_PTL`_ website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image for an Panther Lake H CRB board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for Panther Lake H CRB:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_ptl_h_crb
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

.. include:: ../../../intel/common/efi_boot.rst
   :start-after: start_include_here

.. _INTEL_PTL: https://edc.intel.com/content/www/us/en/secure/design/confidential/products/platforms/details/panther-lake/panther-lake-u-h-processor-external-design-specification-volume-1-of-2/
