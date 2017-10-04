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
#ifndef __DMA_H__
#define __DMA_H__

//*****************************************************************************
//
//! \addtogroup dma_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>

//*****************************************************************************
//
// A structure that defines an entry in the channel control table.  These
// fields are used by the DMA controller and normally it is not necessary for
// software to directly read or write fields in the table.
//
//*****************************************************************************
typedef struct _DMA_ControlTable
{
    //
    // The ending source address of the data transfer.
    //
    volatile void *srcEndAddr;

    //
    // The ending destination address of the data transfer.
    //
    volatile void *dstEndAddr;

    //
    // The channel control mode.
    //
    volatile uint32_t control;

    //
    // An unused location.
    //
    volatile uint32_t spare;
} DMA_ControlTable;

//*****************************************************************************
//
//! A helper macro for building scatter-gather task table entries.
//!
//! This macro is intended to be used to help populate a table of DMA tasks
//! for a scatter-gather transfer.  This macro will calculate the values for
//! the fields of a task structure entry based on the input parameters.
//!
//! There are specific requirements for the values of each parameter.  No
//! checking is done so it is up to the caller to ensure that correct values
//! are used for the parameters.
//!
//! The \b transferCount parameter is the number of items that will be
//! transferred by this task.  It must be in the range 1-1024.
//!
//! The \b itemSize parameter is the bit size of the transfer data.  It must
//! be one of \b UDMA_SIZE_8, \b UDMA_SIZE_16, or \b UDMA_SIZE_32.
//!
//! The \e srcIncrement parameter is the increment size for the source data.
//! It must be one of \b UDMA_SRC_INC_8, \b UDMA_SRC_INC_16,
//! \b UDMA_SRC_INC_32, or \b UDMA_SRC_INC_NONE.
//!
//! The \b srcAddr parameter is a void pointer to the beginning of the source
//! data.
//!
//! The \b dstIncrement parameter is the increment size for the destination
//! data.  It must be one of \b UDMA_DST_INC_8, \b UDMA_DST_INC_16,
//! \b UDMA_DST_INC_32, or \b UDMA_DST_INC_NONE.
//!
//! The \b dstAddr parameter is a void pointer to the beginning of the
//! location where the data will be transferred.
//!
//! The \b arbSize parameter is the arbitration size for the transfer, and
//! must be one of \b UDMA_ARB_1, \b UDMA_ARB_2, \b UDMA_ARB_4, and so on
//! up to \b UDMA_ARB_1024.  This is used to select the arbitration size in
//! powers of 2, from 1 to 1024.
//!
//! The \e mode parameter is the mode to use for this transfer task.  It
//! must be one of \b UDMA_MODE_BASIC, \b UDMA_MODE_AUTO,
//! \b UDMA_MODE_MEM_SCATTER_GATHER, or \b UDMA_MODE_PER_SCATTER_GATHER.  Note
//! that normally all tasks will be one of the scatter-gather modes while the
//! last task is a task list will be AUTO or BASIC.
//!
//! This macro is intended to be used to initialize individual entries of
//! a structure of DMA_ControlTable type, like this:
//!
//! \code{.c}
//!     DMA_ControlTable MyTaskList[] =
//!     {
//!         DMA_TaskStructEntry(Task1Count, UDMA_SIZE_8,
//!                             UDMA_SRC_INC_8, MySourceBuf,
//!                             UDMA_DST_INC_8, MyDestBuf,
//!                             UDMA_ARB_8, UDMA_MODE_MEM_SCATTER_GATHER),
//!         DMA_TaskStructEntry(Task2Count, ... ),
//!     }
//! \endcode
//!
//! \param transferCount is the count of items to transfer for this task.
//! \param itemSize is the bit size of the items to transfer for this task.
//! \param srcIncrement is the bit size increment for source data.
//! \param srcAddr is the starting address of the data to transfer.
//! \param dstIncrement is the bit size increment for destination data.
//! \param dstAddr is the starting address of the destination data.
//! \param arbSize is the arbitration size to use for the transfer task.
//! \param mode is the transfer mode for this task.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define DMA_TaskStructEntry(transferCount,                                     \
                            itemSize,                                          \
                            srcIncrement,                                      \
                            srcAddr,                                           \
                            dstIncrement,                                      \
                            dstAddr,                                           \
                            arbSize,                                           \
                            mode)                                              \
    {                                                                          \
        (((srcIncrement) == UDMA_SRC_INC_NONE) ? (void *)(srcAddr) :           \
            ((void *)(&((uint8_t *)(srcAddr))[((transferCount) <<              \
                                         ((srcIncrement) >> 26)) - 1]))),      \
            (((dstIncrement) == UDMA_DST_INC_NONE) ? (void *)(dstAddr) :       \
            ((void *)(&((uint8_t *)(dstAddr))[((transferCount) <<              \
                                         ((dstIncrement) >> 30)) - 1]))),      \
        (srcIncrement) | (dstIncrement) | (itemSize) | (arbSize) |             \
        (((transferCount) - 1) << 4) |                                         \
        ((((mode) == UDMA_MODE_MEM_SCATTER_GATHER) ||                          \
          ((mode) == UDMA_MODE_PER_SCATTER_GATHER)) ?                          \
                (mode) | UDMA_MODE_ALT_SELECT : (mode)), 0                     \
    }

