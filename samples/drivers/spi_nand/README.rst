.. zephyr:code-sample:: spi-nand
   :name: JEDEC SPI-NAND flash
   :relevant-api: flash_interface

   Use the flash API to interact with an SPI NAND serial flash memory device.

Overview
********

This sample demonstrates using the flash API on a SPI NAND serial flash
memory device.  While trivial it is an example of direct access and
allows confirmation that the flash is working.

Building and Running
********************

The application will build only for a target that has a devicetree node with
one of the following bindings as a compatible:

* :dtcompatible:`jedec,spi-nand`.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi_nand
   :board: stm32l562e_dk
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.3.0-2142-gca01d2e1d748  ***

   JEDEC QSPI-NAND SPI flash testing
   ==========================

   Test 1: Flash erase
   Flash erase succeeded!

   Test 2: Flash write
   Attempting to write 4 bytes
   Data read matches data written. Good!
