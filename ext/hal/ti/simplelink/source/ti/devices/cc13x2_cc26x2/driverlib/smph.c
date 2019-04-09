/******************************************************************************
*  Filename:       smph.c
*  Revised:        2017-04-26 18:27:45 +0200 (Wed, 26 Apr 2017)
*  Revision:       48852
*
*  Description:    Driver for the MCU Semaphore.
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

#include "smph.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  SMPHAcquire
    #define SMPHAcquire                     NOROM_SMPHAcquire
#endif

//*****************************************************************************
//
// Acquire a semaphore
//
//*****************************************************************************
void
SMPHAcquire(uint32_t ui32Semaphore)
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

    // Wait for semaphore to be release such that it can be claimed
    // Semaphore register reads 1 when lock was acquired otherwise 0
    // (i.e. SMPH_CLAIMED).
    while(HWREG(SMPH_BASE + SMPH_O_SMPH0 + 4 * ui32Semaphore) ==
            SMPH_CLAIMED)
    {
    }
}
