.. zephyr:code-sample:: crc_drivers
   :name: Cyclic Redundancy Check Drivers (CRC)
   :relevant-api: crc_interface

   Compute and verify a CRC checksum using the CRC driver API.

Overview
********

This sample demonstrates how to use the :ref:`CRC driver API <crc_api>`.

Building and Running
********************

Building and Running for Renesas RA8M1
======================================

The sample can be built and executed for the
:zephyr:board:`ek_ra8m1` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/crc
   :board: ek_ra8m1
   :goals: build flash
   :compact:

To build for another board, change "ek_ra8m1" above to that board's name.

Sample Output
=============

.. code-block:: console

   crc_example: CRC verification succeed.

.. note:: If the CRC is not supported, the output will be an error message.

Expected Behavior
*****************

When the sample runs, it should:

1. Compute the CRC8 values of predefined data.
2. Verify the CRC result.
