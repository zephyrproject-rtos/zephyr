//*****************************************************************************
//
//  udma.c
//
//  Driver for the micro-DMA controller.
//
//  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup uDMA_Micro_Direct_Memory_Access_api
//! @{
//
//*****************************************************************************


#include "inc/hw_types.h"
#include "inc/hw_udma.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "debug.h"
#include "interrupt.h"
#include "udma.h"


//*****************************************************************************
//
//! Enables the uDMA controller for use.
//!
//! This function enables the uDMA controller.  The uDMA controller must be
//! enabled before it can be configured and used.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAEnable(void)
{
    //
    // Set the master enable bit in the config register.
    //
    HWREG(UDMA_BASE + UDMA_O_CFG) = UDMA_CFG_MASTEN;
}

//*****************************************************************************
//
//! Disables the uDMA controller for use.
//!
//! This function disables the uDMA controller.  Once disabled, the uDMA
//! controller cannot operate until re-enabled with uDMAEnable().
//!
//! \return None.
//
//*****************************************************************************
void
uDMADisable(void)
{
    //
    // Clear the master enable bit in the config register.
    //
    HWREG(UDMA_BASE + UDMA_O_CFG) = 0;
}

//*****************************************************************************
//
//! Gets the uDMA error status.
//!
//! This function returns the uDMA error status.  It should be called from
//! within the uDMA error interrupt handler to determine if a uDMA error
//! occurred.
//!
//! \return Returns non-zero if a uDMA error is pending.
//
//*****************************************************************************
unsigned long
uDMAErrorStatusGet(void)
{
    //
    // Return the uDMA error status.
    //
    return(HWREG(UDMA_BASE + UDMA_O_ERRCLR));
}

//*****************************************************************************
//
//! Clears the uDMA error interrupt.
//!
//! This function clears a pending uDMA error interrupt.  This function should
//! be called from within the uDMA error interrupt handler to clear the
//! interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAErrorStatusClear(void)
{
    //
    // Clear the uDMA error interrupt.
    //
    HWREG(UDMA_BASE + UDMA_O_ERRCLR) = 1;
}

//*****************************************************************************
//
//! Enables a uDMA channel for operation.
//!
//! \param ulChannelNum is the channel number to enable.
//!
//! This function enables a specific uDMA channel for use.  This function must
//! be used to enable a channel before it can be used to perform a uDMA
//! transfer.
//!
//! When a uDMA transfer is completed, the channel is automatically disabled by
//! the uDMA controller.  Therefore, this function should be called prior to
//! starting up any new transfer.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelEnable(unsigned long ulChannelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum & 0xffff) < 32);

    //
    // Set the bit for this channel in the enable set register.
    //
    HWREG(UDMA_BASE + UDMA_O_ENASET) = 1 << (ulChannelNum & 0x1f);
}

//*****************************************************************************
//
//! Disables a uDMA channel for operation.
//!
//! \param ulChannelNum is the channel number to disable.
//!
//! This function disables a specific uDMA channel.  Once disabled, a channel
//! cannot respond to uDMA transfer requests until re-enabled via
//! uDMAChannelEnable().
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelDisable(unsigned long ulChannelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum & 0xffff) < 32);

    //
    // Set the bit for this channel in the enable clear register.
    //
    HWREG(UDMA_BASE + UDMA_O_ENACLR) = 1 << (ulChannelNum & 0x1f);
}

//*****************************************************************************
//
//! Checks if a uDMA channel is enabled for operation.
//!
//! \param ulChannelNum is the channel number to check.
//!
//! This function checks to see if a specific uDMA channel is enabled.  This
//! function can be used to check the status of a transfer, as the channel is
//! automatically disabled at the end of a transfer.
//!
//! \return Returns \b true if the channel is enabled, \b false if disabled.
//
//*****************************************************************************
tBoolean
uDMAChannelIsEnabled(unsigned long ulChannelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum & 0xffff) < 32);

    //
    // AND the specified channel bit with the enable register and return the
    // result.
    //
    return((HWREG(UDMA_BASE + UDMA_O_ENASET) &
            (1 << (ulChannelNum & 0x1f))) ? true : false);
}

