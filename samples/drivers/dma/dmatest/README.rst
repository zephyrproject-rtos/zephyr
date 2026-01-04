.. zephyr:code-sample:: dma
   :name: DMA test
   :relevant-api: dma_interface

   Direct mode of transfer.

Overview
********

This sample demonstrates how to use the dma driver with a simple direct
mode of transfer. It transfer data directly by writing the source and
destination address in the corresponding registers and also writing the
length of bytes to be transferred.
By default, the DMA that is normally used for the Zephyr shell
is used, so that almost every board should be supported.

Building and Running
********************

Build and flash the sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dma/dmatest
   :board: versalnet_rpu
   :goals: build flash