//*****************************************************************************
//
// Flags that can be passed to DMA_enableChannelAttribute(),
// DMA_disableChannelAttribute(), and returned from DMA_getChannelAttribute().
//
//*****************************************************************************
#define UDMA_ATTR_USEBURST      0x00000001
#define UDMA_ATTR_ALTSELECT     0x00000002
#define UDMA_ATTR_HIGH_PRIORITY 0x00000004
#define UDMA_ATTR_REQMASK       0x00000008
#define UDMA_ATTR_ALL           0x0000000F

//*****************************************************************************
//
// DMA control modes that can be passed to DMAModeSet() and returned
// DMAModeGet().
//
//*****************************************************************************
#define UDMA_MODE_STOP          0x00000000
#define UDMA_MODE_BASIC         0x00000001
#define UDMA_MODE_AUTO          0x00000002
#define UDMA_MODE_PINGPONG      0x00000003
#define UDMA_MODE_MEM_SCATTER_GATHER                                          \
                                0x00000004
#define UDMA_MODE_PER_SCATTER_GATHER                                          \
                                0x00000006
#define UDMA_MODE_ALT_SELECT    0x00000001

//*****************************************************************************
//
// Channel configuration values that can be passed to DMAControlSet().
//
//*****************************************************************************
#define UDMA_DST_INC_8          0x00000000
#define UDMA_DST_INC_16         0x40000000
#define UDMA_DST_INC_32         0x80000000
#define UDMA_DST_INC_NONE       0xc0000000
#define UDMA_SRC_INC_8          0x00000000
#define UDMA_SRC_INC_16         0x04000000
#define UDMA_SRC_INC_32         0x08000000
#define UDMA_SRC_INC_NONE       0x0c000000
#define UDMA_SIZE_8             0x00000000
#define UDMA_SIZE_16            0x11000000
#define UDMA_SIZE_32            0x22000000
#define UDMA_DST_PROT_PRIV      0x00200000
#define UDMA_SRC_PROT_PRIV      0x00040000
#define UDMA_ARB_1              0x00000000
#define UDMA_ARB_2              0x00004000
#define UDMA_ARB_4              0x00008000
#define UDMA_ARB_8              0x0000c000
#define UDMA_ARB_16             0x00010000
#define UDMA_ARB_32             0x00014000
#define UDMA_ARB_64             0x00018000
#define UDMA_ARB_128            0x0001c000
#define UDMA_ARB_256            0x00020000
#define UDMA_ARB_512            0x00024000
#define UDMA_ARB_1024           0x00028000
#define UDMA_NEXT_USEBURST      0x00000008

//*****************************************************************************
//
// Flags to be OR'd with the channel ID to indicate if the primary or alternate
// control structure should be used.
//
//*****************************************************************************
#define UDMA_PRI_SELECT         0x00000000
#define UDMA_ALT_SELECT         0x00000008

//*****************************************************************************
//
// Values that can be passed to DMA_assignChannel() to select peripheral
// mapping for each channel.  The channels named RESERVED may be assigned
// to a peripheral in future parts.
//
//*****************************************************************************
//
// Channel 0
//
#define DMA_CH0_RESERVED0          0x00000000
#define DMA_CH0_EUSCIA0TX          0x01000000
#define DMA_CH0_EUSCIB0TX0         0x02000000
#define DMA_CH0_EUSCIB3TX1         0x03000000
#define DMA_CH0_EUSCIB2TX2         0x04000000
#define DMA_CH0_EUSCIB1TX3         0x05000000
#define DMA_CH0_TIMERA0CCR0        0x06000000
#define DMA_CH0_AESTRIGGER0        0x07000000

//
// Channel 1
//
#define DMA_CH1_RESERVED0          0x00000001
#define DMA_CH1_EUSCIA0RX          0x01000001
#define DMA_CH1_EUSCIB0RX0         0x02000001
#define DMA_CH1_EUSCIB3RX1         0x03000001
#define DMA_CH1_EUSCIB2RX2         0x04000001
#define DMA_CH1_EUSCIB1RX3         0x05000001
#define DMA_CH1_TIMERA0CCR2        0x06000001
#define DMA_CH1_AESTRIGGER1        0x07000001

//
// Channel 2
//
#define DMA_CH2_RESERVED0          0x00000002
#define DMA_CH2_EUSCIA1TX          0x01000002
#define DMA_CH2_EUSCIB1TX0         0x02000002
#define DMA_CH2_EUSCIB0TX1         0x03000002
#define DMA_CH2_EUSCIB3TX2         0x04000002
#define DMA_CH2_EUSCIB2TX3         0x05000002
#define DMA_CH2_TIMERA1CCR0        0x06000002
#define DMA_CH2_AESTRIGGER2        0x07000002

//
// Channel 3
//
#define DMA_CH3_RESERVED0          0x00000003
#define DMA_CH3_EUSCIA1RX          0x01000003
#define DMA_CH3_EUSCIB1RX0         0x02000003
#define DMA_CH3_EUSCIB0RX1         0x03000003
#define DMA_CH3_EUSCIB3RX2         0x04000003
#define DMA_CH3_EUSCIB2RX3         0x05000003
#define DMA_CH3_TIMERA1CCR2        0x06000003
#define DMA_CH3_RESERVED1          0x07000003

//
// Channel 4
//
#define DMA_CH4_RESERVED0          0x00000004
#define DMA_CH4_EUSCIA2TX          0x01000004
#define DMA_CH4_EUSCIB2TX0         0x02000004
#define DMA_CH4_EUSCIB1TX1         0x03000004
#define DMA_CH4_EUSCIB0TX2         0x04000004
#define DMA_CH4_EUSCIB3TX3         0x05000004
#define DMA_CH4_TIMERA2CCR0        0x06000004
#define DMA_CH4_RESERVED1          0x07000004

//
// Channel 5
//
#define DMA_CH5_RESERVED0          0x00000005
#define DMA_CH5_EUSCIA2RX          0x01000005
#define DMA_CH5_EUSCIB2RX0         0x02000005
#define DMA_CH5_EUSCIB1RX1         0x03000005
#define DMA_CH5_EUSCIB0RX2         0x04000005
#define DMA_CH5_EUSCIB3RX3         0x05000005
#define DMA_CH5_TIMERA2CCR2        0x06000005
#define DMA_CH5_RESERVED1          0x07000005

//
// Channel 6
//
#define DMA_CH6_RESERVED0          0x00000006
#define DMA_CH6_EUSCIA3TX          0x01000006
#define DMA_CH6_EUSCIB3TX0         0x02000006
#define DMA_CH6_EUSCIB2TX1         0x03000006
#define DMA_CH6_EUSCIB1TX2         0x04000006
#define DMA_CH6_EUSCIB0TX3         0x05000006
#define DMA_CH6_TIMERA3CCR0        0x06000006
#define DMA_CH6_EXTERNALPIN        0x07000006

