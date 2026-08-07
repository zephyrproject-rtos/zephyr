.. _intel_adl_n:

Alder Lake N
############

Overview
********
Alder Lake processor is a 64-bit multi-core processor built on 10-nanometer
technology process.

Currently supported is N-processor line, Single Chip Platform that consists of
the Processor Die and Alder Lake N Platform Controller Hub (ADL-N PCH) Die on
the same package as Multi-Chip Package (MCP).

Proposed branding for Adler Lake N is Intel Processor (N100,N200) and
Intel Core i3 (N300, N305).

Alder Lake N Customer Reference Board (ADL-N CRB) and Alder Lake Reference
Validation Platform (ADL-N RVP) are example implementations of compact single
board computer with high performance for IoT edge devices.

This board configuration enables kernel support for the Alder Lake N boards.

Hardware
********

General information about the board can be found at the `INTEL_ADL`_ website.

Connections and IOs
===================

Refer to the `INTEL_ADL`_ website for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image for an Alder Lake N CRB board.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for Alder Lake N CRB:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_adl_crb
      :goals: build

   .. note::

      A Zephyr EFI image file named :file:`zephyr.efi` is automatically
      created in the build directory after the application is built.

Booting the Alder Lake N CRB Board using UEFI
=============================================

.. include:: ../../../intel/common/efi_boot.rst
   :start-after: start_include_here

.. _INTEL_ADL: https://edc.intel.com/content/www/us/en/design/products/platforms/processor-and-core-i3-n-series-datasheet-volume-1-of-2/
