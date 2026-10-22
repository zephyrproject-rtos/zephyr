.. zephyr:code-sample:: nxp_smartdma_mem_to_mem
   :name: NXP SmartDMA memory-to-memory
   :relevant-api: dma_interface

   Use the NXP SmartDMA coprocessor to perform a memory-to-memory
   transfer through the Zephyr DMA API.

Overview
********

This sample demonstrates how to use the NXP SmartDMA coprocessor to perform a
memory-to-memory transfer with no CPU involvement in the data path.

The SmartDMA is a small programmable coprocessor found on several NXP MCX
devices. It executes firmware routines that operate directly on system memory.
This sample installs the standard MCUX SmartDMA firmware and invokes one of its
memory-to-memory routines, which reads a source buffer from memory, applies the
firmware's fixed byte transformation, and writes the result to a destination
buffer in memory. The ``kSMARTDMA_RGB565To888`` routine and the
:c:struct:`smartdma_rgb565_rgb888_param_t` type are the SDK-defined identifiers
for the routine used here; they are used purely as the mechanism to exercise a
SmartDMA memory-to-memory transfer.

The SmartDMA firmware routine is selected through the Zephyr DMA API by setting
the ``dma_slot`` field to the firmware routine index. The firmware reads its
parameter block (source buffer, destination buffer, size and a private stack)
from the address the SmartDMA driver programs from ``dma_config.head_block``.
This sample therefore aliases ``head_block`` to point at the firmware parameter
structure.

When the routine completes, the SmartDMA raises an interrupt that invokes the
DMA callback, which the sample uses to know the transfer has finished. The
sample then recomputes the firmware's byte transformation on the CPU and
verifies the output buffer matches.

Requirements
************

This sample requires a board with an NXP SmartDMA coprocessor and its clock
enabled. It has been tested on the :zephyr:board:`mcx_n9xx_evk` and
:zephyr:board:`mcx_n5xx_evk`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/smartdma_mem_to_mem
   :board: mcx_n9xx_evk/mcxn947/cpu0
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   Starting SmartDMA memory-to-memory transfer
   Transfer complete, results:
   word 0: in 0x0000 -> out 00 00 00
   ...
   Result check passed: all 32 words transformed correctly
   SmartDMA memory-to-memory sample done