//
// Channel 7
//
#define DMA_CH7_RESERVED0          0x00000007
#define DMA_CH7_EUSCIA3RX          0x01000007
#define DMA_CH7_EUSCIB3RX0         0x02000007
#define DMA_CH7_EUSCIB2RX1         0x03000007
#define DMA_CH7_EUSCIB1RX2         0x04000007
#define DMA_CH7_EUSCIB0RX3         0x05000007
#define DMA_CH7_TIMERA3CCR2        0x06000007
#define DMA_CH7_ADC14              0x07000007

//
//  Different interrupt handlers to pass into DMA_registerInterrupt and
//   DMA_unregisterInterrupt and other Int functions
//
#define DMA_INT0   INT_DMA_INT0
#define DMA_INT1   INT_DMA_INT1
#define DMA_INT2   INT_DMA_INT2
#define DMA_INT3   INT_DMA_INT3
#define DMA_INTERR INT_DMA_ERR

#define DMA_CHANNEL_0       0
#define DMA_CHANNEL_1       1
#define DMA_CHANNEL_2       2
#define DMA_CHANNEL_3       3
#define DMA_CHANNEL_4       4
#define DMA_CHANNEL_5       5
#define DMA_CHANNEL_6       6
#define DMA_CHANNEL_7       7

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! Enables the DMA controller for use.
//!
//! This function enables the DMA controller.  The DMA controller must be
//! enabled before it can be configured and used.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_enableModule(void);

//*****************************************************************************
//
//! Disables the DMA controller for use.
//!
//! This function disables the DMA controller.  Once disabled, the DMA
//! controller cannot operate until re-enabled with DMA_enableModule().
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_disableModule(void);

//*****************************************************************************
//
//! Gets the DMA error status.
//!
//! This function returns the DMA error status.  It should be called from
//! within the DMA error interrupt handler to determine if a DMA error
//! occurred.
//!
//! \return Returns non-zero if a DMA error is pending.
//
//*****************************************************************************
extern uint32_t DMA_getErrorStatus(void);

//*****************************************************************************
//
//! Clears the DMA error interrupt.
//!
//! This function clears a pending DMA error interrupt.  This function should
//! be called from within the DMA error interrupt handler to clear the
//! interrupt.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_clearErrorStatus(void);

//*****************************************************************************
//
//! Enables a DMA channel for operation.
//!
//! \param channelNum is the channel number to enable.
//!
//! When a DMA transfer is completed, the channel is automatically disabled by
//! the DMA controller.  Therefore, this function should be called prior to
//! starting up any new transfer.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_enableChannel(uint32_t channelNum);

//*****************************************************************************
//
//! Disables a DMA channel for operation.
//!
//! \param channelNum is the channel number to disable.
//!
//! This function disables a specific DMA channel.  Once disabled, a channel
//! cannot respond to DMA transfer requests until re-enabled via
//! DMA_enableChannel().
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_disableChannel(uint32_t channelNum);

//*****************************************************************************
//
//! Checks if a DMA channel is enabled for operation.
//!
//! \param channelNum is the channel number to check.
//!
//! This function checks to see if a specific DMA channel is enabled.  This
//! function can be used to check the status of a transfer, as the channel is
//! automatically disabled at the end of a transfer.
//!
//! \return Returns \b true if the channel is enabled, \b false if disabled.
//
//*****************************************************************************
extern bool DMA_isChannelEnabled(uint32_t channelNum);

//*****************************************************************************
//
//! Sets the base address for the channel control table.
//!
//! \param controlTable is a pointer to the 1024-byte-aligned base address
//! of the DMA channel control table.
//!
//! This function configures the base address of the channel control table.
//! This table resides in system memory and holds control information for each
//! DMA channel.  The table must be aligned on a 1024-byte boundary.  The base
//! address must be configured before any of the channel functions can be used.
//!
//! The size of the channel control table depends on the number of DMA
//! channels and the transfer modes that are used.  Refer to the introductory
//! text and the microcontroller datasheet for more information about the
//! channel control table.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_setControlBase(void *controlTable);

