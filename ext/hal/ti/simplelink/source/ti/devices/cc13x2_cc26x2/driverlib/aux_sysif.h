/******************************************************************************
*  Filename:       aux_sysif.h
*  Revised:        2017-06-27 08:41:49 +0200 (Tue, 27 Jun 2017)
*  Revision:       49245
*
*  Description:    Defines and prototypes for the AUX System Interface
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
//! \addtogroup auxsysif_api
//! @{
//
//*****************************************************************************

#ifndef __AUX_SYSIF_H__
#define __AUX_SYSIF_H__

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
#include "../inc/hw_aux_sysif.h"
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
    #define AUXSYSIFOpModeChange            NOROM_AUXSYSIFOpModeChange
#endif


//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for AUX operational modes.
//
//*****************************************************************************
#define AUX_SYSIF_OPMODE_TARGET_PDLP (AUX_SYSIF_OPMODEREQ_REQ_PDLP)
#define AUX_SYSIF_OPMODE_TARGET_PDA  (AUX_SYSIF_OPMODEREQ_REQ_PDA)
#define AUX_SYSIF_OPMODE_TARGET_LP   (AUX_SYSIF_OPMODEREQ_REQ_LP)
#define AUX_SYSIF_OPMODE_TARGET_A    (AUX_SYSIF_OPMODEREQ_REQ_A)

//*****************************************************************************
//
//! \brief Changes the AUX operational mode to the requested target mode.
//!
//! This function controls the change of the AUX operational mode.
//! The function controls the change of the current operational mode to the
//! operational mode target by adhering to rules specified by HW.
//!
//! \param targetOpMode
//!     AUX operational mode:
//!     - \ref AUX_SYSIF_OPMODE_TARGET_PDLP (Powerdown operational mode with wakeup to lowpower mode)
//!     - \ref AUX_SYSIF_OPMODE_TARGET_PDA  (Powerdown operational mode with wakeup to active mode)
//!     - \ref AUX_SYSIF_OPMODE_TARGET_LP   (Lowpower operational mode)
//!     - \ref AUX_SYSIF_OPMODE_TARGET_A    (Active operational mode)
//!
//! \return None
//
//*****************************************************************************
extern void AUXSYSIFOpModeChange(uint32_t targetOpMode);

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_AUXSYSIFOpModeChange
        #undef  AUXSYSIFOpModeChange
        #define AUXSYSIFOpModeChange            ROM_AUXSYSIFOpModeChange
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

#endif // __AUX_SYSIF_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
