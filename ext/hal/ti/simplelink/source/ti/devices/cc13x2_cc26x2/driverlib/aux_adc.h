/******************************************************************************
*  Filename:       aux_adc.h
*  Revised:        2018-02-07 09:45:39 +0100 (Wed, 07 Feb 2018)
*  Revision:       51437
*
*  Description:    Defines and prototypes for the AUX Analog-to-Digital
*                  Converter
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
//! \addtogroup auxadc_api
//! @{
//
//*****************************************************************************

#ifndef __AUX_ADC_H__
#define __AUX_ADC_H__

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
#include "../inc/hw_event.h"
#include "../inc/hw_adi.h"
#include "../inc/hw_adi_4_aux.h"
#include "../inc/hw_aux_anaif.h"
#include "rom.h"

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
    #define AUXADCDisable                   NOROM_AUXADCDisable
    #define AUXADCEnableAsync               NOROM_AUXADCEnableAsync
    #define AUXADCEnableSync                NOROM_AUXADCEnableSync
    #define AUXADCDisableInputScaling       NOROM_AUXADCDisableInputScaling
    #define AUXADCFlushFifo                 NOROM_AUXADCFlushFifo
    #define AUXADCReadFifo                  NOROM_AUXADCReadFifo
    #define AUXADCPopFifo                   NOROM_AUXADCPopFifo
    #define AUXADCGetAdjustmentGain         NOROM_AUXADCGetAdjustmentGain
    #define AUXADCGetAdjustmentOffset       NOROM_AUXADCGetAdjustmentOffset
    #define AUXADCValueToMicrovolts         NOROM_AUXADCValueToMicrovolts
    #define AUXADCMicrovoltsToValue         NOROM_AUXADCMicrovoltsToValue
    #define AUXADCAdjustValueForGainAndOffset NOROM_AUXADCAdjustValueForGainAndOffset
    #define AUXADCUnadjustValueForGainAndOffset NOROM_AUXADCUnadjustValueForGainAndOffset
#endif

//*****************************************************************************
//
// Defines for ADC reference sources.
//
//*****************************************************************************
#define AUXADC_REF_FIXED                    (0 << ADI_4_AUX_ADCREF0_SRC_S)
#define AUXADC_REF_VDDS_REL                 (1 << ADI_4_AUX_ADCREF0_SRC_S)

//*****************************************************************************
//
// Defines for the ADC FIFO status bits.
//
//*****************************************************************************
#define AUXADC_FIFO_EMPTY_M                 (AUX_ANAIF_ADCFIFOSTAT_EMPTY_M)
#define AUXADC_FIFO_ALMOST_FULL_M           (AUX_ANAIF_ADCFIFOSTAT_ALMOST_FULL_M)
#define AUXADC_FIFO_FULL_M                  (AUX_ANAIF_ADCFIFOSTAT_FULL_M)
#define AUXADC_FIFO_UNDERFLOW_M             (AUX_ANAIF_ADCFIFOSTAT_UNDERFLOW_M)
#define AUXADC_FIFO_OVERFLOW_M              (AUX_ANAIF_ADCFIFOSTAT_OVERFLOW_M)

//*****************************************************************************
//
// Defines for supported ADC triggers.
//
//*****************************************************************************
#define AUXADC_TRIGGER_MANUAL               (EVENT_AUXSEL0_EV_NONE)
#define AUXADC_TRIGGER_GPT0A                (EVENT_AUXSEL0_EV_GPT0A)
#define AUXADC_TRIGGER_GPT0B                (EVENT_AUXSEL0_EV_GPT0B)
#define AUXADC_TRIGGER_GPT1A                (EVENT_AUXSEL0_EV_GPT1A)
#define AUXADC_TRIGGER_GPT1B                (EVENT_AUXSEL0_EV_GPT1B)
#define AUXADC_TRIGGER_GPT2A                (EVENT_AUXSEL0_EV_GPT2A)
#define AUXADC_TRIGGER_GPT2B                (EVENT_AUXSEL0_EV_GPT2B)
#define AUXADC_TRIGGER_GPT3A                (EVENT_AUXSEL0_EV_GPT3A)
#define AUXADC_TRIGGER_GPT3B                (EVENT_AUXSEL0_EV_GPT3B)
// Additional triggers specific for cc26x2 and cc13x2 devices
#define AUXADC_TRIGGER_GPT0A_CMP            (EVENT_AUXSEL0_EV_GPT0A_CMP)
#define AUXADC_TRIGGER_GPT0B_CMP            (EVENT_AUXSEL0_EV_GPT0B_CMP)
#define AUXADC_TRIGGER_GPT1A_CMP            (EVENT_AUXSEL0_EV_GPT1A_CMP)
#define AUXADC_TRIGGER_GPT1B_CMP            (EVENT_AUXSEL0_EV_GPT1B_CMP)
#define AUXADC_TRIGGER_GPT2A_CMP            (EVENT_AUXSEL0_EV_GPT2A_CMP)
#define AUXADC_TRIGGER_GPT2B_CMP            (EVENT_AUXSEL0_EV_GPT2B_CMP)
#define AUXADC_TRIGGER_GPT3A_CMP            (EVENT_AUXSEL0_EV_GPT3A_CMP)
#define AUXADC_TRIGGER_GPT3B_CMP            (EVENT_AUXSEL0_EV_GPT3B_CMP)

//*****************************************************************************
//
// Defines for ADC sampling type for synchronous operation.
//
//*****************************************************************************
#define AUXADC_SAMPLE_TIME_2P7_US           3
#define AUXADC_SAMPLE_TIME_5P3_US           4
#define AUXADC_SAMPLE_TIME_10P6_US          5
#define AUXADC_SAMPLE_TIME_21P3_US          6
#define AUXADC_SAMPLE_TIME_42P6_US          7
#define AUXADC_SAMPLE_TIME_85P3_US          8
#define AUXADC_SAMPLE_TIME_170_US           9
#define AUXADC_SAMPLE_TIME_341_US           10
#define AUXADC_SAMPLE_TIME_682_US           11
#define AUXADC_SAMPLE_TIME_1P37_MS          12
#define AUXADC_SAMPLE_TIME_2P73_MS          13
#define AUXADC_SAMPLE_TIME_5P46_MS          14
#define AUXADC_SAMPLE_TIME_10P9_MS          15

//*****************************************************************************
//
// Equivalent voltages for fixed ADC reference, in microvolts.
//
//*****************************************************************************
#define AUXADC_FIXED_REF_VOLTAGE_NORMAL     4300000
#define AUXADC_FIXED_REF_VOLTAGE_UNSCALED   1478500


//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************


//*****************************************************************************
//
//! \brief Disables the ADC.
//!
//! This function must be called:
//! - Before re-enabling the ADC using \ref AUXADCEnableAsync() or
//!   \ref AUXADCEnableSync()
//! - Before entering system standby
//
//*****************************************************************************
extern void AUXADCDisable(void);

//*****************************************************************************
//
//! \brief Enables the ADC for asynchronous operation.
//!
//! In asynchronous operation, the ADC samples continuously between
//! conversions.
//!
//! The ADC trigger starts the conversion. Note that the first conversion may
//! be invalid if the sampling period is too short.
//!
//! ADC input scaling is enabled by default after device reset, and is also re-
//! enabled by \ref AUXADCDisable(). To disable input scaling, call
//! \ref AUXADCDisableInputScaling() before calling \ref AUXADCEnableAsync().
//!
//! \param refSource
//!     ADC reference source:
//!     - \ref AUXADC_REF_FIXED (nominally 4.3 V)
//!     - \ref AUXADC_REF_VDDS_REL (nominally VDDS)
//! \param trigger
//!     ADC conversion trigger:
//!     - \ref AUXADC_TRIGGER_MANUAL
//!     - \ref AUXADC_TRIGGER_GPT0A
//!     - \ref AUXADC_TRIGGER_GPT0B
//!     - \ref AUXADC_TRIGGER_GPT1A
//!     - \ref AUXADC_TRIGGER_GPT1B
//!     - \ref AUXADC_TRIGGER_GPT2A
//!     - \ref AUXADC_TRIGGER_GPT2B
//!     - \ref AUXADC_TRIGGER_GPT3A
//!     - \ref AUXADC_TRIGGER_GPT3B
//
//*****************************************************************************
extern void AUXADCEnableAsync(uint32_t refSource, uint32_t trigger);

//*****************************************************************************
//
//! \brief Enables the ADC for synchronous operation.
//!
//! In synchronous operation, the ADC is idle between a conversion and
//! subsequent samplings.
//!
//! The ADC trigger starts sampling with specified duration, followed by the
//! conversion. Note that the first conversion may be invalid if the initial
//! sampling period is too short.
//!
//! ADC input scaling is enabled by default after device reset, and is also re-
//! enabled by \ref AUXADCDisable(). To disable input scaling, call
//! \ref AUXADCDisableInputScaling() before calling \ref AUXADCEnableSync().
//!
//! \param refSource
//!     ADC reference source:
//!     - \ref AUXADC_REF_FIXED (nominally 4.3 V)
//!     - \ref AUXADC_REF_VDDS_REL (nominally VDDS)
//! \param sampleTime
//!     ADC sampling time:
//!     - \ref AUXADC_SAMPLE_TIME_2P7_US
//!     - \ref AUXADC_SAMPLE_TIME_5P3_US
//!     - \ref AUXADC_SAMPLE_TIME_10P6_US
//!     - \ref AUXADC_SAMPLE_TIME_21P3_US
//!     - \ref AUXADC_SAMPLE_TIME_42P6_US
//!     - \ref AUXADC_SAMPLE_TIME_85P3_US
//!     - \ref AUXADC_SAMPLE_TIME_170_US
//!     - \ref AUXADC_SAMPLE_TIME_341_US
//!     - \ref AUXADC_SAMPLE_TIME_682_US
//!     - \ref AUXADC_SAMPLE_TIME_1P37_MS
//!     - \ref AUXADC_SAMPLE_TIME_2P73_MS
//!     - \ref AUXADC_SAMPLE_TIME_5P46_MS
//!     - \ref AUXADC_SAMPLE_TIME_10P9_MS
//! \param trigger
//!     ADC conversion trigger:
//!     - \ref AUXADC_TRIGGER_MANUAL
//!     - \ref AUXADC_TRIGGER_GPT0A
//!     - \ref AUXADC_TRIGGER_GPT0B
//!     - \ref AUXADC_TRIGGER_GPT1A
//!     - \ref AUXADC_TRIGGER_GPT1B
//!     - \ref AUXADC_TRIGGER_GPT2A
//!     - \ref AUXADC_TRIGGER_GPT2B
//!     - \ref AUXADC_TRIGGER_GPT3A
//!     - \ref AUXADC_TRIGGER_GPT3B
//
//*****************************************************************************
extern void AUXADCEnableSync(uint32_t refSource, uint32_t sampleTime, uint32_t trigger);

//*****************************************************************************
//
//! \brief Disables scaling of the ADC input.
//!
//! By default, the ADC operates internally on a version of the input signal
//! that has been scaled down by a factor <tt>1408 / 4095</tt>. This function
//! disables that scaling, allowing for a trade-off between dynamic range and
//! and resolution.
//!
//! \note This function must only be called while the ADC is disabled, before
//!       calling \ref AUXADCEnableSync() or \ref AUXADCEnableAsync().
//! \note Different input maximum ratings apply when input scaling is disabled.
//!       Violating these may damage the device.
//
//*****************************************************************************
extern void AUXADCDisableInputScaling(void);

//*****************************************************************************
//
//! \brief Flushes the ADC FIFO.
//!
//! This empties the FIFO and clears the underflow/overflow flags.
//!
//! Note: This function must only be called while the ADC is enabled.
//
//*****************************************************************************
extern void AUXADCFlushFifo(void);

//*****************************************************************************
//
//! \brief Generates a single manual ADC trigger.
//!
//! For synchronous mode, the trigger starts sampling followed by conversion.
//! For asynchronous mode, the trigger starts conversion.
//
//*****************************************************************************
__STATIC_INLINE void
AUXADCGenManualTrigger(void)
{
    HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCTRIG) = 0;
}

//*****************************************************************************
//
//! \brief Returns flags indicating the status of the ADC FIFO.
//!
//! The flags indicate FIFO empty, full and almost full, and whether
//! overflow/underflow has occurred.
//!
//! \return
//!     A combination (bitwise OR) of the following flags:
//!     - \ref AUXADC_FIFO_EMPTY_M
//!     - \ref AUXADC_FIFO_ALMOST_FULL_M
//!     - \ref AUXADC_FIFO_FULL_M
//!     - \ref AUXADC_FIFO_UNDERFLOW_M
//!     - \ref AUXADC_FIFO_OVERFLOW_M
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AUXADCGetFifoStatus(void)
{
    return HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCFIFOSTAT);
}

//*****************************************************************************
//
//! \brief Waits for and returns the first sample in the ADC FIFO.
//!
//! This function waits until there is at least one sample in the ADC FIFO. It
//! then pops and returns the first sample from the FIFO.
//!
//! \note This procedure will deadlock if called without setting up ADC trigger
//!       generation in advance. The trigger can either be manual or periodical
//!       (using a GPT).
//!
//! \return The first (12-bit) sample from the ADC FIFO
//
//*****************************************************************************
extern uint32_t AUXADCReadFifo(void);

//*****************************************************************************
//
//! \brief Returns the first sample in the ADC FIFO, without waiting.
//!
//! This function does not wait, and must only be called when there is at least
//! one sample in the ADC FIFO. Otherwise the call will generate FIFO underflow
//! (\ref AUXADC_FIFO_UNDERFLOW_M).
//!
//! \return The first (12-bit) sample from the ADC FIFO, or an undefined value
//!         if the FIFO is empty
//
//*****************************************************************************
extern uint32_t AUXADCPopFifo(void);

//*****************************************************************************
//
//! \brief Selects internal or external input for the ADC.
//!
//! Note that calling this function also selects the same input for AUX_COMPB.
//!
//! \param input
//!     Internal/external input selection:
//!     - \ref ADC_COMPB_IN_DCOUPL
//!     - \ref ADC_COMPB_IN_VSS
//!     - \ref ADC_COMPB_IN_VDDS
//!     - \ref ADC_COMPB_IN_AUXIO7
//!     - \ref ADC_COMPB_IN_AUXIO6
//!     - \ref ADC_COMPB_IN_AUXIO5
//!     - \ref ADC_COMPB_IN_AUXIO4
//!     - \ref ADC_COMPB_IN_AUXIO3
//!     - \ref ADC_COMPB_IN_AUXIO2
//!     - \ref ADC_COMPB_IN_AUXIO1
//!     - \ref ADC_COMPB_IN_AUXIO0
//
//*****************************************************************************
__STATIC_INLINE void
AUXADCSelectInput(uint32_t input)
{
    HapiSelectADCCompBInput(input);
}

//*****************************************************************************
//
//! \brief Returns the gain value used when adjusting for ADC gain/offset.
//!
//! The function returns the gain value to be used with
//! \ref AUXADCAdjustValueForGainAndOffset() or
//! \ref AUXADCUnadjustValueForGainAndOffset(). The gain value is found during
//! chip manufacturing and is stored in the factory configuration, FCFG1.
//!
//! \param refSource
//!     ADC reference source:
//!     - \ref AUXADC_REF_FIXED (nominally 4.3 V)
//!     - \ref AUXADC_REF_VDDS_REL (nominally VDDS)
//!
//! \return
//!     The gain value to be used in adjustments
//
//*****************************************************************************
extern int32_t AUXADCGetAdjustmentGain(uint32_t refSource);

//*****************************************************************************
//
//! \brief Returns the offset value used when adjusting for ADC gain/offset.
//!
//! The function returns the offset value to be used with
//! \ref AUXADCAdjustValueForGainAndOffset() or
//! \ref AUXADCUnadjustValueForGainAndOffset(). The offset value is found
//! during chip manufacturing and is stored in the factory configuration,
//! FCFG1.
//!
//! \param refSource
//!     ADC reference source:
//!     - \ref AUXADC_REF_FIXED (nominally 4.3 V)
//!     - \ref AUXADC_REF_VDDS_REL (nominally VDDS)
//!
//! \return
//!     The offset value to be used in adjustments
//
//*****************************************************************************
extern int32_t AUXADCGetAdjustmentOffset(uint32_t refSource);

//*****************************************************************************
//
//! \brief Converts an "adjusted" ADC value to microvolts.
//!
//! This function can only be used when measuring with fixed ADC reference
//! (\ref AUXADC_REF_FIXED). The specified reference voltage accounts for
//! whether the sampled ADC input is scaled down before conversion or not.
//!
//! \param fixedRefVoltage
//!     Fixed reference voltage, in microvolts
//!     - \ref AUXADC_FIXED_REF_VOLTAGE_NORMAL when using scaled input (normal)
//!     - \ref AUXADC_FIXED_REF_VOLTAGE_UNSCALED when using unscaled input
//! \param adcValue
//!     The ADC value
//!
//! \return
//!     The corresponding number of microvolts
//
//*****************************************************************************
extern int32_t AUXADCValueToMicrovolts(int32_t fixedRefVoltage, int32_t adcValue);

//*****************************************************************************
//
//! \brief Converts a number of microvolts to corresponding "adjusted" ADC value.
//!
//! This function can only be used when measuring with fixed ADC reference
//! (\ref AUXADC_REF_FIXED). The specified reference voltage accounts for
//! whether the sampled ADC input is scaled down before conversion or not.
//!
//! \param fixedRefVoltage
//!     Fixed reference voltage, in microvolts
//!     - \ref AUXADC_FIXED_REF_VOLTAGE_NORMAL when using scaled input (normal)
//!     - \ref AUXADC_FIXED_REF_VOLTAGE_UNSCALED when using unscaled input
//! \param microvolts
//!     The number of microvolts
//!
//! \return
//!     The corresponding expected ADC value (adjusted for ADC gain/offset)
//
//*****************************************************************************
extern int32_t AUXADCMicrovoltsToValue(int32_t fixedRefVoltage, int32_t microvolts);

//*****************************************************************************
//
//! \brief Performs ADC value gain and offset adjustment.
//!
//! This function takes a measured ADC value compensates for the internal gain
//! and offset in the ADC.
//!
//! \param adcValue
//!     12-bit ADC unadjusted value
//! \param gain
//!     Gain adjustment value provided by \ref AUXADCGetAdjustmentGain()
//! \param offset
//!     Offset adjustment value provided by \ref AUXADCGetAdjustmentOffset()
//!
//! \return
//!     12-bit ADC adjusted value
//
//*****************************************************************************
extern int32_t AUXADCAdjustValueForGainAndOffset(int32_t adcValue, int32_t gain, int32_t offset);

//*****************************************************************************
//
//! \brief Performs the inverse of the ADC value gain and offset adjustment.
//!
//! This function finds the expected measured ADC value, without gain and
//! offset compensation, for a given "ideal" ADC value. The function can for
//! example be used to find ADC value thresholds to be used in Sensor
//! Controller task configurations.
//!
//! \param adcValue
//!     12-bit ADC adjusted value
//! \param gain
//!     Gain adjustment value provided by \ref AUXADCGetAdjustmentGain()
//! \param offset
//!     Offset adjustment value provided by \ref AUXADCGetAdjustmentOffset()
//!
//! \return
//!     12-bit ADC unadjusted value
//
//*****************************************************************************
extern int32_t AUXADCUnadjustValueForGainAndOffset(int32_t adcValue, int32_t gain, int32_t offset);

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_AUXADCDisable
        #undef  AUXADCDisable
        #define AUXADCDisable                   ROM_AUXADCDisable
    #endif
    #ifdef ROM_AUXADCEnableAsync
        #undef  AUXADCEnableAsync
        #define AUXADCEnableAsync               ROM_AUXADCEnableAsync
    #endif
    #ifdef ROM_AUXADCEnableSync
        #undef  AUXADCEnableSync
        #define AUXADCEnableSync                ROM_AUXADCEnableSync
    #endif
    #ifdef ROM_AUXADCDisableInputScaling
        #undef  AUXADCDisableInputScaling
        #define AUXADCDisableInputScaling       ROM_AUXADCDisableInputScaling
    #endif
    #ifdef ROM_AUXADCFlushFifo
        #undef  AUXADCFlushFifo
        #define AUXADCFlushFifo                 ROM_AUXADCFlushFifo
    #endif
    #ifdef ROM_AUXADCReadFifo
        #undef  AUXADCReadFifo
        #define AUXADCReadFifo                  ROM_AUXADCReadFifo
    #endif
    #ifdef ROM_AUXADCPopFifo
        #undef  AUXADCPopFifo
        #define AUXADCPopFifo                   ROM_AUXADCPopFifo
    #endif
    #ifdef ROM_AUXADCGetAdjustmentGain
        #undef  AUXADCGetAdjustmentGain
        #define AUXADCGetAdjustmentGain         ROM_AUXADCGetAdjustmentGain
    #endif
    #ifdef ROM_AUXADCGetAdjustmentOffset
        #undef  AUXADCGetAdjustmentOffset
        #define AUXADCGetAdjustmentOffset       ROM_AUXADCGetAdjustmentOffset
    #endif
    #ifdef ROM_AUXADCValueToMicrovolts
        #undef  AUXADCValueToMicrovolts
        #define AUXADCValueToMicrovolts         ROM_AUXADCValueToMicrovolts
    #endif
    #ifdef ROM_AUXADCMicrovoltsToValue
        #undef  AUXADCMicrovoltsToValue
        #define AUXADCMicrovoltsToValue         ROM_AUXADCMicrovoltsToValue
    #endif
    #ifdef ROM_AUXADCAdjustValueForGainAndOffset
        #undef  AUXADCAdjustValueForGainAndOffset
        #define AUXADCAdjustValueForGainAndOffset ROM_AUXADCAdjustValueForGainAndOffset
    #endif
    #ifdef ROM_AUXADCUnadjustValueForGainAndOffset
        #undef  AUXADCUnadjustValueForGainAndOffset
        #define AUXADCUnadjustValueForGainAndOffset ROM_AUXADCUnadjustValueForGainAndOffset
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

#endif // __AUX_ADC_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
