.. _sdhc:

.. zephyr:code-sample:: sdhc
   :name: SDHC direct read/write
   :relevant-api: sdhc_interface

Overview
********

This sample demonstrates how to use the SDIO CMD52 (direct) command to read
from and write to SDIO registers using the Zephyr SDHC API.

The sample performs the following:
- Initializes the SDIO interface
- Reads a register using CMD52
- Writes a value using CMD52
- Verifies the write by reading back and checking the result

Requirements
************

- A board with SDIO host (SDMMC) support.
- A connected SDIO device (e.g., Wi-Fi module)
- Proper DTS configuration enabling SDMMC

Building and Running
********************

.. zephyr-app-commands::
      :zephyr-app: samples/drivers/sdhc
      :board: arduino_giga_r1/stm32h747xx/m7
      :goals: build flash
      :compact:

Sample output
*************

Connect to the console output to observe CMD52 results.
When the sample runs successfully, you should see output similar to the following:

.. code-block:: console

      I: SDIO Init Passed Successfully
      I: Card does not support CMD8, assuming legacy card
      WLAN MAC Address : FC:84:A7:38:E4:49
      WLAN Firmware    : wl0: Jul 18 2021 19:15:39 version 7.45.98.120 (56df937 CY) FWID 01-69db62cf
      WLAN CLM         : API: 12.2 Data: 9.10.39 Compiler: 1.29.4 ClmImport: 1.36.3 Creation: 2021-07-18 19:03:20
      WHD VERSION      : 3.3.2.25168 : v3.3.2 : GCC 12.2 : 2024-12-06 06:53:17 +0000
      *** Booting Zephyr OS build v4.1.0-6920-ga34249351280 ***
      I: CMD52 READ passed: data = 0x05
      I: CMD52 WRITE passed: wrote 0x07
      I: read data after write was as expected, data = 0x07
