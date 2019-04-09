/******************************************************************************
*  Filename:       setup.h
*  Revised:        2018-10-24 11:23:04 +0200 (Wed, 24 Oct 2018)
*  Revision:       52993
*
*  Description:    Prototypes and defines for the setup API.
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
//! \addtogroup system_control_group
//! @{
//! \addtogroup setup_api
//! @{
//
//*****************************************************************************

#ifndef __SETUP_H__
#define __SETUP_H__

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

// Hardware headers
#include "../inc/hw_types.h"
// Driverlib headers
// - None needed

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
    #define SetupTrimDevice                 NOROM_SetupTrimDevice
#endif

//*****************************************************************************
//
//! \brief Performs the necessary trim of the device which is not done in ROM boot code.
//!
//! This function should only execute coming from ROM boot.
//!
//! The following is handled by this function:
//! - Checks if the driverlib variant used by the application is supported by the
//!   device. Execution is halted in case of unsupported driverlib variant.
//! - Configures VIMS cache mode based on setting in CCFG.
//! - Configures functionalities like DCDC and XOSC dependent on startup modes like
//!   cold reset, wakeup from shutdown and wakeup from from powerdown.
//! - Configures VIMS power domain control.
//! - Configures optimal wait time for flash FSM in cases where flash pump wakes up from sleep.
//!
//! \note The current implementation does not take soft reset into account. However,
//! it does no damage to execute it again. It only consumes time.
//!
//! \note This function is called by the compiler specific device startup codes
//! that are integrated in the SimpleLink SDKs for CC13xx/CC26XX devices.
//!
//! \return None
//
//*****************************************************************************
extern void SetupTrimDevice( void );

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_SetupTrimDevice
        #undef  SetupTrimDevice
        #define SetupTrimDevice                 ROM_SetupTrimDevice
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

#endif //  __SETUP_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
