.. _lpc54s018_dma_sample:

LPC54S018 DMA Sample
###################

Overview
********

This sample demonstrates the DMA controller on the LPC54S018 board.
It shows:

- Memory-to-memory transfers
- Peripheral-to-memory transfers (UART RX)
- Memory-to-peripheral transfers (UART TX)
- Linked descriptor transfers
- Hardware-triggered transfers

Requirements
************

This sample requires an LPC54S018 board with:

- UART console connected (FLEXCOMM0)
- Optional: Loopback wire between TX and RX pins for UART DMA test

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dma
   :board: lpcxpresso54s018
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** LPC54S018 DMA Sample ***
   Testing memory-to-memory transfer...
   Source buffer: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
   DMA transfer started
   DMA transfer completed
   Destination buffer: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
   Memory-to-memory transfer: PASSED
   
   Testing UART DMA transfers...
   Type some text and press Enter:
   Hello DMA!
   Received via DMA: Hello DMA!
   UART DMA transfer: PASSED
   
   Testing linked descriptors...
   Block 1 transferred
   Block 2 transferred
   Block 3 transferred
   Linked descriptor transfer: PASSED
   
   All DMA tests completed successfully!