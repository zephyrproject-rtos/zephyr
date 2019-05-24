/******************************************************************************
*  Filename:       i2c.c
*  Revised:        2017-04-26 18:27:45 +0200 (Wed, 26 Apr 2017)
*  Revision:       48852
*
*  Description:    Driver for the I2C module
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

#include "i2c.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  I2CMasterInitExpClk
    #define I2CMasterInitExpClk             NOROM_I2CMasterInitExpClk
    #undef  I2CMasterErr
    #define I2CMasterErr                    NOROM_I2CMasterErr
    #undef  I2CIntRegister
    #define I2CIntRegister                  NOROM_I2CIntRegister
    #undef  I2CIntUnregister
    #define I2CIntUnregister                NOROM_I2CIntUnregister
#endif

//*****************************************************************************
//
// Initializes the I2C Master block
//
//*****************************************************************************
void
I2CMasterInitExpClk(uint32_t ui32Base, uint32_t ui32I2CClk,
                    bool bFast)
{
    uint32_t ui32SCLFreq;
    uint32_t ui32TPR;

    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Must enable the device before doing anything else.
    I2CMasterEnable(I2C0_BASE);

    // Get the desired SCL speed.
    if(bFast == true)
    {
        ui32SCLFreq = 400000;
    }
    else
    {
        ui32SCLFreq = 100000;
    }

    // Compute the clock divider that achieves the fastest speed less than or
    // equal to the desired speed. The numerator is biased to favor a larger
    // clock divider so that the resulting clock is always less than or equal
    // to the desired clock, never greater.
    ui32TPR = ((ui32I2CClk + (2 * 10 * ui32SCLFreq) - 1) / (2 * 10 * ui32SCLFreq)) - 1;
    HWREG(I2C0_BASE + I2C_O_MTPR) = ui32TPR;
}

//*****************************************************************************
//
// Gets the error status of the I2C Master module
//
//*****************************************************************************
uint32_t
I2CMasterErr(uint32_t ui32Base)
{
    uint32_t ui32Err;

    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Get the raw error state.
    ui32Err = HWREG(I2C0_BASE + I2C_O_MSTAT);

    // If the I2C master is busy, then all the other status bits are invalid,
    // and there is no error to report.
    if(ui32Err & I2C_MSTAT_BUSY)
    {
        return(I2C_MASTER_ERR_NONE);
    }

    // Check for errors.
    if(ui32Err & (I2C_MSTAT_ERR | I2C_MSTAT_ARBLST))
    {
        return(ui32Err & (I2C_MSTAT_ARBLST | I2C_MSTAT_DATACK_N | I2C_MSTAT_ADRACK_N));
    }
    else
    {
        return(I2C_MASTER_ERR_NONE);
    }
}

//*****************************************************************************
//
// Registers an interrupt handler for the I2C module
//
//*****************************************************************************
void
I2CIntRegister(uint32_t ui32Base, void (*pfnHandler)(void))
{
    uint32_t ui32Int;

    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Get the interrupt number.
    ui32Int = INT_I2C_IRQ;

    // Register the interrupt handler, returning an error if an error occurs.
    IntRegister(ui32Int, pfnHandler);

    // Enable the I2C interrupt.
    IntEnable(ui32Int);
}

//*****************************************************************************
//
// Unregisters an interrupt handler for the I2C module
//
//*****************************************************************************
void
I2CIntUnregister(uint32_t ui32Base)
{
    uint32_t ui32Int;

    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Get the interrupt number.
    ui32Int = INT_I2C_IRQ;

    // Disable the interrupt.
    IntDisable(ui32Int);

    // Unregister the interrupt handler.
    IntUnregister(ui32Int);
}
