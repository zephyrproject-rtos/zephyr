.. zephyr:board:: intel_wcl_crb

Overview
********
Wildcat lake processor is a 64-bit multi-core processor.

Wildcat Lake is based on a Hybrid architecture, utilizing P-cores E-cores Xe3 cores
for performance and efficiency.

For more information about Wildcat Lake Processor lines, P-cores, and E-cores
please refer to `INTEL_WCL`_.

This board configuration enables kernel support for the Wildcat Lake boards.

Hardware
********

.. zephyr:board-supported-hw::

General information about the board can be found at the `INTEL_WCL`_. website.

Connections and IOs
===================

Refer to the `INTEL_WCL`_. website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image for an Wildcat Lake RVP  board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for Wildcat Lake RVP:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_wcl_crb
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

.. include:: ../../../intel/common/efi_boot.rst
   :start-after: start_include_here

.. _INTEL_WCL: https://edc.intel.com/content/www/us/en/secure/design/confidential/products/platforms/details/wildcat-lake-processor-external-design-specification-volume-1-of-2/
