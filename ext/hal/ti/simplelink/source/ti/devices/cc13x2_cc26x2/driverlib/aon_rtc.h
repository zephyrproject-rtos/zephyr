/******************************************************************************
*  Filename:       aon_rtc.h
*  Revised:        2017-08-16 15:13:43 +0200 (Wed, 16 Aug 2017)
*  Revision:       49593
*
*  Description:    Defines and prototypes for the AON RTC
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
//! \addtogroup aonrtc_api
//! @{
//
//*****************************************************************************

#ifndef __AON_RTC_H__
#define __AON_RTC_H__

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
#include "../inc/hw_aon_rtc.h"
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
    #define AONRTCCurrent64BitValueGet      NOROM_AONRTCCurrent64BitValueGet
#endif

//*****************************************************************************
//
// Values that can be passed to most of the AON_RTC APIs as the ui32Channel
// parameter.
//
//*****************************************************************************
#define AON_RTC_CH_NONE            0x0 // RTC No channel
#define AON_RTC_CH0                0x1 // RTC Channel 0
#define AON_RTC_CH1                0x2 // RTC Channel 1
#define AON_RTC_CH2                0x4 // RTC Channel 2
#define AON_RTC_ACTIVE             0x8 // RTC Active

//*****************************************************************************
//
// Values that can be passed to AONRTCConfigDelay as the ui32Delay parameter.
//
//*****************************************************************************
#define AON_RTC_CONFIG_DELAY_NODELAY 0 // NO DELAY
#define AON_RTC_CONFIG_DELAY_1       1 // Delay of   1 clk cycle
#define AON_RTC_CONFIG_DELAY_2       2 // Delay of   2 clk cycles
#define AON_RTC_CONFIG_DELAY_4       3 // Delay of   4 clk cycles
#define AON_RTC_CONFIG_DELAY_8       4 // Delay of   8 clk cycles
#define AON_RTC_CONFIG_DELAY_16      5 // Delay of  16 clk cycles
#define AON_RTC_CONFIG_DELAY_32      6 // Delay of  32 clk cycles
#define AON_RTC_CONFIG_DELAY_48      7 // Delay of  48 clk cycles
#define AON_RTC_CONFIG_DELAY_64      8 // Delay of  64 clk cycles
#define AON_RTC_CONFIG_DELAY_80      9 // Delay of  80 clk cycles
#define AON_RTC_CONFIG_DELAY_96     10 // Delay of  96 clk cycles
#define AON_RTC_CONFIG_DELAY_112    11 // Delay of 112 clk cycles
#define AON_RTC_CONFIG_DELAY_128    12 // Delay of 128 clk cycles
#define AON_RTC_CONFIG_DELAY_144    13 // Delay of 144 clk cycles

//*****************************************************************************
//
// Values that can be passed to AONRTCSetModeCH1 as the ui32Mode
// parameter.
//
//*****************************************************************************
#define AON_RTC_MODE_CH1_CAPTURE     1 // Capture mode
#define AON_RTC_MODE_CH1_COMPARE     0 // Compare Mode

//*****************************************************************************
//
// Values that can be passed to AONRTCSetModeCH2 as the ui32Mode
// parameter.
//
//*****************************************************************************
#define AON_RTC_MODE_CH2_CONTINUOUS    1 // Continuous mode
#define AON_RTC_MODE_CH2_NORMALCOMPARE 0 // Normal compare mode

//*****************************************************************************
//
// Mutliplication factor for converting from seconds to corresponding time in
// the "CompareValue" format.
// The factor correspond to the compare value format described in the registers
//   \ref AON_RTC_O_CH0CMP, \ref AON_RTC_O_CH1CMP and \ref AON_RTC_O_CH2CMP.
// Example1:
//   4 milliseconds in CompareValue format can be written like this:
//   ((uint32_t)( 0.004 * FACTOR_SEC_TO_COMP_VAL_FORMAT ))
// Example2:
//   4 seconds in CompareValue format can be written like this:
//   ( 4 * FACTOR_SEC_TO_COMP_VAL_FORMAT )
//
//*****************************************************************************
#define FACTOR_SEC_TO_COMP_VAL_FORMAT   0x00010000

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Enable the RTC.
//!
//! Enable the AON Real Time Clock.
//!
//! \note Event generation for each of the three channels must also be enabled
//!  using the function AONRTCChannelEnable().
//!
//! \return None
//!
//! \sa AONRTCChannelEnable()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCEnable(void)
{
    // Enable RTC.
    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_EN_BITN) = 1;
}

//*****************************************************************************
//
//! \brief Disable the RTC.
//!
//! Disable the AON Real Time Clock.
//!
//! \note Event generation for each of the three channels can also be disabled
//! using the function AONRTCChannelDisable().
//!
//! \return None
//!
//! \sa AONRTCChannelDisable()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCDisable(void)
{
    // Disable RTC
    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_EN_BITN) = 0;
}

//*****************************************************************************
//
//! \brief Reset the RTC.
//!
//! Reset the AON Real Time Clock.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCReset(void)
{
    // Reset RTC.
    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RESET_BITN) = 1;
}

//*****************************************************************************
//
//! \brief Check if the RTC is active (enabled).
//!
//! \return Returns the status of the RTC.
//! - false : RTC is disabled
//! - true  : RTC is enabled
//
//*****************************************************************************
__STATIC_INLINE bool
AONRTCActive(void)
{
    // Read if RTC is enabled
    return(HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_EN_BITN));
}

//*****************************************************************************
//
//! \brief Check if an RTC channel is active (enabled).
//!
//! \param ui32Channel specifies the RTC channel to check status of.
//! Parameter must be one (and only one) of the following:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//!
//! \return Returns the status of the requested channel:
//! - false : Channel is disabled
//! - true  : Channel is enabled
//
//*****************************************************************************
__STATIC_INLINE bool
AONRTCChannelActive(uint32_t ui32Channel)
{
    uint32_t uint32Status = 0;

    if(ui32Channel & AON_RTC_CH0)
    {
        uint32Status = HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH0_EN_BITN);
    }

    if(ui32Channel & AON_RTC_CH1)
    {
        uint32Status = HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH1_EN_BITN);
    }

    if(ui32Channel & AON_RTC_CH2)
    {
        uint32Status = HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH2_EN_BITN);
    }

    return(uint32Status);
}

//*****************************************************************************
//
//! \brief Configure Event Delay for the RTC.
//!
//! Each event from the three individual channels can generate a delayed
//! event. The delay time for these events is set using this function.
//! The delay is measured in clock cycles.
//!
//! \note There is only one delay setting shared for all three channels.
//!
//! \param ui32Delay specifies the delay time for delayed events.
//! Parameter must be one of the following:
//! - \ref AON_RTC_CONFIG_DELAY_NODELAY
//! - \ref AON_RTC_CONFIG_DELAY_1
//! - \ref AON_RTC_CONFIG_DELAY_2
//! - \ref AON_RTC_CONFIG_DELAY_4
//! - \ref AON_RTC_CONFIG_DELAY_8
//! - \ref AON_RTC_CONFIG_DELAY_16
//! - \ref AON_RTC_CONFIG_DELAY_32
//! - \ref AON_RTC_CONFIG_DELAY_48
//! - \ref AON_RTC_CONFIG_DELAY_64
//! - \ref AON_RTC_CONFIG_DELAY_80
//! - \ref AON_RTC_CONFIG_DELAY_96
//! - \ref AON_RTC_CONFIG_DELAY_112
//! - \ref AON_RTC_CONFIG_DELAY_128
//! - \ref AON_RTC_CONFIG_DELAY_144
//!
//! \return None.
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCDelayConfig(uint32_t ui32Delay)
{
    uint32_t ui32Cfg;

    // Check the arguments.
    ASSERT(ui32Delay <= AON_RTC_CONFIG_DELAY_144);


    ui32Cfg =  HWREG(AON_RTC_BASE + AON_RTC_O_CTL);
    ui32Cfg &= ~(AON_RTC_CTL_EV_DELAY_M);
    ui32Cfg |= (ui32Delay << AON_RTC_CTL_EV_DELAY_S);

    HWREG(AON_RTC_BASE + AON_RTC_O_CTL) = ui32Cfg;
}

//*****************************************************************************
//
//! \brief Configure the source of the combined event.
//!
//! A combined delayed event can be generated from a combination of the three
//! delayed events. Delayed events form the specified channels are OR'ed
//! together to generate the combined event.
//!
//! \param ui32Channels specifies the channels that are to be used for
//! generating the combined event.
//! The parameter must be the bitwise OR of any of the following:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//! - \ref AON_RTC_CH_NONE
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCCombinedEventConfig(uint32_t ui32Channels)
{
    uint32_t ui32Cfg;

    // Check the arguments.
    ASSERT( (ui32Channels & (AON_RTC_CH0 | AON_RTC_CH1 | AON_RTC_CH2)) ||
            (ui32Channels == AON_RTC_CH_NONE) );

    ui32Cfg =  HWREG(AON_RTC_BASE + AON_RTC_O_CTL);
    ui32Cfg &= ~(AON_RTC_CTL_COMB_EV_MASK_M);
    ui32Cfg |= (ui32Channels << AON_RTC_CTL_COMB_EV_MASK_S);

    HWREG(AON_RTC_BASE + AON_RTC_O_CTL) = ui32Cfg;
}

//*****************************************************************************
//
//! \brief Clear event from a specified channel.
//!
//! In case of an active event from the specified channel, the event
//! will be cleared (de-asserted).
//!
//! \param ui32Channel clears the event from one or more RTC channels:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCEventClear(uint32_t ui32Channel)
{
    // Check the arguments.
    ASSERT((ui32Channel == AON_RTC_CH0) ||
           (ui32Channel == AON_RTC_CH1) ||
           (ui32Channel == AON_RTC_CH2));

    if(ui32Channel & AON_RTC_CH0)
    {
        HWREG(AON_RTC_BASE + AON_RTC_O_EVFLAGS) = AON_RTC_EVFLAGS_CH0;
    }

    if(ui32Channel & AON_RTC_CH1)
    {
        HWREG(AON_RTC_BASE + AON_RTC_O_EVFLAGS) = AON_RTC_EVFLAGS_CH1;
    }

    if(ui32Channel & AON_RTC_CH2)
    {
        HWREG(AON_RTC_BASE + AON_RTC_O_EVFLAGS) = AON_RTC_EVFLAGS_CH2;
    }
}

//*****************************************************************************
//
//! \brief Get event status for a specified channel.
//!
//! In case of an active event from the specified channel,
//! this call will return \c true otherwise \c false.
//!
//! \param ui32Channel specifies the channel from which to query the event state.
//! The parameter must be one (and only one) of the following:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//!
//! \return Returns \c true if an event has occurred for the given channel,
//! otherwise \c false.
//
//*****************************************************************************
__STATIC_INLINE bool
AONRTCEventGet(uint32_t ui32Channel)
{
    uint32_t uint32Event = 0;

    // Check the arguments.
    ASSERT((ui32Channel == AON_RTC_CH0) ||
           (ui32Channel == AON_RTC_CH1) ||
           (ui32Channel == AON_RTC_CH2));

    if(ui32Channel & AON_RTC_CH0)
    {
        uint32Event = HWREGBITW(AON_RTC_BASE + AON_RTC_O_EVFLAGS, AON_RTC_EVFLAGS_CH0_BITN);
    }

    if(ui32Channel & AON_RTC_CH1)
    {
        uint32Event = HWREGBITW(AON_RTC_BASE + AON_RTC_O_EVFLAGS, AON_RTC_EVFLAGS_CH1_BITN);
    }

    if(ui32Channel & AON_RTC_CH2)
    {
        uint32Event = HWREGBITW(AON_RTC_BASE + AON_RTC_O_EVFLAGS, AON_RTC_EVFLAGS_CH2_BITN);
    }

    return(uint32Event);
}

//*****************************************************************************
//
//! \brief Get integer part (seconds) of RTC free-running timer.
//!
//! Get the value in seconds of RTC free-running timer, i.e. the integer part.
//! The fractional part is returned from a call to AONRTCFractionGet().
//!
//! \note It is recommended to use \ref AONRTCCurrentCompareValueGet() instead
//! of this function if the <16.16> format is sufficient.
//!
//! \note To read a consistent pair of integer and fractional parts,
//! \ref AONRTCSecGet() must be called first to trigger latching of the
//! fractional part, which is then read by \ref AONRTCFractionGet(). Interrupts
//! must be disabled to ensure that these operations are performed atomically.
//!
//! \return Returns the integer part of RTC free running timer.
//!
//! \sa \ref AONRTCFractionGet() \ref AONRTCCurrentCompareValueGet()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCSecGet(void)
{
    // The following read gets the seconds, but also latches the fractional
    // part.
    return(HWREG(AON_RTC_BASE + AON_RTC_O_SEC));
}

//*****************************************************************************
//
//! \brief Get fractional part (sub-seconds) of RTC free-running timer.
//!
//! Get the value of the fractional part of RTC free-running timer, i.e. the
//! sub-second part.
//!
//! \note It is recommended to use \ref AONRTCCurrentCompareValueGet() instead
//! of this function if the <16.16> format is sufficient.
//!
//! \note To read a consistent pair of integer and fractional parts,
//! \ref AONRTCSecGet() must be called first to trigger latching of the
//! fractional part, which is then read by \ref AONRTCFractionGet(). Interrupts
//! must be disabled to ensure that these operations are performed atomically.
//!
//! \return Returns the fractional part of RTC free running timer.
//!
//! \sa \ref AONRTCSecGet() \ref AONRTCCurrentCompareValueGet()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCFractionGet(void)
{
    // Note1: It is recommended to use AON RTCCurrentCompareValueGet() instead
    //        of this function if the <16.16> format is sufficient.
    // Note2: AONRTCSecGet() must be called before this function to get a
    //        consistent reading.
    // Note3: Interrupts must be disabled between the call to AONRTCSecGet() and this
    //        call since there are interrupt functions that reads AON_RTC_O_SEC
    return(HWREG(AON_RTC_BASE + AON_RTC_O_SUBSEC));
}

//*****************************************************************************
//
//! \brief Get the sub second increment of the RTC.
//!
//! Get the value of the sub-second increment which is added to the RTC
//! absolute time on every clock tick.
//!
//! \note For a precise and temperature independent LF clock (e.g. an LF XTAL)
//! this value would stay the same across temperature. For temperatue
//! dependent clock sources like an RC oscillator, this value will change
//! over time if the application includes functionality for doing temperature
//! compensation of the RTC clock source. The default value corresponds to a
//! LF clock frequency of exactly 32.768 kHz.
//!
//! \return Returns the sub-second increment of the RTC added to the overall
//! value on every RTC clock tick.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCSubSecIncrGet(void)
{
    return(HWREG(AON_RTC_BASE + AON_RTC_O_SUBSECINC));
}

//*****************************************************************************
//
//! \brief Set operational mode of channel 1.
//!
//! Set the operational mode of channel 1. It can be capture or compare mode.
//! In capture mode, an external event causes the value of the free running
//! counter to be stored, to remember the time of the event.
//!
//! \note The default mode is compare.
//!
//! \param ui32Mode specifies the mode for channel 1.
//! The parameter must be one of the following:
//! - \ref AON_RTC_MODE_CH1_CAPTURE
//! - \ref AON_RTC_MODE_CH1_COMPARE
//!
//! \return None
//!
//!  \sa AONRTCModeCh1Get()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCModeCh1Set(uint32_t ui32Mode)
{
    // Check the arguments.
    ASSERT((ui32Mode == AON_RTC_MODE_CH1_CAPTURE) ||
           (ui32Mode == AON_RTC_MODE_CH1_COMPARE));

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH1_CAPT_EN_BITN) = ui32Mode;
}

//*****************************************************************************
//
//! \brief Get operational mode of channel 1.
//!
//! Get the operational mode of channel 1. It can be capture or compare mode.
//! In capture mode, an external event causes the value of the free running
//! counter to be stored, to remember the time of the event.
//!
//! \return Returns the operational mode of channel 1, one of:
//! - \ref AON_RTC_MODE_CH1_CAPTURE
//! - \ref AON_RTC_MODE_CH1_COMPARE
//!
//! \sa AONRTCModeCh1Set()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCModeCh1Get(void)
{
    return(HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH1_CAPT_EN_BITN));
}

//*****************************************************************************
//
//! \brief Set operational mode of channel 2.
//!
//! Set the operational mode of channel 2. It can be in continuous compare
//! mode or normal compare mode.
//! In continuous mode, a value is automatically incremented to the channel 2
//! compare register, upon a channel 2 compare event. This allows channel 2 to
//! generate a series of completely equidistant events.
//! The increment value is set by the AONRTCIncValueCh2Set() call.
//!
//! \note The default mode is normal compare.
//!
//! \param ui32Mode specifies the mode for channel 2.
//! The parameter must be one of the following:
//! - \ref AON_RTC_MODE_CH2_CONTINUOUS
//! - \ref AON_RTC_MODE_CH2_NORMALCOMPARE
//!
//! \return None
//!
//! \sa AONRTCIncValueCh2Set(), AONRTCIncValueCh2Get()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCModeCh2Set(uint32_t ui32Mode)
{
    // Check the arguments.
    ASSERT((ui32Mode == AON_RTC_MODE_CH2_CONTINUOUS) ||
           (ui32Mode == AON_RTC_MODE_CH2_NORMALCOMPARE));

    HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH2_CONT_EN_BITN) = ui32Mode;
}

//*****************************************************************************
//
//! \brief Get operational mode of channel 2.
//!
//! Get the operational mode of channel 2. It can be in continuous compare
//! mode or normal compare mode.
//! In continuous mode, a value is automatically incremented to the channel 2
//! compare register, upon a channel 2 compare event. This allows channel 2 to
//! generate a series of completely equidistant events.
//! The increment value is set by the AONRTCIncValueCh2Set() call.
//!
//! \return Returns the operational mode of channel 2, i.e. one of:
//! - \ref AON_RTC_MODE_CH2_CONTINUOUS
//! - \ref AON_RTC_MODE_CH2_NORMALCOMPARE
//!
//! \sa AONRTCIncValueCh2Set(), AONRTCIncValueCh2Get()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCModeCh2Get(void)
{
    return(HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH2_CONT_EN_BITN));
}

//*****************************************************************************
//
//! \brief Enable event operation for the specified channel.
//!
//! Enable the event generation for the specified channel.
//!
//! \note The RTC free running clock  must also be enabled globally using the
//! AONRTCEnable() call.
//!
//! \param ui32Channel specifies one or more channels to enable:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//!
//! \return None
//!
//! \sa AONRTCEnable()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCChannelEnable(uint32_t ui32Channel)
{
    // Check the arguments.
    ASSERT((ui32Channel == AON_RTC_CH0) ||
           (ui32Channel == AON_RTC_CH1) ||
           (ui32Channel == AON_RTC_CH2));

    if(ui32Channel & AON_RTC_CH0)
    {
        HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH0_EN_BITN) = 1;
    }

    if(ui32Channel & AON_RTC_CH1)
    {
        HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH1_EN_BITN) = 1;
    }

    if(ui32Channel & AON_RTC_CH2)
    {
        HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH2_EN_BITN) = 1;
    }
}

//*****************************************************************************
//
//! \brief Disable event operation for the specified channel.
//!
//! Disable the event generation for the specified channel.
//!
//! \note The RTC free running clock  can also be disabled globally using the
//! AONRTCDisable() call.
//!
//! \param ui32Channel specifies one or more channels to disable:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//!
//! \return None
//!
//! \sa AONRTCDisable()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCChannelDisable(uint32_t ui32Channel)
{
    // Check the arguments.
    ASSERT((ui32Channel == AON_RTC_CH0) ||
           (ui32Channel == AON_RTC_CH1) ||
           (ui32Channel == AON_RTC_CH2));

    if(ui32Channel & AON_RTC_CH0)
    {
        HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH0_EN_BITN) = 0;
    }

    if(ui32Channel & AON_RTC_CH1)
    {
        HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH1_EN_BITN) = 0;
    }

    if(ui32Channel & AON_RTC_CH2)
    {
        HWREGBITW(AON_RTC_BASE + AON_RTC_O_CHCTL, AON_RTC_CHCTL_CH2_EN_BITN) = 0;
    }
}

//*****************************************************************************
//
//! \brief Set the compare value for the given channel.
//!
//! Set compare value for the specified channel.
//!
//! The format of the compare value is a 16 bit integer and 16 bit fractional
//! format <16 sec.16 subsec>. The current value of the RTC counter
//! can be retrieved in a format compatible to the compare register using
//! \ref AONRTCCurrentCompareValueGet()
//!
//! \param ui32Channel specifies one or more channels to set compare value for:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//! \param ui32CompValue is the compare value to set for the specified channel.
//! - Format: <16 sec.16 subsec>
//!
//! \return None
//!
//! \sa AONRTCCurrentCompareValueGet()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCCompareValueSet(uint32_t ui32Channel, uint32_t ui32CompValue)
{
    // Check the arguments.
    ASSERT((ui32Channel == AON_RTC_CH0) ||
           (ui32Channel == AON_RTC_CH1) ||
           (ui32Channel == AON_RTC_CH2));

    if(ui32Channel & AON_RTC_CH0)
    {
        HWREG(AON_RTC_BASE + AON_RTC_O_CH0CMP) = ui32CompValue;
    }

    if(ui32Channel & AON_RTC_CH1)
    {
        HWREG(AON_RTC_BASE + AON_RTC_O_CH1CMP) = ui32CompValue;
    }

    if(ui32Channel & AON_RTC_CH2)
    {
        HWREG(AON_RTC_BASE + AON_RTC_O_CH2CMP) = ui32CompValue;
    }
}

//*****************************************************************************
//
//! \brief Get the compare value for the given channel.
//!
//! Get compare value for the specified channel.
//!
//! \param ui32Channel specifies a channel.
//! The parameter must be one (and only one) of the following:
//! - \ref AON_RTC_CH0
//! - \ref AON_RTC_CH1
//! - \ref AON_RTC_CH2
//!
//! \return Returns the stored compare value for the given channel.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCCompareValueGet(uint32_t ui32Channel)
{
    uint32_t ui32Value = 0;

    // Check the arguments
    ASSERT((ui32Channel == AON_RTC_CH0) ||
           (ui32Channel == AON_RTC_CH1) ||
           (ui32Channel == AON_RTC_CH2));

    if(ui32Channel & AON_RTC_CH0)
    {
        ui32Value = HWREG(AON_RTC_BASE + AON_RTC_O_CH0CMP);
    }

    if(ui32Channel & AON_RTC_CH1)
    {
        ui32Value = HWREG(AON_RTC_BASE + AON_RTC_O_CH1CMP);
    }

    if(ui32Channel & AON_RTC_CH2)
    {
        ui32Value = HWREG(AON_RTC_BASE + AON_RTC_O_CH2CMP);
    }

    return(ui32Value);
}

//*****************************************************************************
//
//! \brief Get the current value of the RTC counter in a format that matches
//! RTC compare values.
//!
//! The compare value registers contains 16 integer and 16 fractional bits.
//! This function will return the current value of the RTC counter in an
//! identical format.
//!
//! \return Returns the current value of the RTC counter in a <16.16> format
//! (SEC[15:0].SUBSEC[31:16]).
//!
//! \sa \ref AONRTCCompareValueSet()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCCurrentCompareValueGet( void )
{
   return ( HWREG( AON_RTC_BASE + AON_RTC_O_TIME ));
}

//*****************************************************************************
//
//! \brief Get the current 64-bit value of the RTC counter.
//!
//! \note Reading SEC both before and after SUBSEC in order to detect if SEC
//! incremented while reading SUBSEC. If SEC incremented, we can't be sure
//! which SEC the SUBSEC belongs to, so repeating the sequence then.
//!
//! \return Returns the current value of the RTC counter in a 64-bits format
//! (SEC[31:0].SUBSEC[31:0]).
//
//*****************************************************************************
extern uint64_t AONRTCCurrent64BitValueGet(void);

//*****************************************************************************
//
//! \brief Set the channel 2 increment value when operating in continuous mode.
//!
//! Set the channel 2 increment value when operating in continuous mode.
//! The specified value is automatically incremented to the channel 2 compare
//! register, upon a channel 2 compare event. This allows channel 2 to generate
//! a series of completely equidistant events.
//!
//! \param ui32IncValue is the increment value when operating in continuous mode.
//!
//! \return None
//!
//! \sa AONRTCIncValueCh2Get()
//
//*****************************************************************************
__STATIC_INLINE void
AONRTCIncValueCh2Set(uint32_t ui32IncValue)
{
    HWREG(AON_RTC_BASE + AON_RTC_O_CH2CMPINC) = ui32IncValue;
}

//*****************************************************************************
//
//! \brief Get the channel2 increment value when operating in continuous mode.
//!
//! Get the channel 2 increment value, when channel 2 is operating in
//! continuous mode.
//! This value is automatically incremented to the channel 2 compare
//! register, upon a channel 2 compare event. This allows channel 2 to
//! generate a series of completely equidistant events.
//!
//! \return Returns the channel 2 increment value when operating in continuous
//! mode.
//!
//! \sa AONRTCIncValueCh2Set()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCIncValueCh2Get(void)
{
    return(HWREG(AON_RTC_BASE + AON_RTC_O_CH2CMPINC));
}

//*****************************************************************************
//
//! \brief Get the channel 1 capture value.
//!
//! Get the channel 1 capture value.
//! The upper 16 bits of the returned value is the lower 16 bits of the
//! integer part of the RTC timer. The lower 16 bits of the returned part
//! is the upper 16 bits of the fractional part.
//!
//! \return Returns the channel 1 capture value.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONRTCCaptureValueCh1Get(void)
{
    return(HWREG(AON_RTC_BASE + AON_RTC_O_CH1CAPT));
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_AONRTCCurrent64BitValueGet
        #undef  AONRTCCurrent64BitValueGet
        #define AONRTCCurrent64BitValueGet      ROM_AONRTCCurrent64BitValueGet
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

#endif //  __AON_RTC_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