//*****************************************************************************
//
//! Sets the base address for the channel control table.
//!
//! \param pControlTable is a pointer to the 1024-byte-aligned base address
//! of the uDMA channel control table.
//!
//! This function configures the base address of the channel control table.
//! This table resides in system memory and holds control information for each
//! uDMA channel.  The table must be aligned on a 1024-byte boundary.  The base
//! address must be configured before any of the channel functions can be used.
//!
//! The size of the channel control table depends on the number of uDMA
//! channels and the transfer modes that are used.  Refer to the introductory
//! text and the microcontroller datasheet for more information about the
//! channel control table.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAControlBaseSet(void *pControlTable)
{
    //
    // Check the arguments.
    //
    ASSERT(((unsigned long)pControlTable & ~0x3FF) ==
            (unsigned long)pControlTable);
    ASSERT((unsigned long)pControlTable >= 0x20000000);

    //
    // Program the base address into the register.
    //
    HWREG(UDMA_BASE + UDMA_O_CTLBASE) = (unsigned long)pControlTable;
}

//*****************************************************************************
//
//! Gets the base address for the channel control table.
//!
//! This function gets the base address of the channel control table.  This
//! table resides in system memory and holds control information for each uDMA
//! channel.
//!
//! \return Returns a pointer to the base address of the channel control table.
//
//*****************************************************************************
void *
uDMAControlBaseGet(void)
{
    //
    // Read the current value of the control base register and return it to
    // the caller.
    //
    return((void *)HWREG(UDMA_BASE + UDMA_O_CTLBASE));
}

//*****************************************************************************
//
//! Gets the base address for the channel control table alternate structures.
//!
//! This function gets the base address of the second half of the channel
//! control table that holds the alternate control structures for each channel.
//!
//! \return Returns a pointer to the base address of the second half of the
//! channel control table.
//
//*****************************************************************************
void *
uDMAControlAlternateBaseGet(void)
{
    //
    // Read the current value of the control base register and return it to
    // the caller.
    //
    return((void *)HWREG(UDMA_BASE + UDMA_O_ALTBASE));
}

//*****************************************************************************
//
//! Requests a uDMA channel to start a transfer.
//!
//! \param ulChannelNum is the channel number on which to request a uDMA
//! transfer.
//!
//! This function allows software to request a uDMA channel to begin a
//! transfer.  This function could be used for performing a memory-to-memory
//! transfer or if for some reason, a transfer needs to be initiated by software
//! instead of the peripheral associated with that channel.
//!
//! \note If the channel is \b UDMA_CHANNEL_SW and interrupts are used, then
//! the completion is signaled on the uDMA dedicated interrupt.  If a
//! peripheral channel is used, then the completion is signaled on the
//! peripheral's interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelRequest(unsigned long ulChannelNum)
{
    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum & 0xffff) < 32);

    //
    // Set the bit for this channel in the software uDMA request register.
    //
    HWREG(UDMA_BASE + UDMA_O_SWREQ) = 1 << (ulChannelNum & 0x1f);
}

//*****************************************************************************
//
//! Enables attributes of a uDMA channel.
//!
//! \param ulChannelNum is the channel to configure.
//! \param ulAttr is a combination of attributes for the channel.
//!
//! This function is used to enable attributes of a uDMA channel.
//!
//! The \e ulAttr parameter is the logical OR of any of the following:
//!
//! - \b UDMA_ATTR_USEBURST is used to restrict transfers to use only burst
//!   mode.
//! - \b UDMA_ATTR_ALTSELECT is used to select the alternate control structure
//!   for this channel (it is very unlikely that this flag should be used).
//! - \b UDMA_ATTR_HIGH_PRIORITY is used to set this channel to high priority.
//! - \b UDMA_ATTR_REQMASK is used to mask the hardware request signal from the
//!   peripheral for this channel.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelAttributeEnable(unsigned long ulChannelNum, unsigned long ulAttr)
{
    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum & 0xffff) < 32);
    ASSERT((ulAttr & ~(UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                       UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK)) == 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelNum parameter, extract just the channel number
    // from this parameter.
    //
    ulChannelNum &= 0x1f;

    //
    // Set the useburst bit for this channel if set in ulConfig.
    //
    if(ulAttr & UDMA_ATTR_USEBURST)
    {
        HWREG(UDMA_BASE + UDMA_O_USEBURSTSET) = 1 << ulChannelNum;
    }

    //
    // Set the alternate control select bit for this channel,
    // if set in ulConfig.
    //
    if(ulAttr & UDMA_ATTR_ALTSELECT)
    {
        HWREG(UDMA_BASE + UDMA_O_ALTSET) = 1 << ulChannelNum;
    }

    //
    // Set the high priority bit for this channel, if set in ulConfig.
    //
    if(ulAttr & UDMA_ATTR_HIGH_PRIORITY)
    {
        HWREG(UDMA_BASE + UDMA_O_PRIOSET) = 1 << ulChannelNum;
    }

    //
    // Set the request mask bit for this channel, if set in ulConfig.
    //
    if(ulAttr & UDMA_ATTR_REQMASK)
    {
        HWREG(UDMA_BASE + UDMA_O_REQMASKSET) = 1 << ulChannelNum;
    }
}

