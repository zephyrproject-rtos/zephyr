.. zephyr:code-sample:: crc
   :name: Cyclic Redundancy Check (CRC)
   :relevant-api: crc_interface

   Compute and verify a CRC checksum using the CRC driver API.

Overview
********

This sample demonstrates how to use the :ref:`CRC driver API <crc_api>`.

Building and Running
********************

Building and Running for ST Nucleo G474RE
=========================================
The sample can be built and executed for the
:ref:`nucleo_g474re_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/crc
   :board: nucleo_g474re
   :goals: build flash
   :compact:
