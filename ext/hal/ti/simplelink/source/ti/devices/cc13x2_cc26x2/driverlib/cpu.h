/******************************************************************************
*  Filename:       cpu.h
*  Revised:        2018-06-04 16:10:13 +0200 (Mon, 04 Jun 2018)
*  Revision:       52111
*
*  Description:    Defines and prototypes for the CPU instruction wrapper
*                  functions.
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
//! \addtogroup system_cpu_group
//! @{
//! \addtogroup cpu_api
//! @{
//
//*****************************************************************************

#ifndef __CPU_H__
#define __CPU_H__

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
#include "../inc/hw_memmap.h"
#include "../inc/hw_cpu_scs.h"

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
    #define CPUcpsid                        NOROM_CPUcpsid
    #define CPUprimask                      NOROM_CPUprimask
    #define CPUcpsie                        NOROM_CPUcpsie
    #define CPUbasepriGet                   NOROM_CPUbasepriGet
    #define CPUdelay                        NOROM_CPUdelay
#endif

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Disable all external interrupts.
//!
//! Use this function to disable all system interrupts. This function is
//! implemented as a wrapper function for the CPSID instruction.
//!
//! \return Returns the state of \b PRIMASK on entry
//
//*****************************************************************************
extern uint32_t CPUcpsid(void);

//*****************************************************************************
//
//! \brief Get the current interrupt state.
//!
//! Use this function to retrieve the current state of the interrupts. This
//! function is implemented as a wrapper function returning the state of
//! PRIMASK.
//!
//! \return Returns the state of the \b PRIMASK (indicating whether interrupts
//! are enabled or disabled).
//
//*****************************************************************************
extern uint32_t CPUprimask(void);

//*****************************************************************************
//
//! \brief Enable all external interrupts.
//!
//! Use this function to enable all system interrupts. This function is
//! implemented as a wrapper function for the CPSIE instruction.
//!
//! \return Returns the state of \b PRIMASK on entry.
//
//*****************************************************************************
extern uint32_t CPUcpsie(void);

//*****************************************************************************
//
//! \brief Get the interrupt priority disable level.
//!
//! Use this function to get the level of priority that will disable
//! interrupts with a lower priority level.
//!
//! \return Returns the value of the \b BASEPRI register.
//
//*****************************************************************************
extern uint32_t CPUbasepriGet(void);

//*****************************************************************************
//
//! \brief Provide a small non-zero delay using a simple loop counter.
//!
//! This function provides means for generating a constant length delay. It
//! is written in assembly to keep the delay consistent across tool chains,
//! avoiding the need to tune the delay based on the tool chain in use.
//!
//! \note It is not recommended using this function for long delays.
//!
//! Notice that interrupts can affect the delay if not manually disabled in advance.
//!
//! The delay depends on where code resides and the path for code fetching:
//! - Code in flash, cache enabled, prefetch enabled  : 4 cycles per loop (Default)
//! - Code in flash, cache enabled, prefetch disabled : 5 cycles per loop
//! - Code in flash, cache disabled                   : 7 cycles per loop
//! - Code in SRAM                                    : 6 cycles per loop
//! - Code in GPRAM                                   : 3 cycles per loop
//!
//! \note If using an RTOS, consider using RTOS provided delay functions because
//! these will not block task scheduling and will potentially save power.
//!
//! Calculate delay count based on the wanted delay in microseconds (us):
//! - ui32Count = [delay in us] * [CPU clock in MHz] / [cycles per loop]
//!
//! Example: 250 us delay with code in flash and with cache and prefetch enabled:
//! - ui32Count = 250 * 48 / 4 = 3000
//!
//! \param ui32Count is the number of delay loop iterations to perform. Number must be greater than zero.
//!
//! \return None
//
//*****************************************************************************
extern void CPUdelay(uint32_t ui32Count);

//*****************************************************************************
//
//! \brief Wait for interrupt.
//!
//! Use this function to let the System CPU wait for the next interrupt. This
//! function is implemented as a wrapper function for the WFI instruction.
//!
//! \return None
//
//*****************************************************************************
#if defined(DOXYGEN)
__STATIC_INLINE void
CPUwfi(void)
{
    // This function is written in assembly. See cpu.h for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
__STATIC_INLINE void
CPUwfi(void)
{
    // Wait for the next interrupt.
    __asm("    wfi\n");
}
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm __STATIC_INLINE void
CPUwfi(void)
{
    // Wait for the next interrupt.
    wfi;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
__STATIC_INLINE void
CPUwfi(void)
{
    // Wait for the next interrupt.
    __asm("    wfi\n");
}
#else
__STATIC_INLINE void __attribute__((always_inline))
CPUwfi(void)
{
    // Wait for the next interrupt.
    __asm volatile ("    wfi\n");
}
#endif

//*****************************************************************************
//
//! \brief Wait for event.
//!
//! Use this function to let the System CPU wait for the next event. This
//! function is implemented as a wrapper function for the WFE instruction.
//!
//! \return None
//
//*****************************************************************************
#if defined(DOXYGEN)
__STATIC_INLINE void
CPUwfe(void)
{
    // This function is written in assembly. See cpu.h for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
__STATIC_INLINE void
CPUwfe(void)
{
    // Wait for the next event.
    __asm("    wfe\n");
}
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm __STATIC_INLINE void
CPUwfe(void)
{
    // Wait for the next event.
    wfe;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
__STATIC_INLINE void
CPUwfe(void)
{
    // Wait for the next event.
    __asm("    wfe\n");
}
#else
__STATIC_INLINE void __attribute__((always_inline))
CPUwfe(void)
{
    // Wait for the next event.
    __asm volatile ("    wfe\n");
}
#endif

//*****************************************************************************
//
//! \brief Send event.
//!
//! Use this function to let the System CPU send an event. This function is
//! implemented as a wrapper function for the SEV instruction.
//!
//! \return None
//
//*****************************************************************************
#if defined(DOXYGEN)
__STATIC_INLINE void
CPUsev(void)
{
    // This function is written in assembly. See cpu.h for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
__STATIC_INLINE void
CPUsev(void)
{
    // Send event.
    __asm("    sev\n");
}
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm __STATIC_INLINE void
CPUsev(void)
{
    // Send event.
    sev;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
__STATIC_INLINE void
CPUsev(void)
{
    // Send event.
    __asm("    sev\n");
}
#else
__STATIC_INLINE void __attribute__((always_inline))
CPUsev(void)
{
    // Send event.
    __asm volatile ("    sev\n");
}
#endif


//*****************************************************************************
//
//! \brief Update the interrupt priority disable level.
//!
//! Use this function to change the level of priority that will disable
//! interrupts with a lower priority level.
//!
//! \param ui32NewBasepri is the new basis priority level to set.
//!
//! \return None
//
//*****************************************************************************
#if defined(DOXYGEN)
__STATIC_INLINE void
CPUbasepriSet(uint32_t ui32NewBasepri)
{
    // This function is written in assembly. See cpu.h for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
__STATIC_INLINE void
CPUbasepriSet(uint32_t ui32NewBasepri)
{
    // Set the BASEPRI register.
    __asm("    msr     BASEPRI, r0\n");
}
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm __STATIC_INLINE void
CPUbasepriSet(uint32_t ui32NewBasepri)
{
    // Set the BASEPRI register.
    msr     BASEPRI, r0;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
__STATIC_INLINE void
CPUbasepriSet(uint32_t ui32NewBasepri)
{
    // Set the BASEPRI register.
    __asm("    msr     BASEPRI, r0\n");
}
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
__STATIC_INLINE void __attribute__ ((naked))
CPUbasepriSet(uint32_t ui32NewBasepri)
{
    // Set the BASEPRI register.
    __asm volatile ("    msr     BASEPRI, %0\n"
                    "    bx      lr\n"
                    : /* No output */
                    : "r" (ui32NewBasepri)
                   );
}
#pragma GCC diagnostic pop
#endif

