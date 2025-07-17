.. zephyr:board:: intel_btl_s_crb

Overview
********
The Bartlett Lake processor is a 64-bit multi-core processor built on Intel's 7 process technology.

It is available in two versions, one version is based on a hybrid architecture utilizing both
P-cores for performance and E-cores for efficiency, while the other version is based solely on
P-cores. These two versions are enabled as two revisions in Zephyr, as listed below:
* Revision "H" - BTL-s Hybrid
* Revision "P" - BTL-s 12P

Default revision H is selected. To build for revision P, use ``intel_btl_s_crb@P``.

The S-Processor line is a 2-Chip Platform that includes the Processor Die and
Platform Controller Hub (PCH-S) Die in the Package.

For more information about Raptor Lake Processor lines, P-cores, and E-cores
please refer to `BTL`_.

This board configuration enables kernel support for the Bartlett Lake S boards.

Hardware
********

.. zephyr:board-supported-hw::

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
