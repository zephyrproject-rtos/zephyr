.. _fat_fs:

FAT Filesystem Sample Application
###################################

Overview
********

This sample app demonstrates use of the filesystem API and uses the FAT file
system driver with SDHC card, SoC flash or external flash chip.

To access device the sample uses :ref:`disk_access_api`.

Requirements for SD card support
********************************

This project requires SD card support and microSD card formatted with FAT filesystem.
See the :ref:`disk_access_api` documentation for Zephyr implementation details.
Boards that by default use SD card for storage:
``arduino_mkrzero``, ``esp_wrover_kit``, ``mimxrt1050_evk``, ``nrf52840_blip``
and  ``olimexino_stm32``.
The sample should be able to run with any other board that has "zephyr,sdmmc-disk"
DT node enabled.

Requirements for setting up FAT FS on SoC flash
***********************************************

For the FAT FS to work with internal flash, the device needs to support erase
pages of size <= 4096 bytes and have at least 64kiB of flash available for
FAT FS partition alone.
Currently the following boards are supported:
``nrf52840dk_nrf52840``

Requirements for setting up FAT FS on external flash
****************************************************

This type of configuration requires external flash device to be available
on DK board. Currently following boards support the configuration:
``nrf52840dk_nrf52840`` by ``nrf52840dk_nrf52840_qspi`` configuration.

Building and Running
********************

Boards with default configurations, for example ``arduino_mkrzero`` or
``nrf52840dk_nrf52840`` using internal flash can be build using command:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/fat_fs
   :board: nrf52840_blip
   :goals: build
   :compact:

Where used example board ``nrf52840_blip`` should be replaced with desired board.

In case when some more specific configuration is to be used for a given board,
for example ``nrf52840dk_nrf52840`` with MX25 device over QSPI, configuration
and DTS overlays need to be also selected. The command would look like this:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/fat_fs
   :board: nrf52840dk_nrf52840
   :gen-args: -DOVERLAY_CONFIG=nrf52840dk_nrf52840_qspi.conf -DDTC_OVERLAY_FILE=nrf52840dk_nrf52840_qspi.overlay
   :goals: build
   :compact:

In case when board with SD card is used FAT microSD card should be present in the
microSD slot. If there are any files or directories present in the card, the
sample lists them out on the debug serial output.

.. warning::
   In case when mount fails the device may get re-formatted to FAT FS.
   To disable this behaviour disable :kconfig:option:`CONFIG_FS_FATFS_MOUNT_MKFS` .
