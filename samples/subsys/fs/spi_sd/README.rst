.. _spi_sd:

FAT Rd/Wr access Sample Application
###################################

Overview
********

This sample app demonstrates the write and read speed performances
of the FAT filesystem API.
The system mounts an SDHC card connected over a SPI bus or to an on-chip
SDHC controller.

Requirements
************

This project requires a SDHC or microSD card formatted with FAT filesystem.
See the :ref:`sdhc_api` documentation for Zephyr implementation details.

Building and Running
********************

This sample can be built for a variety of boards, including ``ci-shield``.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/spi_sd
   :board: your_board_name
   :shield: ci_shield
   :goals: build

To run this sample, a FAT formatted microSD card should be present in the
microSD slot.
The application is storing a test file on the card, root directory.
If there are any files or directories present in the card, the
sample lists them out on the debug serial output.
