.. zephyr:code-sample:: crc_subsys
   :name: Cyclic Redundancy Check Subsystem (CRC Subsys)
   :relevant-api: crc_new_interface

   Compute and verify a CRC computation using the CRC subsys API.

Overview
********

This sample demonstrates how to use the :ref:`CRC New Function API`
If the board does not have a CRC chosen node, the sample will use a software-based
CRC implementation from ``zephyr/lib/crc``.

Configuration Options
*********************

This sample supports the following Kconfig options:

- ``CONFIG_CRC_NEW``: Enable CRC functionality.
- ``CONFIG_CRC_SOFTWARE_BASED``: Use a software-based CRC implementation if a CRC chosen node is present.
- ``CONFIG_CRC_HARDWARE_ACCELERATOR``: Use hardware acceleration for CRC if a CRC chosen node is not present.

These options can be modified in the project's ``prj.conf`` file or passed via CMake arguments.

Building and Running
********************

Building and Running for Renesas RA8M1
======================================

The sample can be built and executed for the
:zephyr:board:`ek_ra8m1` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/crc
   :board: ek_ra8m1
   :goals: build flash
   :compact:

To build for another board, change "ek_ra8m1" above to that board's name.

Sample Output
=============

.. code-block:: console

   subsys_crc_example: Result of CRC32 IEEE: 0xCEA4A6C2
   subsys_crc_example: Result of CRC8 CCITT: 0x96
   subsys_crc_example: CRC computation completed successfully

.. note::
   If the board does not support a hardware CRC driver, the computation will fall
   back to a software-based implementation.

Expected Behavior
*****************

When the sample runs, it should:

1. Compute the CRC32 and CRC8 values of predefined data.
2. Print the computed CRC values.
