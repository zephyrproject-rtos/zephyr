/******************************************************************************
*  Filename:       sys_ctrl.h
*  Revised:        2018-09-17 14:58:51 +0200 (Mon, 17 Sep 2018)
*  Revision:       52634
*
*  Description:    Defines and prototypes for the System Controller.
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
//! \addtogroup sysctrl_api
//! @{
//
//*****************************************************************************

#ifndef __SYSCTRL_H__
#define __SYSCTRL_H__

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
#include "../inc/hw_sysctl.h"
#include "../inc/hw_prcm.h"
#include "../inc/hw_nvic.h"
#include "../inc/hw_aon_ioc.h"
#include "../inc/hw_ddi_0_osc.h"
#include "../inc/hw_rfc_pwr.h"
#include "../inc/hw_prcm.h"
#include "../inc/hw_adi_3_refsys.h"
#include "../inc/hw_aon_pmctl.h"
#include "../inc/hw_aon_rtc.h"
#include "../inc/hw_fcfg1.h"
#include "interrupt.h"
#include "debug.h"
#include "pwr_ctrl.h"
#include "osc.h"
#include "prcm.h"
#include "adi.h"
#include "ddi.h"
#include "cpu.h"
#include "vims.h"

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
    #define SysCtrlIdle                     NOROM_SysCtrlIdle
    #define SysCtrlShutdownWithAbort        NOROM_SysCtrlShutdownWithAbort
    #define SysCtrlShutdown                 NOROM_SysCtrlShutdown
    #define SysCtrlStandby                  NOROM_SysCtrlStandby
    #define SysCtrlSetRechargeBeforePowerDown NOROM_SysCtrlSetRechargeBeforePowerDown
    #define SysCtrlAdjustRechargeAfterPowerDown NOROM_SysCtrlAdjustRechargeAfterPowerDown
    #define SysCtrl_DCDC_VoltageConditionalControl NOROM_SysCtrl_DCDC_VoltageConditionalControl
    #define SysCtrlResetSourceGet           NOROM_SysCtrlResetSourceGet
#endif

//*****************************************************************************
//
// Defines for the settings of the main XOSC
//
//*****************************************************************************
#define SYSCTRL_SYSBUS_ON       0x00000001
#define SYSCTRL_SYSBUS_OFF      0x00000000

//*****************************************************************************
//
// Defines for the different power modes of the System CPU
//
//*****************************************************************************
#define CPU_RUN                 0x00000000
#define CPU_SLEEP               0x00000001
#define CPU_DEEP_SLEEP          0x00000002

//*****************************************************************************
//
// Defines for SysCtrlSetRechargeBeforePowerDown
//
//*****************************************************************************
#define XOSC_IN_HIGH_POWER_MODE 0 // When xosc_hf is in HIGH_POWER_XOSC
#define XOSC_IN_LOW_POWER_MODE  1 // When xosc_hf is in LOW_POWER_XOSC

//*****************************************************************************
//
// Defines for the vimsPdMode parameter of SysCtrlIdle and SysCtrlStandby
//
//*****************************************************************************
#define VIMS_ON_CPU_ON_MODE     0 // VIMS power domain is only powered when CPU power domain is powered
#define VIMS_ON_BUS_ON_MODE     1 // VIMS power domain is powered whenever the BUS power domain is powered
#define VIMS_NO_PWR_UP_MODE     2 // VIMS power domain is not powered up at next wakeup.

//*****************************************************************************
//
// Defines for the rechargeMode parameter of SysCtrlStandby
//
//*****************************************************************************
#define SYSCTRL_PREFERRED_RECHARGE_MODE                                       \
                                0xFFFFFFFF // Preferred recharge mode

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Force the system into idle mode.
//!
//! This function forces the system into IDLE mode by configuring the requested
//! VIMS mode, enabling cache retention and powering off the CPU power domain.
//!
//! \param vimsPdMode selects the requested VIMS power domain mode
//! The parameter must be one of the following:
//! - \ref VIMS_ON_CPU_ON_MODE
//! - \ref VIMS_ON_BUS_ON_MODE
//! - \ref VIMS_NO_PWR_UP_MODE
//!
//! \return None
//
//*****************************************************************************
extern void SysCtrlIdle(uint32_t vimsPdMode);

//*****************************************************************************
//
//! \brief Try to enter shutdown but abort if wakeup event happened before shutdown.
//!
//! This function puts the device in shutdown state if no wakeup events are
//! detected before shutdown.
//!
//! Compared to the basic \ref SysCtrlShutdown() function this function makes sure
//! that wakeup events that happen before actual shutdown are also detected. This
//! function either enters shutdown with a guaranteed wakeup detection or returns
//! to the caller function due to a pre-shutdown wakeup event.
//!
//! See \ref SysCtrlShutdown() for basic information about how to configure the device before
//! shutdown and how to wakeup from shutdown.
//!
//! This function uses IO edge detection in addition to the mandatory wakeup configuration.
//! Additional requirements to the application for this function are:
//! - \b Before :
//!   - When the application configures an IO for wakeup (see \ref IOCIOShutdownSet())
//!     the application must also configure the same IO for edge detection
//!     (see \ref IOCIOIntSet()).
//!   - Edge detection must use the same polarity as the wakeup configuration.
//!   - Application must enable peripheral power domain (see \ref PRCMPowerDomainOn())
//!     and enable GPIO module in the peripheral power domain (see \ref PRCMPeripheralRunEnable()).
//! - \b After :
//!   - An edge, with same polarity as a wakeup event, was detected on a wakeup
//!     enabled IO before shutdown, and the shutdown was aborted. The application must
//!     clear the event generated by the edge detect (see \ref GPIO_clearEventDio()) and
//!     decide what happens next.
//!
//! Useful functions related to shutdown:
//! - \ref IOCIOShutdownSet() : Enables wakeup from shutdown.
//! - \ref IOCIOIntSet() : Enables IO edge detection.
//! - \ref PRCMPowerDomainOn() : Enables peripheral power domain.
//! - \ref PRCMPeripheralRunEnable() : Enables GPIO module.
//! - \ref SysCtrlResetSourceGet() : Detects wakeup from shutdown.
//! - \ref PowerCtrlPadSleepDisable() : Unlatches outputs (disables pad sleep) after
//!    wakeup from shutdown.
//! - \ref GPIO_clearEventDio() : Clears edge detects.
//!
//! It is recommended to disable interrupts before calling this function because:
//! - Pads are in sleep mode while this function runs.
//! - An interrupt routine might be terminated if it is triggered after the decision
//!   to enter shutdown.
//!
//! \return None
//
//*****************************************************************************
extern void SysCtrlShutdownWithAbort(void);

//*****************************************************************************
//
//! \brief Enable shutdown of the device.
//!
//! This function puts the device in shutdown state. The device automatically
//! latches all outputs (pads in sleep) before it turns off all internal power
//! supplies.
//!
//! JTAG must be disconnected and JTAG power domain must be off before device can
//! enter shutdown. This function waits until the device satisfies all shutdown
//! conditions before it enters shutdown.
//!
//! \note The application must unlatch the outputs when the device wakes up from shutdown.
//! It is recommended that any outputs that need to be restored after a wakeup from
//! shutdown are restored before outputs are unlatched in order to avoid glitches.
//!
//! See \ref PowerCtrlPadSleepDisable() for information about how to unlatch outputs
//! (disable pad sleep) after wakeup from shutdown.
//!
//! \note Wakeup events are only detected after the device enters shutdown.
//!
//! See \ref IOCIOShutdownSet() for information about how to enable wakeup from shutdown.
//!
//! See \ref SysCtrlResetSourceGet() for information about how to detect wakeup
//! from shutdown.
//!
//! It is recommended to disable interrupts before calling this function. Shutdown
//! happens immediately when the device satisfies all shutdown conditions thus
//! interrupt routines triggered after this function is called might be
//! aborted.
//!
//! \return This function does \b not return.
//
//*****************************************************************************
extern void SysCtrlShutdown(void);

//*****************************************************************************
//
//! \brief Force the system into standby mode.
//!
//! This function forces all power domains (RFCORE, SERIAL, PERIPHERAL) off.
//! The VIMS and CPU power domains are turned off by the HW when the
//! \ref PRCMDeepSleep() function is called.
//! The IOs are latched (frozen) before the power domains are turned off to
//! avoid glitches.
//! The VIMS retention (cache) and VIMS module are turned off if requested.
//! The deep-sleep clock for the crypto and DMA modules are turned off,
//! as they must be off in order to enter standby.
//! This function assumes that the LF clock has already been switched to
//! and that the LF clock qualifiers must have been disabled/bypassed.
//!
//! In internal regulator mode the adaptive recharge functionality is enabled
//! with fixed parameter values.
//! In external regulator mode the recharge functionality is disabled.
//!
//! \note This function is optimized to execute with TI-RTOS. There might be
//! application specific prerequisites you would want to do before entering
//! standby which deviate from this specific implementation.
//!
//! \param retainCache selects if VIMS cache shall be retained or not.
//! - false : VIMS cache is not retained
//! - true : VIMS cache is retained
//! \param vimsPdMode selects the VIMS power domain mode.
//! The parameter must be one of the following:
//! - \ref VIMS_ON_CPU_ON_MODE
//! - \ref VIMS_NO_PWR_UP_MODE
//! \param rechargeMode specifies the requested recharge mode.
//! The parameter must be one of the following:
//! - \ref SYSCTRL_PREFERRED_RECHARGE_MODE : Preferred recharge mode specified by TI
//!
//! \return None
//
//*****************************************************************************
extern void SysCtrlStandby(bool retainCache, uint32_t vimsPdMode, uint32_t rechargeMode);

//*****************************************************************************
//
//! \brief Get the CPU core clock frequency.
//!
//! Use this function to get the current clock frequency for the CPU.
//!
//! The CPU can run from 48 MHz and down to 750kHz. The frequency is defined
//! by the combined division factor of the SYSBUS and the CPU clock divider.
//!
//! \return Returns the current CPU core clock frequency.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
SysCtrlClockGet( void )
{
    // Return fixed clock speed
    return( GET_MCU_CLOCK );
}

//*****************************************************************************
//
//! \brief Sync all accesses to the AON register interface.
//!
//! When this function returns, all writes to the AON register interface are
//! guaranteed to have propagated to hardware. The function will return
//! immediately if no AON writes are pending; otherwise, it will wait for the next
//! AON clock before returning.
//!
//! \return None
//!
//! \sa \ref SysCtrlAonUpdate()
//
//*****************************************************************************
__STATIC_INLINE void
SysCtrlAonSync(void)
{
    // Sync the AON interface
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

//*****************************************************************************
//
//! \brief Update all interfaces to AON.
//!
//! When this function returns, at least 1 clock cycle has progressed on the
//! AON domain, so that any outstanding updates to and from the AON interface
//! is guaranteed to be in sync.
//!
//! \note This function should primarily be used after wakeup from sleep modes,
//! as it will guarantee that all shadow registers on the AON interface are updated
//! before reading any AON registers from the MCU domain. If a write has been
//! done to the AON interface it is sufficient to call the \ref SysCtrlAonSync().
//!
//! \return None
//!
//! \sa \ref SysCtrlAonSync()
//
//*****************************************************************************
__STATIC_INLINE void
SysCtrlAonUpdate(void)
{
    // Force a clock cycle on the AON interface to guarantee all registers are
    // in sync.
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC) = 1;
    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

//*****************************************************************************
//
//! \brief Set Recharge values before entering Power Down.
//!
//! This function shall be called just before entering Power Down.
//! This function typically does nothing (default setting), but
//! if temperature compensated recharge level are enabled (by setting
//! CCFG_MODE_CONF_VDDR_TRIM_SLEEP_TC = 0)
//! it adds temperature compensation to the recharge level.
//!
//! \param xoscPowerMode (typically running in XOSC_IN_HIGH_POWER_MODE all the time).
//! - \ref XOSC_IN_HIGH_POWER_MODE : When xosc_hf is in HIGH_POWER_XOSC.
//! - \ref XOSC_IN_LOW_POWER_MODE  : When xosc_hf is in LOW_POWER_XOSC.
//!
//! \return None
//
//*****************************************************************************
extern void SysCtrlSetRechargeBeforePowerDown( uint32_t xoscPowerMode );

//*****************************************************************************
//
//! \brief Adjust Recharge calculations to be used next.
//!
//! Nothing to be done but keeping this function for platform compatibility.
//!
//! \return None
//
//*****************************************************************************
extern void SysCtrlAdjustRechargeAfterPowerDown( uint32_t vddrRechargeMargin );

//*****************************************************************************
//
//! \brief Turns DCDC on or off depending of what is considered to be optimal usage.
//!
//! This function controls the DCDC only if both the following CCFG settings are \c true:
//! - DCDC is configured to be used.
//! - Alternative DCDC settings are defined and enabled.
//!
//! The DCDC is configured in accordance to the CCFG settings when turned on.
//!
//! This function should be called periodically.
//!
//! \return None
//
//*****************************************************************************
extern void SysCtrl_DCDC_VoltageConditionalControl( void );

//*****************************************************************************
// \name Return values from calling SysCtrlResetSourceGet()
//@{
//*****************************************************************************
#define RSTSRC_PWR_ON                 (( AON_PMCTL_RESETCTL_RESET_SRC_PWR_ON    ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S ))
#define RSTSRC_PIN_RESET              (( AON_PMCTL_RESETCTL_RESET_SRC_PIN_RESET ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S ))
#define RSTSRC_VDDS_LOSS              (( AON_PMCTL_RESETCTL_RESET_SRC_VDDS_LOSS ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S ))
#define RSTSRC_VDDR_LOSS              (( AON_PMCTL_RESETCTL_RESET_SRC_VDDR_LOSS ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S ))
#define RSTSRC_CLK_LOSS               (( AON_PMCTL_RESETCTL_RESET_SRC_CLK_LOSS  ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S ))
#define RSTSRC_SYSRESET               (( AON_PMCTL_RESETCTL_RESET_SRC_SYSRESET  ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S ))
#define RSTSRC_WARMRESET              (( AON_PMCTL_RESETCTL_RESET_SRC_WARMRESET ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S ))
#define RSTSRC_WAKEUP_FROM_SHUTDOWN  ((( AON_PMCTL_RESETCTL_RESET_SRC_M         ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S )) + 1 )
#define RSTSRC_WAKEUP_FROM_TCK_NOISE ((( AON_PMCTL_RESETCTL_RESET_SRC_M         ) >> ( AON_PMCTL_RESETCTL_RESET_SRC_S )) + 2 )
//@}

//*****************************************************************************
//
//! \brief Returns the reset source (including "wakeup from shutdown").
//!
//! In case of \ref RSTSRC_WAKEUP_FROM_SHUTDOWN the application is
//! responsible for unlatching the outputs (disable pad sleep).
//! See \ref PowerCtrlPadSleepDisable() for more information.
//!
//! \return Returns the reset source.
//! - \ref RSTSRC_PWR_ON
//! - \ref RSTSRC_PIN_RESET
//! - \ref RSTSRC_VDDS_LOSS
//! - \ref RSTSRC_VDDR_LOSS
//! - \ref RSTSRC_CLK_LOSS
//! - \ref RSTSRC_SYSRESET
//! - \ref RSTSRC_WARMRESET
//! - \ref RSTSRC_WAKEUP_FROM_SHUTDOWN
//! - \ref RSTSRC_WAKEUP_FROM_TCK_NOISE
//
//*****************************************************************************
extern uint32_t SysCtrlResetSourceGet( void );

//*****************************************************************************
//
//! \brief Perform a full system reset.
//!
//! \return The chip will reset and hence never return from this call.
//
//*****************************************************************************
__STATIC_INLINE void
SysCtrlSystemReset( void )
{
   // Disable CPU interrupts
   CPUcpsid();
   // Write reset register
   HWREGBITW( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL, AON_PMCTL_RESETCTL_SYSRESET_BITN ) = 1;
   // Finally, wait until the above write propagates
   while ( 1 ) {
      // Do nothing, just wait for the reset (and never return from here)
   }
}

//*****************************************************************************
//
//! \brief Enables reset if OSC clock loss event is asserted.
//!
//! Clock loss circuit in analog domain must be enabled as well in order to
//! actually enable for a clock loss reset to occur
//! \ref OSCClockLossEventEnable().
//!
//! \note This function shall typically not be called because the clock loss
//! reset functionality is controlled by the boot code (a factory configuration
//! defines whether it is set or not).
//!
//! \return None
//!
//! \sa \ref SysCtrlClockLossResetDisable(), \ref OSCClockLossEventEnable()
//
//*****************************************************************************
__STATIC_INLINE void
SysCtrlClockLossResetEnable(void)
{
    // Set clock loss enable bit in AON_SYSCTRL using bit banding
    HWREGBITW(AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL, AON_PMCTL_RESETCTL_CLK_LOSS_EN_BITN) = 1;
}

//*****************************************************************************
//
//! \brief Disables reset due to OSC clock loss event.
//!
//! \note This function shall typically not be called because the clock loss
//! reset functionality is controlled by the boot code (a factory configuration
//! defines whether it is set or not).
//!
//! \return None
//!
//! \sa \ref SysCtrlClockLossResetEnable()
//
//*****************************************************************************
__STATIC_INLINE void
SysCtrlClockLossResetDisable(void)
{
    // Clear clock loss enable bit in AON_SYSCTRL using bit banding
    HWREGBITW(AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL, AON_PMCTL_RESETCTL_CLK_LOSS_EN_BITN) = 0;
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_SysCtrlIdle
        #undef  SysCtrlIdle
        #define SysCtrlIdle                     ROM_SysCtrlIdle
    #endif
    #ifdef ROM_SysCtrlShutdownWithAbort
        #undef  SysCtrlShutdownWithAbort
        #define SysCtrlShutdownWithAbort        ROM_SysCtrlShutdownWithAbort
    #endif
    #ifdef ROM_SysCtrlShutdown
        #undef  SysCtrlShutdown
        #define SysCtrlShutdown                 ROM_SysCtrlShutdown
    #endif
    #ifdef ROM_SysCtrlStandby
        #undef  SysCtrlStandby
        #define SysCtrlStandby                  ROM_SysCtrlStandby
    #endif
    #ifdef ROM_SysCtrlSetRechargeBeforePowerDown
        #undef  SysCtrlSetRechargeBeforePowerDown
        #define SysCtrlSetRechargeBeforePowerDown ROM_SysCtrlSetRechargeBeforePowerDown
    #endif
    #ifdef ROM_SysCtrlAdjustRechargeAfterPowerDown
        #undef  SysCtrlAdjustRechargeAfterPowerDown
        #define SysCtrlAdjustRechargeAfterPowerDown ROM_SysCtrlAdjustRechargeAfterPowerDown
    #endif
    #ifdef ROM_SysCtrl_DCDC_VoltageConditionalControl
        #undef  SysCtrl_DCDC_VoltageConditionalControl
        #define SysCtrl_DCDC_VoltageConditionalControl ROM_SysCtrl_DCDC_VoltageConditionalControl
    #endif
    #ifdef ROM_SysCtrlResetSourceGet
        #undef  SysCtrlResetSourceGet
        #define SysCtrlResetSourceGet           ROM_SysCtrlResetSourceGet
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

#endif //  __SYSCTRL_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