//*****************************************************************************
//
//! Disables attributes of a uDMA channel.
//!
//! \param ulChannelNum is the channel to configure.
//! \param ulAttr is a combination of attributes for the channel.
//!
//! This function is used to disable attributes of a uDMA channel.
//!
//! The \e ulAttr parameter is the logical OR of any of the following:
//!
//! - \b UDMA_ATTR_USEBURST is used to restrict transfers to use only burst
//!   mode.
//! - \b UDMA_ATTR_ALTSELECT is used to select the alternate control structure
//!   for this channel.
//! - \b UDMA_ATTR_HIGH_PRIORITY is used to set this channel to high priority.
//! - \b UDMA_ATTR_REQMASK is used to mask the hardware request signal from the
//!   peripheral for this channel.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelAttributeDisable(unsigned long ulChannelNum, unsigned long ulAttr)
{
    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum & 0xffff) < 32);
    ASSERT((ulAttr & ~(UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                       UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK)) == 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelNum parameter, extract just the channel number
    // from this parameter.
    //
    ulChannelNum &= 0x1f;

    //
    // Clear the useburst bit for this channel if set in ulConfig.
    //
    if(ulAttr & UDMA_ATTR_USEBURST)
    {
        HWREG(UDMA_BASE + UDMA_O_USEBURSTCLR) = 1 << ulChannelNum;
    }

    //
    // Clear the alternate control select bit for this channel, if set in
    // ulConfig.
    //
    if(ulAttr & UDMA_ATTR_ALTSELECT)
    {
        HWREG(UDMA_BASE + UDMA_O_ALTCLR) = 1 << ulChannelNum;
    }

    //
    // Clear the high priority bit for this channel, if set in ulConfig.
    //
    if(ulAttr & UDMA_ATTR_HIGH_PRIORITY)
    {
        HWREG(UDMA_BASE + UDMA_O_PRIOCLR) = 1 << ulChannelNum;
    }

    //
    // Clear the request mask bit for this channel, if set in ulConfig.
    //
    if(ulAttr & UDMA_ATTR_REQMASK)
    {
        HWREG(UDMA_BASE + UDMA_O_REQMASKCLR) = 1 << ulChannelNum;
    }
}

//*****************************************************************************
//
//! Gets the enabled attributes of a uDMA channel.
//!
//! \param ulChannelNum is the channel to configure.
//!
//! This function returns a combination of flags representing the attributes of
//! the uDMA channel.
//!
//! \return Returns the logical OR of the attributes of the uDMA channel, which
//! can be any of the following:
//! - \b UDMA_ATTR_USEBURST is used to restrict transfers to use only burst
//!   mode.
//! - \b UDMA_ATTR_ALTSELECT is used to select the alternate control structure
//!   for this channel.
//! - \b UDMA_ATTR_HIGH_PRIORITY is used to set this channel to high priority.
//! - \b UDMA_ATTR_REQMASK is used to mask the hardware request signal from the
//!   peripheral for this channel.
//
//*****************************************************************************
unsigned long
uDMAChannelAttributeGet(unsigned long ulChannelNum)
{
    unsigned long ulAttr = 0;

    //
    // Check the arguments.
    //
    ASSERT((ulChannelNum & 0xffff) < 32);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelNum parameter, extract just the channel number
    // from this parameter.
    //
    ulChannelNum &= 0x1f;

    //
    // Check to see if useburst bit is set for this channel.
    //
    if(HWREG(UDMA_BASE + UDMA_O_USEBURSTSET) & (1 << ulChannelNum))
    {
        ulAttr |= UDMA_ATTR_USEBURST;
    }

    //
    // Check to see if the alternate control bit is set for this channel.
    //
    if(HWREG(UDMA_BASE + UDMA_O_ALTSET) & (1 << ulChannelNum))
    {
        ulAttr |= UDMA_ATTR_ALTSELECT;
    }

    //
    // Check to see if the high priority bit is set for this channel.
    //
    if(HWREG(UDMA_BASE + UDMA_O_PRIOSET) & (1 << ulChannelNum))
    {
        ulAttr |= UDMA_ATTR_HIGH_PRIORITY;
    }

    //
    // Check to see if the request mask bit is set for this channel.
    //
    if(HWREG(UDMA_BASE + UDMA_O_REQMASKSET) & (1 << ulChannelNum))
    {
        ulAttr |= UDMA_ATTR_REQMASK;
    }

    //
    // Return the configuration flags.
    //
    return(ulAttr);
}

