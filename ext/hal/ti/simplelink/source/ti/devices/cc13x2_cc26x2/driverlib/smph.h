/******************************************************************************
*  Filename:       smph.h
*  Revised:        2016-10-06 17:21:09 +0200 (Thu, 06 Oct 2016)
*  Revision:       47343
*
*  Description:    Defines and prototypes for the MCU Semaphore.
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
//! \addtogroup mcusemaphore_api
//! @{
//
//*****************************************************************************

#ifndef __SMPH_H__
#define __SMPH_H__

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
#include "../inc/hw_smph.h"
#include "../inc/hw_memmap.h"
#include "debug.h"

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
    #define SMPHAcquire                     NOROM_SMPHAcquire
#endif

//*****************************************************************************
//
// General constants and defines
//
//*****************************************************************************
#define SMPH_FREE     0x00000001 // MCU Semaphore has not been claimed
#define SMPH_CLAIMED  0x00000000 // MCU Semaphore has been claimed

//*****************************************************************************
//
// Values that can be passed to SMPHAcquire, SMPHTryAcquire and SMPHRelease
// as the ui32Semaphore parameter.
//
//*****************************************************************************
#define SMPH_0    0 // MCU Semaphore  0
#define SMPH_1    1 // MCU Semaphore  1
#define SMPH_2    2 // MCU Semaphore  2
#define SMPH_3    3 // MCU Semaphore  3
#define SMPH_4    4 // MCU Semaphore  4
#define SMPH_5    5 // MCU Semaphore  5
#define SMPH_6    6 // MCU Semaphore  6
#define SMPH_7    7 // MCU Semaphore  7
#define SMPH_8    8 // MCU Semaphore  8
#define SMPH_9    9 // MCU Semaphore  9
#define SMPH_10  10 // MCU Semaphore 10
#define SMPH_11  11 // MCU Semaphore 11
#define SMPH_12  12 // MCU Semaphore 12
#define SMPH_13  13 // MCU Semaphore 13
#define SMPH_14  14 // MCU Semaphore 14
#define SMPH_15  15 // MCU Semaphore 15
#define SMPH_16  16 // MCU Semaphore 16
#define SMPH_17  17 // MCU Semaphore 17
#define SMPH_18  18 // MCU Semaphore 18
#define SMPH_19  19 // MCU Semaphore 19
#define SMPH_20  20 // MCU Semaphore 20
#define SMPH_21  21 // MCU Semaphore 21
#define SMPH_22  22 // MCU Semaphore 22
#define SMPH_23  23 // MCU Semaphore 23
#define SMPH_24  24 // MCU Semaphore 24
#define SMPH_25  25 // MCU Semaphore 25
#define SMPH_26  26 // MCU Semaphore 26
#define SMPH_27  27 // MCU Semaphore 27
#define SMPH_28  28 // MCU Semaphore 28
#define SMPH_29  29 // MCU Semaphore 29
#define SMPH_30  30 // MCU Semaphore 30
#define SMPH_31  31 // MCU Semaphore 31

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Acquire a semaphore.
//!
//! This function acquires the given semaphore, blocking the call until
//! the semaphore is available.
//!
//! \param ui32Semaphore is the semaphore number.
//! - \ref SMPH_0
//! - \ref SMPH_1
//! - ...
//! - \ref SMPH_31
//!
//! \return None
//
//*****************************************************************************
extern void SMPHAcquire(uint32_t ui32Semaphore);

//*****************************************************************************
//
//! \brief Try to Acquire a semaphore.
//!
//! This function tries to acquire the given semaphore, if the semaphore
//! could not be claimed the function returns false.
//!
//! \param ui32Semaphore is the semaphore number.
//! - \ref SMPH_0
//! - \ref SMPH_1
//! - ...
//! - \ref SMPH_31
//!
//! \return Returns if a semaphore was acquired
//! - \c true  : Semaphore acquired.
//! - \c false : Semaphore \b not acquired.
//
//*****************************************************************************
__STATIC_INLINE bool
SMPHTryAcquire(uint32_t ui32Semaphore)
{
    uint32_t ui32SemaReg;

    // Check the arguments.
    ASSERT((ui32Semaphore == SMPH_0) ||
           (ui32Semaphore == SMPH_1) ||
           (ui32Semaphore == SMPH_2) ||
           (ui32Semaphore == SMPH_3) ||
           (ui32Semaphore == SMPH_4) ||
           (ui32Semaphore == SMPH_5) ||
           (ui32Semaphore == SMPH_6) ||
           (ui32Semaphore == SMPH_7) ||
           (ui32Semaphore == SMPH_8) ||
           (ui32Semaphore == SMPH_9) ||
           (ui32Semaphore == SMPH_10) ||
           (ui32Semaphore == SMPH_11) ||
           (ui32Semaphore == SMPH_12) ||
           (ui32Semaphore == SMPH_13) ||
           (ui32Semaphore == SMPH_14) ||
           (ui32Semaphore == SMPH_15) ||
           (ui32Semaphore == SMPH_16) ||
           (ui32Semaphore == SMPH_17) ||
           (ui32Semaphore == SMPH_18) ||
           (ui32Semaphore == SMPH_19) ||
           (ui32Semaphore == SMPH_20) ||
           (ui32Semaphore == SMPH_21) ||
           (ui32Semaphore == SMPH_22) ||
           (ui32Semaphore == SMPH_23) ||
           (ui32Semaphore == SMPH_24) ||
           (ui32Semaphore == SMPH_25) ||
           (ui32Semaphore == SMPH_26) ||
           (ui32Semaphore == SMPH_27) ||
           (ui32Semaphore == SMPH_28) ||
           (ui32Semaphore == SMPH_29) ||
           (ui32Semaphore == SMPH_30) ||
           (ui32Semaphore == SMPH_31));

    // Semaphore register reads 1 if lock was acquired
    // (i.e. SMPH_FREE).
    ui32SemaReg = HWREG(SMPH_BASE + SMPH_O_SMPH0 + 4 * ui32Semaphore);

    return (ui32SemaReg == SMPH_FREE);
}

//*****************************************************************************
//
//! \brief Release a semaphore.
//!
//! This function releases the given semaphore.
//!
//! \note It is up to the application to provide the convention for clearing
//! semaphore.
//!
//! \param ui32Semaphore is the semaphore number.
//! - \ref SMPH_0
//! - \ref SMPH_1
//! - ...
//! - \ref SMPH_31
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SMPHRelease(uint32_t ui32Semaphore)
{
    // Check the arguments.
    ASSERT((ui32Semaphore == SMPH_0) ||
           (ui32Semaphore == SMPH_1) ||
           (ui32Semaphore == SMPH_2) ||
           (ui32Semaphore == SMPH_3) ||
           (ui32Semaphore == SMPH_4) ||
           (ui32Semaphore == SMPH_5) ||
           (ui32Semaphore == SMPH_6) ||
           (ui32Semaphore == SMPH_7) ||
           (ui32Semaphore == SMPH_8) ||
           (ui32Semaphore == SMPH_9) ||
           (ui32Semaphore == SMPH_10) ||
           (ui32Semaphore == SMPH_11) ||
           (ui32Semaphore == SMPH_12) ||
           (ui32Semaphore == SMPH_13) ||
           (ui32Semaphore == SMPH_14) ||
           (ui32Semaphore == SMPH_15) ||
           (ui32Semaphore == SMPH_16) ||
           (ui32Semaphore == SMPH_17) ||
           (ui32Semaphore == SMPH_18) ||
           (ui32Semaphore == SMPH_19) ||
           (ui32Semaphore == SMPH_20) ||
           (ui32Semaphore == SMPH_21) ||
           (ui32Semaphore == SMPH_22) ||
           (ui32Semaphore == SMPH_23) ||
           (ui32Semaphore == SMPH_24) ||
           (ui32Semaphore == SMPH_25) ||
           (ui32Semaphore == SMPH_26) ||
           (ui32Semaphore == SMPH_27) ||
           (ui32Semaphore == SMPH_28) ||
           (ui32Semaphore == SMPH_29) ||
           (ui32Semaphore == SMPH_30) ||
           (ui32Semaphore == SMPH_31));

    // No check before release, it is up to the application to provide the
    // conventions for who and when a semaphore can be released.
    HWREG(SMPH_BASE + SMPH_O_SMPH0 + 4 * ui32Semaphore) = SMPH_FREE;
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_SMPHAcquire
        #undef  SMPHAcquire
        #define SMPHAcquire                     ROM_SMPHAcquire
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

#endif //  __SMPH_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
