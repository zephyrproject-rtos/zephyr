/*
 * Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "fsl_tsi_v4.h"

void TSI_Init(TSI_Type *base, const tsi_config_t *config)
{
    assert(config != NULL);

    bool is_module_enabled = false;
    bool is_int_enabled = false;

    CLOCK_EnableClock(kCLOCK_Tsi0);
    if (base->GENCS & TSI_GENCS_TSIEN_MASK)
    {
        is_module_enabled = true;
        TSI_EnableModule(base, false);
    }
    if (base->GENCS & TSI_GENCS_TSIIEN_MASK)
    {
        is_int_enabled = true;
        TSI_DisableInterrupts(base, kTSI_GlobalInterruptEnable);
    }

    TSI_SetHighThreshold(base, config->thresh);
    TSI_SetLowThreshold(base, config->thresl);
    TSI_SetElectrodeOSCPrescaler(base, config->prescaler);
    TSI_SetReferenceChargeCurrent(base, config->refchrg);
    TSI_SetElectrodeChargeCurrent(base, config->extchrg);
    TSI_SetNumberOfScans(base, config->nscn);
    TSI_SetAnalogMode(base, config->mode);
    TSI_SetOscVoltageRails(base, config->dvolt);
    TSI_SetElectrodeSeriesResistor(base, config->resistor);
    TSI_SetFilterBits(base, config->filter);

    if (is_module_enabled)
    {
        TSI_EnableModule(base, true);
    }
    if (is_int_enabled)
    {
        TSI_EnableInterrupts(base, kTSI_GlobalInterruptEnable);
    }
}

void TSI_Deinit(TSI_Type *base)
{
    base->GENCS = 0U;
    base->DATA = 0U;
    base->TSHD = 0U;
    CLOCK_DisableClock(kCLOCK_Tsi0);
}

void TSI_GetNormalModeDefaultConfig(tsi_config_t *userConfig)
{
    userConfig->thresh = 0U;
    userConfig->thresl = 0U;
    userConfig->prescaler = kTSI_ElecOscPrescaler_2div;
    userConfig->extchrg = kTSI_ExtOscChargeCurrent_4uA;
    userConfig->refchrg = kTSI_RefOscChargeCurrent_4uA;
    userConfig->nscn = kTSI_ConsecutiveScansNumber_5time;
    userConfig->mode = kTSI_AnalogModeSel_Capacitive;
    userConfig->dvolt = kTSI_OscVolRailsOption_0;
    userConfig->resistor = kTSI_SeriesResistance_32k;
    userConfig->filter = kTSI_FilterBits_3;
}

void TSI_GetLowPowerModeDefaultConfig(tsi_config_t *userConfig)
{
    userConfig->thresh = 400U;
    userConfig->thresl = 0U;
    userConfig->prescaler = kTSI_ElecOscPrescaler_2div;
    userConfig->extchrg = kTSI_ExtOscChargeCurrent_4uA;
    userConfig->refchrg = kTSI_RefOscChargeCurrent_4uA;
    userConfig->nscn = kTSI_ConsecutiveScansNumber_5time;
    userConfig->mode = kTSI_AnalogModeSel_Capacitive;
    userConfig->dvolt = kTSI_OscVolRailsOption_0;
    userConfig->resistor = kTSI_SeriesResistance_32k;
    userConfig->filter = kTSI_FilterBits_3;
}

void TSI_Calibrate(TSI_Type *base, tsi_calibration_data_t *calBuff)
{
    assert(calBuff != NULL);

    uint8_t i = 0U;
    bool is_int_enabled = false;

    if (base->GENCS & TSI_GENCS_TSIIEN_MASK)
    {
        is_int_enabled = true;
        TSI_DisableInterrupts(base, kTSI_GlobalInterruptEnable);
    }
    for (i = 0U; i < FSL_FEATURE_TSI_CHANNEL_COUNT; i++)
    {
        TSI_SetMeasuredChannelNumber(base, i);
        TSI_StartSoftwareTrigger(base);
        while (!(TSI_GetStatusFlags(base) & kTSI_EndOfScanFlag))
        {
        }
        calBuff->calibratedData[i] = TSI_GetCounter(base);
        TSI_ClearStatusFlags(base, kTSI_EndOfScanFlag);
    }
    if (is_int_enabled)
    {
        TSI_EnableInterrupts(base, kTSI_GlobalInterruptEnable);
    }
}

void TSI_EnableInterrupts(TSI_Type *base, uint32_t mask)
{
    uint32_t regValue = base->GENCS & (~ALL_FLAGS_MASK);

    if (mask & kTSI_GlobalInterruptEnable)
    {
        regValue |= TSI_GENCS_TSIIEN_MASK;
    }
    if (mask & kTSI_OutOfRangeInterruptEnable)
    {
        regValue &= (~TSI_GENCS_ESOR_MASK);
    }
    if (mask & kTSI_EndOfScanInterruptEnable)
    {
        regValue |= TSI_GENCS_ESOR_MASK;
    }

    base->GENCS = regValue;     /* write value to register */
}

void TSI_DisableInterrupts(TSI_Type *base, uint32_t mask)
{
    uint32_t regValue = base->GENCS & (~ALL_FLAGS_MASK);

    if (mask & kTSI_GlobalInterruptEnable)
    {
        regValue &= (~TSI_GENCS_TSIIEN_MASK);
    }
    if (mask & kTSI_OutOfRangeInterruptEnable)
    {
        regValue |= TSI_GENCS_ESOR_MASK;
    }
    if (mask & kTSI_EndOfScanInterruptEnable)
    {
        regValue &= (~TSI_GENCS_ESOR_MASK);
    }

    base->GENCS = regValue;     /* write value to register */
}

void TSI_ClearStatusFlags(TSI_Type *base, uint32_t mask)
{
    uint32_t regValue = base->GENCS & (~ALL_FLAGS_MASK);

    if (mask & kTSI_EndOfScanFlag)
    {
        regValue |= TSI_GENCS_EOSF_MASK;
    }
    if (mask & kTSI_OutOfRangeFlag)
    {
        regValue |= TSI_GENCS_OUTRGF_MASK;
    }

    base->GENCS = regValue;     /* write value to register */
}
