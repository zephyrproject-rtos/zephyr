.. zephyr:code-sample:: cdns_nand
   :name: Cadence NAND Flash Driver
   :relevant-api: flash_interface

   Use the Cadence NAND flash driver to perform erase, write, read, and
   post-erase verification operations via Zephyr's flash API.

Overview
********

This sample demonstrates using the :ref:`Flash API <flash_api>` with the Cadence NAND
flash controller on the Intel Agilex 5 SoC FPGA. The NAND device is accessed via a ``nand``
alias defined in the board overlay, using the standard flash driver interface.

The sample performs a full erase-write-read-verify cycle on a region of the NAND flash,
followed by a second erase and blank verification. It also shows how to query device
parameters such as page size and write block size before performing any operations.

Requirements
************

- :zephyr:board:`intel_socfpga_agilex5_socdk`
- External NAND flash memory connected to the board's Cadence NAND controller

Wiring
******

No additional wiring is required as the NAND flash memory is routed directly on the
development board.

Building and Running
********************

Build and flash the sample for the :zephyr:board:`intel_socfpga_agilex5_socdk` board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/soc_flash_nand
   :board: intel_socfpga_agilex5_socdk
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   Nand flash driver test sample
   Nand flash driver block size 20000
   The Page size of 800
   Nand flash driver data erase successful....
   Nand flash driver write completed....
   Nand flash driver read completed....
   Nand flash driver read verified
   Nand flash driver data erase successful....
   Nand flash driver read verified after erase....
   Nand flash driver test sample completed....


References
**********

- :zephyr:board:`intel_socfpga_agilex5_socdk`
