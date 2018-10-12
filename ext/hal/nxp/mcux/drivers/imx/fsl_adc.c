/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_adc.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.adc_12b1msps_sar"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for ADC module.
 *
 * @param base ADC peripheral base address
 */
static uint32_t ADC_GetInstance(ADC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to ADC bases for each instance. */
static ADC_Type *const s_adcBases[] = ADC_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to ADC clocks for each instance. */
static const clock_ip_name_t s_adcClocks[] = ADC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t ADC_GetInstance(ADC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_adcBases); instance++)
    {
        if (s_adcBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_adcBases));

    return instance;
}

void ADC_Init(ADC_Type *base, const adc_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_adcClocks[ADC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    /* ADCx_CFG */
    tmp32 = base->CFG & (ADC_CFG_AVGS_MASK | ADC_CFG_ADTRG_MASK); /* Reserve AVGS and ADTRG bits. */
    tmp32 |= ADC_CFG_REFSEL(config->referenceVoltageSource) | ADC_CFG_ADSTS(config->samplePeriodMode) |
             ADC_CFG_ADICLK(config->clockSource) | ADC_CFG_ADIV(config->clockDriver) | ADC_CFG_MODE(config->resolution);
    if (config->enableOverWrite)
    {
        tmp32 |= ADC_CFG_OVWREN_MASK;
    }
    if (config->enableLongSample)
    {
        tmp32 |= ADC_CFG_ADLSMP_MASK;
    }
    if (config->enableLowPower)
    {
        tmp32 |= ADC_CFG_ADLPC_MASK;
    }
    if (config->enableHighSpeed)
    {
        tmp32 |= ADC_CFG_ADHSC_MASK;
    }
    base->CFG = tmp32;

    /* ADCx_GC  */
    tmp32 = base->GC & ~(ADC_GC_ADCO_MASK | ADC_GC_ADACKEN_MASK);
    if (config->enableContinuousConversion)
    {
        tmp32 |= ADC_GC_ADCO_MASK;
    }
    if (config->enableAsynchronousClockOutput)
    {
        tmp32 |= ADC_GC_ADACKEN_MASK;
    }
    base->GC = tmp32;
}

void ADC_Deinit(ADC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_adcClocks[ADC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void ADC_GetDefaultConfig(adc_config_t *config)
{
    assert(NULL != config);

    config->enableAsynchronousClockOutput = true;
    config->enableOverWrite = false;
    config->enableContinuousConversion = false;
    config->enableHighSpeed = false;
    config->enableLowPower = false;
    config->enableLongSample = false;
    config->referenceVoltageSource = kADC_ReferenceVoltageSourceAlt0;
    config->samplePeriodMode = kADC_SamplePeriod2or12Clocks;
    config->clockSource = kADC_ClockSourceAD;
    config->clockDriver = kADC_ClockDriver1;
    config->resolution = kADC_Resolution12Bit;
}

void ADC_SetChannelConfig(ADC_Type *base, uint32_t channelGroup, const adc_channel_config_t *config)
{
    assert(NULL != config);
    assert(channelGroup < ADC_HC_COUNT);

    uint32_t tmp32;

    tmp32 = ADC_HC_ADCH(config->channelNumber);
    if (config->enableInterruptOnConversionCompleted)
    {
        tmp32 |= ADC_HC_AIEN_MASK;
    }
    base->HC[channelGroup] = tmp32;
}

/*
 *To complete calibration, the user must follow the below procedure:
 *  1. Configure ADC_CFG with actual operating values for maximum accuracy.
 *  2. Configure the ADC_GC values along with CAL bit.
 *  3. Check the status of CALF bit in ADC_GS and the CAL bit in ADC_GC.
 *  4. When CAL bit becomes '0' then check the CALF status and COCO[0] bit status.
 */
status_t ADC_DoAutoCalibration(ADC_Type *base)
{
    status_t status = kStatus_Success;
#if !(defined(FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE) && FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE)
    bool bHWTrigger = false;

    /* The calibration would be failed when in hardwar mode.
     * Remember the hardware trigger state here and restore it later if the hardware trigger is enabled.*/
    if (0U != (ADC_CFG_ADTRG_MASK & base->CFG))
    {
        bHWTrigger = true;
        ADC_EnableHardwareTrigger(base, false);
    }
#endif

    /* Clear the CALF and launch the calibration. */
    base->GS = ADC_GS_CALF_MASK; /* Clear the CALF. */
    base->GC |= ADC_GC_CAL_MASK; /* Launch the calibration. */

    /* Check the status of CALF bit in ADC_GS and the CAL bit in ADC_GC. */
    while (0U != (base->GC & ADC_GC_CAL_MASK))
    {
        /* Check the CALF when the calibration is active. */
        if (0U != (ADC_GetStatusFlags(base) & kADC_CalibrationFailedFlag))
        {
            status = kStatus_Fail;
            break;
        }
    }

    /* When CAL bit becomes '0' then check the CALF status and COCO[0] bit status. */
    if (0U == ADC_GetChannelStatusFlags(base, 0U)) /* Check the COCO[0] bit status. */
    {
        status = kStatus_Fail;
    }
    if (0U != (ADC_GetStatusFlags(base) & kADC_CalibrationFailedFlag)) /* Check the CALF status. */
    {
        status = kStatus_Fail;
    }

    /* Clear conversion done flag. */
    ADC_GetChannelConversionValue(base, 0U);

#if !(defined(FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE) && FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE)
    /* Restore original trigger mode. */
    if (true == bHWTrigger)
    {
        ADC_EnableHardwareTrigger(base, true);
    }
#endif

    return status;
}

void ADC_SetOffsetConfig(ADC_Type *base, const adc_offest_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

    tmp32 = ADC_OFS_OFS(config->offsetValue);
    if (config->enableSigned)
    {
        tmp32 |= ADC_OFS_SIGN_MASK;
    }
    base->OFS = tmp32;
}

void ADC_SetHardwareCompareConfig(ADC_Type *base, const adc_hardware_compare_config_t *config)
{
    uint32_t tmp32;

    tmp32 = base->GC & ~(ADC_GC_ACFE_MASK | ADC_GC_ACFGT_MASK | ADC_GC_ACREN_MASK);
    if (NULL == config) /* Pass "NULL" to disable the feature. */
    {
        base->GC = tmp32;
        return;
    }
    /* Enable the feature. */
    tmp32 |= ADC_GC_ACFE_MASK;

    /* Select the hardware compare working mode. */
    switch (config->hardwareCompareMode)
    {
        case kADC_HardwareCompareMode0:
            break;
        case kADC_HardwareCompareMode1:
            tmp32 |= ADC_GC_ACFGT_MASK;
            break;
        case kADC_HardwareCompareMode2:
            tmp32 |= ADC_GC_ACREN_MASK;
            break;
        case kADC_HardwareCompareMode3:
            tmp32 |= ADC_GC_ACFGT_MASK | ADC_GC_ACREN_MASK;
            break;
        default:
            break;
    }
    base->GC = tmp32;

    /* Load the compare values. */
    tmp32 = ADC_CV_CV1(config->value1) | ADC_CV_CV2(config->value2);
    base->CV = tmp32;
}

void ADC_SetHardwareAverageConfig(ADC_Type *base, adc_hardware_average_mode_t mode)
{
    uint32_t tmp32;

    if (mode == kADC_HardwareAverageDiasable)
    {
        base->GC &= ~ADC_GC_AVGE_MASK;
    }
    else
    {
        tmp32 = base->CFG & ~ADC_CFG_AVGS_MASK;
        tmp32 |= ADC_CFG_AVGS(mode);
        base->CFG = tmp32;
        base->GC |= ADC_GC_AVGE_MASK; /* Enable the hardware compare. */
    }
}

void ADC_ClearStatusFlags(ADC_Type *base, uint32_t mask)
{
    uint32_t tmp32 = 0;

    if (0U != (mask & kADC_CalibrationFailedFlag))
    {
        tmp32 |= ADC_GS_CALF_MASK;
    }
    if (0U != (mask & kADC_ConversionActiveFlag))
    {
        tmp32 |= ADC_GS_ADACT_MASK;
    }
    base->GS = tmp32;
}
