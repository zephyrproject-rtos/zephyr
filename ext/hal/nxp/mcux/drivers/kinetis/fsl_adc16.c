/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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

#include "fsl_adc16.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for ADC16 module.
 *
 * @param base ADC16 peripheral base address
 */
static uint32_t ADC16_GetInstance(ADC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to ADC16 bases for each instance. */
static ADC_Type *const s_adc16Bases[] = ADC_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to ADC16 clocks for each instance. */
static const clock_ip_name_t s_adc16Clocks[] = ADC16_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t ADC16_GetInstance(ADC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_adc16Bases); instance++)
    {
        if (s_adc16Bases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_adc16Bases));

    return instance;
}

void ADC16_Init(ADC_Type *base, const adc16_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_adc16Clocks[ADC16_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* ADCx_CFG1. */
    tmp32 = ADC_CFG1_ADICLK(config->clockSource) | ADC_CFG1_MODE(config->resolution);
    if (kADC16_LongSampleDisabled != config->longSampleMode)
    {
        tmp32 |= ADC_CFG1_ADLSMP_MASK;
    }
    tmp32 |= ADC_CFG1_ADIV(config->clockDivider);
    if (config->enableLowPower)
    {
        tmp32 |= ADC_CFG1_ADLPC_MASK;
    }
    base->CFG1 = tmp32;

    /* ADCx_CFG2. */
    tmp32 = base->CFG2 & ~(ADC_CFG2_ADACKEN_MASK | ADC_CFG2_ADHSC_MASK | ADC_CFG2_ADLSTS_MASK);
    if (kADC16_LongSampleDisabled != config->longSampleMode)
    {
        tmp32 |= ADC_CFG2_ADLSTS(config->longSampleMode);
    }
    if (config->enableHighSpeed)
    {
        tmp32 |= ADC_CFG2_ADHSC_MASK;
    }
    if (config->enableAsynchronousClock)
    {
        tmp32 |= ADC_CFG2_ADACKEN_MASK;
    }
    base->CFG2 = tmp32;

    /* ADCx_SC2. */
    tmp32 = base->SC2 & ~(ADC_SC2_REFSEL_MASK);
    tmp32 |= ADC_SC2_REFSEL(config->referenceVoltageSource);
    base->SC2 = tmp32;

    /* ADCx_SC3. */
    if (config->enableContinuousConversion)
    {
        base->SC3 |= ADC_SC3_ADCO_MASK;
    }
    else
    {
        base->SC3 &= ~ADC_SC3_ADCO_MASK;
    }
}

void ADC16_Deinit(ADC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_adc16Clocks[ADC16_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void ADC16_GetDefaultConfig(adc16_config_t *config)
{
    assert(NULL != config);

    config->referenceVoltageSource = kADC16_ReferenceVoltageSourceVref;
    config->clockSource = kADC16_ClockSourceAsynchronousClock;
    config->enableAsynchronousClock = true;
    config->clockDivider = kADC16_ClockDivider8;
    config->resolution = kADC16_ResolutionSE12Bit;
    config->longSampleMode = kADC16_LongSampleDisabled;
    config->enableHighSpeed = false;
    config->enableLowPower = false;
    config->enableContinuousConversion = false;
}

#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
status_t ADC16_DoAutoCalibration(ADC_Type *base)
{
    bool bHWTrigger = false;
    volatile uint32_t tmp32; /* 'volatile' here is for the dummy read of ADCx_R[0] register. */
    status_t status = kStatus_Success;

    /* The calibration would be failed when in hardwar mode.
     * Remember the hardware trigger state here and restore it later if the hardware trigger is enabled.*/
    if (0U != (ADC_SC2_ADTRG_MASK & base->SC2))
    {
        bHWTrigger = true;
        base->SC2 &= ~ADC_SC2_ADTRG_MASK;
    }

    /* Clear the CALF and launch the calibration. */
    base->SC3 |= ADC_SC3_CAL_MASK | ADC_SC3_CALF_MASK;
    while (0U == (kADC16_ChannelConversionDoneFlag & ADC16_GetChannelStatusFlags(base, 0U)))
    {
        /* Check the CALF when the calibration is active. */
        if (0U != (kADC16_CalibrationFailedFlag & ADC16_GetStatusFlags(base)))
        {
            status = kStatus_Fail;
            break;
        }
    }
    tmp32 = base->R[0]; /* Dummy read to clear COCO caused by calibration. */

    /* Restore the hardware trigger setting if it was enabled before. */
    if (bHWTrigger)
    {
        base->SC2 |= ADC_SC2_ADTRG_MASK;
    }
    /* Check the CALF at the end of calibration. */
    if (0U != (kADC16_CalibrationFailedFlag & ADC16_GetStatusFlags(base)))
    {
        status = kStatus_Fail;
    }
    if (kStatus_Success != status) /* Check if the calibration process is succeed. */
    {
        return status;
    }

    /* Calculate the calibration values. */
    tmp32 = base->CLP0 + base->CLP1 + base->CLP2 + base->CLP3 + base->CLP4 + base->CLPS;
    tmp32 = 0x8000U | (tmp32 >> 1U);
    base->PG = tmp32;

#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
    tmp32 = base->CLM0 + base->CLM1 + base->CLM2 + base->CLM3 + base->CLM4 + base->CLMS;
    tmp32 = 0x8000U | (tmp32 >> 1U);
    base->MG = tmp32;
#endif /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */

    return kStatus_Success;
}
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */

#if defined(FSL_FEATURE_ADC16_HAS_MUX_SELECT) && FSL_FEATURE_ADC16_HAS_MUX_SELECT
void ADC16_SetChannelMuxMode(ADC_Type *base, adc16_channel_mux_mode_t mode)
{
    if (kADC16_ChannelMuxA == mode)
    {
        base->CFG2 &= ~ADC_CFG2_MUXSEL_MASK;
    }
    else /* kADC16_ChannelMuxB. */
    {
        base->CFG2 |= ADC_CFG2_MUXSEL_MASK;
    }
}
#endif /* FSL_FEATURE_ADC16_HAS_MUX_SELECT */

void ADC16_SetHardwareCompareConfig(ADC_Type *base, const adc16_hardware_compare_config_t *config)
{
    uint32_t tmp32 = base->SC2 & ~(ADC_SC2_ACFE_MASK | ADC_SC2_ACFGT_MASK | ADC_SC2_ACREN_MASK);

    if (!config) /* Pass "NULL" to disable the feature. */
    {
        base->SC2 = tmp32;
        return;
    }
    /* Enable the feature. */
    tmp32 |= ADC_SC2_ACFE_MASK;

    /* Select the hardware compare working mode. */
    switch (config->hardwareCompareMode)
    {
        case kADC16_HardwareCompareMode0:
            break;
        case kADC16_HardwareCompareMode1:
            tmp32 |= ADC_SC2_ACFGT_MASK;
            break;
        case kADC16_HardwareCompareMode2:
            tmp32 |= ADC_SC2_ACREN_MASK;
            break;
        case kADC16_HardwareCompareMode3:
            tmp32 |= ADC_SC2_ACFGT_MASK | ADC_SC2_ACREN_MASK;
            break;
        default:
            break;
    }
    base->SC2 = tmp32;

    /* Load the compare values. */
    base->CV1 = ADC_CV1_CV(config->value1);
    base->CV2 = ADC_CV2_CV(config->value2);
}

#if defined(FSL_FEATURE_ADC16_HAS_HW_AVERAGE) && FSL_FEATURE_ADC16_HAS_HW_AVERAGE
void ADC16_SetHardwareAverage(ADC_Type *base, adc16_hardware_average_mode_t mode)
{
    uint32_t tmp32 = base->SC3 & ~(ADC_SC3_AVGE_MASK | ADC_SC3_AVGS_MASK);

    if (kADC16_HardwareAverageDisabled != mode)
    {
        tmp32 |= ADC_SC3_AVGE_MASK | ADC_SC3_AVGS(mode);
    }
    base->SC3 = tmp32;
}
#endif /* FSL_FEATURE_ADC16_HAS_HW_AVERAGE */

#if defined(FSL_FEATURE_ADC16_HAS_PGA) && FSL_FEATURE_ADC16_HAS_PGA
void ADC16_SetPGAConfig(ADC_Type *base, const adc16_pga_config_t *config)
{
    uint32_t tmp32;

    if (!config) /* Passing "NULL" is to disable the feature. */
    {
        base->PGA = 0U;
        return;
    }

    /* Enable the PGA and set the gain value. */
    tmp32 = ADC_PGA_PGAEN_MASK | ADC_PGA_PGAG(config->pgaGain);

    /* Configure the misc features for PGA. */
    if (config->enableRunInNormalMode)
    {
        tmp32 |= ADC_PGA_PGALPb_MASK;
    }
#if defined(FSL_FEATURE_ADC16_HAS_PGA_CHOPPING) && FSL_FEATURE_ADC16_HAS_PGA_CHOPPING
    if (config->disablePgaChopping)
    {
        tmp32 |= ADC_PGA_PGACHPb_MASK;
    }
#endif /* FSL_FEATURE_ADC16_HAS_PGA_CHOPPING */
#if defined(FSL_FEATURE_ADC16_HAS_PGA_OFFSET_MEASUREMENT) && FSL_FEATURE_ADC16_HAS_PGA_OFFSET_MEASUREMENT
    if (config->enableRunInOffsetMeasurement)
    {
        tmp32 |= ADC_PGA_PGAOFSM_MASK;
    }
#endif /* FSL_FEATURE_ADC16_HAS_PGA_OFFSET_MEASUREMENT */
    base->PGA = tmp32;
}
#endif /* FSL_FEATURE_ADC16_HAS_PGA */

uint32_t ADC16_GetStatusFlags(ADC_Type *base)
{
    uint32_t ret = 0;

    if (0U != (base->SC2 & ADC_SC2_ADACT_MASK))
    {
        ret |= kADC16_ActiveFlag;
    }
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
    if (0U != (base->SC3 & ADC_SC3_CALF_MASK))
    {
        ret |= kADC16_CalibrationFailedFlag;
    }
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */
    return ret;
}

void ADC16_ClearStatusFlags(ADC_Type *base, uint32_t mask)
{
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
    if (0U != (mask & kADC16_CalibrationFailedFlag))
    {
        base->SC3 |= ADC_SC3_CALF_MASK;
    }
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */
}

void ADC16_SetChannelConfig(ADC_Type *base, uint32_t channelGroup, const adc16_channel_config_t *config)
{
    assert(channelGroup < ADC_SC1_COUNT);
    assert(NULL != config);

    uint32_t sc1 = ADC_SC1_ADCH(config->channelNumber); /* Set the channel number. */

#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
    /* Enable the differential conversion. */
    if (config->enableDifferentialConversion)
    {
        sc1 |= ADC_SC1_DIFF_MASK;
    }
#endif /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */
    /* Enable the interrupt when the conversion is done. */
    if (config->enableInterruptOnConversionCompleted)
    {
        sc1 |= ADC_SC1_AIEN_MASK;
    }
    base->SC1[channelGroup] = sc1;
}

uint32_t ADC16_GetChannelStatusFlags(ADC_Type *base, uint32_t channelGroup)
{
    assert(channelGroup < ADC_SC1_COUNT);

    uint32_t ret = 0U;

    if (0U != (base->SC1[channelGroup] & ADC_SC1_COCO_MASK))
    {
        ret |= kADC16_ChannelConversionDoneFlag;
    }
    return ret;
}