//*****************************************************************************
//
//! Sets the control parameters for a uDMA channel control structure.
//!
//! \param ulChannelStructIndex is the logical OR of the uDMA channel number
//! with \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//! \param ulControl is logical OR of several control values to set the control
//! parameters for the channel.
//!
//! This function is used to set control parameters for a uDMA transfer.  These
//! parameters are typically not changed often.
//!
//! The \e ulChannelStructIndex parameter should be the logical OR of the
//! channel number with one of \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT to
//! choose whether the primary or alternate data structure is used.
//!
//! The \e ulControl parameter is the logical OR of five values: the data size,
//! the source address increment, the destination address increment, the
//! arbitration size, and the use burst flag.  The choices available for each
//! of these values is described below.
//!
//! Choose the data size from one of \b UDMA_SIZE_8, \b UDMA_SIZE_16, or
//! \b UDMA_SIZE_32 to select a data size of 8, 16, or 32 bits.
//!
//! Choose the source address increment from one of \b UDMA_SRC_INC_8,
//! \b UDMA_SRC_INC_16, \b UDMA_SRC_INC_32, or \b UDMA_SRC_INC_NONE to select
//! an address increment of 8-bit bytes, 16-bit halfwords, 32-bit words, or
//! to select non-incrementing.
//!
//! Choose the destination address increment from one of \b UDMA_DST_INC_8,
//! \b UDMA_DST_INC_16, \b UDMA_DST_INC_32, or \b UDMA_DST_INC_NONE to select
//! an address increment of 8-bit bytes, 16-bit halfwords, 32-bit words, or
//! to select non-incrementing.
//!
//! The arbitration size determines how many items are transferred before
//! the uDMA controller re-arbitrates for the bus.  Choose the arbitration size
//! from one of \b UDMA_ARB_1, \b UDMA_ARB_2, \b UDMA_ARB_4, \b UDMA_ARB_8,
//! through \b UDMA_ARB_1024 to select the arbitration size from 1 to 1024
//! items, in powers of 2.
//!
//! The value \b UDMA_NEXT_USEBURST is used to force the channel to only
//! respond to burst requests at the tail end of a scatter-gather transfer.
//!
//! \note The address increment cannot be smaller than the data size.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelControlSet(unsigned long ulChannelStructIndex,
                      unsigned long ulControl)
{
    tDMAControlTable *pCtl;

    //
    // Check the arguments.
    //
    ASSERT((ulChannelStructIndex & 0xffff) < 64);
    ASSERT(HWREG(UDMA_BASE + UDMA_O_CTLBASE) != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    ulChannelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    pCtl = (tDMAControlTable *)HWREG(UDMA_BASE+UDMA_O_CTLBASE);

    //
    // Get the current control word value and mask off the fields to be
    // changed, then OR in the new settings.
    //
    pCtl[ulChannelStructIndex].ulControl =
        ((pCtl[ulChannelStructIndex].ulControl &
          ~(UDMA_CHCTL_DSTINC_M |
            UDMA_CHCTL_DSTSIZE_M |
            UDMA_CHCTL_SRCINC_M |
            UDMA_CHCTL_SRCSIZE_M |
            UDMA_CHCTL_ARBSIZE_M |
            UDMA_CHCTL_NXTUSEBURST)) |
         ulControl);
}

//*****************************************************************************
//
//! Sets the transfer parameters for a uDMA channel control structure.
//!
//! \param ulChannelStructIndex is the logical OR of the uDMA channel number
//! with either \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//! \param ulMode is the type of uDMA transfer.
//! \param pvSrcAddr is the source address for the transfer.
//! \param pvDstAddr is the destination address for the transfer.
//! \param ulTransferSize is the number of data items to transfer.
//!
//! This function is used to configure the parameters for a uDMA transfer.
//! These parameters are typically changed often.  The function
//! uDMAChannelControlSet() MUST be called at least once for this channel prior
//! to calling this function.
//!
//! The \e ulChannelStructIndex parameter should be the logical OR of the
//! channel number with one of \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT to
//! choose whether the primary or alternate data structure is used.
//!
//! The \e ulMode parameter should be one of the following values:
//!
//! - \b UDMA_MODE_STOP stops the uDMA transfer.  The controller sets the mode
//!   to this value at the end of a transfer.
//! - \b UDMA_MODE_BASIC to perform a basic transfer based on request.
//! - \b UDMA_MODE_AUTO to perform a transfer that always completes once
//!   started even if the request is removed.
//! - \b UDMA_MODE_PINGPONG to set up a transfer that switches between the
//!   primary and alternate control structures for the channel.  This mode
//!   allows use of ping-pong buffering for uDMA transfers.
//! - \b UDMA_MODE_MEM_SCATTER_GATHER to set up a memory scatter-gather
//!   transfer.
//! - \b UDMA_MODE_PER_SCATTER_GATHER to set up a peripheral scatter-gather
//!   transfer.
//!
//! The \e pvSrcAddr and \e pvDstAddr parameters are pointers to the first
//! location of the data to be transferred.  These addresses should be aligned
//! according to the item size.  The compiler takes care of this alignment if
//! the pointers are pointing to storage of the appropriate data type.
//!
//! The \e ulTransferSize parameter is the number of data items, not the number
//! of bytes. The value of this parameter should not exceed 1024.
//!
//! The two scatter-gather modes, memory and peripheral, are actually different
//! depending on whether the primary or alternate control structure is
//! selected.  This function looks for the \b UDMA_PRI_SELECT and
//! \b UDMA_ALT_SELECT flag along with the channel number and sets the
//! scatter-gather mode as appropriate for the primary or alternate control
//! structure.
//!
//! The channel must also be enabled using uDMAChannelEnable() after calling
//! this function.  The transfer does not begin until the channel has been
//! configured and enabled.  Note that the channel is automatically disabled
//! after the transfer is completed, meaning that uDMAChannelEnable() must be
//! called again after setting up the next transfer.
//!
//! \note Great care must be taken to not modify a channel control structure
//! that is in use or else the results are unpredictable, including the
//! possibility of undesired data transfers to or from memory or peripherals.
//! For BASIC and AUTO modes, it is safe to make changes when the channel is
//! disabled, or the uDMAChannelModeGet() returns \b UDMA_MODE_STOP.  For
//! PINGPONG or one of the SCATTER_GATHER modes, it is safe to modify the
//! primary or alternate control structure only when the other is being used.
//! The uDMAChannelModeGet() function returns \b UDMA_MODE_STOP when a
//! channel control structure is inactive and safe to modify.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelTransferSet(unsigned long ulChannelStructIndex,
                       unsigned long ulMode, void *pvSrcAddr, void *pvDstAddr,
                       unsigned long ulTransferSize)
{
    tDMAControlTable *pControlTable;
    unsigned long ulControl;
    unsigned long ulInc;
    unsigned long ulBufferBytes;

    //
    // Check the arguments.
    //
    ASSERT((ulChannelStructIndex & 0xffff) < 64);
    ASSERT(HWREG(UDMA_BASE + UDMA_O_CTLBASE) != 0);
    ASSERT(ulMode <= UDMA_MODE_PER_SCATTER_GATHER);
    ASSERT((unsigned long)pvSrcAddr >= 0x20000000);
    ASSERT((unsigned long)pvDstAddr >= 0x20000000);
    ASSERT((ulTransferSize != 0) && (ulTransferSize <= 1024));

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    ulChannelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    pControlTable = (tDMAControlTable *)HWREG(UDMA_BASE + UDMA_O_CTLBASE);

    //
    // Get the current control word value and mask off the mode and size
    // fields.
    //
    ulControl = (pControlTable[ulChannelStructIndex].ulControl &
                 ~(UDMA_CHCTL_XFERSIZE_M | UDMA_CHCTL_XFERMODE_M));

    //
    // Adjust the mode if the alt control structure is selected.
    //
    if(ulChannelStructIndex & UDMA_ALT_SELECT)
    {
        if((ulMode == UDMA_MODE_MEM_SCATTER_GATHER) ||
           (ulMode == UDMA_MODE_PER_SCATTER_GATHER))
        {
            ulMode |= UDMA_MODE_ALT_SELECT;
        }
    }

    //
    // Set the transfer size and mode in the control word (but don't write the
    // control word yet as it could kick off a transfer).
    //
    ulControl |= ulMode | ((ulTransferSize - 1) << 4);

    //
    // Get the address increment value for the source, from the control word.
    //
    ulInc = (ulControl & UDMA_CHCTL_SRCINC_M);

    //
    // Compute the ending source address of the transfer.  If the source
    // increment is set to none, then the ending address is the same as the
    // beginning.
    //
    if(ulInc != UDMA_SRC_INC_NONE)
    {
        ulInc = ulInc >> 26;
        ulBufferBytes = ulTransferSize << ulInc;
        pvSrcAddr = (void *)((unsigned long)pvSrcAddr + ulBufferBytes - 1);
    }

    //
    // Load the source ending address into the control block.
    //
    pControlTable[ulChannelStructIndex].pvSrcEndAddr = pvSrcAddr;

    //
    // Get the address increment value for the destination, from the control
    // word.
    //
    ulInc = ulControl & UDMA_CHCTL_DSTINC_M;

    //
    // Compute the ending destination address of the transfer.  If the
    // destination increment is set to none, then the ending address is the
    // same as the beginning.
    //
    if(ulInc != UDMA_DST_INC_NONE)
    {
        //
        // There is a special case if this is setting up a scatter-gather
        // transfer.  The destination pointer must point to the end of
        // the alternate structure for this channel instead of calculating
        // the end of the buffer in the normal way.
        //
        if((ulMode == UDMA_MODE_MEM_SCATTER_GATHER) ||
           (ulMode == UDMA_MODE_PER_SCATTER_GATHER))
        {
            pvDstAddr =
                (void *)&pControlTable[ulChannelStructIndex |
                                       UDMA_ALT_SELECT].ulSpare;
        }
        //
        // Not a scatter-gather transfer, calculate end pointer normally.
        //
        else
        {
            ulInc = ulInc >> 30;
            ulBufferBytes = ulTransferSize << ulInc;
            pvDstAddr = (void *)((unsigned long)pvDstAddr + ulBufferBytes - 1);
        }
    }

    //
    // Load the destination ending address into the control block.
    //
    pControlTable[ulChannelStructIndex].pvDstEndAddr = pvDstAddr;

    //
    // Write the new control word value.
    //
    pControlTable[ulChannelStructIndex].ulControl = ulControl;
}