//*****************************************************************************
//
//! \brief Disable CPU write buffering (recommended for debug purpose only).
//!
//! This function helps debugging "bus fault crashes".
//! Disables write buffer use during default memory map accesses.
//!
//! This causes all bus faults to be precise bus faults but decreases the
//! performance of the processor because the stores to memory have to complete
//! before the next instruction can be executed.
//!
//! \return None
//!
//! \sa \ref CPU_WriteBufferEnable()
//
//*****************************************************************************
__STATIC_INLINE void
CPU_WriteBufferDisable( void )
{
    HWREGBITW( CPU_SCS_BASE + CPU_SCS_O_ACTLR, CPU_SCS_ACTLR_DISDEFWBUF_BITN ) = 1;
}

//*****************************************************************************
//
//! \brief Enable CPU write buffering (default setting).
//!
//! Re-enables write buffer during default memory map accesses if
//! \ref CPU_WriteBufferDisable() has been used for bus fault debugging.
//!
//! \return None
//!
//! \sa \ref CPU_WriteBufferDisable()
//
//*****************************************************************************
__STATIC_INLINE void
CPU_WriteBufferEnable( void )
{
    HWREGBITW( CPU_SCS_BASE + CPU_SCS_O_ACTLR, CPU_SCS_ACTLR_DISDEFWBUF_BITN ) = 0;
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_CPUcpsid
        #undef  CPUcpsid
        #define CPUcpsid                        ROM_CPUcpsid
    #endif
    #ifdef ROM_CPUprimask
        #undef  CPUprimask
        #define CPUprimask                      ROM_CPUprimask
    #endif
    #ifdef ROM_CPUcpsie
        #undef  CPUcpsie
        #define CPUcpsie                        ROM_CPUcpsie
    #endif
    #ifdef ROM_CPUbasepriGet
        #undef  CPUbasepriGet
        #define CPUbasepriGet                   ROM_CPUbasepriGet
    #endif
    #ifdef ROM_CPUdelay
        #undef  CPUdelay
        #define CPUdelay                        ROM_CPUdelay
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

#endif // __CPU_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
