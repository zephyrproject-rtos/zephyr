.. zephyr:code-sample:: mec172xevb_assy6906-qmspi_ldma
   :name: MEC172x QMSPI LDMA
   :relevant-api: spi

   Use the QMSPI interface with LDMA on the MEC172x EVB to access an SPI flash
   device through the Zephyr SPI API.

Overview
********

This sample demonstrates the Quad-SPI (QMSPI) interface with Local DMA (LDMA)
on the Microchip MEC172x EVB (:zephyr:board:`mec172xevb_assy6906`).

It exercises SPI flash operations through the Zephyr SPI API, including:

* Reading the JEDEC ID of the connected SPI flash device
* Reading SPI flash status registers
* Erasing sectors and programming pages
* Reading data using slow, fast, dual, and quad SPI transfer modes
* Verifying data integrity after erase, program, and read operations

If :kconfig:option:`CONFIG_SPI_ASYNC` is enabled, the sample also exercises asynchronous
SPI reads.

Requirements
************

This sample requires the following hardware:

* Microchip MEC172x EVB (:zephyr:board:`mec172xevb_assy6906`)
* One of the following SPI flash devices connected to QSPI0:
  ``W25Q128``, ``W25Q128JV``, or ``SST26VF016B``

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/microchip/mec172xevb_assy6906/qmspi_ldma
   :board: mec172xevb_assy6906
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   JEDEC ID = 0x001840ef
   W25Q128 16Mbyte SPI flash
   SPI Flash Status1 = 0x00
   SPI Flash Status2 = 0x02
   Quad-Enable bit is set. WP# and HOLD# are IO[2] and IO[3]
   Transmit SPI flash Write-Enable
   Transmit Erase-Sector at 0x00c10000

   Read and check erased sectors using Read 1-1-1-0
   Sector at 0x00c10000 is erased

   Program erased sectors
   Read 36864 bytes from 0x00c10000 using SPI Read 1-1-1-0
   Data matches
   Read 36864 bytes from 0x00c10000 using SPI Read 1-1-1-8
   Data matches
   Read 36864 bytes from 0x00c10000 using SPI Read 1-1-2-8
   Data matches
   Read 36864 bytes from 0x00c10000 using SPI Read 1-1-4-8
   Data matches

   Program End
