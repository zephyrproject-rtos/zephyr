/******************************************************************************
*  Filename:       osc.h
*  Revised:        2019-02-14 09:35:31 +0100 (Thu, 14 Feb 2019)
*  Revision:       54539
*
*  Description:    Defines and prototypes for the system oscillator control.
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
//! \addtogroup osc_api
//! @{
//
//*****************************************************************************

#ifndef __OSC_H__
#define __OSC_H__

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
#include "../inc/hw_aon_pmctl.h"
#include "../inc/hw_ccfg.h"
#include "../inc/hw_fcfg1.h"
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ddi.h"
#include "../inc/hw_ddi_0_osc.h"
#include "rom.h"
#include "ddi.h"
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
    #define OSCClockSourceSet               NOROM_OSCClockSourceSet
    #define OSCClockSourceGet               NOROM_OSCClockSourceGet
    #define OSCHF_GetStartupTime            NOROM_OSCHF_GetStartupTime
    #define OSCHF_TurnOnXosc                NOROM_OSCHF_TurnOnXosc
    #define OSCHF_AttemptToSwitchToXosc     NOROM_OSCHF_AttemptToSwitchToXosc
    #define OSCHF_SwitchToRcOscTurnOffXosc  NOROM_OSCHF_SwitchToRcOscTurnOffXosc
    #define OSCHF_DebugGetCrystalAmplitude  NOROM_OSCHF_DebugGetCrystalAmplitude
    #define OSCHF_DebugGetExpectedAverageCrystalAmplitude NOROM_OSCHF_DebugGetExpectedAverageCrystalAmplitude
    #define OSC_HPOSC_Debug_InitFreqOffsetParams NOROM_OSC_HPOSC_Debug_InitFreqOffsetParams
    #define OSC_HPOSCInitializeFrequencyOffsetParameters NOROM_OSC_HPOSCInitializeFrequencyOffsetParameters
    #define OSC_HPOSCRelativeFrequencyOffsetGet NOROM_OSC_HPOSCRelativeFrequencyOffsetGet
    #define OSC_AdjustXoscHfCapArray        NOROM_OSC_AdjustXoscHfCapArray
    #define OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert NOROM_OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert
    #define OSC_HPOSCRtcCompensate          NOROM_OSC_HPOSCRtcCompensate
#endif

//*****************************************************************************
//
// Defines for the High Frequency XTAL Power mode
//
//*****************************************************************************
#define LOW_POWER_XOSC          1
#define HIGH_POWER_XOSC         0

//*****************************************************************************
//
// Defines for the High Frequency XTAL Power mode
//
//*****************************************************************************
#define OSC_SRC_CLK_HF          0x00000001
#define OSC_SRC_CLK_LF          0x00000004

#define OSC_RCOSC_HF            0x00000000
#define OSC_XOSC_HF             0x00000001
#define OSC_RCOSC_LF            0x00000002
#define OSC_XOSC_LF             0x00000003

#define SCLK_HF_RCOSC_HF        0
#define SCLK_HF_XOSC_HF         1

#define SCLK_LF_FROM_RCOSC_HF   0
#define SCLK_LF_FROM_XOSC_HF    1
#define SCLK_LF_FROM_RCOSC_LF   2
#define SCLK_LF_FROM_XOSC_LF    3

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Set Power Mode for High Frequency XTAL Oscillator.
//!
//! \param ui32Mode is the power mode for the HF XTAL.
//! - \ref LOW_POWER_XOSC
//! - \ref HIGH_POWER_XOSC
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
OSCXHfPowerModeSet(uint32_t ui32Mode)
{
    // Check the arguments.
    ASSERT((ui32Mode == LOW_POWER_XOSC) ||
           (ui32Mode == HIGH_POWER_XOSC));

    // Change the power mode.
    DDI16BitWrite(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_CTL0, DDI_0_OSC_CTL0_XOSC_HF_POWER_MODE,
                  ui32Mode);
}

//*****************************************************************************
//
//! \brief Enables OSC clock loss event detection.
//!
//! Enables the clock loss event flag to be raised if a clock loss is detected.
//!
//! \note OSC clock loss event must be disabled before SCLK_LF clock source is
//! changed (by calling \ref OSCClockSourceSet()) and remain disabled until the
//! change is confirmed (by calling \ref OSCClockSourceGet()).
//!
//! \return None
//!
//! \sa \ref OSCClockLossEventDisable()
//
//*****************************************************************************
__STATIC_INLINE void
OSCClockLossEventEnable( void )
{
    DDI16BitfieldWrite( AUX_DDI0_OSC_BASE, DDI_0_OSC_O_CTL0,
        DDI_0_OSC_CTL0_CLK_LOSS_EN_M,
        DDI_0_OSC_CTL0_CLK_LOSS_EN_S, 1 );
}

//*****************************************************************************
//
//! \brief Disables OSC clock loss event detection.
//!
//! Disabling the OSC clock loss event does also clear the clock loss event flag.
//!
//! \note OSC clock loss event must be disabled before SCLK_LF clock source is
//! changed (by calling \ref OSCClockSourceSet()) and remain disabled until the
//! change is confirmed (by calling \ref OSCClockSourceGet()).
//!
//! \return None
//!
//! \sa \ref OSCClockLossEventEnable()
//
//*****************************************************************************
__STATIC_INLINE void
OSCClockLossEventDisable( void )
{
    DDI16BitfieldWrite( AUX_DDI0_OSC_BASE, DDI_0_OSC_O_CTL0,
        DDI_0_OSC_CTL0_CLK_LOSS_EN_M,
        DDI_0_OSC_CTL0_CLK_LOSS_EN_S, 0 );
}

//*****************************************************************************
//
//! \brief Configure the oscillator input to the a source clock.
//!
//! Use this function to set the oscillator source for one or more of the
//! system source clocks.
//!
//! When selecting the high frequency clock source (OSC_SRC_CLK_HF), this function will not do
//! the actual switch. Enabling the high frequency XTAL can take several hundred
//! micro seconds, so the actual switch is done in a separate function, \ref OSCHfSourceSwitch(),
//! leaving System CPU free to perform other tasks as the XTAL starts up.
//!
//! \note The High Frequency (\ref OSC_SRC_CLK_HF) can only be derived from the
//! high frequency oscillator. The Low Frequency source clock (\ref OSC_SRC_CLK_LF)
//! can be derived from all 4 oscillators.
//!
//! \note If enabling \ref OSC_XOSC_LF it is not safe to go to powerdown/shutdown
//! until the LF clock is running which can be checked using \ref OSCClockSourceGet().
//!
//! \note Clock loss reset generation must be disabled before SCLK_LF (\ref OSC_SRC_CLK_LF)
//! clock source is changed and remain disabled until the change is confirmed.
//!
//! \param ui32SrcClk is the source clocks to configure.
//! - \ref OSC_SRC_CLK_HF
//! - \ref OSC_SRC_CLK_LF
//! \param ui32Osc is the oscillator that drives the source clock.
//! - \ref OSC_RCOSC_HF
//! - \ref OSC_XOSC_HF
//! - \ref OSC_RCOSC_LF (only when ui32SrcClk is \ref OSC_SRC_CLK_LF)
//! - \ref OSC_XOSC_LF (only when ui32SrcClk is \ref OSC_SRC_CLK_LF)
//!
//! \sa \ref OSCClockSourceGet(), \ref OSCHfSourceSwitch()
//!
//! \return None
//
//*****************************************************************************
extern void OSCClockSourceSet(uint32_t ui32SrcClk, uint32_t ui32Osc);

//*****************************************************************************
//
//! \brief Get the source clock settings.
//!
//! Use this function to get the oscillator source for one of the system source
//! clocks.
//!
//! \param ui32SrcClk is the source clock to check.
//! - \ref OSC_SRC_CLK_HF
//! - \ref OSC_SRC_CLK_LF
//!
//! \return Returns the type of oscillator that drives the clock source.
//! - \ref OSC_RCOSC_HF
//! - \ref OSC_XOSC_HF
//! - \ref OSC_RCOSC_LF
//! - \ref OSC_XOSC_LF
//!
//! \sa \ref OSCClockSourceSet(), \ref OSCHfSourceSwitch()
//
//*****************************************************************************
extern uint32_t OSCClockSourceGet(uint32_t ui32SrcClk);

//*****************************************************************************
//
//! \brief Check if the HF clock source is ready to be switched.
//!
//! If a request to switch the HF clock source has been made, this function
//! can be used to check if the clock source is ready to be switched.
//!
//! Once the HF clock source is ready the switch can be performed by calling
//! the \ref OSCHfSourceSwitch()
//!
//! \return Returns status of HF clock source:
//! - \c true  : HF clock source is ready.
//! - \c false : HF clock source is \b not ready.
//
//*****************************************************************************
__STATIC_INLINE bool
OSCHfSourceReady(void)
{
    // Return the readiness of the HF clock source
    return (DDI16BitfieldRead(AUX_DDI0_OSC_BASE, DDI_0_OSC_O_STAT0,
                              DDI_0_OSC_STAT0_PENDINGSCLKHFSWITCHING_M,
                              DDI_0_OSC_STAT0_PENDINGSCLKHFSWITCHING_S)) ?
        true : false;
}

//*****************************************************************************
//
//! \brief Switch the high frequency clock.
//!
//! When switching the HF clock source the clock period might be prolonged
//! leaving the clock 'stuck-at' high or low for a few cycles. To ensure that
//! this does not coincide with a read access to the Flash, potentially
//! freezing the device, the HF clock source switch must be executed from ROM.
//!
//! \note This function will not return until the clock source has been
//! switched. It is left to the programmer to ensure, that there is a pending
//! request for a HF clock source switch before this function is called.
//!
//! \return None
//!
//! \sa \ref OSCClockSourceSet()
//
//*****************************************************************************
__STATIC_INLINE void
OSCHfSourceSwitch(void)
{
    // Read target clock (lower half of the 32-bit CTL0 register)
    uint16_t hfSrc = HWREGH(AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0) & DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL_M;

    // If target clock source is RCOSC, change clock source for DCDC to RCOSC
    if(hfSrc == DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL_RCOSC)
    {
        // Force DCDC to use RCOSC before switching SCLK_HF to RCOSC
        HWREG(AUX_DDI0_OSC_BASE + DDI_O_MASK16B + (DDI_0_OSC_O_CTL0 << 1) + 4) = DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL_M | (DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL_M >> 16);
        // Dummy read to ensure that the write has propagated
        HWREGH(AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0);
    }

    // Switch the HF clock source
    HapiHFSourceSafeSwitch();

    // If target clock source is XOSC, change clock source for DCDC to "auto"
    if(hfSrc == DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL_XOSC)
    {
        // Set DCDC clock source back to "auto" after SCLK_HF was switched to XOSC
        HWREG(AUX_DDI0_OSC_BASE + DDI_O_MASK16B + (DDI_0_OSC_O_CTL0 << 1) + 4) = DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL_M;
    }
}

//*****************************************************************************
//
//! \brief Identifies if HPOSC is enabled.
//!
//! This function checks if the device supports HPOSC and that HPOSC is selected
//! as HF oscillator for use when the radio is active.
//!
//! \return Returns status of HPOSC functionality:
//! - \c true  : HPOSC is enabled.
//! - \c false : HPOSC is not enabled.
//
//*****************************************************************************
__STATIC_INLINE bool
OSC_IsHPOSCEnabled(void)
{
    bool enabled = false;

    if((( HWREG(CCFG_BASE + CCFG_O_MODE_CONF) & CCFG_MODE_CONF_XOSC_FREQ_M) == CCFG_MODE_CONF_XOSC_FREQ_HPOSC) &&
       (( HWREG(FCFG1_BASE + FCFG1_O_OSC_CONF) & FCFG1_OSC_CONF_HPOSC_OPTION) == 0))
    {
        enabled = true;
    }

    return (enabled);
}

//*****************************************************************************
//
//! \brief Identifies if HPOSC is enabled and that SCLK_LF is derived from XOSC_HF.
//!
//! This function checks if the device supports HPOSC and that HPOSC is selected
//! as HF oscillator for use when the radio is active and also that SCLK_LF is
//! derived from XOSC_HF.
//!
//! \return Returns status of HPOSC and SCLK_LF configuration:
//! - \c true  : HPOSC is enabled and SCLK_LF is derived from XOSC_HF.
//! - \c false : Either HPOSC not enabled or SCLK_LF is not derived from XOSC_HF.
//
//*****************************************************************************
__STATIC_INLINE bool
OSC_IsHPOSCEnabledWithHfDerivedLfClock(void)
{
    bool enabled = false;

    // Check configuration by reading lower half of the 32-bit CTL0 register
    uint16_t regVal = HWREGH(AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0);
    if( ( ( regVal & DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_M ) == DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_XOSCHFDLF ) &&
        ( ( regVal & DDI_0_OSC_CTL0_HPOSC_MODE_EN_M   ) == DDI_0_OSC_CTL0_HPOSC_MODE_EN             )   )
    {
            enabled = true;
    }

    return (enabled);
}

//*****************************************************************************
//
//! \brief Returns maximum startup time (in microseconds) of XOSC_HF.
//!
//! The startup time depends on several factors. This function calculates the
//! maximum startup time based on statistical information.
//!
//! \param timeUntilWakeupInMs indicates how long time (milliseconds) to the
//! startup will occur.
//!
//! \return Time margin to use in microseconds.
//
//*****************************************************************************
extern uint32_t OSCHF_GetStartupTime( uint32_t timeUntilWakeupInMs );

//*****************************************************************************
//
//! \brief Turns on XOSC_HF (but without switching to XOSC_HF).
//!
//! This function simply indicates the need for XOSC_HF to the hardware which
//! initiates the XOSC_HF startup.
//!
//! \return None
//
//*****************************************************************************
extern void OSCHF_TurnOnXosc( void );

//*****************************************************************************
//
//! \brief Switch to XOSC_HF if XOSC_HF is ready.
//!
//! This is a non-blocking function checking if the XOSC_HF is ready and
//! performs the switching if ready. The function is somewhat blocking in the
//! case where switching is performed.
//!
//! \return Returns status of the XOSC_HF switching:
//! - \c true  : Switching to XOSC_HF has occurred.
//! - \c false : Switching has not occurred.
//
//*****************************************************************************
extern bool OSCHF_AttemptToSwitchToXosc( void );

//*****************************************************************************
//
//! \brief Switch to RCOSC_HF and turn off XOSC_HF.
//!
//! This operation takes approximately 50 microseconds (can be shorter if
//! RCOSC_HF already was running).
//!
//! \return None
//
//*****************************************************************************
extern void OSCHF_SwitchToRcOscTurnOffXosc( void );

//*****************************************************************************
//
//! \brief Get crystal amplitude (assuming crystal is running).
//!
//! \note This is a debug function only.
//! It is hence not recommended to call this function in normal operation.
//!
//! This function uses an on-chip ADC and peak detector for reading the crystal
//! amplitude. The measurement time is set to 4 milliseconds and this function
//! does not return before the measurement is done.
//!
//! Expected value is \ref OSCHF_DebugGetExpectedAverageCrystalAmplitude +/- 50 millivolt.
//!
//! \return Returns crystal amplitude in millivolt.
//!
//! \sa OSCHF_DebugGetExpectedAverageCrystalAmplitude()
//
//*****************************************************************************
extern uint32_t OSCHF_DebugGetCrystalAmplitude( void );

//*****************************************************************************
//
//! \brief Get the expected average crystal amplitude.
//!
//! \note This is a debug function only.
//! It is hence not recommended to call this function in normal operation.
//!
//! This function read the configured high and low thresholds and returns
//! the mean value converted to millivolt.
//!
//! \return Returns expected average crystal amplitude in millivolt.
//!
//! \sa OSCHF_DebugGetCrystalAmplitude()
//
//*****************************************************************************
extern uint32_t OSCHF_DebugGetExpectedAverageCrystalAmplitude( void );

//*****************************************************************************
//
//! \brief Data structure for experimental HPOSC polynomials calculation.
//!
//! The structure of the meas_1, meas_2 and meas_3 parameter is
//! as defined in FCFG1_O_HPOSC_MEAS_1, 2 and 3.
//!
//! \sa OSC_HPOSC_Debug_InitFreqOffsetParams()
//
//*****************************************************************************
typedef struct {
   uint32_t    meas_1   ; //!< Measurement set 1 (typically at room temp)
   uint32_t    meas_2   ; //!< Measurement set 2 (typically at high temp)
   uint32_t    meas_3   ; //!< Measurement set 3 (typically at low temp)
   int32_t     offsetD1 ; //!< Offset to measurement set 1
   int32_t     offsetD2 ; //!< Offset to measurement set 2
   int32_t     offsetD3 ; //!< Offset to measurement set 3
   int32_t     polyP3   ; //!< The P3 polynomial
} HposcDebugData_t;

//*****************************************************************************
//
//! \brief Debug function to calculate the HPOSC polynomials for experimental data sets.
//!
//! \param pDebugData pointer to the input data collected in \ref HposcDebugData_t
//!
//! \return None
//!
//! \sa OSC_HPOSCInitializeFrequencyOffsetParameters()
//
//*****************************************************************************
extern void OSC_HPOSC_Debug_InitFreqOffsetParams( HposcDebugData_t * pDebugData );

//*****************************************************************************
//
//! \brief HPOSC initialization function. Must always be called before using HPOSC.
//!
//! Calculates the fitting curve parameters (polynomials) to used by the
//! HPOSC temperature compensation.
//!
//! \return None
//!
//! \sa OSC_HPOSC_Debug_InitFreqOffsetParams()
//
//*****************************************************************************
extern void OSC_HPOSCInitializeFrequencyOffsetParameters( void );

//*****************************************************************************
//
//! \brief Calculate the temperature dependent relative frequency offset of HPOSC
//!
//! The HPOSC (High Precision Oscillator) frequency will vary slightly with chip temperature.
//! The frequency offset from the nominal value can be predicted based on
//! second order linear interpolation using coefficients measured in chip
//! production and stored as factory configuration parameters.
//!
//! This function calculates the relative frequency offset, defined as:
//! <pre>
//!     F_HPOSC = F_nom * (1 + d/(2^22))
//! </pre>
//! where
//! -   F_HPOSC is the current HPOSC frequency.
//! -   F_nom is the nominal oscillator frequency, assumed to be 48.000 MHz.
//! -   d is the relative frequency offset (the value returned).
//!
//! By knowing the relative frequency offset it is then possible to compensate
//! any timing related values accordingly.
//!
//! \param tempDegC is the chip temperature in degrees Celsius. Use the
//! function \ref AONBatMonTemperatureGetDegC() to get current chip temperature.
//!
//! \return Returns the relative frequency offset parameter d.
//!
//! \sa OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert(), AONBatMonTemperatureGetDegC()
//
//*****************************************************************************
extern int32_t OSC_HPOSCRelativeFrequencyOffsetGet( int32_t tempDegC );

//*****************************************************************************
//
//! \brief Adjust the XOSC HF cap array relative to the factory setting
//!
//! The cap array factory setting (FCFG) can be converted to a number in the range 0 - 63.
//! Both this function and the customer configuration (CCFG) setting can apply a delta to the FCFG setting.
//! The CCFG setting is automatically applied at boot time (See ../startup_files/ccfg.c).
//! Calling this function will discard the CCFG setting and adjust relative to the FCFG setting.
//!
//! \note Adjusted value will not take effect before XOSC_HF is stopped and restarted
//!
//! \param capArrDelta specifies number of step to adjust the cap array relative to the factory setting.
//!
//! \return None
//
//*****************************************************************************
extern void OSC_AdjustXoscHfCapArray( int32_t capArrDelta );

//*****************************************************************************
//
//! \brief Converts the relative frequency offset of HPOSC to the RF Core parameter format.
//!
//! The HPOSC (High Precision Oscillator) clock is used by the RF Core.
//! To compensate for a frequency offset in the frequency of the clock source,
//! a frequency offset parameter can be provided as part of the radio configuration
//! override setting list to enable compensation of the RF synthesizer frequency,
//! symbol timing, and radio timer to still achieve correct frequencies.
//!
//! The RF Core takes a relative frequency offset parameter defined differently
//! compared to the relative frequency offset parameter returned from function
//! \ref OSC_HPOSCRelativeFrequencyOffsetGet() and thus needs to be converted:
//! <pre>
//!     F_nom = F_HPOSC * (1 + RfCoreRelFreqOffset/(2^22))
//! </pre>
//! where
//! -   F_nom is the nominal oscillator frequency, assumed to be 48.000 MHz.
//! -   F_HPOSC is the current HPOSC frequency.
//! -   RfCoreRelFreqOffset is the relative frequency offset in the "RF Core" format (the value returned).
//!
//! \param HPOSC_RelFreqOffset is the relative frequency offset parameter d returned from \ref OSC_HPOSCRelativeFrequencyOffsetGet()
//!
//! \return Returns the relative frequency offset in RF Core format.
//!
//! \sa OSC_HPOSCRelativeFrequencyOffsetGet()
//
//*****************************************************************************
extern int16_t OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert( int32_t HPOSC_RelFreqOffset );

//*****************************************************************************
//
//! \brief Compensate the RTC increment based on the relative frequency offset of HPOSC
//!
//! The HPOSC (High Precision Oscillator) frequency will vary slightly with chip temperature.
//! This variation forces the RTC increment to be compensated if SCLK_LF is configured
//! to be derived from the HF clock of HPOSC.
//! This function must only be called if SCLK_LF is configured to be derived from
//! the HF clock of HPOSC. The status of this configuration can be determined
//! by calling the \ref OSC_IsHPOSCEnabledWithHfDerivedLfClock() function.
//!
//! This function first calculates the HPOSC frequency, defined as:
//! <pre>
//!     F_HPOSC = F_nom * (1 + d/(2^22))
//! </pre>
//! where
//! -   F_HPOSC is the current HPOSC frequency.
//! -   F_nom is the nominal oscillator frequency, assumed to be 48.000 MHz.
//! -   d is the relative frequency offset given by the input argument relFreqOffset.
//! Then the SCLK_LF frequency is calculated, defined as:
//! <pre>
//!     F_SCLK_LF = F_HPOSC / 1536
//! </pre>
//! Then the RTC increment SUBSECINC is calculated, defined as;
//! <pre>
//!     SUBSECINC = (2^38) / F_SCLK_LF
//! </pre>
//! Finally the RTC module is updated with the calculated SUBSECINC value.
//!
//! \param relFreqOffset is the relative frequency offset parameter d returned from \ref OSC_HPOSCRelativeFrequencyOffsetGet()
//!
//! \return None
//!
//
//*****************************************************************************
extern void OSC_HPOSCRtcCompensate( int32_t relFreqOffset );

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_OSCClockSourceSet
        #undef  OSCClockSourceSet
        #define OSCClockSourceSet               ROM_OSCClockSourceSet
    #endif
    #ifdef ROM_OSCClockSourceGet
        #undef  OSCClockSourceGet
        #define OSCClockSourceGet               ROM_OSCClockSourceGet
    #endif
    #ifdef ROM_OSCHF_GetStartupTime
        #undef  OSCHF_GetStartupTime
        #define OSCHF_GetStartupTime            ROM_OSCHF_GetStartupTime
    #endif
    #ifdef ROM_OSCHF_TurnOnXosc
        #undef  OSCHF_TurnOnXosc
        #define OSCHF_TurnOnXosc                ROM_OSCHF_TurnOnXosc
    #endif
    #ifdef ROM_OSCHF_AttemptToSwitchToXosc
        #undef  OSCHF_AttemptToSwitchToXosc
        #define OSCHF_AttemptToSwitchToXosc     ROM_OSCHF_AttemptToSwitchToXosc
    #endif
    #ifdef ROM_OSCHF_SwitchToRcOscTurnOffXosc
        #undef  OSCHF_SwitchToRcOscTurnOffXosc
        #define OSCHF_SwitchToRcOscTurnOffXosc  ROM_OSCHF_SwitchToRcOscTurnOffXosc
    #endif
    #ifdef ROM_OSCHF_DebugGetCrystalAmplitude
        #undef  OSCHF_DebugGetCrystalAmplitude
        #define OSCHF_DebugGetCrystalAmplitude  ROM_OSCHF_DebugGetCrystalAmplitude
    #endif
    #ifdef ROM_OSCHF_DebugGetExpectedAverageCrystalAmplitude
        #undef  OSCHF_DebugGetExpectedAverageCrystalAmplitude
        #define OSCHF_DebugGetExpectedAverageCrystalAmplitude ROM_OSCHF_DebugGetExpectedAverageCrystalAmplitude
    #endif
    #ifdef ROM_OSC_HPOSC_Debug_InitFreqOffsetParams
        #undef  OSC_HPOSC_Debug_InitFreqOffsetParams
        #define OSC_HPOSC_Debug_InitFreqOffsetParams ROM_OSC_HPOSC_Debug_InitFreqOffsetParams
    #endif
    #ifdef ROM_OSC_HPOSCInitializeFrequencyOffsetParameters
        #undef  OSC_HPOSCInitializeFrequencyOffsetParameters
        #define OSC_HPOSCInitializeFrequencyOffsetParameters ROM_OSC_HPOSCInitializeFrequencyOffsetParameters
    #endif
    #ifdef ROM_OSC_HPOSCRelativeFrequencyOffsetGet
        #undef  OSC_HPOSCRelativeFrequencyOffsetGet
        #define OSC_HPOSCRelativeFrequencyOffsetGet ROM_OSC_HPOSCRelativeFrequencyOffsetGet
    #endif
    #ifdef ROM_OSC_AdjustXoscHfCapArray
        #undef  OSC_AdjustXoscHfCapArray
        #define OSC_AdjustXoscHfCapArray        ROM_OSC_AdjustXoscHfCapArray
    #endif
    #ifdef ROM_OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert
        #undef  OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert
        #define OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert ROM_OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert
    #endif
    #ifdef ROM_OSC_HPOSCRtcCompensate
        #undef  OSC_HPOSCRtcCompensate
        #define OSC_HPOSCRtcCompensate          ROM_OSC_HPOSCRtcCompensate
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

#endif // __OSC_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
