.. zephyr:code-sample:: crc_drivers
   :name: Cyclic Redundancy Check Drivers (CRC)
   :relevant-api: crc_interface

   Compute and verify a CRC checksum using the CRC driver API.

Overview
********

This sample demonstrates how to use the :ref:`CRC driver API <crc_api>`.
It computes a single CRC variant selected via sample Kconfig and logs
the computed result. Optionally, it verifies against an expected value.

Configuration
*************

Select the CRC variant to run using the sample's Kconfig:

- ``SAMPLE_CRC_VARIANT_CRC8``
- ``SAMPLE_CRC_VARIANT_CRC16_CCITT``
- ``SAMPLE_CRC_VARIANT_CRC32_IEEE``
- ``SAMPLE_CRC_VARIANT_CRC32_C``

The sample performs a build-time capability check and will emit a
compile error if the selected variant is not supported by the target
driver/platform.

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

The console logs the selected variant and the computed value. If an
expected value is provided, the sample verifies and logs the outcome.

.. code-block:: console

   crc_example: CRC8 result: 0x000000b2
   crc_example: CRC8 verification succeeded (expected 0x000000b2)

.. note:: If the selected CRC variant is not supported by the target,
   the build will fail with an error message describing the mismatch.

Expected Behavior
*****************

When the sample runs, it should:

1. Compute the selected CRC variant for predefined data.
2. Log the computed CRC value.
3. If an expected value is configured, verify it and log success/failure.
