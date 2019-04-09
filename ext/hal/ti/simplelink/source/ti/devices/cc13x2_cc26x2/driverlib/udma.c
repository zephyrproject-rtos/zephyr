/******************************************************************************
*  Filename:       udma.c
*  Revised:        2017-04-26 18:27:45 +0200 (Wed, 26 Apr 2017)
*  Revision:       48852
*
*  Description:    Driver for the uDMA controller
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#include "udma.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  uDMAChannelAttributeEnable
    #define uDMAChannelAttributeEnable      NOROM_uDMAChannelAttributeEnable
    #undef  uDMAChannelAttributeDisable
    #define uDMAChannelAttributeDisable     NOROM_uDMAChannelAttributeDisable
    #undef  uDMAChannelAttributeGet
    #define uDMAChannelAttributeGet         NOROM_uDMAChannelAttributeGet
    #undef  uDMAChannelControlSet
    #define uDMAChannelControlSet           NOROM_uDMAChannelControlSet
    #undef  uDMAChannelTransferSet
    #define uDMAChannelTransferSet          NOROM_uDMAChannelTransferSet
    #undef  uDMAChannelScatterGatherSet
    #define uDMAChannelScatterGatherSet     NOROM_uDMAChannelScatterGatherSet
    #undef  uDMAChannelSizeGet
    #define uDMAChannelSizeGet              NOROM_uDMAChannelSizeGet
    #undef  uDMAChannelModeGet
    #define uDMAChannelModeGet              NOROM_uDMAChannelModeGet
#endif

//*****************************************************************************
//
// Enables attributes of a uDMA channel
//
//*****************************************************************************
void
uDMAChannelAttributeEnable(uint32_t ui32Base, uint32_t ui32ChannelNum,
                           uint32_t ui32Attr)
{
    // Check the arguments.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelNum < UDMA_NUM_CHANNELS);
    ASSERT((ui32Attr & ~(UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                         UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK)) == 0);

    // Set the useburst bit for this channel if set in ui32Attr.
    if(ui32Attr & UDMA_ATTR_USEBURST)
    {
        HWREG(ui32Base + UDMA_O_SETBURST) = 1 << ui32ChannelNum;
    }

    // Set the alternate control select bit for this channel,
    // if set in ui32Attr.
    if(ui32Attr & UDMA_ATTR_ALTSELECT)
    {
        HWREG(ui32Base + UDMA_O_SETCHNLPRIALT) = 1 << ui32ChannelNum;
    }

    // Set the high priority bit for this channel, if set in ui32Attr.
    if(ui32Attr & UDMA_ATTR_HIGH_PRIORITY)
    {
        HWREG(ui32Base + UDMA_O_SETCHNLPRIORITY) = 1 << ui32ChannelNum;
    }

    // Set the request mask bit for this channel, if set in ui32Attr.
    if(ui32Attr & UDMA_ATTR_REQMASK)
    {
        HWREG(ui32Base + UDMA_O_SETREQMASK) = 1 << ui32ChannelNum;
    }
}

//*****************************************************************************
//
// Disables attributes of an uDMA channel
//
//*****************************************************************************
void
uDMAChannelAttributeDisable(uint32_t ui32Base, uint32_t ui32ChannelNum,
                            uint32_t ui32Attr)
{
    // Check the arguments.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelNum < UDMA_NUM_CHANNELS);
    ASSERT((ui32Attr & ~(UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                         UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK)) == 0);

    // Clear the useburst bit for this channel if set in ui32Attr.
    if(ui32Attr & UDMA_ATTR_USEBURST)
    {
        HWREG(ui32Base + UDMA_O_CLEARBURST) = 1 << ui32ChannelNum;
    }

    // Clear the alternate control select bit for this channel, if set in
    // ululAttr.
    if(ui32Attr & UDMA_ATTR_ALTSELECT)
    {
        HWREG(ui32Base + UDMA_O_CLEARCHNLPRIALT) = 1 << ui32ChannelNum;
    }

    // Clear the high priority bit for this channel, if set in ui32Attr.
    if(ui32Attr & UDMA_ATTR_HIGH_PRIORITY)
    {
        HWREG(ui32Base + UDMA_O_CLEARCHNLPRIORITY) = 1 << ui32ChannelNum;
    }

    // Clear the request mask bit for this channel, if set in ui32Attr.
    if(ui32Attr & UDMA_ATTR_REQMASK)
    {
        HWREG(ui32Base + UDMA_O_CLEARREQMASK) = 1 << ui32ChannelNum;
    }
}

//*****************************************************************************
//
// Gets the enabled attributes of a uDMA channel
//
//*****************************************************************************
uint32_t
uDMAChannelAttributeGet(uint32_t ui32Base, uint32_t ui32ChannelNum)
{
    uint32_t ui32Attr = 0;

    // Check the arguments.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelNum < UDMA_NUM_CHANNELS);

    // Check to see if useburst bit is set for this channel.
    if(HWREG(ui32Base + UDMA_O_SETBURST) & (1 << ui32ChannelNum))
    {
        ui32Attr |= UDMA_ATTR_USEBURST;
    }

    // Check to see if the alternate control bit is set for this channel.
    if(HWREG(ui32Base + UDMA_O_SETCHNLPRIALT) & (1 << ui32ChannelNum))
    {
        ui32Attr |= UDMA_ATTR_ALTSELECT;
    }

    // Check to see if the high priority bit is set for this channel.
    if(HWREG(ui32Base + UDMA_O_SETCHNLPRIORITY) & (1 << ui32ChannelNum))
    {
        ui32Attr |= UDMA_ATTR_HIGH_PRIORITY;
    }

    // Check to see if the request mask bit is set for this channel.
    if(HWREG(ui32Base + UDMA_O_SETREQMASK) & (1 << ui32ChannelNum))
    {
        ui32Attr |= UDMA_ATTR_REQMASK;
    }

    // Return the configuration flags.
    return(ui32Attr);
}

//*****************************************************************************
//
// Sets the control parameters for a uDMA channel control structure
//
//*****************************************************************************
void
uDMAChannelControlSet(uint32_t ui32Base, uint32_t ui32ChannelStructIndex,
                      uint32_t ui32Control)
{
    tDMAControlTable *pControlTable;

    // Check the arguments.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelStructIndex < (UDMA_NUM_CHANNELS * 2));
    ASSERT(HWREG(ui32Base + UDMA_O_CTRL) != 0);

    // Get the base address of the control table.
    pControlTable = (tDMAControlTable *)HWREG(ui32Base + UDMA_O_CTRL);

    // Get the current control word value and mask off the fields to be
    // changed, then OR in the new settings.
    pControlTable[ui32ChannelStructIndex].ui32Control =
        ((pControlTable[ui32ChannelStructIndex].ui32Control &
          ~(UDMA_DST_INC_M |
            UDMA_SRC_INC_M |
            UDMA_SIZE_M |
            UDMA_ARB_M |
            UDMA_NEXT_USEBURST)) |
         ui32Control);
}

//*****************************************************************************
//
// Sets the transfer parameters for a uDMA channel control structure
//
//*****************************************************************************
void
uDMAChannelTransferSet(uint32_t ui32Base, uint32_t ui32ChannelStructIndex,
                       uint32_t ui32Mode, void *pvSrcAddr, void *pvDstAddr,
                       uint32_t ui32TransferSize)
{
    tDMAControlTable *pControlTable;
    uint32_t ui32Control;
    uint32_t ui32Inc;
    uint32_t ui32BufferBytes;

    // Check the arguments.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelStructIndex < (UDMA_NUM_CHANNELS * 2));
    ASSERT(HWREG(ui32Base + UDMA_O_CTRL) != 0);
    ASSERT(ui32Mode <= UDMA_MODE_PER_SCATTER_GATHER);
    ASSERT((uint32_t)pvSrcAddr >= SRAM_BASE);
    ASSERT((uint32_t)pvDstAddr >= SRAM_BASE);
    ASSERT((ui32TransferSize != 0) && (ui32TransferSize <= UDMA_XFER_SIZE_MAX));

    // Get the base address of the control table.
    pControlTable = (tDMAControlTable *)HWREG(ui32Base + UDMA_O_CTRL);

    // Get the current control word value and mask off the mode and size
    // fields.
    ui32Control = (pControlTable[ui32ChannelStructIndex].ui32Control &
                   ~(UDMA_XFER_SIZE_M | UDMA_MODE_M));

    // Adjust the mode if the alt control structure is selected.
    if(ui32ChannelStructIndex & UDMA_ALT_SELECT)
    {
        if((ui32Mode == UDMA_MODE_MEM_SCATTER_GATHER) ||
           (ui32Mode == UDMA_MODE_PER_SCATTER_GATHER))
        {
            ui32Mode |= UDMA_MODE_ALT_SELECT;
        }
    }

    // Set the transfer size and mode in the control word (but don't write the
    // control word yet as it could kick off a transfer).
    ui32Control |= ui32Mode | ((ui32TransferSize - 1) << UDMA_XFER_SIZE_S);

    // Get the address increment value for the source, from the control word.
    ui32Inc = (ui32Control & UDMA_SRC_INC_M);

    // Compute the ending source address of the transfer.  If the source
    // increment is set to none, then the ending address is the same as the
    // beginning.
    if(ui32Inc != UDMA_SRC_INC_NONE)
    {
        ui32Inc = ui32Inc >> UDMA_SRC_INC_S;
        ui32BufferBytes = ui32TransferSize << ui32Inc;
        pvSrcAddr = (void *)((uint32_t)pvSrcAddr + ui32BufferBytes - (1 << ui32Inc));
    }

    // Load the source ending address into the control block.
    pControlTable[ui32ChannelStructIndex].pvSrcEndAddr = pvSrcAddr;

    // Get the address increment value for the destination, from the control
    // word.
    ui32Inc = ui32Control & UDMA_DST_INC_M;

    // Compute the ending destination address of the transfer.  If the
    // destination increment is set to none, then the ending address is the
    // same as the beginning.
    if(ui32Inc != UDMA_DST_INC_NONE)
    {
        // There is a special case if this is setting up a scatter-gather
        // transfer.  The destination pointer needs to point to the end of
        // the alternate structure for this channel instead of calculating
        // the end of the buffer in the normal way.
        if((ui32Mode == UDMA_MODE_MEM_SCATTER_GATHER) ||
           (ui32Mode == UDMA_MODE_PER_SCATTER_GATHER))
        {
            pvDstAddr =
                (void *)&pControlTable[ui32ChannelStructIndex |
                                       UDMA_ALT_SELECT].ui32Spare;
        }
        // Not a scatter-gather transfer, calculate end pointer normally.
        else
        {
            ui32Inc = ui32Inc >> UDMA_DST_INC_S;
            ui32BufferBytes = ui32TransferSize << ui32Inc;
            pvDstAddr = (void *)((uint32_t)pvDstAddr + ui32BufferBytes - 1);
        }
    }

    // Load the destination ending address into the control block.
    pControlTable[ui32ChannelStructIndex].pvDstEndAddr = pvDstAddr;

    // Write the new control word value.
    pControlTable[ui32ChannelStructIndex].ui32Control = ui32Control;
}

//*****************************************************************************
//
// Configures a uDMA channel for scatter-gather mode
//
//*****************************************************************************
void
uDMAChannelScatterGatherSet(uint32_t ui32Base, uint32_t ui32ChannelNum,
                            uint32_t ui32TaskCount, void *pvTaskList,
                            uint32_t ui32IsPeriphSG)
{
    tDMAControlTable *pControlTable;
    tDMAControlTable *pTaskTable;

    // Check the parameters.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelNum < UDMA_NUM_CHANNELS);
    ASSERT(HWREG(ui32Base + UDMA_O_CTRL) != 0);
    ASSERT(pvTaskList != 0);
    ASSERT(ui32TaskCount <= UDMA_XFER_SIZE_MAX);
    ASSERT(ui32TaskCount != 0);

    // Get the base address of the control table.
    pControlTable = (tDMAControlTable *)HWREG(ui32Base + UDMA_O_CTRL);

    // Get a handy pointer to the task list.
    pTaskTable = (tDMAControlTable *)pvTaskList;

    // Compute the ending address for the source pointer. This will be the
    // last element of the last task in the task table.
    pControlTable[ui32ChannelNum].pvSrcEndAddr =
        &pTaskTable[ui32TaskCount - 1].ui32Spare;

    // Compute the ending address for the destination pointer. This will be
    // the end of the alternate structure for this channel.
    pControlTable[ui32ChannelNum].pvDstEndAddr =
        &pControlTable[ui32ChannelNum | UDMA_ALT_SELECT].ui32Spare;

    // Compute the control word.  Most configurable items are fixed for
    // scatter-gather. Item and increment sizes are all 32-bit and arb
    // size must be 4. The count is the number of items in the task list
    // times 4 (4 words per task).
    pControlTable[ui32ChannelNum].ui32Control =
        (UDMA_DST_INC_32 | UDMA_SRC_INC_32 |
         UDMA_SIZE_32 | UDMA_ARB_4 |
         (((ui32TaskCount * 4) - 1) << UDMA_XFER_SIZE_S) |
         (ui32IsPeriphSG ? UDMA_MODE_PER_SCATTER_GATHER :
          UDMA_MODE_MEM_SCATTER_GATHER));

    // Scatter-gather operations can leave the alt bit set.  So if doing
    // back to back scatter-gather transfers, the second attempt may not
    // work correctly because the alt bit is set.  Therefore, clear the
    // alt bit here to ensure that it is always cleared before a new SG
    // transfer is started.
    HWREG(ui32Base + UDMA_O_CLEARCHNLPRIALT) = 1 << ui32ChannelNum;

}

//*****************************************************************************
//
// Gets the current transfer size for a uDMA channel control structure
//
//*****************************************************************************
uint32_t
uDMAChannelSizeGet(uint32_t ui32Base, uint32_t ui32ChannelStructIndex)
{
    tDMAControlTable *pControlTable;
    uint32_t ui32Control;

    // Check the arguments.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelStructIndex < (UDMA_NUM_CHANNELS * 2));
    ASSERT(HWREG(ui32Base + UDMA_O_CTRL) != 0);

    // Get the base address of the control table.
    pControlTable = (tDMAControlTable *)HWREG(ui32Base + UDMA_O_CTRL);

    // Get the current control word value and mask off all but the size field
    // and the mode field.
    ui32Control = (pControlTable[ui32ChannelStructIndex].ui32Control &
                   (UDMA_XFER_SIZE_M | UDMA_MODE_M));

    // If the size field and mode field are 0 then the transfer is finished
    // and there are no more items to transfer.
    if(ui32Control == 0)
    {
        return(0);
    }

    // Otherwise, if either the size field or more field is non-zero, then
    // not all the items have been transferred.
    else
    {
        // Shift the size field and add one, then return to user.
        return((ui32Control >> UDMA_XFER_SIZE_S) + 1);
    }
}

//*****************************************************************************
//
// Gets the transfer mode for a uDMA channel control structure
//
//*****************************************************************************
uint32_t
uDMAChannelModeGet(uint32_t ui32Base, uint32_t ui32ChannelStructIndex)
{
    tDMAControlTable *pControlTable;
    uint32_t ui32Control;

    // Check the arguments.
    ASSERT(uDMABaseValid(ui32Base));
    ASSERT(ui32ChannelStructIndex < (UDMA_NUM_CHANNELS * 2));
    ASSERT(HWREG(ui32Base + UDMA_O_CTRL) != 0);

    // Get the base address of the control table.
    pControlTable = (tDMAControlTable *)HWREG(ui32Base + UDMA_O_CTRL);

    // Get the current control word value and mask off all but the mode field.
    ui32Control = (pControlTable[ui32ChannelStructIndex].ui32Control &
                   UDMA_MODE_M);

    // Check if scatter/gather mode, and if so, mask off the alt bit.
    if(((ui32Control & ~UDMA_MODE_ALT_SELECT) == UDMA_MODE_MEM_SCATTER_GATHER) ||
       ((ui32Control & ~UDMA_MODE_ALT_SELECT) == UDMA_MODE_PER_SCATTER_GATHER))
    {
        ui32Control &= ~UDMA_MODE_ALT_SELECT;
    }

    // Return the mode to the caller.
    return(ui32Control);
}
