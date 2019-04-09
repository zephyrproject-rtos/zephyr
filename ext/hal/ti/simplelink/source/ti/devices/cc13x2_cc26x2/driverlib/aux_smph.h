/******************************************************************************
*  Filename:       aux_smph.h
*  Revised:        2016-10-06 17:21:09 +0200 (Thu, 06 Oct 2016)
*  Revision:       47343
*
*  Description:    Defines and prototypes for the AUX Semaphore
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
//! \addtogroup aux_group
//! @{
//! \addtogroup auxsmph_api
//! @{
//
//*****************************************************************************

#ifndef __AUX_SMPH_H__
#define __AUX_SMPH_H__

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
#include "../inc/hw_aux_smph.h"
#include "../inc/hw_memmap.h"
#include "debug.h"

//*****************************************************************************
//
// General constants and defines
//
//*****************************************************************************
#define AUX_SMPH_FREE     0x00000001 // MCU Semaphore has not been claimed
#define AUX_SMPH_CLAIMED  0x00000000 // MCU Semaphore has been claimed

//*****************************************************************************
//
// Values that can be passed to AUXSMPHAcquire and AUXSMPHRelease
// as the ui32Semaphore parameter.
//
//*****************************************************************************
#define AUX_SMPH_0                0 // AUX Semaphore  0
#define AUX_SMPH_1                1 // AUX Semaphore  1
#define AUX_SMPH_2                2 // AUX Semaphore  2
#define AUX_SMPH_3                3 // AUX Semaphore  3
#define AUX_SMPH_4                4 // AUX Semaphore  4
#define AUX_SMPH_5                5 // AUX Semaphore  5
#define AUX_SMPH_6                6 // AUX Semaphore  6
#define AUX_SMPH_7                7 // AUX Semaphore  7

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Acquire an AUX semaphore.
//!
//! This function acquires the given AUX semaphore, blocking the call until
//! the semaphore is available.
//!
//! \note The semaphore can also be acquired by the dedicated Sensor Controller.
//! The System CPU master can thus be competing for the shared resource, i.e.
//! the specified semaphore.
//!
//! \param ui32Semaphore is the semaphore number.
//! - \ref AUX_SMPH_0
//! - \ref AUX_SMPH_1
//! - \ref AUX_SMPH_2
//! - \ref AUX_SMPH_3
//! - \ref AUX_SMPH_4
//! - \ref AUX_SMPH_5
//! - \ref AUX_SMPH_6
//! - \ref AUX_SMPH_7
//!
//! \return None
//!
//! \sa AUXSMPHTryAcquire(), AUXSMPHRelease()
//
//*****************************************************************************
__STATIC_INLINE void
AUXSMPHAcquire(uint32_t ui32Semaphore)
{
    // Check the arguments.
    ASSERT((ui32Semaphore == AUX_SMPH_0) ||
           (ui32Semaphore == AUX_SMPH_1) ||
           (ui32Semaphore == AUX_SMPH_2) ||
           (ui32Semaphore == AUX_SMPH_3) ||
           (ui32Semaphore == AUX_SMPH_4) ||
           (ui32Semaphore == AUX_SMPH_5) ||
           (ui32Semaphore == AUX_SMPH_6) ||
           (ui32Semaphore == AUX_SMPH_7));

    // Wait for semaphore to be released such that it can be claimed
    // Semaphore register reads 1 when lock was acquired otherwise 0
    // (i.e. AUX_SMPH_CLAIMED).
    while(HWREG(AUX_SMPH_BASE + AUX_SMPH_O_SMPH0 + 4 * ui32Semaphore) ==
            AUX_SMPH_CLAIMED)
    {
    }
}

//*****************************************************************************
//
//! \brief Try to acquire an AUX semaphore.
//!
//! This function tries to acquire the given AUX semaphore, if the semaphore
//! could not be claimed the function returns false.
//!
//! \note The semaphore can also be acquired by the dedicated Sensor Controller.
//! The System CPU master can thus be competing for the shared resource, i.e.
//! the specified semaphore.
//!
//! \param ui32Semaphore is the semaphore number.
//! - \ref AUX_SMPH_0
//! - \ref AUX_SMPH_1
//! - \ref AUX_SMPH_2
//! - \ref AUX_SMPH_3
//! - \ref AUX_SMPH_4
//! - \ref AUX_SMPH_5
//! - \ref AUX_SMPH_6
//! - \ref AUX_SMPH_7
//!
//! \return Returns true if semaphore was acquired, false otherwise
//!
//! \sa AUXSMPHAcquire(), AUXSMPHRelease()
//
//*****************************************************************************
__STATIC_INLINE bool
AUXSMPHTryAcquire(uint32_t ui32Semaphore)
{
    uint32_t ui32SemaReg;

    // Check the arguments.
    ASSERT((ui32Semaphore == AUX_SMPH_0) ||
           (ui32Semaphore == AUX_SMPH_1) ||
           (ui32Semaphore == AUX_SMPH_2) ||
           (ui32Semaphore == AUX_SMPH_3) ||
           (ui32Semaphore == AUX_SMPH_4) ||
           (ui32Semaphore == AUX_SMPH_5) ||
           (ui32Semaphore == AUX_SMPH_6) ||
           (ui32Semaphore == AUX_SMPH_7));

    // AUX Semaphore register reads 1 if lock was acquired
    // (i.e. SMPH_FREE when read) but subsequent reads will read 0.
    ui32SemaReg = HWREG(AUX_SMPH_BASE + AUX_SMPH_O_SMPH0 + 4 * ui32Semaphore);

    return (ui32SemaReg == AUX_SMPH_FREE);
}

//*****************************************************************************
//
//! \brief Release an AUX semaphore by System CPU master.
//!
//! This function releases the given AUX semaphore.
//!
//! \note It is up to the application to provide the convention for clearing
//! semaphore.
//!
//! \note The semaphore can also be acquired by the dedicated Sensor Controller.
//! The System CPU master can thus be competing for the shared resource, i.e.
//! the specified semaphore.
//!
//! \param ui32Semaphore is the semaphore number.
//! - \ref AUX_SMPH_0
//! - \ref AUX_SMPH_1
//! - \ref AUX_SMPH_2
//! - \ref AUX_SMPH_3
//! - \ref AUX_SMPH_4
//! - \ref AUX_SMPH_5
//! - \ref AUX_SMPH_6
//! - \ref AUX_SMPH_7
//!
//! \return None
//!
//! \sa AUXSMPHAcquire(), AUXSMPHTryAcquire()
//
//*****************************************************************************
__STATIC_INLINE void
AUXSMPHRelease(uint32_t ui32Semaphore)
{
    // Check the arguments.
    ASSERT((ui32Semaphore == AUX_SMPH_0) ||
           (ui32Semaphore == AUX_SMPH_1) ||
           (ui32Semaphore == AUX_SMPH_2) ||
           (ui32Semaphore == AUX_SMPH_3) ||
           (ui32Semaphore == AUX_SMPH_4) ||
           (ui32Semaphore == AUX_SMPH_5) ||
           (ui32Semaphore == AUX_SMPH_6) ||
           (ui32Semaphore == AUX_SMPH_7));

    // No check before release. It is up to the application to provide the
    // conventions for who and when a semaphore can be released.
    HWREG(AUX_SMPH_BASE + AUX_SMPH_O_SMPH0 + 4 * ui32Semaphore) =
        AUX_SMPH_FREE;
}

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __AUX_SMPH_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
