/******************************************************************************
*  Filename:       vims.h
*  Revised:        2016-10-06 17:21:09 +0200 (Thu, 06 Oct 2016)
*  Revision:       47343
*
*  Description:    Defines and prototypes for the VIMS.
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
//! \addtogroup vims_api
//! @{
//
//*****************************************************************************

#ifndef __VIMS_H__
#define __VIMS_H__

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
#include "../inc/hw_vims.h"
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
    #define VIMSConfigure                   NOROM_VIMSConfigure
    #define VIMSModeSet                     NOROM_VIMSModeSet
    #define VIMSModeGet                     NOROM_VIMSModeGet
    #define VIMSModeSafeSet                 NOROM_VIMSModeSafeSet
#endif

//*****************************************************************************
//
// Values that can be passed to VIMSModeSet() as the ui32IntFlags parameter,
// and returned from VIMSModeGet().
//
//*****************************************************************************
#define VIMS_MODE_CHANGING 0x4                   // VIMS mode is changing now and VIMS_MODE
                                                 // can not be changed at moment.
#define VIMS_MODE_DISABLED (VIMS_CTL_MODE_GPRAM) // Disabled mode (GPRAM enabled).
#define VIMS_MODE_ENABLED  (VIMS_CTL_MODE_CACHE) // Enabled mode, only USERCODE is cached.
#define VIMS_MODE_OFF      (VIMS_CTL_MODE_OFF)   // VIMS Cache RAM is off

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

#ifdef DRIVERLIB_DEBUG
//*****************************************************************************
//
//! \brief Checks a VIMS base address.
//!
//! This function determines if the VIMS base address is valid.
//!
//! \param ui32Base is the base address of the VIMS.
//!
//! \return Returns \c true if the base address is valid and \c false
//! otherwise.
//
//*****************************************************************************
static bool
VIMSBaseValid(uint32_t ui32Base)
{
    return(ui32Base == VIMS_BASE);
}
#endif

//*****************************************************************************
//
//! \brief Configures the VIMS.
//!
//! This function sets general control settings of the VIMS system.
//!
//! \note The VIMS mode must be set using the \ref VIMSModeSet() call.
//!
//! \param ui32Base is the base address of the VIMS.
//! \param bRoundRobin specifies the arbitration method.
//! - \c true  : Round Robin arbitration between the two available read/write interfaces
//! (i.e. Icode/Dcode and Sysbus) is to be used.
//! - \c false : Strict arbitration will be used, where Icode/Dcode
//! is preferred over the Sysbus.
//! \param bPrefetch specifies if prefetching is to be used.
//! - \c true  : Cache is to prefetch tag data for the following address.
//! - \c false : No prefetch.
//!
//! \return None
//!
//! \sa \ref VIMSModeSet()
//
//*****************************************************************************
extern void VIMSConfigure(uint32_t ui32Base, bool bRoundRobin,
                          bool bPrefetch);

//*****************************************************************************
//
//! \brief Set the operational mode of the VIMS.
//!
//! This function sets the operational mode of the VIMS.
//!
//! Upon reset the VIMS will be in \ref VIMS_MODE_CHANGING mode.
//!   In this mode the VIMS will initialize the cache (GP) RAM (to all zeros).
//!   The GP RAM will not be operational (read/write will result in bus fault).
//!   The Cache will not be operational.
//!   Reads and writes to flash will be uncached.
//!   After a short delay (approx. 1029 clock cycles) the VIMS will
//!   automatically switch mode to \ref VIMS_MODE_DISABLED (GPRAM enabled).
//!
//! In \ref VIMS_MODE_DISABLED mode, the cache is disabled but the GP RAM is
//! accessible:
//!   The GP RAM will be accessible.
//!   The Cache will not be operational.
//!   Reads from flash will be uncached.
//!   From this mode, the VIMS may be put in \ref VIMS_MODE_ENABLED (CACHE mode).
//!
//! In \ref VIMS_MODE_ENABLED mode, the cache is enabled for \b USERCODE space.
//!   The GP RAM will not be operational (read/write will result in bus fault).
//!   The Cache will be operational for SYSCODE space.
//!   Reads from flash in USERCODE space will be uncached.
//!
//! In \ref VIMS_MODE_OFF the cache RAM is off to conserve power.
//!
//! \note The VIMS must be invalidated when switching mode.
//! This is done by setting VIMS_MODE_OFF before setting any new mode.
//! This is automatically handled in \ref VIMSModeSafeSet()
//!
//! \note It is highly recommended that the VIMS is put in disabled mode before
//! \b writing to flash, since the cache will not be updated nor invalidated
//! by flash writes. The line buffers should also be disabled when updating the
//! flash. Once \ref VIMSModeSet() is used to set the VIMS in
//! \ref VIMS_MODE_CHANGING mode, the user should check using
//! \ref VIMSModeGet() when the mode switches to \ref VIMS_MODE_DISABLED. Only when
//! the mode has changed the cache has been completely invalidated.
//!
//! \note Access from System Bus is never cached. Only access through ICODE
//! DCODE bus from the System CPU is cached.
//!
//! \param ui32Base is the base address of the VIMS.
//! \param ui32Mode is the operational mode.
//! - \ref VIMS_MODE_DISABLED (GPRAM enabled)
//! - \ref VIMS_MODE_ENABLED  (CACHE mode)
//! - \ref VIMS_MODE_OFF
//!
//! \return None
//!
//! \sa \ref VIMSModeGet() and \ref VIMSModeSafeSet()
//
//*****************************************************************************
extern void VIMSModeSet(uint32_t ui32Base, uint32_t ui32Mode);

//*****************************************************************************
//
//! \brief Get the current operational mode of the VIMS.
//!
//! This function returns the operational mode of the VIMS.
//!
//! \param ui32Base is the base address of the VIMS.
//!
//! \return Returns one of:
//! - \ref VIMS_MODE_CHANGING
//! - \ref VIMS_MODE_DISABLED (GPRAM enabled)
//! - \ref VIMS_MODE_ENABLED  (CACHE mode)
//! - \ref VIMS_MODE_OFF
//!
//! \sa \ref VIMSModeSet()
//
//*****************************************************************************
extern uint32_t VIMSModeGet(uint32_t ui32Base);

//*****************************************************************************
//
//! \brief Set the operational mode of the VIMS in a safe sequence.
//!
//! This function sets the operational mode of the VIMS in a safe sequence
//!
//! Upon reset the VIMS will be in \ref VIMS_MODE_CHANGING mode.
//!   In this mode the VIMS will initialize the cache (GP) RAM (to all zeros).
//!   The GP RAM will not be operational (read/write will result in bus fault).
//!   The Cache will not be operational (read/write to flash will be uncached).
//!   After a short delay (approx. 1029 clock cycles) the VIMS will
//!   automatically switch mode to \ref VIMS_MODE_DISABLED (GPRAM enabled).
//!
//! In \ref VIMS_MODE_DISABLED mode, the cache is disabled but the GP RAM is
//! accessible:
//!   The GP RAM will be accessible.
//!   The Cache will not be operational.
//!   Reads from flash will be uncached.
//!   From this mode, the VIMS may be put in \ref VIMS_MODE_ENABLED (CACHE mode).
//!
//! In \ref VIMS_MODE_ENABLED mode, the cache is enabled for \b USERCODE space.
//!   The GP RAM will not be operational (read/write will result in bus fault).
//!   The Cache will be operational for SYSCODE space.
//!   Reads from flash in USERCODE space will be uncached.
//!
//! In \ref VIMS_MODE_OFF the cache RAM is off to conserve power.
//!
//! \note The VIMS must be invalidated when switching mode.
//! This is done by setting VIMS_MODE_OFF before setting any new mode.
//! This is automatically handled in this function.
//!
//! \note It is highly recommended that the VIMS is put in disabled mode before
//! \b writing to flash, since the cache will not be updated nor invalidated
//! by flash writes. The line buffers should also be disabled when updating the
//! flash.
//!
//! \note Access from System Bus is never cached. Only access through ICODE
//! DCODE bus from the System CPU is cached.
//!
//! \param ui32Base is the base address of the VIMS.
//! \param ui32NewMode is the new operational mode:
//! - \ref VIMS_MODE_DISABLED (GPRAM enabled)
//! - \ref VIMS_MODE_ENABLED  (CACHE mode)
//! - \ref VIMS_MODE_OFF
//! \param blocking shall be set to TRUE if further code execution shall be
//! blocked (delayed) until mode change is completed.
//!
//! \return None
//!
//! \sa \ref VIMSModeSet() and \ref VIMSModeGet()
//
//*****************************************************************************
extern void VIMSModeSafeSet( uint32_t ui32Base    ,
                             uint32_t ui32NewMode ,
                             bool     blocking    );

//*****************************************************************************
//
//! \brief Disable VIMS linebuffers.
//!
//! Linebuffers should only be disabled when attempting to update the flash, to
//! ensure that the content of the buffers is not stale. As soon as flash is
//! updated the linebuffers should be reenabled. Failing to enable
//! will have a performance impact.
//!
//! \param ui32Base is the base address of the VIMS.
//!
//! \return None.
//
//*****************************************************************************
__STATIC_INLINE void
VIMSLineBufDisable(uint32_t ui32Base)
{
    // Disable line buffers
    HWREG(ui32Base + VIMS_O_CTL) |= VIMS_CTL_IDCODE_LB_DIS_M |
                                        VIMS_CTL_SYSBUS_LB_DIS_M;
}

//*****************************************************************************
//
//! \brief Enable VIMS linebuffers.
//!
//! Linebuffers should only be disabled when attempting to update the flash, to
//! ensure that the content of the buffers is not stale. As soon as flash is
//! updated the linebuffers should be reenabled. Failing to enable
//! will have a performance impact.
//!
//! \param ui32Base is the base address of the VIMS.
//!
//! \return None.
//
//*****************************************************************************
__STATIC_INLINE void
VIMSLineBufEnable(uint32_t ui32Base)
{
    // Enable linebuffers
    HWREG(ui32Base + VIMS_O_CTL) &= ~(VIMS_CTL_IDCODE_LB_DIS_M |
                                          VIMS_CTL_SYSBUS_LB_DIS_M);
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_VIMSConfigure
        #undef  VIMSConfigure
        #define VIMSConfigure                   ROM_VIMSConfigure
    #endif
    #ifdef ROM_VIMSModeSet
        #undef  VIMSModeSet
        #define VIMSModeSet                     ROM_VIMSModeSet
    #endif
    #ifdef ROM_VIMSModeGet
        #undef  VIMSModeGet
        #define VIMSModeGet                     ROM_VIMSModeGet
    #endif
    #ifdef ROM_VIMSModeSafeSet
        #undef  VIMSModeSafeSet
        #define VIMSModeSafeSet                 ROM_VIMSModeSafeSet
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

#endif //  __VIMS_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
