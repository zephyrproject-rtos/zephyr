/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#include <stdint.h>

#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/dma.h>

void DMA_enableModule(void)
{
    //
    // Set the master enable bit in the config register.
    //
    DMA_Control->CFG = DMA_CFG_MASTEN;
}

void DMA_disableModule(void)
{
    //
    // Clear the master enable bit in the config register.
    //
    DMA_Control->CFG = 0;
}

uint32_t DMA_getErrorStatus(void)
{
    //
    // Return the DMA error status.
    //
    return DMA_Control->ERRCLR;
}

void DMA_clearErrorStatus(void)
{
    //
    // Clear the DMA error interrupt.
    //
    DMA_Control->ERRCLR = 1;
}

void DMA_enableChannel(uint32_t channelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((channelNum & 0xffff) < 8);

    //
    // Set the bit for this channel in the enable set register.
    //
    DMA_Control->ENASET = 1 << (channelNum & 0x0F);
}

void DMA_disableChannel(uint32_t channelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((channelNum & 0xffff) < 8);

    //
    // Set the bit for this channel in the enable clear register.
    //
    DMA_Control->ENACLR = 1 << (channelNum & 0x0F);
}

bool DMA_isChannelEnabled(uint32_t channelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((channelNum & 0xffff) < 8);

    //
    // AND the specified channel bit with the enable register and return the
    // result.
    //
    return ((DMA_Control->ENASET & (1 << (channelNum & 0x0F))) ? true : false);
}

void DMA_setControlBase(void *controlTable)
{
    //
    // Check the arguments.
    //
    ASSERT(((uint32_t) controlTable & ~0x3FF) == (uint32_t) controlTable);
    ASSERT((uint32_t) controlTable >= 0x20000000);

    //
    // Program the base address into the register.
    //
    DMA_Control->CTLBASE = (uint32_t) controlTable;
}

void* DMA_getControlBase(void)
{
    //
    // Read the current value of the control base register and return it to
    // the caller.
    //
    return ((void *) DMA_Control->CTLBASE);
}

void* DMA_getControlAlternateBase(void)
{
    //
    // Read the current value of the control base register and return it to
    // the caller.
    //
    return ((void *) DMA_Control->ALTBASE);
}

void DMA_requestChannel(uint32_t channelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((channelNum & 0xffff) < 8);

    //
    // Set the bit for this channel in the software DMA request register.
    //
    DMA_Control->SWREQ = 1 << (channelNum & 0x0F);
}

