.. _fat_fs:

FAT Filesystem Sample Application
###################################

Overview
********

This sample app demonstrates use of the filesystem API and uses the FAT file
system driver to mount an SDHC card connected over a SPI bus or to an on-chip
SDHC controller.

Requirements
************

This project requires a SDHC or microSD card formatted with FAT filesystem.
See the :ref:`sdhc_api` documentation for Zephyr implementation details.

Building and Running
********************

This sample can be built for a variety of boards, including ``nrf52840_blip``,
``mimxrt1050_evk``, and ``olimexino_stm32``.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/fat_fs
   :board: nrf52840_blip
   :goals: build
   :compact:

To run this sample, a FAT formatted microSD card should be present in the
microSD slot. If there are any files or directories present in the card, the
sample lists them out on the debug serial output.
