.. _intel_rpl_crb:

Raptor Lake CRB
###############

Overview
********
Raptor Lake processor is a 13th generation 64-bit multi-core processor built
on a 10-nanometer technology process. Raptor Lake is based on a Hybrid
architecture, utilizing P-cores for performance and E-Cores for efficiency.

Raptor Lake S and Raptor Lake P processor lines are supported.

The S-Processor line is a 2-Chip Platform that includes the Processor Die and
Platform Controller Hub (PCH-S) Die in the Package.

The P-Processor line is a 2-Die Multi Chip Package (MCP) that includes the
Processor Die and Platform Controller Hub (PCH-P) Die on the same package as
the Processor Die.

For more information about Raptor Lake Processor lines, P-cores, and E-cores
please refer to `RPL`_.

Raptor Lake Customer Reference Board (RPL CRB) is an example implementation of a
compact single board computer with high performance for IoT edge devices. The
supported boards are ``intel_rpl_s_crb`` and ``intel_rpl_p_crb``.

These board configurations enable kernel support for the supported Raptor Lake
boards.

Hardware
********

General information about the board can be found at the `RPL`_.

.. include:: ../../../../soc/intel/raptor_lake/doc/supported_features.txt


Connections and IOs
===================

Refer to the `RPL`_ for more information.

Programming and Debugging
*************************
Use the following procedures for booting an image on an RPL CRB board.

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

.. include:: ../../../intel/common/efi_boot.rst
   :start-after: start_include_here

.. _RPL: https://edc.intel.com/content/www/us/en/design/products/platforms/details/raptor-lake-s/13th-generation-core-processors-datasheet-volume-1-of-2/
