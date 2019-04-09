/******************************************************************************
*  Filename:       trng.h
*  Revised:        2017-05-23 12:08:52 +0200 (Tue, 23 May 2017)
*  Revision:       49048
*
*  Description:    Defines and prototypes for the true random number gen.
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
//! \addtogroup peripheral_group
//! @{
//! \addtogroup trng_api
//! @{
//
//*****************************************************************************

#ifndef __TRNG_H__
#define __TRNG_H__

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
#include "../inc/hw_trng.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "debug.h"
#include "interrupt.h"
#include "cpu.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define TRNGConfigure                   NOROM_TRNGConfigure
    #define TRNGNumberGet                   NOROM_TRNGNumberGet
#endif

//*****************************************************************************
//
//
//
//*****************************************************************************
#define TRNG_NUMBER_READY       0x00000001  //
#define TRNG_FRO_SHUTDOWN       0x00000002  //
#define TRNG_NEED_CLOCK         0x80000000  //

#define TRNG_HI_WORD            0x00000001
#define TRNG_LOW_WORD           0x00000002

//*****************************************************************************
//
// API Function and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Configure the true random number generator.
//!
//! Use this function to set the minimum and maximum number of samples required
//! in each generation of a new random number.
//!
//! \param ui32MinSamplesPerCycle is the minimum number of samples per each
//! generated random number. Constraints:
//! - Value must be bigger than or equal to 2^6 and less than 2^14.
//! - The 6 LSBs of the argument are truncated.
//! - If the value is zero, the number of samples is fixed to the value determined
//!   by ui32MaxSamplesPerCycle. To ensure same entropy in all generated random
//!   numbers the value 0 should be used.
//! \param ui32MaxSamplesPerCycle is the maximum number of samples per each
//! generated random number. Constraints:
//! - Value must be between 2^8 and 2^24 (both included).
//! - The 8 LSBs of the argument are truncated.
//! - Value 0 and 2^24 both give the highest possible value.
//! \param ui32ClocksPerSample is the number of clock cycles for each time
//! a new sample is generated from the FROs.
//! - 0  : Every sample.
//! - 1  : Every second sample.
//! - ...
//! - 15 : Every 16. sample.
//!
//! \return None
//
//*****************************************************************************
extern void TRNGConfigure(uint32_t ui32MinSamplesPerCycle,
                          uint32_t ui32MaxSamplesPerCycle,
                          uint32_t ui32ClocksPerSample);

//*****************************************************************************
//
//! \brief Enable the TRNG.
//!
//! Enable the TRNG to start preparing a random number.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TRNGEnable(void)
{
    // Enable the TRNG.
    HWREGBITW(TRNG_BASE + TRNG_O_CTL, TRNG_CTL_TRNG_EN_BITN) = 1;
}

//*****************************************************************************
//
//! \brief Disable the TRNG module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TRNGDisable(void)
{
    // Enable the TRNG
    HWREGBITW(TRNG_BASE + TRNG_O_CTL, TRNG_CTL_TRNG_EN_BITN) = 0;
}

//*****************************************************************************
//
//! \brief Get a random number from the generator.
//!
//! Use this function to get either the high or low part of the 64 bit
//! generated number.
//!
//! \note Data from this register is only valid if the TRNG has produced a
//! number. Use \ref TRNGStatusGet() to poll the for status. After calling this
//! function a new random number will be generated.
//!
//! \param ui32Word determines if whether to return the high or low 32 bits.
//! - \ref TRNG_HI_WORD
//! - \ref TRNG_LOW_WORD
//!
//! \return Return either the high or low part of the 64 bit generated random
//! number.
//
//*****************************************************************************
extern uint32_t TRNGNumberGet(uint32_t ui32Word);

//*****************************************************************************
//
//! \brief Get the status of the TRNG.
//!
//! Use this function to retrieve the status of the TRNG.
//!
//! \return Returns the current status of the TRNG module.
//! The returned status is a bitwise OR'ed combination of:
//! - \ref TRNG_NUMBER_READY
//! - \ref TRNG_FRO_SHUTDOWN
//! - \ref TRNG_NEED_CLOCK
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TRNGStatusGet(void)
{
    // Return the status.
    return (HWREG(TRNG_BASE + TRNG_O_IRQFLAGSTAT));
}

//*****************************************************************************
//
//! \brief Reset the TRNG.
//!
//! Use this function to reset the TRNG module. Reset will be low for
//! approximately 5 clock cycles.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TRNGReset(void)
{
    // Reset the TRNG.
    HWREG(TRNG_BASE + TRNG_O_SWRESET) = 1;
}

//*****************************************************************************
//
//! \brief Enables individual TRNG interrupt sources.
//!
//! This function enables the indicated TRNG interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \param ui32IntFlags is the bit mask of the interrupt sources to be enabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref TRNG_NUMBER_READY
//! - \ref TRNG_FRO_SHUTDOWN
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TRNGIntEnable(uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT((ui32IntFlags & TRNG_NUMBER_READY) ||
           (ui32IntFlags & TRNG_FRO_SHUTDOWN));

    // Enable the specified interrupts.
    HWREG(TRNG_BASE + TRNG_O_IRQFLAGMASK) |= ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Disables individual TRNG interrupt sources.
//!
//! This function disables the indicated TRNG interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \param ui32IntFlags is the bit mask of the interrupt sources to be disabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref TRNG_NUMBER_READY
//! - \ref TRNG_FRO_SHUTDOWN
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TRNGIntDisable(uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT((ui32IntFlags & TRNG_NUMBER_READY) ||
           (ui32IntFlags & TRNG_FRO_SHUTDOWN));

    // Disable the specified interrupts.
    HWREG(TRNG_BASE + TRNG_O_IRQFLAGMASK) &= ~ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Gets the current interrupt status of the TRNG module.
//!
//! This function returns the interrupt status for the specified TRNG. Either
//! the raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \param bMasked selects either raw or masked interrupt status.
//! - \c true  : Masked interrupt.
//! - \c false : Raw interrupt.
//!
//! \return Returns the current interrupt status, enumerated as:
//! - \ref TRNG_NUMBER_READY
//! - \ref TRNG_FRO_SHUTDOWN
//
//*****************************************************************************
__STATIC_INLINE uint32_t
TRNGIntStatus(bool bMasked)
{
    uint32_t ui32Mask;

    // Return either the interrupt status or the raw interrupt status as
    // requested.
    if(bMasked)
    {
        ui32Mask = HWREG(TRNG_BASE + TRNG_O_IRQFLAGMASK);
        return(ui32Mask & HWREG(TRNG_BASE + TRNG_O_IRQFLAGSTAT));
    }
    else
    {
        return(HWREG(TRNG_BASE + TRNG_O_IRQFLAGSTAT) & 0x00000003);
    }
}

//*****************************************************************************
//
//! \brief Clears TRNG interrupt sources.
//!
//! The specified TRNG interrupt sources are cleared, so that they no longer
//! assert. This function must be called in the interrupt handler to keep the
//! interrupt from being recognized again immediately upon exit.
//!
//! \note Due to write buffers and synchronizers in the system it may take several
//! clock cycles from a register write clearing an event in a module and until the
//! event is actually cleared in the NVIC of the system CPU. It is recommended to
//! clear the event source early in the interrupt service routine (ISR) to allow
//! the event clear to propagate to the NVIC before returning from the ISR.
//! At the same time, an early event clear allows new events of the same type to be
//! pended instead of ignored if the event is cleared later in the ISR.
//! It is the responsibility of the programmer to make sure that enough time has passed
//! before returning from the ISR to avoid false re-triggering of the cleared event.
//! A simple, although not necessarily optimal, way of clearing an event before
//! returning from the ISR is:
//! -# Write to clear event (interrupt source). (buffered write)
//! -# Dummy read from the event source module. (making sure the write has propagated)
//! -# Wait two system CPU clock cycles (user code or two NOPs). (allowing cleared event to propagate through any synchronizers)
//!
//! \param ui32IntFlags is a bit mask of the interrupt sources to be cleared.
//! The parameter is the bitwise OR of any of the following:
//! - \ref TRNG_NUMBER_READY
//! - \ref TRNG_FRO_SHUTDOWN
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
TRNGIntClear(uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT((ui32IntFlags & TRNG_NUMBER_READY) ||
           (ui32IntFlags & TRNG_FRO_SHUTDOWN));

    // Clear the requested interrupt sources.
    HWREG(TRNG_BASE + TRNG_O_IRQFLAGCLR) = ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Registers an interrupt handler for a TRNG interrupt in the dynamic interrupt table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific TRNG interrupts must be enabled via \ref TRNGIntEnable(). It is the interrupt
//! handler's responsibility to clear the interrupt source.
//!
//! \param pfnHandler is a pointer to the function to be called when the
//! TRNG interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void
TRNGIntRegister(void (*pfnHandler)(void))
{
    // Register the interrupt handler.
    IntRegister(INT_TRNG_IRQ, pfnHandler);

    // Enable the TRNG interrupt.
    IntEnable(INT_TRNG_IRQ);
}

//*****************************************************************************
//
//! \brief Unregisters an interrupt handler for a TRNG interrupt in the dynamic interrupt table.
//!
//! This function does the actual unregistering of the interrupt handler. It
//! clears the handler to be called when a Crypto interrupt occurs. This
//! function also masks off the interrupt in the interrupt controller so that
//! the interrupt handler no longer is called.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void
TRNGIntUnregister(void)
{
    // Disable the interrupt.
    IntDisable(INT_TRNG_IRQ);

    // Unregister the interrupt handler.
    IntUnregister(INT_TRNG_IRQ);
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_TRNGConfigure
        #undef  TRNGConfigure
        #define TRNGConfigure                   ROM_TRNGConfigure
    #endif
    #ifdef ROM_TRNGNumberGet
        #undef  TRNGNumberGet
        #define TRNGNumberGet                   ROM_TRNGNumberGet
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // __TRNG_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