//*****************************************************************************
//
//! Gets the base address for the channel control table.
//!
//! This function gets the base address of the channel control table.  This
//! table resides in system memory and holds control information for each DMA
//! channel.
//!
//! \return Returns a pointer to the base address of the channel control table.
//
//*****************************************************************************
extern void* DMA_getControlBase(void);

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
extern void* DMA_getControlAlternateBase(void);

//*****************************************************************************
//
//! Requests a DMA channel to start a transfer.
//!
//! \param channelNum is the channel number on which to request a DMA
//! transfer.
//!
//! This function allows software to request a DMA channel to begin a
//! transfer.  This function could be used for performing a memory-to-memory
//! transfer, or if for some reason a transfer needs to be initiated by
//! software instead of the peripheral associated with that channel.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_requestChannel(uint32_t channelNum);

//*****************************************************************************
//
//! Enables attributes of a DMA channel.
//!
//! \param channelNum is the channel to configure.
//! \param attr is a combination of attributes for the channel.
//!
//! This function is used to enable attributes of a DMA channel.
//!
//! The \e attr parameter is the logical OR of any of the following:
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
extern void DMA_enableChannelAttribute(uint32_t channelNum, uint32_t attr);

//*****************************************************************************
//
//! Disables attributes of a DMA channel.
//!
//! \param channelNum is the channel to configure.
//! \param attr is a combination of attributes for the channel.
//!
//! This function is used to disable attributes of a DMA channel.
//!
//! The \e attr parameter is the logical OR of any of the following:
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
extern void DMA_disableChannelAttribute(uint32_t channelNum, uint32_t attr);

//*****************************************************************************
//
//! Gets the enabled attributes of a DMA channel.
//!
//! \param channelNum is the channel to configure.
//!
//! This function returns a combination of flags representing the attributes of
//! the DMA channel.
//!
//! \return Returns the logical OR of the attributes of the DMA channel, which
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
extern uint32_t DMA_getChannelAttribute(uint32_t channelNum);

//*****************************************************************************
//
//! Sets the control parameters for a DMA channel control structure.
//!
//! \param channelStructIndex is the logical OR of the DMA channel number
//! with \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//! \param control is logical OR of several control values to set the control
//! parameters for the channel.
//!
//! This function is used to set control parameters for a DMA transfer.  These
//! parameters are typically not changed often.
//!
//! The \e channelStructIndex parameter should be the logical OR of the
//! channel number with one of \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT to
//! choose whether the primary or alternate data structure is used.
//!
//! The \e control parameter is the logical OR of five values: the data size,
//! the source address increment, the destination address increment, the
//! arbitration size, and the use burst flag.  The choices available for each
//! of these values is described below.
//!
//! Choose the data size from one of \b UDMA_SIZE_8, \b UDMA_SIZE_16, or
//! \b UDMA_SIZE_32 to select a data size of 8, 16, or 32 bits.
//!
//! Choose the source address increment from one of \b UDMA_SRC_INC_8,
//! \b UDMA_SRC_INC_16, \b UDMA_SRC_INC_32, or \b UDMA_SRC_INC_NONE to select
//! an address increment of 8-bit bytes, 16-bit half-words, 32-bit words, or
//! to select non-incrementing.
//!
//! Choose the destination address increment from one of \b UDMA_DST_INC_8,
//! \b UDMA_DST_INC_16, \b UDMA_DST_INC_32, or \b UDMA_SRC_INC_8 to select
//! an address increment of 8-bit bytes, 16-bit half-words, 32-bit words, or
//! to select non-incrementing.
//!
//! The arbitration size determines how many items are transferred before
//! the DMA controller re-arbitrates for the bus.  Choose the arbitration size
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
extern void DMA_setChannelControl(uint32_t channelStructIndex,
        uint32_t control);

