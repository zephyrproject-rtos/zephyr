/******************************************************************************
*  Filename:       adi.h
*  Revised:        2016-11-17 16:39:28 +0100 (Thu, 17 Nov 2016)
*  Revision:       47706
*
*  Description:    Defines and prototypes for the ADI master interface.
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

//*****************************************************************************
//
//! \addtogroup analog_group
//! @{
//! \addtogroup adi_api
//! @{
//
//*****************************************************************************

#ifndef __ADI_H__
#define __ADI_H__

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
#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_uart.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_adi.h"
#include "debug.h"
#include "ddi.h"

//*****************************************************************************
//
// Number of registers in the ADI slave
//
//*****************************************************************************
#define ADI_SLAVE_REGS      16


//*****************************************************************************
//
// Defines that is used to control the ADI slave and master
//
//*****************************************************************************
#define ADI_PROTECT         0x00000080
#define ADI_ACK             0x00000001
#define ADI_SYNC            0x00000000

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

#ifdef DRIVERLIB_DEBUG
//*****************************************************************************
//
//! \internal
//! \brief Checks an ADI base address.
//!
//! This function determines if an ADI port base address is valid.
//!
//! \param ui32Base is the base address of the ADI port.
//!
//! \return Returns \c true if the base address is valid and \c false
//! otherwise
//
//*****************************************************************************
static bool
ADIBaseValid(uint32_t ui32Base)
{
    return(ui32Base == ADI2_BASE || ui32Base == ADI3_BASE ||
           ui32Base == AUX_ADI4_BASE);
}
#endif





//*****************************************************************************
//
//! \brief Write an 8 bit value to a register in an ADI slave.
//!
//! This function will write a value to a single register in the analog domain.
//! The access to the registers in the analog domain is either 8, 16, or 32 bit
//! aligned. You can only do 16 bit access on registers 0-1 / 2-3, etc. Similarly
//! 32 bit accesses are always performed on register 0-3 / 4-7, etc. Addresses
//! for the registers and values being written to the registers will be
//! truncated according to this access scheme.
//!
//! \note This operation is write only for the specified register. No
//! previous value of the register will be kept (i.e. this is NOT
//! read-modify-write on the register).
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the register to write.
//! \param ui8Val is the 8 bit value to write to the register.
//!
//! \return None
//!
//! \sa ADI16RegWrite(), ADI32RegWrite()
//
//*****************************************************************************
__STATIC_INLINE void
ADI8RegWrite(uint32_t ui32Base, uint32_t ui32Reg, uint8_t ui8Val)
{
    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Write the value to the register.
    HWREGB(ui32Base + ui32Reg) = ui8Val;
}

//*****************************************************************************
//
//! \brief Write a 16 bit value to 2 registers in the ADI slave.
//!
//! This function will write a value to 2 consecutive registers in the analog
//! domain. The access to the registers in the analog domain is either 8, 16
//! or 32 bit aligned. You can only do 16 bit access on registers 0-1 / 2-3,
//! etc. Similarly 32 bit accesses are always performed on register 0-3 / 4-7,
//! etc. Addresses for the registers and values being written
//! to the registers will be truncated according to this access scheme.
//!
//! \note The byte addressing bit will be ignored, to ensure 16 bit access
//! to the ADI slave.
//!
//! \note This operation is write only for the specified register. No
//! previous value of the register will be kept (i.e. this is NOT
//! read-modify-write on the register).
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the register to write.
//! \param ui16Val is the 16 bit value to write to the register.
//!
//! \return None
//!
//! \sa ADI8RegWrite(), ADI32RegWrite()
//
//*****************************************************************************
__STATIC_INLINE void
ADI16RegWrite(uint32_t ui32Base, uint32_t ui32Reg,
              uint16_t ui16Val)
{
    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Write the value to the register.
    HWREGH(ui32Base + (ui32Reg & 0xFE)) = ui16Val;
}

//*****************************************************************************
//
//! \brief Write a 32 bit value to 4 registers in the ADI slave.
//!
//! This function will write a value to 4 consecutive registers in the analog
//! domain. The access to the registers in the analog domain is either 8, 16
//! or 32 bit aligned. You can only do 16 bit access on registers 0-1 / 2-3,
//! etc. Similarly 32 bit accesses are always performed on register 0-3 / 4-7,
//! etc. Addresses for the registers and values being written
//! to the registers will be truncated according to this access scheme.
//!
//! \note The byte and half word addressing bits will be ignored, to ensure
//! 32 bit access to the ADI slave.
//!
//! \note This operation is write only for the specified register. No
//! previous value of the register will be kept (i.e. this is NOT
//! read-modify-write on the register).
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the register to write.
//! \param ui32Val is the 32 bit value to write to the register.
//!
//! \return None
//!
//! \sa ADI8RegWrite(), ADI16RegWrite()
//
//*****************************************************************************
__STATIC_INLINE void
ADI32RegWrite(uint32_t ui32Base, uint32_t ui32Reg, uint32_t ui32Val)
{
    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Write the value to the register.
    HWREG(ui32Base + (ui32Reg & 0xFC)) = ui32Val;
}

//*****************************************************************************
//
//! \brief Read the value of an 8 bit register in the ADI slave.
//!
//! This function will read an 8 bit register in the analog domain and return
//! the value as the lower 8 bits of an \c uint32_t. The access to the
//! registers in the analog domain is either 8, 16 or 32 bit aligned. You can
//! only do 16 bit access on registers 0-1 / 2-3, etc. Similarly 32 bit accesses
//! are always performed on register 0-3 / 4-7, etc. Addresses for the
//! registers and values being written to the registers will be truncated
//! according to this access scheme.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the 8 bit register to read.
//!
//! \return Returns the 8 bit value of the analog register in the least
//! significant byte of the \c uint32_t.
//!
//! \sa ADI16RegRead(), ADI32RegRead()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
ADI8RegRead(uint32_t ui32Base, uint32_t ui32Reg)
{
    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Read the register and return the value.
    return(HWREGB(ui32Base + ui32Reg));
}

//*****************************************************************************
//
//! \brief Read the value in a 16 bit register.
//!
//! This function will read 2 x 8 bit registers in the analog domain and return
//! the value as the lower 16 bits of an \c uint32_t. The access to the
//! registers in the analog domain is either 8, 16 or 32 bit aligned. You can
//! only do 16 bit access on registers 0-1 / 2-3, etc. Similarly 32 bit accesses
//! are always performed on register 0-3 / 4-7, etc. Addresses for the
//! registers and values being written to the registers will be truncated
//! according to this access scheme.
//!
//! \note The byte addressing bit will be ignored, to ensure 16 bit access
//! to the ADI slave.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the 16 bit register to read.
//!
//! \return Returns the 16 bit value of the 2 analog register in the 2 least
//! significant bytes of the \c uint32_t.
//!
//! \sa ADI8RegRead(), ADI32RegRead()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
ADI16RegRead(uint32_t ui32Base, uint32_t ui32Reg)
{
    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Read the registers and return the value.
    return(HWREGH(ui32Base + (ui32Reg & 0xFE)));
}

//*****************************************************************************
//
//! \brief Read the value in a 32 bit register.
//!
//! This function will read 4 x 8 bit registers in the analog domain and return
//! the value as an \c uint32_t. The access to the registers in the analog
//! domain is either 8, 16 or 32 bit aligned. You can only do 16 bit access on
//! registers 0-1 / 2-3, etc. Similarly 32 bit accesses are always performed on
//! register 0-3 / 4-7, etc. Addresses for the registers and values being
//! written to the registers will be truncated according to this access scheme.
//!
//! \note The byte and half word addressing bits will be ignored, to ensure
//! 32 bit access to the ADI slave.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the 32 bit register to read.
//!
//! \return Returns the 32 bit value of the 4 analog registers.
//!
//! \sa ADI8RegRead(), ADI16RegRead()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
ADI32RegRead(uint32_t ui32Base, uint32_t ui32Reg)
{
    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Read the registers and return the value.
    return(HWREG(ui32Base + (ui32Reg & 0xFC)));
}

//*****************************************************************************
//
//! \brief Set specific bits in a single 8 bit ADI register.
//!
//! This function will set bits in a single register in the analog domain.
//! The access to the registers in the analog domain is either 8, 16 or 32 bit
//! aligned, but arranged in chunks of 32 bits. You can only do 16 bit access
//! on registers 0-1 / 2-3, etc. Similarly 32 bit accesses are always
//! performed on register 0-3 / 4-7 etc. Addresses for the registers and values
//! being written to the registers will be truncated according to this access
//! scheme.
//!
//! \note This operation is write only for the specified register.
//! This function is used to set bits in a specific 8 bit register in the
//! ADI slave. Only bits in the selected register are affected by the
//! operation.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the base register to assert the bits in.
//! \param ui8Val is the 8 bit one-hot encoded value specifying which
//! bits to set in the register.
//!
//! \return None
//!
//! \sa ADI16BitsSet(), ADI32BitsSet()
//
//*****************************************************************************
__STATIC_INLINE void
ADI8BitsSet(uint32_t ui32Base, uint32_t ui32Reg, uint8_t ui8Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_SET;

    // Set the selected bits.
    HWREGB(ui32Base + ui32RegOffset + ui32Reg) = ui8Val;
}

//*****************************************************************************
//
//! \brief Set specific bits in 2 x 8 bit ADI slave registers.
//!
//! This function will set bits in 2 registers in the analog domain.
//! The access to the registers in the analog domain is either 8, 16 or 32 bit
//! aligned, but arranged in chunks of 32 bits. You can only do 16 bit access
//! on registers 0-1 / 2-3, etc. Similarly 32 bit accesses are always
//! performed on register 0-3 / 4-7 etc. Addresses for the registers and values
//! being written to the registers will be truncated according to this access
//! scheme.
//!
//! \note This operation is write only for the specified register.
//! This function is used to set bits in 2 consecutive 8 bit registers in the
//! ADI slave. Only bits in the selected registers are affected by the
//! operation.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the base register to assert the bits in.
//! \param ui16Val is the 16 bit one-hot encoded value specifying which
//! bits to set in the registers.
//!
//! \return None
//!
//! \sa ADI8BitsSet(), ADI32BitsSet()
//
//*****************************************************************************
__STATIC_INLINE void
ADI16BitsSet(uint32_t ui32Base, uint32_t ui32Reg, uint16_t ui16Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_SET;

    // Set the selected bits.
    HWREGH(ui32Base + ui32RegOffset + (ui32Reg & 0xFE)) = ui16Val;
}

//*****************************************************************************
//
//! \brief Set specific bits in 4 x 8 bit ADI slave registers.
//!
//! This function will set bits in 4 registers in the analog domain.
//! The access to the registers in the analog domain is either 8, 16 or 32 bit
//! aligned, but arranged in chunks of 32 bits. You can only do 16 bit access
//! on registers 0-1 / 2-3, etc. Similarly 32 bit accesses are always
//! performed on register 0-3 / 4-7 etc. Addresses for the registers and values
//! being written to the registers will be truncated according to this access
//! scheme.
//!
//! \note This operation is write only for the specified register.
//! This function is used to set bits in 4 consecutive 8 bit registers in the
//! ADI slave. Only bits in the selected registers are affected by the
//! operation.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the base register to assert the bits in.
//! \param ui32Val is the 32 bit one-hot encoded value specifying which
//! bits to set in the registers.
//!
//! \return None
//!
//! \sa ADI8BitsSet(), ADI16BitsSet()
//
//*****************************************************************************
__STATIC_INLINE void
ADI32BitsSet(uint32_t ui32Base, uint32_t ui32Reg, uint32_t ui32Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_SET;

    // Set the selected bits.
    HWREG(ui32Base + ui32RegOffset + (ui32Reg & 0xFC)) = ui32Val;
}

//*****************************************************************************
//
//! \brief Clear specific bits in an 8 bit ADI register.
//!
//! This function will clear bits in a register in the analog domain.
//! The access to the registers in the analog domain is either 8, 16 or 32 bit
//! aligned, but arranged in chunks of 32 bits. You can only do 16 bit access
//! on registers 0-1 / 2-3, etc. Similarly 32 bit accesses are always
//! performed on register 0-3 / 4-7 etc. Addresses for the registers and values
//! being written to the registers will be truncated according to this access
//! scheme.
//!
//! \note This operation is write only for the specified register.
//! This function is used to clear bits in a specific 8 bit register in
//! the ADI slave. Only bits in the selected register are affected by the
//! operation.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the base registers to clear the bits in.
//! \param ui8Val is the 8 bit one-hot encoded value specifying which
//! bits to clear in the register.
//!
//! \return None
//!
//! \sa ADI16BitsClear(), ADI32BitsClear()
//
//*****************************************************************************
__STATIC_INLINE void
ADI8BitsClear(uint32_t ui32Base, uint32_t ui32Reg, uint8_t ui8Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_CLR;

    // Set the selected bits.
    HWREGB(ui32Base + ui32RegOffset + ui32Reg) = ui8Val;
}

//*****************************************************************************
//
//! \brief Clear specific bits in two 8 bit ADI register.
//!
//! This function will clear bits in 2 registers in the analog domain.
//! The access to the registers in the analog domain is either 8, 16 or 32 bit
//! aligned, but arranged in chunks of 32 bits. You can only do 16 bit access
//! on registers 0-1 / 2-3, etc. Similarly 32 bit accesses are always
//! performed on register 0-3 / 4-7 etc. Addresses for the registers and values
//! being written to the registers will be truncated according to this access
//! scheme.
//!
//! \note This operation is write only for the specified register.
//! This function is used to clear bits in 2 consecutive 8 bit registers in
//! the ADI slave. Only bits in the selected registers are affected by the
//! operation.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the base registers to clear the bits in.
//! \param ui16Val is the 16 bit one-hot encoded value specifying which
//! bits to clear in the registers.
//!
//! \return None
//!
//! \sa ADI8BitsClear(), ADI32BitsClear()
//
//*****************************************************************************
__STATIC_INLINE void
ADI16BitsClear(uint32_t ui32Base, uint32_t ui32Reg, uint16_t ui16Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_CLR;

    // Set the selected bits.
    HWREGH(ui32Base + ui32RegOffset + (ui32Reg & 0xFE)) = ui16Val;
}

//*****************************************************************************
//
//! \brief Clear specific bits in four 8 bit ADI register.
//!
//! This function will clear bits in 4 registers in the analog domain.
//! The access to the registers in the analog domain is either 8, 16 or 32 bit
//! aligned, but arranged in chunks of 32 bits. You can only do 16 bit access
//! on registers 0-1 / 2-3, etc. Similarly 32 bit accesses are always
//! performed on register 0-3 / 4-7 etc. Addresses for the registers and values
//! being written to the registers will be truncated according to this access
//! scheme.
//!
//! \note This operation is write only for the specified register.
//! This function is used to clear bits in 4 consecutive 8 bit registers in
//! the ADI slave. Only bits in the selected registers are affected by the
//! operation.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is ADI base address.
//! \param ui32Reg is the base registers to clear the bits in.
//! \param ui32Val is the 32 bit one-hot encoded value specifying which
//! bits to clear in the registers.
//!
//! \return None
//!
//! \sa ADI8BitsClear(), ADI16BitsClear()
//
//*****************************************************************************
__STATIC_INLINE void
ADI32BitsClear(uint32_t ui32Base, uint32_t ui32Reg, uint32_t ui32Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_CLR;

    // Set the selected bits.
    HWREG(ui32Base + ui32RegOffset + (ui32Reg & 0xFC)) = ui32Val;
}

//*****************************************************************************
//
//! \brief Set a value on any 4 bits inside an 8 bit register in the ADI slave.
//!
//! This function allows halfbyte (4 bit) access to the ADI slave registers.
//! The parameter \c bWriteHigh determines whether to write to the lower
//! or higher part of the 8 bit register.
//!
//! Use this function to write any value in the range 0-3 bits aligned on a
//! half byte boundary. Fx. for writing the value 0b101 to bits 1 to 3 the
//! \c ui8Val = 0xA and the \c ui8Mask = 0xE. Bit 0 will not be affected by
//! the operation, as the corresponding bit is not set in the \c ui8Mask.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is the base address of the ADI port.
//! \param ui32Reg is the Least Significant Register in the ADI slave that
//! will be affected by the write operation.
//! \param bWriteHigh defines which part of the register to write in.
//! - \c true: Write upper half byte of register.
//! - \c false: Write lower half byte of register.
//! \param ui8Mask is the mask defining which of the 4 bits that should be
//! overwritten. The mask must be defined in the lower half of the 8 bits of
//! the parameter.
//! \param ui8Val is the value to write. The value must be defined in the lower
//! half of the 8 bits of the parameter.
//!
//! \return None
//!
//! \sa ADI8SetValBit(), ADI16SetValBit
//
//*****************************************************************************
__STATIC_INLINE void
ADI4SetValBit(uint32_t ui32Base, uint32_t ui32Reg, bool bWriteHigh,
              uint8_t ui8Mask, uint8_t ui8Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);
    ASSERT(!(ui8Val & 0xF0));
    ASSERT(!(ui8Mask & 0xF0));

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_MASK4B + (ui32Reg << 1) + (bWriteHigh ? 1 : 0);

    // Set the selected bits.
    HWREGB(ui32Base + ui32RegOffset) = (ui8Mask << 4) | ui8Val;
}

//*****************************************************************************
//
//! \brief Set a value on any bits inside an 8 bit register in the ADI slave.
//!
//! This function allows byte (8 bit) access to the ADI slave registers.
//!
//! Use this function to write any value in the range 0-7 bits aligned on a
//! byte boundary. Fx. for writing the value 0b101 to bits 1 and 3 the
//! \c ui16Val = 0x0A and the \c ui16Mask = 0x0E. Bits 0 and 5-7 will not be affected
//! by the operation, as the corresponding bits are not set in the
//! \c ui16Mask.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is the base address of the ADI port.
//! \param ui32Reg is the Least Significant Register in the ADI slave that
//! will be affected by the write operation.
//! \param ui16Mask is the mask defining which of the 8 bit that should be
//! overwritten. The mask must be defined in the lower half of the 16 bits.
//! \param ui16Val is the value to write. The value must be defined in the lower
//! half of the 16 bits.
//!
//! \return None
//!
//! \sa ADI4SetValBit(), ADI16SetValBit()
//
//*****************************************************************************
__STATIC_INLINE void
ADI8SetValBit(uint32_t ui32Base, uint32_t ui32Reg, uint16_t ui16Mask,
              uint16_t ui16Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);
    ASSERT(!(ui16Val & 0xFF00));
    ASSERT(!(ui16Mask & 0xFF00));

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_MASK8B + (ui32Reg << 1);

    // Set the selected bits.
    HWREGH(ui32Base + ui32RegOffset) = (ui16Mask << 8) | ui16Val;
}

//*****************************************************************************
//
//! \brief Set a value on any bits inside an 2 x 8 bit register aligned on a
//! half-word (byte) boundary in the ADI slave.
//!
//! This function allows 2 byte (16 bit) access to the ADI slave registers.
//!
//! Use this function to write any value in the range 0-15 bits aligned on a
//! half-word (byte) boundary. Fx. for writing the value 0b101 to bits 1 and 3 the
//! \c ui32Val = 0x000A and the \c ui32Mask = 0x000E. Bits 0 and 5-15 will not
//! be affected by the operation, as the corresponding bits are not set
//! in the \c ui32Mask.
//!
//! \note AUX_ADI4_BASE : Both the AUX module and the clock for the AUX SMPH module must be
//! enabled before calling this function.
//!
//! \param ui32Base is the base address of the ADI port.
//! \param ui32Reg is the Least Significant Register in the ADI slave that
//! will be affected by the write operation.
//! \param ui32Mask is the mask defining which of the 16 bit that should be
//! overwritten. The mask must be defined in the lower half of the 32 bits.
//! \param ui32Val is the value to write. The value must be defined in the lower
//! half of the 32 bits.
//!
//! \return None
//!
//! \sa ADI4SetValBit(), ADI8SetValBit()
//
//*****************************************************************************
__STATIC_INLINE void
ADI16SetValBit(uint32_t ui32Base, uint32_t ui32Reg, uint32_t ui32Mask,
               uint32_t ui32Val)
{
    uint32_t ui32RegOffset;

    // Check the arguments.
    ASSERT(ADIBaseValid(ui32Base));
    ASSERT(ui32Reg < ADI_SLAVE_REGS);
    ASSERT(!(ui32Val & 0xFFFF0000));
    ASSERT(!(ui32Mask & 0xFFFF0000));

    // Get the correct address of the first register used for setting bits
    // in the ADI slave.
    ui32RegOffset = ADI_O_MASK16B + ((ui32Reg << 1) & 0xFC);

    // Set the selected bits.
    HWREG(ui32Base + ui32RegOffset) = (ui32Mask << 16) | ui32Val;
}

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __ADI_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