void DMA_enableChannelAttribute(uint32_t channelNum, uint32_t attr)
{
    //
    // Check the arguments.
    //
    ASSERT((channelNum & 0xffff) < 8);
    ASSERT(
            (attr
                    & ~(UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT
                            | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK))
            == 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelNum parameter, extract just the channel number
    // from this parameter.
    //
    channelNum &= 0x0F;

    //
    // Set the useburst bit for this channel if set in config.
    //
    if (attr & UDMA_ATTR_USEBURST)
    {
        DMA_Control->USEBURSTSET = 1 << channelNum;
    }

    //
    // Set the alternate control select bit for this channel,
    // if set in config.
    //
    if (attr & UDMA_ATTR_ALTSELECT)
    {
        DMA_Control->ALTSET = 1 << channelNum;
    }

    //
    // Set the high priority bit for this channel, if set in config.
    //
    if (attr & UDMA_ATTR_HIGH_PRIORITY)
    {
        DMA_Control->PRIOSET = 1 << channelNum;
    }

    //
    // Set the request mask bit for this channel, if set in config.
    //
    if (attr & UDMA_ATTR_REQMASK)
    {
        DMA_Control->REQMASKSET = 1 << channelNum;
    }
}

void DMA_disableChannelAttribute(uint32_t channelNum, uint32_t attr)
{
    //
    // Check the arguments.
    //
    ASSERT((channelNum & 0xffff) < 8);
    ASSERT(
            (attr
                    & ~(UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT
                            | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK))
            == 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelNum parameter, extract just the channel number
    // from this parameter.
    //
    channelNum &= 0x0F;

    //
    // Clear the useburst bit for this channel if set in config.
    //
    if (attr & UDMA_ATTR_USEBURST)
    {
        DMA_Control->USEBURSTCLR = 1 << channelNum;
    }

    //
    // Clear the alternate control select bit for this channel, if set in
    // config.
    //
    if (attr & UDMA_ATTR_ALTSELECT)
    {
        DMA_Control->ALTCLR = 1 << channelNum;
    }

    //
    // Clear the high priority bit for this channel, if set in config.
    //
    if (attr & UDMA_ATTR_HIGH_PRIORITY)
    {
        DMA_Control->PRIOCLR = 1 << channelNum;
    }

    //
    // Clear the request mask bit for this channel, if set in config.
    //
    if (attr & UDMA_ATTR_REQMASK)
    {
        DMA_Control->REQMASKCLR = 1 << channelNum;
    }
}

uint32_t DMA_getChannelAttribute(uint32_t channelNum)
{
    uint32_t attr = 0;

    //
    // Check the arguments.
    //
    ASSERT((channelNum & 0xffff) < 8);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelNum parameter, extract just the channel number
    // from this parameter.
    //
    channelNum &= 0x0F;

    //
    // Check to see if useburst bit is set for this channel.
    //
    if (DMA_Control->USEBURSTSET & (1 << channelNum))
    {
        attr |= UDMA_ATTR_USEBURST;
    }

    //
    // Check to see if the alternate control bit is set for this channel.
    //
    if (DMA_Control->ALTSET & (1 << channelNum))
    {
        attr |= UDMA_ATTR_ALTSELECT;
    }

    //
    // Check to see if the high priority bit is set for this channel.
    //
    if (DMA_Control->PRIOSET & (1 << channelNum))
    {
        attr |= UDMA_ATTR_HIGH_PRIORITY;
    }

    //
    // Check to see if the request mask bit is set for this channel.
    //
    if (DMA_Control->REQMASKSET & (1 << channelNum))
    {
        attr |= UDMA_ATTR_REQMASK;
    }

    //
    // Return the configuration flags.
    //
    return (attr);
}

void DMA_setChannelControl(uint32_t channelStructIndex, uint32_t control)
{
    DMA_ControlTable *pCtl;

    //
    // Check the arguments.
    //
    ASSERT((channelStructIndex & 0xffff) < 64);
    ASSERT(DMA_Control->CTLBASE != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    channelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    pCtl = (DMA_ControlTable *) DMA_Control->CTLBASE;

    //
    // Get the current control word value and mask off the fields to be
    // changed, then OR in the new settings.
    //
    pCtl[channelStructIndex].control = ((pCtl[channelStructIndex].control
            & ~(UDMA_CHCTL_DSTINC_M | UDMA_CHCTL_DSTSIZE_M | UDMA_CHCTL_SRCINC_M
                    | UDMA_CHCTL_SRCSIZE_M | UDMA_CHCTL_ARBSIZE_M
                    | UDMA_CHCTL_NXTUSEBURST)) | control);
}

void DMA_setChannelTransfer(uint32_t channelStructIndex, uint32_t mode,
        void *srcAddr, void *dstAddr, uint32_t transferSize)
{
    DMA_ControlTable *controlTable;
    uint32_t control;
    uint32_t increment;
    uint32_t bufferBytes;

    //
    // Check the arguments.
    //
    ASSERT((channelStructIndex & 0xffff) < 64);
    ASSERT(DMA->CTLBASE != 0);
    ASSERT(mode <= UDMA_MODE_PER_SCATTER_GATHER);
    ASSERT((transferSize != 0) && (transferSize <= 1024));

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    channelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    controlTable = (DMA_ControlTable *) DMA_Control->CTLBASE;

    //
    // Get the current control word value and mask off the mode and size
    // fields.
    //
    control = (controlTable[channelStructIndex].control
            & ~(UDMA_CHCTL_XFERSIZE_M | UDMA_CHCTL_XFERMODE_M));

    //
    // Adjust the mode if the alt control structure is selected.
    //
    if (channelStructIndex & UDMA_ALT_SELECT)
    {
        if ((mode == UDMA_MODE_MEM_SCATTER_GATHER)
                || (mode == UDMA_MODE_PER_SCATTER_GATHER))
        {
            mode |= UDMA_MODE_ALT_SELECT;
        }
    }

    //
    // Set the transfer size and mode in the control word (but don't write the
    // control word yet as it could kick off a transfer).
    //
    control |= mode | ((transferSize - 1) << 4);

    //
    // Get the address increment value for the source, from the control word.
    //
    increment = (control & UDMA_CHCTL_SRCINC_M);

    //
    // Compute the ending source address of the transfer.  If the source
    // increment is set to none, then the ending address is the same as the
    // beginning.
    //
    if (increment != UDMA_SRC_INC_NONE)
    {
        increment = increment >> 26;
        bufferBytes = transferSize << increment;
        srcAddr = (void *) ((uint32_t) srcAddr + bufferBytes - 1);
    }

    //
    // Load the source ending address into the control block.
    //
    controlTable[channelStructIndex].srcEndAddr = srcAddr;

    //
    // Get the address increment value for the destination, from the control
    // word.
    //
    increment = control & UDMA_CHCTL_DSTINC_M;

    //
    // Compute the ending destination address of the transfer.  If the
    // destination increment is set to none, then the ending address is the
    // same as the beginning.
    //
    if (increment != UDMA_DST_INC_NONE)
    {
        //
        // There is a special case if this is setting up a scatter-gather
        // transfer.  The destination pointer must point to the end of
        // the alternate structure for this channel instead of calculating
        // the end of the buffer in the normal way.
        //
        if ((mode == UDMA_MODE_MEM_SCATTER_GATHER)
                || (mode == UDMA_MODE_PER_SCATTER_GATHER))
        {
            dstAddr = (void *) &controlTable[channelStructIndex
                    | UDMA_ALT_SELECT].spare;
        }
        //
        // Not a scatter-gather transfer, calculate end pointer normally.
        //
        else
        {
            increment = increment >> 30;
            bufferBytes = transferSize << increment;
            dstAddr = (void *) ((uint32_t) dstAddr + bufferBytes - 1);
        }
    }

    //
    // Load the destination ending address into the control block.
    //
    controlTable[channelStructIndex].dstEndAddr = dstAddr;

    //
    // Write the new control word value.
    //
    controlTable[channelStructIndex].control = control;
}

void DMA_setChannelScatterGather(uint32_t channelNum, uint32_t taskCount,
        void *taskList, uint32_t isPeriphSG)
{
    DMA_ControlTable *controlTable;
    DMA_ControlTable *pTaskTable;

    //
    // Check the parameters
    //
    ASSERT((channelNum & 0xffff) < 8);
    ASSERT(DMA->CTLBASE != 0);
    ASSERT(taskList != 0);
    ASSERT(taskCount <= 1024);
    ASSERT(taskCount != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelNum parameter, extract just the channel number
    // from this parameter.
    //
    channelNum &= 0x0F;

    //
    // Get the base address of the control table.
    //
    controlTable = (DMA_ControlTable *) DMA_Control->CTLBASE;

    //
    // Get a handy pointer to the task list
    //
    pTaskTable = (DMA_ControlTable *) taskList;

    //
    // Compute the ending address for the source pointer.  This address is the
    // last element of the last task in the task table
    //
    controlTable[channelNum].srcEndAddr = &pTaskTable[taskCount - 1].spare;

    //
    // Compute the ending address for the destination pointer.  This address
    // is the end of the alternate structure for this channel.
    //
    controlTable[channelNum].dstEndAddr = &controlTable[channelNum
            | UDMA_ALT_SELECT].spare;

    //
    // Compute the control word.  Most configurable items are fixed for
    // scatter-gather.  Item and increment sizes are all 32-bit and arb
    // size must be 4.  The count is the number of items in the task list
    // times 4 (4 words per task).
    //
    controlTable[channelNum].control = (UDMA_CHCTL_DSTINC_32
            | UDMA_CHCTL_DSTSIZE_32 | UDMA_CHCTL_SRCINC_32
            | UDMA_CHCTL_SRCSIZE_32 | UDMA_CHCTL_ARBSIZE_4
            | (((taskCount * 4) - 1) << UDMA_CHCTL_XFERSIZE_S)
            | (isPeriphSG ?
            UDMA_CHCTL_XFERMODE_PER_SG :
                            UDMA_CHCTL_XFERMODE_MEM_SG));

    //
    // Scatter-gather operations can leave the alt bit set.  So if doing
    // back to back scatter-gather transfers, the second attempt may not
    // work correctly because the alt bit is set.  Therefore, clear the
    // alt bit here to ensure that it is always cleared before a new SG
    // transfer is started.
    //
    DMA_Control->ALTCLR = 1 << channelNum;
}

uint32_t DMA_getChannelSize(uint32_t channelStructIndex)
{
    DMA_ControlTable *controlTable;
    uint32_t control;

    //
    // Check the arguments.
    //
    ASSERT((channelStructIndex & 0xffff) < 16);
    ASSERT(DMA->CTLBASE != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    channelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    controlTable = (DMA_ControlTable *) DMA_Control->CTLBASE;

    //
    // Get the current control word value and mask off all but the size field
    // and the mode field.
    //
    control = (controlTable[channelStructIndex].control
            & (UDMA_CHCTL_XFERSIZE_M | UDMA_CHCTL_XFERMODE_M));

    //
    // If the size field and mode field are 0 then the transfer is finished
    // and there are no more items to transfer
    //
    if (control == 0)
    {
        return (0);
    }

    //
    // Otherwise, if either the size field or more field is non-zero, then
    // not all the items have been transferred.
    //
    else
    {
        //
        // Shift the size field and add one, then return to user.
        //
        return ((control >> 4) + 1);
    }
}

uint32_t DMA_getChannelMode(uint32_t channelStructIndex)
{
    DMA_ControlTable *controlTable;
    uint32_t control;

    //
    // Check the arguments.
    //
    ASSERT((channelStructIndex & 0xffff) < 64);
    ASSERT(DMA->CTLBASE != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the channelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    channelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    controlTable = (DMA_ControlTable *) DMA_Control->CTLBASE;

    //
    // Get the current control word value and mask off all but the mode field.
    //
    control =
            (controlTable[channelStructIndex].control & UDMA_CHCTL_XFERMODE_M);

    //
    // Check if scatter/gather mode, and if so, mask off the alt bit.
    //
    if (((control & ~UDMA_MODE_ALT_SELECT) == UDMA_MODE_MEM_SCATTER_GATHER)
            || ((control & ~UDMA_MODE_ALT_SELECT)
                    == UDMA_MODE_PER_SCATTER_GATHER))
    {
        control &= ~UDMA_MODE_ALT_SELECT;
    }

    //
    // Return the mode to the caller.
    //
    return (control);
}

void DMA_assignChannel(uint32_t mapping)
{
    switch (mapping)
    {
    case DMA_CH0_RESERVED0:
    case DMA_CH0_EUSCIA0TX:
    case DMA_CH0_EUSCIB0TX0:
    case DMA_CH0_EUSCIB3TX1:
    case DMA_CH0_EUSCIB2TX2:
    case DMA_CH0_EUSCIB1TX3:
    case DMA_CH0_TIMERA0CCR0:
    case DMA_CH0_AESTRIGGER0:
        DMA_Channel->CH_SRCCFG[0] = (mapping >> 24) & 0x1F;
        break;
    case DMA_CH1_RESERVED0:
    case DMA_CH1_EUSCIA0RX:
    case DMA_CH1_EUSCIB0RX0:
    case DMA_CH1_EUSCIB3RX1:
    case DMA_CH1_EUSCIB2RX2:
    case DMA_CH1_EUSCIB1RX3:
    case DMA_CH1_TIMERA0CCR2:
    case DMA_CH1_AESTRIGGER1:
        DMA_Channel->CH_SRCCFG[1] = (mapping >> 24) & 0x1F;
        break;
    case DMA_CH2_RESERVED0:
    case DMA_CH2_EUSCIA1TX:
    case DMA_CH2_EUSCIB1TX0:
    case DMA_CH2_EUSCIB0TX1:
    case DMA_CH2_EUSCIB3TX2:
    case DMA_CH2_EUSCIB2TX3:
    case DMA_CH2_TIMERA1CCR0:
    case DMA_CH2_AESTRIGGER2:
        DMA_Channel->CH_SRCCFG[2] = (mapping >> 24) & 0x1F;
        break;
    case DMA_CH3_RESERVED0:
    case DMA_CH3_EUSCIA1RX:
    case DMA_CH3_EUSCIB1RX0:
    case DMA_CH3_EUSCIB0RX1:
    case DMA_CH3_EUSCIB3RX2:
    case DMA_CH3_EUSCIB2RX3:
    case DMA_CH3_TIMERA1CCR2:
    case DMA_CH3_RESERVED1:
        DMA_Channel->CH_SRCCFG[3] = (mapping >> 24) & 0x1F;
        break;
    case DMA_CH4_RESERVED0:
    case DMA_CH4_EUSCIA2TX:
    case DMA_CH4_EUSCIB2TX0:
    case DMA_CH4_EUSCIB1TX1:
    case DMA_CH4_EUSCIB0TX2:
    case DMA_CH4_EUSCIB3TX3:
    case DMA_CH4_TIMERA2CCR0:
    case DMA_CH4_RESERVED1:
        DMA_Channel->CH_SRCCFG[4] = (mapping >> 24) & 0x1F;
        break;
    case DMA_CH5_RESERVED0:
    case DMA_CH5_EUSCIA2RX:
    case DMA_CH5_EUSCIB2RX0:
    case DMA_CH5_EUSCIB1RX1:
    case DMA_CH5_EUSCIB0RX2:
    case DMA_CH5_EUSCIB3RX3:
    case DMA_CH5_TIMERA2CCR2:
    case DMA_CH5_RESERVED1:
        DMA_Channel->CH_SRCCFG[5] = (mapping >> 24) & 0x1F;
        break;
    case DMA_CH6_RESERVED0:
    case DMA_CH6_EUSCIA3TX:
    case DMA_CH6_EUSCIB3TX0:
    case DMA_CH6_EUSCIB2TX1:
    case DMA_CH6_EUSCIB1TX2:
    case DMA_CH6_EUSCIB0TX3:
    case DMA_CH6_TIMERA3CCR0:
    case DMA_CH6_EXTERNALPIN:
        DMA_Channel->CH_SRCCFG[6] = (mapping >> 24) & 0x1F;
        break;
    case DMA_CH7_RESERVED0:
    case DMA_CH7_EUSCIA3RX:
    case DMA_CH7_EUSCIB3RX0:
    case DMA_CH7_EUSCIB2RX1:
    case DMA_CH7_EUSCIB1RX2:
    case DMA_CH7_EUSCIB0RX3:
    case DMA_CH7_TIMERA3CCR2:
    case DMA_CH7_ADC14:
        DMA_Channel->CH_SRCCFG[7] = (mapping >> 24) & 0x1F;
        break;
    default:
        ASSERT(false);
    }

}

void DMA_assignInterrupt(uint32_t interruptNumber, uint32_t channel)
{
    ASSERT(
            interruptNumber == DMA_INT1 || interruptNumber == DMA_INT2
            || interruptNumber == DMA_INT3);

    if (interruptNumber == DMA_INT1)
    {
        DMA_Channel->INT1_SRCCFG = (DMA_Channel->INT1_SRCCFG
                & ~DMA_INT1_SRCCFG_INT_SRC_MASK) | channel;
    } else if (interruptNumber == DMA_INT2)
    {
        DMA_Channel->INT2_SRCCFG = (DMA_Channel->INT2_SRCCFG
                & ~DMA_INT1_SRCCFG_INT_SRC_MASK) | channel;
    } else if (interruptNumber == DMA_INT3)
    {
        DMA_Channel->INT3_SRCCFG = (DMA_Channel->INT3_SRCCFG
                & ~DMA_INT1_SRCCFG_INT_SRC_MASK) | channel;
    }

    /* Enabling the assigned interrupt */
    DMA_enableInterrupt(interruptNumber);
}

void DMA_requestSoftwareTransfer(uint32_t channel)
{
    DMA_Channel->SW_CHTRIG |= (1 << channel);
}

uint32_t DMA_getInterruptStatus(void)
{
    return DMA_Channel->INT0_SRCFLG;
}

void DMA_clearInterruptFlag(uint32_t channel)
{
    DMA_Channel->INT0_CLRFLG |= (1 << channel);
}

void DMA_enableInterrupt(uint32_t interruptNumber)
{
    ASSERT(
            (interruptNumber == DMA_INT0) || (interruptNumber == DMA_INT1)
            || (interruptNumber == DMA_INT2)
            || (interruptNumber == DMA_INT3));

    if (interruptNumber == DMA_INT1)
    {
        DMA_Channel->INT1_SRCCFG |= DMA_INT1_SRCCFG_EN;
    } else if (interruptNumber == DMA_INT2)
    {
        DMA_Channel->INT2_SRCCFG |= DMA_INT2_SRCCFG_EN;
    } else if (interruptNumber == DMA_INT3)
    {
        DMA_Channel->INT3_SRCCFG |= DMA_INT3_SRCCFG_EN;
    }

}

void DMA_disableInterrupt(uint32_t interruptNumber)
{
    ASSERT(
            (interruptNumber == DMA_INT0) || (interruptNumber == DMA_INT1)
            || (interruptNumber == DMA_INT2)
            || (interruptNumber == DMA_INT3));

    if (interruptNumber == DMA_INT1)
    {
        DMA_Channel->INT1_SRCCFG &= ~DMA_INT1_SRCCFG_EN;
    } else if (interruptNumber == DMA_INT2)
    {
        DMA_Channel->INT2_SRCCFG &= ~DMA_INT2_SRCCFG_EN;
    } else if (interruptNumber == DMA_INT3)
    {
        DMA_Channel->INT3_SRCCFG &= ~DMA_INT3_SRCCFG_EN;
    }
}

void DMA_registerInterrupt(uint32_t interruptNumber, void (*intHandler)(void))
{
    //
    // Check the arguments.
    //
    ASSERT(intHandler);
    ASSERT(
            (interruptNumber == DMA_INT0) || (interruptNumber == DMA_INT1)
            || (interruptNumber == DMA_INT2)
            || (interruptNumber == DMA_INT3)
            || (interruptNumber == DMA_INTERR));

    //
    // Register the interrupt handler.
    //
    Interrupt_registerInterrupt(interruptNumber, intHandler);

    //
    // Enable the memory management fault.
    //
    Interrupt_enableInterrupt(interruptNumber);

}

void DMA_unregisterInterrupt(uint32_t interruptNumber)
{
    ASSERT(
            (interruptNumber == DMA_INT0) || (interruptNumber == DMA_INT1)
            || (interruptNumber == DMA_INT2)
            || (interruptNumber == DMA_INT3)
            || (interruptNumber == DMA_INTERR));

    //
    // Disable the interrupt.
    //
    Interrupt_disableInterrupt(interruptNumber);

    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(interruptNumber);
}