//*****************************************************************************
//
//! Sets the transfer parameters for a DMA channel control structure.
//!
//! \param channelStructIndex is the logical OR of the DMA channel number
//! with either \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//! \param mode is the type of DMA transfer.
//! \param srcAddr is the source address for the transfer.
//! \param dstAddr is the destination address for the transfer.
//! \param transferSize is the number of data items to transfer.
//!
//! This function is used to configure the parameters for a DMA transfer.
//! These parameters are typically changed often.  The function
//! DMA_setChannelControl() MUST be called at least once for this channel prior
//! to calling this function.
//!
//! The \e channelStructIndex parameter should be the logical OR of the
//! channel number with one of \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT to
//! choose whether the primary or alternate data structure is used.
//!
//! The \e mode parameter should be one of the following values:
//!
//! - \b UDMA_MODE_STOP stops the DMA transfer.  The controller sets the mode
//!   to this value at the end of a transfer.
//! - \b UDMA_MODE_BASIC to perform a basic transfer based on request.
//! - \b UDMA_MODE_AUTO to perform a transfer that always completes once
//!   started even if the request is removed.
//! - \b UDMA_MODE_PINGPONG to set up a transfer that switches between the
//!   primary and alternate control structures for the channel.  This mode
//!   allows use of ping-pong buffering for DMA transfers.
//! - \b UDMA_MODE_MEM_SCATTER_GATHER to set up a memory scatter-gather
//!   transfer.
//! - \b UDMA_MODE_PER_SCATTER_GATHER to set up a peripheral scatter-gather
//!   transfer.
//!
//! The \e srcAddr and \e dstAddr parameters are pointers to the first
//! location of the data to be transferred.  These addresses should be aligned
//! according to the item size.  The compiler takes care of this alignment if
//! the pointers are pointing to storage of the appropriate data type.
//!
//! The \e transferSize parameter is the number of data items, not the number
//! of bytes.
//!
//! The two scatter-gather modes, memory and peripheral, are actually different
//! depending on whether the primary or alternate control structure is
//! selected.  This function looks for the \b UDMA_PRI_SELECT and
//! \b UDMA_ALT_SELECT flag along with the channel number and sets the
//! scatter-gather mode as appropriate for the primary or alternate control
//! structure.
//!
//! The channel must also be enabled using DMA_enableChannel() after calling
//! this function.  The transfer does not begin until the channel has been
//! configured and enabled.  Note that the channel is automatically disabled
//! after the transfer is completed, meaning that DMA_enableChannel() must be
//! called again after setting up the next transfer.
//!
//! \note Great care must be taken to not modify a channel control structure
//! that is in use or else the results are unpredictable, including the
//! possibility of undesired data transfers to or from memory or peripherals.
//! For BASIC and AUTO modes, it is safe to make changes when the channel is
//! disabled, or the DMA_getChannelMode() returns \b UDMA_MODE_STOP.  For
//! PINGPONG or one of the SCATTER_GATHER modes, it is safe to modify the
//! primary or alternate control structure only when the other is being used.
//! The DMA_getChannelMode() function returns \b UDMA_MODE_STOP when a
//! channel control structure is inactive and safe to modify.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_setChannelTransfer(uint32_t channelStructIndex, uint32_t mode,
        void *srcAddr, void *dstAddr, uint32_t transferSize);

//*****************************************************************************
//
//! Configures a DMA channel for scatter-gather mode.
//!
//! \param channelNum is the DMA channel number.
//! \param taskCount is the number of scatter-gather tasks to execute.
//! \param taskList is a pointer to the beginning of the scatter-gather
//! task list.
//! \param isPeriphSG is a flag to indicate it is a peripheral scatter-gather
//! transfer (else it is memory scatter-gather transfer)
//!
//! This function is used to configure a channel for scatter-gather mode.
//! The caller must have already set up a task list and must pass a pointer to
//! the start of the task list as the \e taskList parameter.  The
//! \e taskCount parameter is the count of tasks in the task list, not the
//! size of the task list.  The flag \e bIsPeriphSG should be used to indicate
//! if scatter-gather should be configured for peripheral or memory
//! operation.
//!
//! \sa DMA_TaskStructEntry
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_setChannelScatterGather(uint32_t channelNum, uint32_t taskCount,
        void *taskList, uint32_t isPeriphSG);

