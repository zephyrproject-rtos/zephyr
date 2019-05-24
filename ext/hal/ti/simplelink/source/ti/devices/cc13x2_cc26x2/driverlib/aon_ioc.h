/******************************************************************************
*  Filename:       aon_ioc.h
*  Revised:        2016-10-06 17:21:09 +0200 (Thu, 06 Oct 2016)
*  Revision:       47343
*
*  Description:    Defines and prototypes for the AON IO Controller
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
//! \addtogroup aon_group
//! @{
//! \addtogroup aonioc_api
//! @{
//
//*****************************************************************************

#ifndef __AON_IOC_H__
#define __AON_IOC_H__

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
#include "../inc/hw_aon_ioc.h"
#include "debug.h"

//*****************************************************************************
//
// Defines for the drive strength
//
//*****************************************************************************
#define AONIOC_DRV_STR_1  0x00000000 // Lowest drive strength
#define AONIOC_DRV_STR_2  0x00000001
#define AONIOC_DRV_STR_3  0x00000003
#define AONIOC_DRV_STR_4  0x00000002
#define AONIOC_DRV_STR_5  0x00000006
#define AONIOC_DRV_STR_6  0x00000007
#define AONIOC_DRV_STR_7  0x00000005
#define AONIOC_DRV_STR_8  0x00000004 // Highest drive strength

#define AONIOC_DRV_LVL_MIN  (AON_IOC_O_IOSTRMIN)
#define AONIOC_DRV_LVL_MED  (AON_IOC_O_IOSTRMED)
#define AONIOC_DRV_LVL_MAX  (AON_IOC_O_IOSTRMAX)

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Configure drive strength values for the manual drive strength options.
//!
//! This function defines the general drive strength settings for the non-AUTO
//! drive strength options in the MCU IOC. Consequently, if all IOs are using the
//! automatic drive strength option this function has no effect.
//!
//! Changing the drive strength values affects all current modes (Low-Current,
//! High-Current, and Extended-Current). Current mode for individual IOs is set in
//! MCU IOC by \ref IOCIODrvStrengthSet().
//!
//! \note Values are Gray encoded. Simply incrementing values to increase drive
//! strength will not work.
//!
//! \param ui32DriveLevel
//! - \ref AONIOC_DRV_LVL_MIN : Minimum drive strength option. Default value is selected
//!        to give minimum 2/4/8 mA @3.3V for Low-Current mode, High-Current mode,
//!        and Extended-Current mode respectively.
//! - \ref AONIOC_DRV_LVL_MED : Medium drive strength option. Default value is selected
//!        to give minimum 2/4/8 mA @2.5V for Low-Current mode, High-Current mode,
//!        and Extended-Current mode respectively.
//! - \ref AONIOC_DRV_LVL_MAX : Maximum drive strength option. Default value is selected
//!        to give minimum 2/4/8 mA @1.8V for Low-Current mode, High-Current mode,
//!        and Extended-Current mode respectively.
//! \param ui32DriveStrength sets the value used by IOs configured as non-AUTO drive strength in MCU IOC.
//! - \ref AONIOC_DRV_STR_1 : Lowest drive strength
//! - \ref AONIOC_DRV_STR_2
//! - \ref AONIOC_DRV_STR_3
//! - \ref AONIOC_DRV_STR_4
//! - \ref AONIOC_DRV_STR_5
//! - \ref AONIOC_DRV_STR_6
//! - \ref AONIOC_DRV_STR_7
//! - \ref AONIOC_DRV_STR_8 : Highest drive strength
//!
//! \return None
//!
//! \sa \ref AONIOCDriveStrengthGet(), \ref IOCIODrvStrengthSet()
//
//*****************************************************************************
__STATIC_INLINE void
AONIOCDriveStrengthSet(uint32_t ui32DriveLevel, uint32_t ui32DriveStrength)
{
    ASSERT((ui32DriveLevel == AONIOC_DRV_LVL_MIN) ||
           (ui32DriveLevel == AONIOC_DRV_LVL_MED) ||
           (ui32DriveLevel == AONIOC_DRV_LVL_MAX));
    ASSERT((ui32DriveStrength == AONIOC_DRV_STR_1) ||
           (ui32DriveStrength == AONIOC_DRV_STR_2) ||
           (ui32DriveStrength == AONIOC_DRV_STR_3) ||
           (ui32DriveStrength == AONIOC_DRV_STR_4) ||
           (ui32DriveStrength == AONIOC_DRV_STR_5) ||
           (ui32DriveStrength == AONIOC_DRV_STR_6) ||
           (ui32DriveStrength == AONIOC_DRV_STR_7) ||
           (ui32DriveStrength == AONIOC_DRV_STR_8));

    // Set the drive strength.
    HWREG(AON_IOC_BASE + ui32DriveLevel) = ui32DriveStrength;
}

//*****************************************************************************
//
//! \brief Get a specific drive level setting for all IOs.
//!
//! Use this function to read the drive strength setting for a specific
//! IO drive level.
//!
//! \note Values are Gray encoded.
//!
//! \param ui32DriveLevel is the specific drive level to get the setting for.
//! - \ref AONIOC_DRV_LVL_MIN : Minimum drive strength option.
//! - \ref AONIOC_DRV_LVL_MED : Medium drive strength option.
//! - \ref AONIOC_DRV_LVL_MAX : Maximum drive strength option.
//!
//! \return Returns the requested drive strength level setting for all IOs.
//! Possible values are:
//! - \ref AONIOC_DRV_STR_1 : Lowest drive strength
//! - \ref AONIOC_DRV_STR_2
//! - \ref AONIOC_DRV_STR_3
//! - \ref AONIOC_DRV_STR_4
//! - \ref AONIOC_DRV_STR_5
//! - \ref AONIOC_DRV_STR_6
//! - \ref AONIOC_DRV_STR_7
//! - \ref AONIOC_DRV_STR_8 : Highest drive strength
//!
//! \sa AONIOCDriveStrengthSet()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONIOCDriveStrengthGet(uint32_t ui32DriveLevel)
{
    // Check the arguments.
    ASSERT((ui32DriveLevel == AONIOC_DRV_LVL_MIN) ||
           (ui32DriveLevel == AONIOC_DRV_LVL_MED) ||
           (ui32DriveLevel == AONIOC_DRV_LVL_MAX));

    // Return the drive strength value.
    return( HWREG(AON_IOC_BASE + ui32DriveLevel) );
}

//*****************************************************************************
//
//! \brief Freeze the IOs.
//!
//! To retain the values of the output IOs during a powerdown/shutdown of the
//! device all IO latches in the AON domain should be frozen in their current
//! state. This ensures that software can regain control of the IOs after a
//! reboot without the IOs first falling back to the default values (i.e. input
//! and no pull).
//!
//! \return None
//!
//! \sa AONIOCFreezeDisable()
//
//*****************************************************************************
__STATIC_INLINE void
AONIOCFreezeEnable(void)
{
    // Set the AON IO latches as static.
    HWREG(AON_IOC_BASE + AON_IOC_O_IOCLATCH) = 0x0;
}

//*****************************************************************************
//
//! \brief Un-freeze the IOs.
//!
//! When rebooting the chip after it has entered powerdown/shutdown mode, the
//! software can regain control of the IOs by setting the IO latches as
//! transparent. The IOs should not be unfrozen before software has restored
//! the functionality of the IO.
//!
//! \return None
//!
//! \sa AONIOCFreezeEnable()
//
//*****************************************************************************
__STATIC_INLINE void
AONIOCFreezeDisable(void)
{
    // Set the AON IOC latches as transparent.
    HWREG(AON_IOC_BASE + AON_IOC_O_IOCLATCH) = AON_IOC_IOCLATCH_EN;
}

//*****************************************************************************
//
//! \brief Disable the 32kHz clock output.
//!
//! When outputting a 32 kHz clock on an IO, the output enable/disable functionality
//! in the IOC is bypassed. Therefore, the programmer needs to call this
//! function to disable the clock output.
//!
//! \return None
//!
//! \sa AONIOC32kHzOutputEnable()
//
//*****************************************************************************
__STATIC_INLINE void
AONIOC32kHzOutputDisable(void)
{
    // Disable the LF clock output.
    HWREG(AON_IOC_BASE + AON_IOC_O_CLK32KCTL) = AON_IOC_CLK32KCTL_OE_N;
}

//*****************************************************************************
//
//! \brief Enable the 32kHz clock output.
//!
//! When outputting a 32 kHz clock on an IO, the output enable/disable functionality
//! in the IOC is bypassed. Therefore, the programmer needs to call this
//! function to enable the clock output.
//!
//! \return None
//!
//! \sa AONIOC32kHzOutputDisable()
//
//*****************************************************************************
__STATIC_INLINE void
AONIOC32kHzOutputEnable(void)
{
    // Enable the LF clock output.
    HWREG(AON_IOC_BASE + AON_IOC_O_CLK32KCTL) = 0x0;
}

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __AON_IOC_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
