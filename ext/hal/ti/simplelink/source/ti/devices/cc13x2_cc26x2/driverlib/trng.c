/******************************************************************************
*  Filename:       trng.c
*  Revised:        2017-04-26 18:27:45 +0200 (Wed, 26 Apr 2017)
*  Revision:       48852
*
*  Description:    Driver for the TRNG module
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

#include "trng.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  TRNGConfigure
    #define TRNGConfigure                   NOROM_TRNGConfigure
    #undef  TRNGNumberGet
    #define TRNGNumberGet                   NOROM_TRNGNumberGet
#endif

//*****************************************************************************
//
// Configure the true random number generator
//
//*****************************************************************************
void
TRNGConfigure(uint32_t ui32MinSamplesPerCycle,
              uint32_t ui32MaxSamplesPerCycle,
              uint32_t ui32ClocksPerSample)
{
    uint32_t ui32Val;

    // Make sure the TRNG is disabled.
    ui32Val = HWREG(TRNG_BASE + TRNG_O_CTL) & ~TRNG_CTL_TRNG_EN;
    HWREG(TRNG_BASE + TRNG_O_CTL) = ui32Val;

    // Configure the startup number of samples.
    ui32Val &= ~TRNG_CTL_STARTUP_CYCLES_M;
    ui32Val |= ((( ui32MaxSamplesPerCycle >> 8 ) << TRNG_CTL_STARTUP_CYCLES_S ) & TRNG_CTL_STARTUP_CYCLES_M );
    HWREG(TRNG_BASE + TRNG_O_CTL) = ui32Val;

    // Configure the minimum and maximum number of samples pr generated number
    // and the number of clocks per sample.
    HWREG(TRNG_BASE + TRNG_O_CFG0) = (
        ((( ui32MaxSamplesPerCycle >> 8 ) << TRNG_CFG0_MAX_REFILL_CYCLES_S ) & TRNG_CFG0_MAX_REFILL_CYCLES_M ) |
        ((( ui32ClocksPerSample         ) << TRNG_CFG0_SMPL_DIV_S          ) & TRNG_CFG0_SMPL_DIV_M          ) |
        ((( ui32MinSamplesPerCycle >> 6 ) << TRNG_CFG0_MIN_REFILL_CYCLES_S ) & TRNG_CFG0_MIN_REFILL_CYCLES_M )   );
}

//*****************************************************************************
//
// Get a random number from the generator
//
//*****************************************************************************
uint32_t
TRNGNumberGet(uint32_t ui32Word)
{
    uint32_t ui32RandomNumber;

    // Check the arguments.
    ASSERT((ui32Word == TRNG_HI_WORD) ||
           (ui32Word == TRNG_LOW_WORD));

    // Return the right requested part of the generated number.
    if(ui32Word == TRNG_HI_WORD)
    {
        ui32RandomNumber = HWREG(TRNG_BASE + TRNG_O_OUT1);
    }
    else
    {
        ui32RandomNumber = HWREG(TRNG_BASE + TRNG_O_OUT0);
    }

    // Initiate generation of new number.
    HWREG(TRNG_BASE + TRNG_O_IRQFLAGCLR) = 0x1;

    // Return the random number.
    return ui32RandomNumber;
}
