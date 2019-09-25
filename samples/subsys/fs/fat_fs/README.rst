.. _fat_fs:

FAT Filesystem Sample Application
###################################

Overview
********

This sample app demonstrates use of the filesystem API and uses the FAT file
system driver to mount an SDHC card connected over SPI bus.

Requirements
************

This project requires a SDHC or microSD card formatted with FAT filesystem.
See the :ref:`SDHC_disks` documentation for Zephyr implementation details.

Building and Running
********************

This sample can be built for an ``nrf52840_blip`` board. It requires
both the ``nrf52840_blip.overlay`` and the ``dts_fixup.h`` for nrf52840_blip
to work:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/fat_fs
   :board: nrf52840_blip
   :goals: build
   :compact:

To run this sample, a FAT formatted microSD card should be present in the
microSD slot of ``nrf52840_blip`` board. If there are any files or directories
present in the card, the sample lists them out on the debug serial output.
