/******************************************************************************
*  Filename:       ddi.c
*  Revised:        2018-06-04 16:10:13 +0200 (Mon, 04 Jun 2018)
*  Revision:       52111
*
*  Description:    Driver for the DDI master interface
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

#include "ddi.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  DDI32RegWrite
    #define DDI32RegWrite                   NOROM_DDI32RegWrite
    #undef  DDI16BitWrite
    #define DDI16BitWrite                   NOROM_DDI16BitWrite
    #undef  DDI16BitfieldWrite
    #define DDI16BitfieldWrite              NOROM_DDI16BitfieldWrite
    #undef  DDI16BitRead
    #define DDI16BitRead                    NOROM_DDI16BitRead
    #undef  DDI16BitfieldRead
    #define DDI16BitfieldRead               NOROM_DDI16BitfieldRead
#endif

//*****************************************************************************
//
// Write a 32 bit value to a register in the DDI slave.
//
//*****************************************************************************
void
DDI32RegWrite(uint32_t ui32Base, uint32_t ui32Reg,
              uint32_t ui32Val)
{
    // Check the arguments.
    ASSERT(DDIBaseValid(ui32Base));
    ASSERT(ui32Reg < DDI_SLAVE_REGS);

    // Write the value to the register.
    HWREG(ui32Base + ui32Reg) = ui32Val;
}

//*****************************************************************************
//
// Write a single bit using a 16-bit maskable write
//
//*****************************************************************************
void
DDI16BitWrite(uint32_t ui32Base, uint32_t ui32Reg,
              uint32_t ui32Mask, uint32_t ui32WrData)
{
    uint32_t ui32RegAddr;
    uint32_t ui32Data;

    // Check the arguments.
    ASSERT(DDIBaseValid(ui32Base));
    ASSERT(!((ui32Mask & 0xFFFF0000) ^ (ui32Mask & 0x0000FFFF)));
    ASSERT(!(ui32WrData & 0xFFFF0000));

    // DDI 16-bit target is on 32-bit boundary so double offset
    ui32RegAddr = ui32Base + (ui32Reg << 1) + DDI_O_MASK16B;

    // Adjust for target bit in high half of the word.
    if(ui32Mask & 0xFFFF0000)
    {
        ui32RegAddr += 4;
        ui32Mask >>= 16;
    }

    // Write mask if data is not zero (to set mask bit), else write '0'.
    ui32Data = ui32WrData ? ui32Mask : 0x0;

    // Update the register.
    HWREG(ui32RegAddr) = (ui32Mask << 16) | ui32Data;
}

//*****************************************************************************
//
// Write a bit field via the DDI using 16-bit maskable write
//
//*****************************************************************************
void
DDI16BitfieldWrite(uint32_t ui32Base, uint32_t ui32Reg,
                   uint32_t ui32Mask, uint32_t ui32Shift,
                   uint16_t ui32Data)
{
    uint32_t ui32RegAddr;
    uint32_t ui32WrData;

    // Check the arguments.
    ASSERT(DDIBaseValid(ui32Base));

    // 16-bit target is on 32-bit boundary so double offset.
    ui32RegAddr = ui32Base + (ui32Reg << 1) + DDI_O_MASK16B;

    // Adjust for target bit in high half of the word.
    if(ui32Shift >= 16)
    {
        ui32Shift = ui32Shift - 16;
        ui32RegAddr += 4;
        ui32Mask = ui32Mask >> 16;
    }

    // Shift data in to position.
    ui32WrData = ui32Data << ui32Shift;

    // Write data.
    HWREG(ui32RegAddr) = (ui32Mask << 16) | ui32WrData;
}

//*****************************************************************************
//
// Read a bit via the DDI using 16-bit READ.
//
//*****************************************************************************
uint16_t
DDI16BitRead(uint32_t ui32Base, uint32_t ui32Reg, uint32_t ui32Mask)
{
    uint32_t ui32RegAddr;
    uint16_t ui16Data;

    // Check the arguments.
    ASSERT(DDIBaseValid(ui32Base));

    // Calculate the address of the register.
    ui32RegAddr = ui32Base + ui32Reg + DDI_O_DIR;

    // Adjust for target bit in high half of the word.
    if(ui32Mask & 0xFFFF0000)
    {
        ui32RegAddr += 2;
        ui32Mask = ui32Mask >> 16;
    }

    // Read a halfword on the DDI interface.
    ui16Data = HWREGH(ui32RegAddr);

    // Mask data.
    ui16Data = ui16Data & ui32Mask;

    // Return masked data.
    return(ui16Data);
}

//*****************************************************************************
//
// Read a bit field via the DDI using 16-bit read.
//
//*****************************************************************************
uint16_t
DDI16BitfieldRead(uint32_t ui32Base, uint32_t ui32Reg,
                  uint32_t ui32Mask, uint32_t ui32Shift)
{
    uint32_t ui32RegAddr;
    uint16_t ui16Data;

    // Check the arguments.
    ASSERT(DDIBaseValid(ui32Base));

    // Calculate the register address.
    ui32RegAddr = ui32Base + ui32Reg + DDI_O_DIR;

    // Adjust for target bit in high half of the word.
    if(ui32Shift >= 16)
    {
        ui32Shift = ui32Shift - 16;
        ui32RegAddr += 2;
        ui32Mask = ui32Mask >> 16;
    }

    // Read the register.
    ui16Data = HWREGH(ui32RegAddr);

    // Mask data and shift into place.
    ui16Data &= ui32Mask;
    ui16Data >>= ui32Shift;

    // Return data.
    return(ui16Data);
}
