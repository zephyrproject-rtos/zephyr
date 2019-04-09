/******************************************************************************
*  Filename:       pwr_ctrl.h
*  Revised:        2017-11-02 15:41:14 +0100 (Thu, 02 Nov 2017)
*  Revision:       50165
*
*  Description:    Defines and prototypes for the System Power Control.
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
//! \addtogroup pwrctrl_api
//! @{
//
//*****************************************************************************

#ifndef __PWR_CTRL_H__
#define __PWR_CTRL_H__

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
#include "../inc/hw_ints.h"
#include "../inc/hw_aon_pmctl.h"
#include "../inc/hw_aon_rtc.h"
#include "../inc/hw_adi_2_refsys.h"
#include "debug.h"
#include "interrupt.h"
#include "osc.h"
#include "cpu.h"
#include "prcm.h"
#include "aon_ioc.h"
#include "adi.h"

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
    #define PowerCtrlSourceSet              NOROM_PowerCtrlSourceSet
#endif

//*****************************************************************************
//
// Defines for the system power states
//
//*****************************************************************************
#define PWRCTRL_ACTIVE          0x00000001
#define PWRCTRL_STANDBY         0x00000002
#define PWRCTRL_POWER_DOWN      0x00000004
#define PWRCTRL_SHUTDOWN        0x00000008

//*****************************************************************************
//
// Defines for the power configuration in the AON System Control 1.2 V
//
//*****************************************************************************
#define PWRCTRL_IOSEG3_ENABLE   0x00000800
#define PWRCTRL_IOSEG2_ENABLE   0x00000400
#define PWRCTRL_IOSEG3_DISABLE  0x00000200
#define PWRCTRL_IOSEG2_DISABLE  0x00000100
#define PWRCTRL_PWRSRC_DCDC     0x00000001
#define PWRCTRL_PWRSRC_GLDO     0x00000000
#define PWRCTRL_PWRSRC_ULDO     0x00000002

//*****************************************************************************
//
// The following are defines for the various reset source for the device.
//
//*****************************************************************************
#define PWRCTRL_RST_POWER_ON    0x00000000  // Reset by power on
#define PWRCTRL_RST_PIN         0x00000001  // Pin reset
#define PWRCTRL_RST_VDDS_BOD    0x00000002  // VDDS Brown Out Detect
#define PWRCTRL_RST_VDD_BOD     0x00000003  // VDD Brown Out Detect
#define PWRCTRL_RST_VDDR_BOD    0x00000004  // VDDR Brown Out Detect
#define PWRCTRL_RST_CLK_LOSS    0x00000005  // Clock loss Reset
#define PWRCTRL_RST_SW_PIN      0x00000006  // SYSRESET or pin reset
#define PWRCTRL_RST_WARM        0x00000007  // Reset via PRCM warm reset request

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Set (Request) the main power source.
//!
//! \note The system will never allow a switch to the \ref PWRCTRL_PWRSRC_ULDO
//! when in active mode. This is only allowed when the system is in lower power
//! mode where no code is executing and no peripherals are active.
//! Assuming that there is an external capacitor available for the
//! \ref PWRCTRL_PWRSRC_DCDC the system can dynamically switch back and forth
//! between the two when in active mode.
//!
//! \note The system will automatically switch to the GLDO / DCDC when waking
//! up from a low power mode.
//!
//! \param ui32PowerConfig is a bitmask indicating the target power source.
//! - \ref PWRCTRL_PWRSRC_DCDC
//! - \ref PWRCTRL_PWRSRC_GLDO
//! - \ref PWRCTRL_PWRSRC_ULDO
//!
//! \return None
//
//*****************************************************************************
extern void PowerCtrlSourceSet(uint32_t ui32PowerConfig);

//*****************************************************************************
//
//! \brief Get the main power source.
//!
//! Use this function to retrieve the current active power source.
//!
//! When the System CPU is active it can never be powered by uLDO as this
//! is too weak a power source.
//!
//! \note Using the DCDC power supply requires an external inductor.
//!
//! \return Returns the main power source.
//! - \ref PWRCTRL_PWRSRC_DCDC
//! - \ref PWRCTRL_PWRSRC_GLDO
//
//*****************************************************************************
__STATIC_INLINE uint32_t
PowerCtrlSourceGet(void)
{
    uint32_t ui32PowerConfig;

    // Return the current power source
    ui32PowerConfig = HWREG(AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL);
    if(ui32PowerConfig & AON_PMCTL_PWRCTL_DCDC_ACTIVE)
    {
        return (PWRCTRL_PWRSRC_DCDC);
    }
    else
    {
        return (PWRCTRL_PWRSRC_GLDO);
    }
}

//*****************************************************************************
//
//! \brief OBSOLETE: Get the last known reset source of the system.
//!
//! \deprecated This function will be removed in a future release.
//! Use \ref SysCtrlResetSourceGet() instead.
//!
//! This function returns reset source but does not cover if waking up from shutdown.
//! This function can be seen as a subset of function \ref SysCtrlResetSourceGet()
//! and will be removed in a future release.
//!
//! \return Returns one of the known reset values.
//! The possible reset sources are:
//! - \ref PWRCTRL_RST_POWER_ON
//! - \ref PWRCTRL_RST_PIN
//! - \ref PWRCTRL_RST_VDDS_BOD
//! - \ref PWRCTRL_RST_VDD_BOD
//! - \ref PWRCTRL_RST_VDDR_BOD
//! - \ref PWRCTRL_RST_CLK_LOSS
//! - \ref PWRCTRL_RST_SW_PIN
//! - \ref PWRCTRL_RST_WARM
//!
//! \sa \ref SysCtrlResetSourceGet()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
PowerCtrlResetSourceGet(void)
{
    //  Get the reset source.
    return (( HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) &
        AON_PMCTL_RESETCTL_RESET_SRC_M ) >>
        AON_PMCTL_RESETCTL_RESET_SRC_S ) ;
}

//*****************************************************************************
//
//! \brief Enables pad sleep in order to latch device outputs before shutdown.
//!
//! See \ref SysCtrlShutdown() for more information about how to enter
//! shutdown and how to wake up from shutdown.
//!
//! \return None
//!
//! \sa \ref PowerCtrlPadSleepDisable()
//
//*****************************************************************************
__STATIC_INLINE void
PowerCtrlPadSleepEnable(void)
{
    HWREG(AON_PMCTL_BASE + AON_PMCTL_O_SLEEPCTL) = 0;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

//*****************************************************************************
//
//! \brief Disables pad sleep in order to unlatch device outputs after wakeup from shutdown.
//!
//! This function must be called by the application after the device wakes up
//! from shutdown.
//!
//! See \ref SysCtrlShutdown() for more information about how to enter
//! shutdown and how to wake up from shutdown.
//!
//! \return None
//!
//! \sa \ref PowerCtrlPadSleepEnable()
//
//*****************************************************************************
__STATIC_INLINE void
PowerCtrlPadSleepDisable(void)
{
    HWREG(AON_PMCTL_BASE + AON_PMCTL_O_SLEEPCTL) = 1;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_PowerCtrlSourceSet
        #undef  PowerCtrlSourceSet
        #define PowerCtrlSourceSet              ROM_PowerCtrlSourceSet
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

#endif // __PWR_CTRL_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
