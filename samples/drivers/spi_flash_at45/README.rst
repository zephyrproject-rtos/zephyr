.. _spi_flash_at45_sample:

AT45 DataFlash driver sample
#############################

Overview
********

This sample shows how to use the AT45 family DataFlash driver and how to
specify devicetree nodes that enable flash chip instances to be handled
by the driver (an overlay with two sample nodes is provided).
The sample writes a defined test region in the flash memory with bytes of
increasing (and overflowing at the 8-bit range) values and then reads it back
to check if the write was successful. The starting value is also increased
in consecutive runs of the sample. Finally, the flash chip is put into low
power state.
In the default configuration, the AT45 flash driver is configured to use
the Read-Modify-Write functionality of DataFlash chips and does not perform
page erasing, as it is not needed in this case. This can be modified by
simply disabling the SPI_FLASH_AT45_USE_READ_MODIFY_WRITE option.

Requirements
************

This sample has been tested on the Nordic Semiconductor nRF9160 DK
(nrf9160dk_nrf9160) board with the AT45DB321E chip connected.
It can be easily adjusted to be usable on other boards and with other
AT45 family chips by just providing a corresponding overlay file.

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/spi_flash_at45`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi_flash_at45
   :board: nrf9160dk_nrf9160
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   DataFlash sample app on nrf9160dk_nrf9160
   Using DATAFLASH_1, chip size: 4194304 bytes (page: 512)
   Reading the first byte of the test region ... OK
   Preparing test content starting with 0x01.
   Writing the first half of the test region... OK
   Writing the second half of the test region... OK
   Reading the whole test region... OK
   Checking the read content... OK
   Putting the flash device into low power state... OK

The sample is supplied with the overlay file that specifies two instances
of AT45 family chips but only the one labeled "DATAFLASH_1" is required
for the sample to work. If the other chip is not connected, the following
log message appears, but apart from that the behavior of the sample stays
unaffected.

.. code-block:: console

   [00:00:00.000,000] <err> spi_flash_at45: Wrong JEDEC ID: ff ff ff, expected: 1f 24 00
