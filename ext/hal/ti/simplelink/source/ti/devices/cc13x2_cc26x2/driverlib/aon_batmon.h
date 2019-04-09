/******************************************************************************
*  Filename:       aon_batmon.h
*  Revised:        2016-10-06 17:21:09 +0200 (Thu, 06 Oct 2016)
*  Revision:       47343
*
*  Description:    Defines and prototypes for the AON Battery and Temperature
*                  Monitor
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
//! \addtogroup aonbatmon_api
//! @{
//
//*****************************************************************************

#ifndef __AON_BATMON_H__
#define __AON_BATMON_H__

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
#include "../inc/hw_aon_batmon.h"
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
    #define AONBatMonTemperatureGetDegC     NOROM_AONBatMonTemperatureGetDegC
#endif


//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Enable the temperature and battery monitoring.
//!
//! This function will enable the measurements of the temperature and the
//! battery voltage.
//!
//! To speed up the measurement of the levels the measurement can be enabled
//! before configuring the battery and temperature settings. When all of the
//! AON_BATMON registers are configured, the calculation of the voltage and
//! temperature values can be enabled (the measurement will now take
//! effect/propagate to other blocks).
//!
//! It is possible to enable both at the same time, after the AON_BATMON
//! registers are configured, but then the first values will be ready at a
//! later point compared to the scenario above.
//!
//! \note Temperature and battery voltage measurements are not done in
//! parallel. The measurement cycle is controlled by a hardware Finite State
//! Machine. First the temperature and then the battery voltage each taking
//! one cycle to complete. However, if the comparator measuring the battery
//! voltage detects a change on the reference value, a new measurement of the
//! battery voltage only is performed immediately after. This has no impact on
//! the cycle count.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AONBatMonEnable(void)
{
    // Enable the measurements.
    HWREG(AON_BATMON_BASE + AON_BATMON_O_CTL) =
        AON_BATMON_CTL_CALC_EN |
        AON_BATMON_CTL_MEAS_EN;
}

//*****************************************************************************
//
//! \brief Disable the temperature and battery monitoring.
//!
//! This function will disable the measurements of the temperature and the
//! battery voltage.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AONBatMonDisable(void)
{
    // Disable the measurements.
    HWREG(AON_BATMON_BASE + AON_BATMON_O_CTL) = 0;
}


//*****************************************************************************
//
//! \brief Get the current temperature measurement as a signed value in Deg Celsius.
//!
//! This function returns an calibrated and rounded value in degree Celsius.
//! The temperature measurements are updated every cycle.
//!
//! \note The temperature drifts slightly depending on the battery voltage.
//! This function compensates for this drift and returns a calibrated temperature.
//!
//! \note Use the function AONBatMonNewTempMeasureReady() to test for a new measurement.
//!
//! \return Returns signed integer part of temperature in Deg C (-256 .. +255)
//!
//! \sa AONBatMonNewTempMeasureReady()
//
//*****************************************************************************
extern int32_t AONBatMonTemperatureGetDegC( void );

//*****************************************************************************
//
//! \brief Get the battery monitor measurement.
//!
//! This function will return the current battery monitor measurement.
//! The battery voltage measurements are updated every cycle.
//!
//! \note The returned value is NOT sign-extended!
//!
//! \note Use the function \ref AONBatMonNewBatteryMeasureReady() to test for
//! a change in measurement.
//!
//! \return Returns the current battery monitor value of the battery voltage
//! measurement in a <int.frac> format size <3.8> in units of volt.
//!
//! \sa AONBatMonNewBatteryMeasureReady()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONBatMonBatteryVoltageGet(void)
{
    uint32_t ui32CurrentBattery;

    ui32CurrentBattery = HWREG(AON_BATMON_BASE + AON_BATMON_O_BAT);

    // Return the current battery voltage measurement.
    return (ui32CurrentBattery >> AON_BATMON_BAT_FRAC_S);
}

//*****************************************************************************
//
//! \brief Check if battery monitor measurement has changed.
//!
//! This function checks if a new battery monitor value is available. If the
//! measurement value has \b changed since last clear the function returns \c true.
//!
//! If the measurement has changed the function will automatically clear the
//! status bit.
//!
//! \note It is always possible to read out the current value of the
//! battery level using AONBatMonBatteryVoltageGet() but this function can be
//! used to check if the measurement has changed.
//!
//! \return Returns \c true if the measurement value has changed and \c false
//! otherwise.
//!
//! \sa AONBatMonNewTempMeasureReady(), AONBatMonBatteryVoltageGet()
//
//*****************************************************************************
__STATIC_INLINE bool
AONBatMonNewBatteryMeasureReady(void)
{
    bool bStatus;

    // Check the status bit.
    bStatus = HWREG(AON_BATMON_BASE + AON_BATMON_O_BATUPD) &
              AON_BATMON_BATUPD_STAT ? true : false;

    // Clear status bit if set.
    if(bStatus)
    {
        HWREG(AON_BATMON_BASE + AON_BATMON_O_BATUPD) = 1;
    }

    // Return status.
    return (bStatus);
}

//*****************************************************************************
//
//! \brief Check if temperature monitor measurement has changed.
//!
//! This function checks if a new temperature value is available. If the
//! measurement value has \b changed since last clear the function returns \c true.
//!
//! If the measurement has changed the function will automatically clear the
//! status bit.
//!
//! \note It is always possible to read out the current value of the
//! temperature using \ref AONBatMonTemperatureGetDegC()
//! but this function can be used to check if the measurement has changed.
//!
//! \return Returns \c true if the measurement value has changed and \c false
//! otherwise.
//!
//! \sa AONBatMonNewBatteryMeasureReady(), AONBatMonTemperatureGetDegC()
//
//*****************************************************************************
__STATIC_INLINE bool
AONBatMonNewTempMeasureReady(void)
{
    bool bStatus;

    // Check the status bit.
    bStatus = HWREG(AON_BATMON_BASE + AON_BATMON_O_TEMPUPD) &
              AON_BATMON_TEMPUPD_STAT ? true : false;

    // Clear status bit if set.
    if(bStatus)
    {
        HWREG(AON_BATMON_BASE + AON_BATMON_O_TEMPUPD) = 1;
    }

    // Return status.
    return (bStatus);
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_AONBatMonTemperatureGetDegC
        #undef  AONBatMonTemperatureGetDegC
        #define AONBatMonTemperatureGetDegC     ROM_AONBatMonTemperatureGetDegC
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

#endif //  __AON_BATMON_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