//*****************************************************************************
//
//! Gets the current transfer size for a DMA channel control structure.
//!
//! \param channelStructIndex is the logical OR of the DMA channel number
//! with either \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//!
//! This function is used to get the DMA transfer size for a channel.  The
//! transfer size is the number of items to transfer, where the size of an item
//! might be 8, 16, or 32 bits.  If a partial transfer has already occurred,
//! then the number of remaining items is returned.  If the transfer is
//! complete, then 0 is returned.
//!
//! \return Returns the number of items remaining to transfer.
//
//*****************************************************************************
extern uint32_t DMA_getChannelSize(uint32_t channelStructIndex);

//*****************************************************************************
//
//! Gets the transfer mode for a DMA channel control structure.
//!
//! \param channelStructIndex is the logical OR of the DMA channel number
//! with either \b UDMA_PRI_SELECT or \b UDMA_ALT_SELECT.
//!
//! This function is used to get the transfer mode for the DMA channel and
//! to query the status of a transfer on a channel.  When the transfer is
//! complete the mode is \b UDMA_MODE_STOP.
//!
//! \return Returns the transfer mode of the specified channel and control
//! structure, which is one of the following values: \b UDMA_MODE_STOP,
//! \b UDMA_MODE_BASIC, \b UDMA_MODE_AUTO, \b UDMA_MODE_PINGPONG,
//! \b UDMA_MODE_MEM_SCATTER_GATHER, or \b UDMA_MODE_PER_SCATTER_GATHER.
//
//*****************************************************************************
extern uint32_t DMA_getChannelMode(uint32_t channelStructIndex);

//*****************************************************************************
//
//! Assigns a peripheral mapping for a DMA channel.
//!
//! \param mapping is a macro specifying the peripheral assignment for
//! a channel.
//!
//! This function assigns a peripheral mapping to a DMA channel.  It is
//! used to select which peripheral is used for a DMA channel.  The parameter
//! \e mapping should be one of the macros named \b UDMA_CHn_tttt from the
//! header file \e dma.h.  For example, to assign DMA channel 0 to the
//! eUSCI AO RX channel, the parameter should be the macro
//! \b UDMA_CH1_EUSCIA0RX.
//!
//! Please consult the data sheet for a table showing all the
//! possible peripheral assignments for the DMA channels for a particular
//! device.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_assignChannel(uint32_t mapping);

//*****************************************************************************
//
//! Initializes a software transfer of the corresponding DMA channel. This is
//! done if the user wants to force a DMA on the specified channel without the
//! hardware precondition. Specific channels can be configured using the
//! DMA_assignChannel function.
//!
//! \param channel is the channel to trigger the interrupt
//!
//!
//! \return None
//
//*****************************************************************************
extern void DMA_requestSoftwareTransfer(uint32_t channel);

//*****************************************************************************
//
//! Assigns a specific DMA channel to the corresponding interrupt handler. For
//! MSP432 devices, there are three configurable interrupts, and one master
//! interrupt. This function will assign a specific DMA channel to the
//! provided configurable DMA interrupt.
//!
//! Note that once a channel is assigned to a configurable interrupt, it will be
//! masked in hardware from the master DMA interrupt (interruptNumber zero). This
//! function can also be used in conjunction with the DMAIntTrigger function
//! to provide the feature to software trigger specific channel interrupts.
//!
//! \param interruptNumber is the configurable interrupt to assign the given
//! channel. Valid values are:
//! - \b DMA_INT1 the first configurable DMA interrupt handler
//! - \b DMA_INT2 the second configurable DMA interrupt handler
//! - \b DMA_INT3 the third configurable DMA interrupt handler
//!
//! \param channel is the channel to assign the interrupt
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_assignInterrupt(uint32_t interruptNumber, uint32_t channel);

