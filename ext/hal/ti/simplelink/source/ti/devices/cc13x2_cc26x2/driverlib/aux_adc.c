/******************************************************************************
*  Filename:       aux_adc.c
*  Revised:        2017-11-20 14:31:35 +0100 (Mon, 20 Nov 2017)
*  Revision:       50315
*
*  Description:    Driver for the AUX Time to Digital Converter interface.
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

#include "aux_adc.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_aux_sysif.h"
#include "../inc/hw_fcfg1.h"
#include "adi.h"
#include "event.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  AUXADCDisable
    #define AUXADCDisable                   NOROM_AUXADCDisable
    #undef  AUXADCEnableAsync
    #define AUXADCEnableAsync               NOROM_AUXADCEnableAsync
    #undef  AUXADCEnableSync
    #define AUXADCEnableSync                NOROM_AUXADCEnableSync
    #undef  AUXADCDisableInputScaling
    #define AUXADCDisableInputScaling       NOROM_AUXADCDisableInputScaling
    #undef  AUXADCFlushFifo
    #define AUXADCFlushFifo                 NOROM_AUXADCFlushFifo
    #undef  AUXADCReadFifo
    #define AUXADCReadFifo                  NOROM_AUXADCReadFifo
    #undef  AUXADCPopFifo
    #define AUXADCPopFifo                   NOROM_AUXADCPopFifo
    #undef  AUXADCGetAdjustmentGain
    #define AUXADCGetAdjustmentGain         NOROM_AUXADCGetAdjustmentGain
    #undef  AUXADCGetAdjustmentOffset
    #define AUXADCGetAdjustmentOffset       NOROM_AUXADCGetAdjustmentOffset
    #undef  AUXADCValueToMicrovolts
    #define AUXADCValueToMicrovolts         NOROM_AUXADCValueToMicrovolts
    #undef  AUXADCMicrovoltsToValue
    #define AUXADCMicrovoltsToValue         NOROM_AUXADCMicrovoltsToValue
    #undef  AUXADCAdjustValueForGainAndOffset
    #define AUXADCAdjustValueForGainAndOffset NOROM_AUXADCAdjustValueForGainAndOffset
    #undef  AUXADCUnadjustValueForGainAndOffset
    #define AUXADCUnadjustValueForGainAndOffset NOROM_AUXADCUnadjustValueForGainAndOffset
#endif

//*****************************************************************************
//
// Disables the ADC
//
//*****************************************************************************
void
AUXADCDisable(void)
{
    // Disable the ADC reference
    ADI8BitsClear(AUX_ADI4_BASE, ADI_4_AUX_O_ADCREF0, ADI_4_AUX_ADCREF0_EN_M | ADI_4_AUX_ADCREF0_REF_ON_IDLE_M | ADI_4_AUX_ADCREF0_SRC_M);

    // Assert reset and disable the ADC
    ADI8BitsClear(AUX_ADI4_BASE, ADI_4_AUX_O_ADC0, ADI_4_AUX_ADC0_EN_M | ADI_4_AUX_ADC0_RESET_N_M | ADI_4_AUX_ADC0_SMPL_MODE_M | ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_M);

    // Ensure that scaling is enabled by default before next use of the ADC
    ADI8BitsClear(AUX_ADI4_BASE, ADI_4_AUX_O_ADC1, ADI_4_AUX_ADC1_SCALE_DIS_M);

    // Flush the FIFO before disabling the clocks
    HWREGBITW(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL, 1) = 1; // CMD: EN(1) -> FLUSH(3)
    HWREGBITW(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL, 1) = 0; // CMD: FLUSH(3) -> EN(1)

    // Disable the ADC clock (no need to wait since IOB_WUC_ADCCLKCTL_ACK goes low immediately)
    HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_ADCCLKCTL) = 0;

    // Disable the ADC data interface
    HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL) = 0;
}

//*****************************************************************************
//
// Enables the ADC for asynchronous operation
//
//*****************************************************************************
void
AUXADCEnableAsync(uint32_t refSource, uint32_t trigger)
{
    // Enable the ADC reference, with the following options:
    // - SRC: Set when using relative reference
    // - REF_ON_IDLE: Always cleared since there is no idle state in asynchronous operation
    ADI8BitsSet(AUX_ADI4_BASE, ADI_4_AUX_O_ADCREF0, refSource | ADI_4_AUX_ADCREF0_EN_M);

    // Enable the ADC clock
    HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_ADCCLKCTL) = AUX_SYSIF_ADCCLKCTL_REQ_M;
    while (!(HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_ADCCLKCTL) & AUX_SYSIF_ADCCLKCTL_ACK_M));

    // Enable the ADC data interface
    if (trigger == AUXADC_TRIGGER_MANUAL) {
        // Manual trigger: No need to configure event routing from GPT
        HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL) = AUX_ANAIF_ADCCTL_START_SRC_NO_EVENT | AUX_ANAIF_ADCCTL_CMD_EN;
    } else {
        // GPT trigger: Configure event routing via MCU_EV to the AUX domain
        HWREG(EVENT_BASE + EVENT_O_AUXSEL0) = trigger;
        HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL) = AUX_ANAIF_ADCCTL_START_SRC_MCU_EV | AUX_ANAIF_ADCCTL_CMD_EN;
    }

    // Configure the ADC
    ADI8BitsSet(AUX_ADI4_BASE, ADI_4_AUX_O_ADC0, ADI_4_AUX_ADC0_SMPL_MODE_M);

    // Release reset and enable the ADC
    ADI8BitsSet(AUX_ADI4_BASE, ADI_4_AUX_O_ADC0, ADI_4_AUX_ADC0_EN_M | ADI_4_AUX_ADC0_RESET_N_M);
}

//*****************************************************************************
//
// Enables the ADC for synchronous operation
//
//*****************************************************************************
void
AUXADCEnableSync(uint32_t refSource, uint32_t sampleTime, uint32_t trigger)
{
    // Enable the ADC reference, with the following options:
    // - SRC: Set when using relative reference
    // - REF_ON_IDLE: Set when using fixed reference and sample time < 21.3 us
    uint8_t adcref0 = refSource | ADI_4_AUX_ADCREF0_EN_M;
    if (!refSource && (sampleTime < AUXADC_SAMPLE_TIME_21P3_US)) {
        adcref0 |= ADI_4_AUX_ADCREF0_REF_ON_IDLE_M;
    }
    ADI8BitsSet(AUX_ADI4_BASE, ADI_4_AUX_O_ADCREF0, adcref0);

    // Enable the ADC clock
    HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_ADCCLKCTL) = AUX_SYSIF_ADCCLKCTL_REQ_M;
    while (!(HWREG(AUX_SYSIF_BASE + AUX_SYSIF_O_ADCCLKCTL) & AUX_SYSIF_ADCCLKCTL_ACK_M));

    // Enable the ADC data interface
    if (trigger == AUXADC_TRIGGER_MANUAL) {
        // Manual trigger: No need to configure event routing from GPT
        HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL) = AUX_ANAIF_ADCCTL_START_SRC_NO_EVENT | AUX_ANAIF_ADCCTL_CMD_EN;
    } else {
        // GPT trigger: Configure event routing via MCU_EV to the AUX domain
        HWREG(EVENT_BASE + EVENT_O_AUXSEL0) = trigger;
        HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL) = AUX_ANAIF_ADCCTL_START_SRC_MCU_EV | AUX_ANAIF_ADCCTL_CMD_EN;
    }

    // Configure the ADC
    ADI8BitsSet(AUX_ADI4_BASE, ADI_4_AUX_O_ADC0, sampleTime << ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_S);

    // Release reset and enable the ADC
    ADI8BitsSet(AUX_ADI4_BASE, ADI_4_AUX_O_ADC0, ADI_4_AUX_ADC0_EN_M | ADI_4_AUX_ADC0_RESET_N_M);
}

//*****************************************************************************
//
// Disables scaling of the ADC input
//
//*****************************************************************************
void
AUXADCDisableInputScaling(void)
{
    ADI8BitsSet(AUX_ADI4_BASE, ADI_4_AUX_O_ADC1, ADI_4_AUX_ADC1_SCALE_DIS_M);
}

//*****************************************************************************
//
// Flushes the ADC FIFO
//
//*****************************************************************************
void
AUXADCFlushFifo(void)
{
    HWREGBITW(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL, 1) = 1; // CMD: EN(1) -> FLUSH(3)
    HWREGBITW(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCCTL, 1) = 0; // CMD: FLUSH(3) -> EN(1)
}

//*****************************************************************************
//
// Waits for and returns the first sample in the ADC FIFO
//
//*****************************************************************************
uint32_t
AUXADCReadFifo(void) {

    // Wait until there is at least one sample in the FIFO
    while (HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCFIFOSTAT) & AUX_ANAIF_ADCFIFOSTAT_EMPTY_M);

    // Return the first sample from the FIFO
    return HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCFIFO);
}

//*****************************************************************************
//
// Returns the first sample in the ADC FIFO, without waiting
//
//*****************************************************************************
uint32_t
AUXADCPopFifo(void) {

    // Return the first sample from the FIFO. If the FIFO is empty, this
    // generates ADC FIFO underflow
    return HWREG(AUX_ANAIF_BASE + AUX_ANAIF_O_ADCFIFO);
}

//*****************************************************************************
//
// Returns the gain value used when adjusting for ADC gain/offset
//
//*****************************************************************************
int32_t
AUXADCGetAdjustmentGain(uint32_t refSource)
{
    int32_t gain;
    if (refSource == AUXADC_REF_FIXED) {
        // AUXADC_REF_FIXED ==> ABS_GAIN
        gain = (HWREG(FCFG1_BASE + FCFG1_O_SOC_ADC_ABS_GAIN) & FCFG1_SOC_ADC_ABS_GAIN_SOC_ADC_ABS_GAIN_TEMP1_M) >> FCFG1_SOC_ADC_ABS_GAIN_SOC_ADC_ABS_GAIN_TEMP1_S;
    } else {
      // AUXADC_REF_VDDS_REL ==> REL_GAIN
        gain = (HWREG(FCFG1_BASE + FCFG1_O_SOC_ADC_REL_GAIN) & FCFG1_SOC_ADC_REL_GAIN_SOC_ADC_REL_GAIN_TEMP1_M) >> FCFG1_SOC_ADC_REL_GAIN_SOC_ADC_REL_GAIN_TEMP1_S;
    }
    return gain;
}

//*****************************************************************************
//
// Returns the offset value used when adjusting for ADC gain/offset
//
//*****************************************************************************
int32_t
AUXADCGetAdjustmentOffset(uint32_t refSource)
{
    int8_t offset;
    if ( refSource == AUXADC_REF_FIXED ) {
        // AUXADC_REF_FIXED ==> ABS_OFFSET
        offset = HWREG(FCFG1_BASE + FCFG1_O_SOC_ADC_OFFSET_INT) >> FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_ABS_OFFSET_TEMP1_S;
    } else {
        // AUXADC_REF_VDDS_REL ==> REL_OFFSET
        offset = HWREG(FCFG1_BASE + FCFG1_O_SOC_ADC_OFFSET_INT) >> FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_REL_OFFSET_TEMP1_S;
    }
    return offset;
}

//*****************************************************************************
//
// Converts an "ideal" ADC value to microvolts
//
//*****************************************************************************
int32_t
AUXADCValueToMicrovolts(int32_t fixedRefVoltage, int32_t adcValue)
{
    // Chop off 4 bits during calculations to avoid 32-bit overflow
    fixedRefVoltage >>= 4;
    return (((adcValue * fixedRefVoltage) + 2047) / 4095) << 4;
}

//*****************************************************************************
//
// Converts a number of microvolts to corresponding "ideal" ADC value
//
//*****************************************************************************
int32_t
AUXADCMicrovoltsToValue(int32_t fixedRefVoltage, int32_t microvolts)
{
    // Chop off 4 bits during calculations to avoid 32-bit overflow
    fixedRefVoltage >>= 4;
    microvolts >>= 4;
    return ((microvolts * 4095) + (fixedRefVoltage / 2)) / fixedRefVoltage;
}

//*****************************************************************************
//
// Performs ADC value gain and offset adjustment
//
//*****************************************************************************
int32_t
AUXADCAdjustValueForGainAndOffset(int32_t adcValue, int32_t gain, int32_t offset)
{
    // Apply gain and offset adjustment
    adcValue = (((adcValue + offset) * gain) + 16384) / 32768;

    // Saturate
    if (adcValue < 0) {
        return 0;
    } else if (adcValue > 4095) {
        return 4095;
    } else {
        return adcValue;
    }
}

//*****************************************************************************
//
// Performs the inverse of the ADC value gain and offset adjustment
//
//*****************************************************************************
int32_t
AUXADCUnadjustValueForGainAndOffset(int32_t adcValue, int32_t gain, int32_t offset)
{
    // Apply inverse gain and offset adjustment
    adcValue = (((adcValue * 32768) + (gain / 2)) / gain) - offset;

    // Saturate
    if (adcValue < 0) {
        return 0;
    } else if (adcValue > 4095) {
        return 4095;
    } else {
        return adcValue;
    }
}
