.. zephyr:board:: intel_nvl_s_rvp

Overview
********

The Nova Lake (NVL) processor is a 64-bit, multi-core processor that includes the
Compute, HUB, PCD, and GPU tiles on the same package.

The Nova Lake H processor series is offered in a 1-Chip single package platform.

For more information about Nova Lake Processor lines
please refer to `INTEL_NVL`_.

This board configuration enables kernel support for the Nova Lake boards.

Hardware
********

.. zephyr:board-supported-hw::

General information about the board can be found at the `INTEL_NVL`_. website.

Connections and IOs
===================

Refer to the `INTEL_NVL`_. website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image for an Nova Lake RVP  board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for Nova Lake RVP:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_nvl_s_rvp
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

.. include:: ../../../intel/common/efi_boot.rst
   :start-after: start_include_here

.. _INTEL_NVL: https://edc.intel.com/content/www/us/en/secure/design/confidential/products/platforms/details/nova-lake-h-hx-processor-external-design-specification-volume-1-of-2/