//*****************************************************************************
//
//! Enables the specified interrupt for the DMA controller. Note for interrupts
//! one through three, specific channels have to be mapped to the interrupt
//! using the DMA_assignInterrupt function.
//!
//! \param interruptNumber identifies which DMA interrupt is to be enabled.
//! This interrupt should be one of the following:
//!
//! - \b DMA_INT0 the master DMA interrupt handler
//! - \b DMA_INT1 the first configurable DMA interrupt handler
//! - \b DMA_INT2 the second configurable DMA interrupt handler
//! - \b DMA_INT3 the third configurable DMA interrupt handler
//! - \b DMA_INTERR the third configurable DMA interrupt handler
//!
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_enableInterrupt(uint32_t interruptNumber);

//*****************************************************************************
//
//! Disables the specified interrupt for the DMA controller.
//!
//! \param interruptNumber identifies which DMA interrupt is to be disabled.
//! This interrupt should be one of the following:
//!
//! - \b DMA_INT0 the master DMA interrupt handler
//! - \b DMA_INT1 the first configurable DMA interrupt handler
//! - \b DMA_INT2 the second configurable DMA interrupt handler
//! - \b DMA_INT3 the third configurable DMA interrupt handler
//! - \b DMA_INTERR the third configurable DMA interrupt handler
//!
//!  Note for interrupts that are associated with a specific DMA channel
//! (DMA_INT1 - DMA_INT3), this function will also enable that specific
//! channel for interrupts.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_disableInterrupt(uint32_t interruptNumber);

//*****************************************************************************
//
//! Gets the DMA controller channel interrupt status for interrupt zero.
//!
//! This function is used to get the interrupt status of the DMA controller.
//! The returned value is a 32-bit bit mask that indicates which channels are
//! requesting an interrupt.  This function can be used from within an
//! interrupt handler to determine or confirm which DMA channel has requested
//! an interrupt.
//!
//! Note that this will only apply to interrupt zero for the DMA
//! controller as only one interrupt can be associated with interrupts one
//! through three. If an interrupt is assigned to an interrupt other
//! than interrupt zero, it will be masked by this function.
//!
//! \return Returns a 32-bit mask which indicates requesting DMA channels.
//! There is a bit for each channel and a 1 indicates that the channel
//! is requesting an interrupt.  Multiple bits can be set.
//
//*****************************************************************************
extern uint32_t DMA_getInterruptStatus(void);

//*****************************************************************************
//
//! Clears the DMA controller channel interrupt mask for interrupt zero.
//!
//! \param channel is the channel interrupt to clear.
//!
//! This function is used to clear  the interrupt status of the DMA controller.
//! Note that only interrupts that weren't assigned to DMA interrupts one
//! through three using the DMA_assignInterrupt function will be affected by
//! thisfunctions. For other DMA interrupts, only one channel can be associated
//! and therefore clearing in unnecessary.
//!
//! \return None
//
//*****************************************************************************
extern void DMA_clearInterruptFlag(uint32_t intChannel);

//*****************************************************************************
//
//! Registers an interrupt handler for the DMA controller.
//!
//! \param interruptNumber identifies which DMA interrupt is to be registered.
//! \param intHandler is a pointer to the function to be called when the
//! interrupt is called.
//!
//! This function registers and enables the handler to be called when the DMA
//! controller generates an interrupt.  The \e interrupt parameter should be
//! one of the following:
//!
//! - \b DMA_INT0 the master DMA interrupt handler
//! - \b DMA_INT1 the first configurable DMA interrupt handler
//! - \b DMA_INT2 the second configurable DMA interrupt handler
//! - \b DMA_INT3 the third configurable DMA interrupt handler
//! - \b DMA_INTERR the third configurable DMA interrupt handler
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_registerInterrupt(uint32_t intChannel,
                                    void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters an interrupt handler for the DMA controller.
//!
//! \param interruptNumber identifies which DMA interrupt to unregister.
//!
//! This function disables and unregisters the handler to be called for the
//! specified DMA interrupt.  The \e interrupt parameter should be one of
//! \b the parameters as documented for the function
//! DMA_registerInterrupt().
//!
//! Note fore interrupts that are associated with a specific DMA channel
//! (DMA_INT1 - DMA_INT3), this function will also disable that specific
//! channel for interrupts.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//!  registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void DMA_unregisterInterrupt(uint32_t intChannel);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __UDMA_H__
