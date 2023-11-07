.. zephyr:code-sample:: soc-flash
   :name: STM32H7 SoC flash
   :relevant-api: flash_interface flash_area_api

   Use the flash API to interact with the SoC flash.

Overview
********

This sample demonstrates using the :ref:`Flash API <flash_api>` on an SoC internal flash.
The sample uses :ref:`Flash map API <flash_map_api>` to obtain device for flash, using
DTS node label, and then directly uses :ref:`Flash API <flash_api>` to perform
flash operations.

Within the sample, user may observe how read/write/erase operations are performed on a
device, and how to first check whether device is ready for operation.

Known issues and Limitations
============================

Only tested when either M4 or M7 is running, not when both are running (co-existence).
Each M4 and M7 owns Bank1 and Bank2 of the flash device respectively.

Building and Running
********************

The application will build for supported STM32H7 series SoC with internal flash memory
access enabled. The sample application do erase/write/read operations over the
fixed-partition defined on that internal flash labeled `scratch_partition` and show
the result of operation to the user console.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/soc_flash
   :board: stm32h747i-disco
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.5.0-1187-g80dfcba8733d ***

   STM32H7 Flash API Sample Application
   ========================================
   Available partitions on SoC Flash
   image1_partition        @0, size 786432
   scratch_partition       @c0000, size 262144

   Flash erase page at 0xc0000, size 262144
      Flash erase succeeded!

   Doing Flash write/read operations
      Wrote 1024 blocks of 32 bytes
      Wrote 2048 blocks of 32 bytes
      Wrote 3072 blocks of 32 bytes
      Wrote 4096 blocks of 32 bytes
      Wrote 5120 blocks of 32 bytes
      Wrote 6144 blocks of 32 bytes
      Wrote 7168 blocks of 32 bytes
      Wrote 8192 blocks of 32 bytes
      read and verified up to 0xc8000
      read and verified up to 0xd0000
      read and verified up to 0xd8000
      read and verified up to 0xe0000
      read and verified up to 0xe8000
      read and verified up to 0xf0000
      read and verified up to 0xf8000
      8192 blocks read and verified correctly
