.. _sdhc:

SDHC CMD52 Read/Write Sample
============================

Overview
--------

This sample demonstrates how to use the SDIO CMD52 (direct) command to read from and write to SDIO registers using the Zephyr SDHC API.

The sample performs the following:
- Initializes the SDIO interface
- Reads a register using CMD52
- Writes a value using CMD52
- Verifies the write by reading back and checking the result

Requirements
------------

- A board with SDIO host (SDMMC) support.
- A connected SDIO device (e.g., Wi-Fi module)
- Proper DTS configuration enabling SDMMC

Building and Running
--------------------
.. zephyr-app-commands::
      :zephyr-app: samples/drivers/sdhc
      :board: arduino_giga_r1/stm32h747xx/m7
      :goals: build flash
      :compact:

Sample output
--------------------
Connect to the console output to observe CMD52 results.
When the sample runs successfully, you should see output similar to the following:

      I: SDIO Init Passed Successfully.
      *** Booting Zephyr OS build v4.1.0-2351-ge261a86d79de ***
      I: CMD52 READ passed: data = 0x40
      I: CMD52 WRITE passed: wrote 0x01
      I: read data after write was as expected, data = 0x41