//*****************************************************************************
//
//! Configures a uDMA channel for scatter-gather mode.
//!
//! \param ulChannelNum is the uDMA channel number.
//! \param ulTaskCount is the number of scatter-gather tasks to execute.
//! \param pvTaskList is a pointer to the beginning of the scatter-gather
//! task list.
//! \param ulIsPeriphSG is a flag to indicate it is a peripheral scatter-gather
//! transfer (else it is memory scatter-gather transfer)
//!
//! This function is used to configure a channel for scatter-gather mode.
//! The caller must have already set up a task list and must pass a pointer to
//! the start of the task list as the \e pvTaskList parameter.  The
//! \e ulTaskCount parameter is the count of tasks in the task list, not the
//! size of the task list.  The flag \e bIsPeriphSG should be used to indicate
//! if scatter-gather should be configured for peripheral or memory
//! operation.
//!
//! \sa uDMATaskStructEntry
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelScatterGatherSet(unsigned long ulChannelNum, unsigned ulTaskCount,
                            void *pvTaskList, unsigned long ulIsPeriphSG)
{
    tDMAControlTable *pControlTable;
    tDMAControlTable *pTaskTable;

    //
    // Check the parameters
    //
    ASSERT((ulChannelNum & 0xffff) < 32);
    ASSERT(HWREG(UDMA_BASE + UDMA_O_CTLBASE) != 0);
    ASSERT(pvTaskList != 0);
    ASSERT(ulTaskCount <= 1024);
    ASSERT(ulTaskCount != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelNum parameter, extract just the channel number
    // from this parameter.
    //
    ulChannelNum &= 0x1f;

    //
    // Get the base address of the control table.
    //
    pControlTable = (tDMAControlTable *)HWREG(UDMA_BASE + UDMA_O_CTLBASE);

    //
    // Get a handy pointer to the task list
    //
    pTaskTable = (tDMAControlTable *)pvTaskList;

    //
    // Compute the ending address for the source pointer.  This address is the
    // last element of the last task in the task table
    //
    pControlTable[ulChannelNum].pvSrcEndAddr =
        &pTaskTable[ulTaskCount - 1].ulSpare;

    //
    // Compute the ending address for the destination pointer.  This address
    // is the end of the alternate structure for this channel.
    //
    pControlTable[ulChannelNum].pvDstEndAddr =
        &pControlTable[ulChannelNum | UDMA_ALT_SELECT].ulSpare;

    //
    // Compute the control word.  Most configurable items are fixed for
    // scatter-gather.  Item and increment sizes are all 32-bit and arb
    // size must be 4.  The count is the number of items in the task list
    // times 4 (4 words per task).
    //
    pControlTable[ulChannelNum].ulControl =
        (UDMA_CHCTL_DSTINC_32 | UDMA_CHCTL_DSTSIZE_32 |
         UDMA_CHCTL_SRCINC_32 | UDMA_CHCTL_SRCSIZE_32 |
         UDMA_CHCTL_ARBSIZE_4 |
         (((ulTaskCount * 4) - 1) << UDMA_CHCTL_XFERSIZE_S) |
         (ulIsPeriphSG ? UDMA_CHCTL_XFERMODE_PER_SG :
          UDMA_CHCTL_XFERMODE_MEM_SG));
}

//*****************************************************************************
//
//! Gets the current transfer size for a uDMA channel control structure.
//!
//! \param ulChannelStructIndex is the logical OR of the uDMA channel number
//! with either \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//!
//! This function is used to get the uDMA transfer size for a channel.  The
//! transfer size is the number of items to transfer, where the size of an item
//! might be 8, 16, or 32 bits.  If a partial transfer has already occurred,
//! then the number of remaining items is returned.  If the transfer is
//! complete, then 0 is returned.
//!
//! \return Returns the number of items remaining to transfer.
//
//*****************************************************************************
unsigned long
uDMAChannelSizeGet(unsigned long ulChannelStructIndex)
{
    tDMAControlTable *pControlTable;
    unsigned long ulControl;

    //
    // Check the arguments.
    //
    ASSERT((ulChannelStructIndex & 0xffff) < 64);
    ASSERT(HWREG(UDMA_BASE + UDMA_O_CTLBASE) != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    ulChannelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    pControlTable = (tDMAControlTable *)HWREG(UDMA_BASE + UDMA_O_CTLBASE);

    //
    // Get the current control word value and mask off all but the size field
    // and the mode field.
    //
    ulControl = (pControlTable[ulChannelStructIndex].ulControl &
                 (UDMA_CHCTL_XFERSIZE_M | UDMA_CHCTL_XFERMODE_M));

    //
    // If the size field and mode field are 0 then the transfer is finished
    // and there are no more items to transfer
    //
    if(ulControl == 0)
    {
        return(0);
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
        return((ulControl >> 4) + 1);
    }
}

//*****************************************************************************
//
//! Gets the transfer mode for a uDMA channel control structure.
//!
//! \param ulChannelStructIndex is the logical OR of the uDMA channel number
//! with either \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//!
//! This function is used to get the transfer mode for the uDMA channel and
//! to query the status of a transfer on a channel.  When the transfer is
//! complete the mode is \b UDMA_MODE_STOP.
//!
//! \return Returns the transfer mode of the specified channel and control
//! structure, which is one of the following values: \b UDMA_MODE_STOP,
//! \b UDMA_MODE_BASIC, \b UDMA_MODE_AUTO, \b UDMA_MODE_PINGPONG,
//! \b UDMA_MODE_MEM_SCATTER_GATHER, or \b UDMA_MODE_PER_SCATTER_GATHER.
//
//*****************************************************************************
unsigned long
uDMAChannelModeGet(unsigned long ulChannelStructIndex)
{
    tDMAControlTable *pControlTable;
    unsigned long ulControl;

    //
    // Check the arguments.
    //
    ASSERT((ulChannelStructIndex & 0xffff) < 64);
    ASSERT(HWREG(UDMA_O_CTLBASE) != 0);

    //
    // In case a channel selector macro (like UDMA_CH0_USB0EP1RX) was
    // passed as the ulChannelStructIndex parameter, extract just the channel
    // index from this parameter.
    //
    ulChannelStructIndex &= 0x3f;

    //
    // Get the base address of the control table.
    //
    pControlTable = (tDMAControlTable *)HWREG(UDMA_BASE + UDMA_O_CTLBASE);

    //
    // Get the current control word value and mask off all but the mode field.
    //
    ulControl = (pControlTable[ulChannelStructIndex].ulControl &
                 UDMA_CHCTL_XFERMODE_M);

    //
    // Check if scatter/gather mode, and if so, mask off the alt bit.
    //
    if(((ulControl & ~UDMA_MODE_ALT_SELECT) == UDMA_MODE_MEM_SCATTER_GATHER) ||
       ((ulControl & ~UDMA_MODE_ALT_SELECT) == UDMA_MODE_PER_SCATTER_GATHER))
    {
        ulControl &= ~UDMA_MODE_ALT_SELECT;
    }

    //
    // Return the mode to the caller.
    //
    return(ulControl);
}

//*****************************************************************************
//
//! Registers an interrupt handler for the uDMA controller.
//!
//! \param ulIntChannel identifies which uDMA interrupt is to be registered.
//! \param pfnHandler is a pointer to the function to be called when the
//! interrupt is activated.
//!
//! This function registers and enables the handler to be called when the uDMA
//! controller generates an interrupt.  The \e ulIntChannel parameter should be
//! one of the following:
//!
//! - \b UDMA_INT_SW to register an interrupt handler to process interrupts
//!   from the uDMA software channel (UDMA_CHANNEL_SW)
//! - \b UDMA_INT_ERR to register an interrupt handler to process uDMA error
//!   interrupts
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \note The interrupt handler for the uDMA is for transfer completion when
//! the channel UDMA_CHANNEL_SW is used and for error interrupts.  The
//! interrupts for each peripheral channel are handled through the individual
//! peripheral interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAIntRegister(unsigned long ulIntChannel, void (*pfnHandler)(void))
{
    //
    // Check the arguments.
    //
    ASSERT(pfnHandler);
    ASSERT((ulIntChannel == UDMA_INT_SW) || (ulIntChannel == UDMA_INT_ERR));

    //
    // Register the interrupt handler.
    //
    IntRegister(ulIntChannel, pfnHandler);

    //
    // Enable the memory management fault.
    //
    IntEnable(ulIntChannel);
}

//*****************************************************************************
//
//! Unregisters an interrupt handler for the uDMA controller.
//!
//! \param ulIntChannel identifies which uDMA interrupt to unregister.
//!
//! This function disables and unregisters the handler to be called for the
//! specified uDMA interrupt.  The \e ulIntChannel parameter should be one of
//! \b UDMA_INT_SW or \b UDMA_INT_ERR as documented for the function
//! uDMAIntRegister().
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAIntUnregister(unsigned long ulIntChannel)
{
    //
    // Disable the interrupt.
    //
    IntDisable(ulIntChannel);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(ulIntChannel);
}

//*****************************************************************************
//
//! Gets the uDMA controller channel interrupt status.
//!
//! This function is used to get the interrupt status of the uDMA controller.
//! The returned value is a 32-bit bit mask that indicates which channels are
//! requesting an interrupt.  This function can be used from within an
//! interrupt handler to determine or confirm which uDMA channel has requested
//! an interrupt.
//!
//! \note This function is only available on devices that have the DMA Channel
//! Interrupt Status Register (DMACHIS).  Please consult the data sheet for
//! your part.
//!
//! \return Returns a 32-bit mask which indicates requesting uDMA channels.
//! There is a bit for each channel and a 1 indicates that the channel
//! is requesting an interrupt.  Multiple bits can be set.
//
//*****************************************************************************
unsigned long
uDMAIntStatus(void)
{


    //
    // Return the value of the uDMA interrupt status register
    //
    return(HWREG(UDMA_BASE + UDMA_O_CHIS));
}

//*****************************************************************************
//
//! Clears uDMA interrupt status.
//!
//! \param ulChanMask is a 32-bit mask with one bit for each uDMA channel.
//!
//! This function clears bits in the uDMA interrupt status register according
//! to which bits are set in \e ulChanMask. There is one bit for each channel.
//! If a a bit is set in \e ulChanMask, then that corresponding channel's
//! interrupt status is cleared (if it was set).
//!
//! \note This function is only available on devices that have the DMA Channel
//! Interrupt Status Register (DMACHIS).  Please consult the data sheet for
//! your part.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAIntClear(unsigned long ulChanMask)
{

    //
    // Clear the requested bits in the uDMA interrupt status register
    //
    HWREG(UDMA_BASE + UDMA_O_CHIS) = ulChanMask;
}

//*****************************************************************************
//
//! Assigns a peripheral mapping for a uDMA channel.
//!
//! \param ulMapping is a macro specifying the peripheral assignment for
//! a channel.
//!
//! This function assigns a peripheral mapping to a uDMA channel.  It is
//! used to select which peripheral is used for a uDMA channel.  The parameter
//! \e ulMapping should be one of the macros named \b UDMA_CHn_tttt from the
//! header file \e udma.h.  For example, to assign uDMA channel 0 to the
//! UART2 RX channel, the parameter should be the macro \b UDMA_CH0_UART2RX.
//!
//! Please consult the data sheet for a table showing all the
//! possible peripheral assignments for the uDMA channels for a particular
//! device.
//!
//! \note This function is only available on devices that have the DMA Channel
//! Map Select registers (DMACHMAP0-3).  Please consult the data sheet for
//! your part.
//!
//! \return None.
//
//*****************************************************************************
void
uDMAChannelAssign(unsigned long ulMapping)
{
    unsigned long ulMapReg;
    unsigned long ulMapShift;
    unsigned long ulChannelNum;

    //
    // Check the parameters
    //
    ASSERT((ulMapping & 0xffffff00) < 0x00050000);


    //
    // Extract the channel number and map encoding value from the parameter.
    //
    ulChannelNum = ulMapping & 0x1f;
    ulMapping = ulMapping >> 16;

    //
    // Find the uDMA channel mapping register and shift value to use for this
    // channel
    //
    ulMapReg = UDMA_BASE + UDMA_O_CHMAP0 + ((ulChannelNum / 8) * 4);
    ulMapShift = (ulChannelNum % 8) * 4;

    //
    // Set the channel map encoding for this channel
    //
    HWREG(ulMapReg) = (HWREG(ulMapReg) & ~(0xf << ulMapShift)) |
                      ulMapping << ulMapShift;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
