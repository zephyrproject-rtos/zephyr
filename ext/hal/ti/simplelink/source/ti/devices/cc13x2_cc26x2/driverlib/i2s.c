/******************************************************************************
*  Filename:       i2s.c
*  Revised:        2017-05-08 12:18:04 +0200 (Mon, 08 May 2017)
*  Revision:       48924
*
*  Description:    Driver for the I2S.
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

#include "i2s.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  I2SEnable
    #define I2SEnable                       NOROM_I2SEnable
    #undef  I2SAudioFormatConfigure
    #define I2SAudioFormatConfigure         NOROM_I2SAudioFormatConfigure
    #undef  I2SChannelConfigure
    #define I2SChannelConfigure             NOROM_I2SChannelConfigure
    #undef  I2SBufferConfig
    #define I2SBufferConfig                 NOROM_I2SBufferConfig
    #undef  I2SPointerUpdate
    #define I2SPointerUpdate                NOROM_I2SPointerUpdate
    #undef  I2SPointerSet
    #define I2SPointerSet                   NOROM_I2SPointerSet
    #undef  I2SSampleStampConfigure
    #define I2SSampleStampConfigure         NOROM_I2SSampleStampConfigure
    #undef  I2SSampleStampGet
    #define I2SSampleStampGet               NOROM_I2SSampleStampGet
#endif

//*****************************************************************************
//
// Global pointer to the current I2S data structure
//
//*****************************************************************************
I2SControlTable *g_pControlTable;

//*****************************************************************************
//
// Enables the I2S module for operation
//
//*****************************************************************************
void
I2SEnable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Make sure the control table pointer is setup to a memory location.
    if(!(g_pControlTable))
    {
        return;
    }

    // Write the address to the first input/output buffer.
    HWREG(I2S0_BASE + I2S_O_AIFINPTRNEXT) = g_pControlTable->ui32InBase;
    g_pControlTable->ui32InOffset = 0;
    HWREG(I2S0_BASE + I2S_O_AIFOUTPTRNEXT) = g_pControlTable->ui32OutBase;
    g_pControlTable->ui32OutOffset = 0;

    // Enable the I2S module.
    HWREG(I2S0_BASE + I2S_O_AIFDMACFG) = (uint32_t)g_pControlTable->ui16DMABufSize - 1;
}

//*****************************************************************************
//
// Configures the I2S module
//
//*****************************************************************************
void
I2SAudioFormatConfigure(uint32_t ui32Base, uint32_t ui32FmtCfg,
                        uint32_t ui32BitClkDelay)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));
    ASSERT(ui32BitClkDelay <= 255);

    // Save the length of the audio words stored in memory.
    g_pControlTable->ui16MemLen = (ui32FmtCfg & I2S_MEM_LENGTH_24) ? 24 : 16;

    // Write the configuration.
    HWREG(I2S0_BASE + I2S_O_AIFFMTCFG) = ui32FmtCfg | (ui32BitClkDelay << I2S_AIFFMTCFG_DATA_DELAY_S);
}

//****************************************************************************
//
// Setup the audio channel configuration
//
//****************************************************************************
void
I2SChannelConfigure(uint32_t ui32Base, uint32_t ui32Chan0Cfg,
                    uint32_t ui32Chan1Cfg)
{
    uint32_t ui32InChan;
    uint32_t ui32OutChan;
    uint32_t ui32ChanMask;

    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));
    ASSERT(ui32Chan0Cfg & (I2S_CHAN_CFG_MASK | I2S_LINE_MASK))
    ASSERT(ui32Chan1Cfg & (I2S_CHAN_CFG_MASK | I2S_LINE_MASK))

    ui32InChan = 0;
    ui32OutChan = 0;

    // Configure input/output channels.
    HWREG(I2S0_BASE + I2S_O_AIFDIRCFG) = (
        (( ui32Chan0Cfg << I2S_AIFDIRCFG_AD0_S) & I2S_AIFDIRCFG_AD0_M ) |
        (( ui32Chan1Cfg << I2S_AIFDIRCFG_AD1_S) & I2S_AIFDIRCFG_AD1_M )   );

    // Configure the valid channel mask.
    HWREG(I2S0_BASE + I2S_O_AIFWMASK0) = (ui32Chan0Cfg >> 8) & I2S_AIFWMASK0_MASK_M;
    HWREG(I2S0_BASE + I2S_O_AIFWMASK1) = (ui32Chan1Cfg >> 8) & I2S_AIFWMASK1_MASK_M;

    // Resolve and save the number of input and output channels.
    ui32ChanMask = (ui32Chan0Cfg & I2S_CHAN_CFG_MASK) >> 8;
    if(ui32Chan0Cfg & I2S_LINE_INPUT)
    {
        while(ui32ChanMask)
        {
            if(ui32ChanMask & 0x1)
            {
                ui32InChan++;
            }
            // Shift down channel mask
            ui32ChanMask >>= 1;
        }

    }
    else if(ui32Chan0Cfg & I2S_LINE_OUTPUT)
    {
        while(ui32ChanMask)
        {
            if(ui32ChanMask & 0x1)
            {
                ui32OutChan++;
            }
            // Shift down channel mask
            ui32ChanMask >>= 1;
        }
    }

    ui32ChanMask = (ui32Chan1Cfg & I2S_CHAN_CFG_MASK) >> 8;
    if(ui32Chan1Cfg & I2S_LINE_INPUT)
    {
        while(ui32ChanMask)
        {
            if(ui32ChanMask & 0x1)
            {
                ui32InChan++;
            }
            // Shift down channel mask
            ui32ChanMask >>= 1;
        }
    }
    else if(ui32Chan1Cfg & I2S_LINE_OUTPUT)
    {
        while(ui32ChanMask)
        {
            if(ui32ChanMask & 0x1)
            {
                ui32OutChan++;
            }
            // Shift down channel mask
            ui32ChanMask >>= 1;
        }
    }

    g_pControlTable->ui8InChan = (uint8_t)ui32InChan;
    g_pControlTable->ui8OutChan = (uint8_t)ui32OutChan;
}

//****************************************************************************
//
// Set the input buffer pointers
//
//****************************************************************************
void
I2SBufferConfig(uint32_t ui32Base, uint32_t ui32InBufBase,
                uint32_t ui32OutBufBase, uint16_t ui16DMABufSize,
                uint16_t ui16ChanBufSize)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));
    ASSERT(ui16DMABufSize > 0);

    // Setup the input data pointer and buffer sizes.
    g_pControlTable->ui16DMABufSize = ui16DMABufSize;
    g_pControlTable->ui16ChBufSize = ui16ChanBufSize;
    g_pControlTable->ui32InBase = ui32InBufBase;
    g_pControlTable->ui32OutBase = ui32OutBufBase;
}

//****************************************************************************
//
// Set the buffer pointers
//
//****************************************************************************
void
I2SPointerSet(uint32_t ui32Base, bool bInput, void * pNextPointer)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Update the next input/output pointer with the correct address.
    if(bInput == true)
    {
        HWREG(I2S0_BASE + I2S_O_AIFINPTRNEXT) = (uint32_t)pNextPointer;
    }
    else
    {
        HWREG(I2S0_BASE + I2S_O_AIFOUTPTRNEXT) = (uint32_t)pNextPointer;
    }
}

//****************************************************************************
//
// Update the buffer pointers
//
//****************************************************************************
void
I2SPointerUpdate(uint32_t ui32Base, bool bInput)
{
    uint32_t ui32NextPtr;

    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Update the next input/output pointer with the correct address.
    if(bInput == true)
    {
        ui32NextPtr = (g_pControlTable->ui8InChan *
                       (g_pControlTable->ui16MemLen >> 3)) *
                      g_pControlTable->ui16DMABufSize;
        g_pControlTable->ui32InOffset = ((g_pControlTable->ui32InOffset +
                                         ui32NextPtr) %
                                         g_pControlTable->ui16ChBufSize);
        HWREG(I2S0_BASE + I2S_O_AIFINPTRNEXT) = g_pControlTable->ui32InOffset +
                                               g_pControlTable->ui32InBase;
    }
    else
    {
        ui32NextPtr = (g_pControlTable->ui8OutChan *
                       (g_pControlTable->ui16MemLen >> 3)) *
                      g_pControlTable->ui16DMABufSize;
        g_pControlTable->ui32OutOffset = ((g_pControlTable->ui32OutOffset +
                                         ui32NextPtr) %
                                         g_pControlTable->ui16ChBufSize);
        HWREG(I2S0_BASE + I2S_O_AIFOUTPTRNEXT) =
                         g_pControlTable->ui32OutOffset +
                         g_pControlTable->ui32OutBase;
    }
}

//*****************************************************************************
//
// Configure the sample stamp generator
//
//*****************************************************************************
void
I2SSampleStampConfigure(uint32_t ui32Base, bool bInput, bool bOutput)
{
    uint32_t ui32Trigger;

    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    ui32Trigger = HWREG(I2S0_BASE + I2S_O_STMPWCNT);
    ui32Trigger = (ui32Trigger + 2) % g_pControlTable->ui16ChBufSize;

    // Setup the sample stamp trigger for input streams.
    if(bInput)
    {
        HWREG(I2S0_BASE + I2S_O_STMPINTRIG) = ui32Trigger;
    }

    // Setup the sample stamp trigger for output streams.
    if(bOutput)
    {
        HWREG(I2S0_BASE + I2S_O_STMPOUTTRIG) = ui32Trigger;
    }

}

//*****************************************************************************
//
// Get the current value of a sample stamp counter
//
//*****************************************************************************
uint32_t
I2SSampleStampGet(uint32_t ui32Base, uint32_t ui32Channel)
{
    uint32_t ui32FrameClkCnt;
    uint32_t ui32SysClkCnt;
    uint32_t ui32PeriodSysClkCnt;
    uint32_t ui32SampleStamp;

    // Get the number of Frame clock counts since last stamp.
    ui32FrameClkCnt = HWREG(I2S0_BASE + I2S_O_STMPWCNTCAPT0);

    // Get the number of system clock ticks since last frame clock edge.
    ui32SysClkCnt = HWREG(I2S0_BASE + I2S_O_STMPXCNTCAPT0);

    // Get the number system clock ticks in the last frame clock period.
    ui32PeriodSysClkCnt = HWREG(I2S0_BASE + I2S_O_STMPXPER);

    // Calculate the sample stamp.
    ui32SampleStamp = (ui32SysClkCnt << 16) / ui32PeriodSysClkCnt;
    ui32SampleStamp = (ui32SampleStamp > I2S_STMP_SATURATION) ?
                      I2S_STMP_SATURATION : ui32SampleStamp;
    ui32SampleStamp |= (ui32FrameClkCnt << 16);

    return (ui32SampleStamp);
}
