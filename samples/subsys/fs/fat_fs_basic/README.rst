.. _fat_fs_basic:

FAT File system Sample Application on internal/external Flash
#############################################################

Overview
********

The sample application demonstrates how to configure and mount FAT file system on internal or
external flash memory.
Sample presents how to configure project, prepare overalays and explains types of objects used in
source code for the purpose of mounting the file system.

Requirements
************

FAT requires at least 128 sectors, in this sample configured to be 512b in size, which means
that there is need to provided 64kiB of flash for the FAT to work.
When the sector size gets changed, the size of flash needs adjustment.

Building and Running
********************

Currently the sample will build for following boards:
	``nrf52840dk_nrf52840`` internal, external NOR on QSPI
	``nrf9160dk_nrf52840``	internal flash only
	``nrf9160dk_nrf9160@0.14.0`` internal, external NOR on SPI

Upon first run the sample will format provided flash partition to FAT file system and will create
single file there. On next runs, a one byte from file will be read incremented and written back
per run.

While building the application, remember to use proper overlay for the board.
