/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_adc16.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.adc16"
#endif

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

/*!
 * brief Initializes the ADC16 module.
 *
 * param base   ADC16 peripheral base address.
 * param config Pointer to configuration structure. See "adc16_config_t".
 */
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

/*!
 * brief De-initializes the ADC16 module.
 *
 * param base ADC16 peripheral base address.
 */
void ADC16_Deinit(ADC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_adc16Clocks[ADC16_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Gets an available pre-defined settings for the converter's configuration.
 *
 * This function initializes the converter configuration structure with available settings. The default values are as
 * follows.
 * code
 *   config->referenceVoltageSource     = kADC16_ReferenceVoltageSourceVref;
 *   config->clockSource                = kADC16_ClockSourceAsynchronousClock;
 *   config->enableAsynchronousClock    = true;
 *   config->clockDivider               = kADC16_ClockDivider8;
 *   config->resolution                 = kADC16_ResolutionSE12Bit;
 *   config->longSampleMode             = kADC16_LongSampleDisabled;
 *   config->enableHighSpeed            = false;
 *   config->enableLowPower             = false;
 *   config->enableContinuousConversion = false;
 * endcode
 * param config Pointer to the configuration structure.
 */
void ADC16_GetDefaultConfig(adc16_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

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
/*!
 * brief  Automates the hardware calibration.
 *
 * This auto calibration helps to adjust the plus/minus side gain automatically.
 * Execute the calibration before using the converter. Note that the hardware trigger should be used
 * during the calibration.
 *
 * param  base ADC16 peripheral base address.
 *
 * return                 Execution status.
 * retval kStatus_Success Calibration is done successfully.
 * retval kStatus_Fail    Calibration has failed.
 */
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
/*!
 * brief Sets the channel mux mode.
 *
 * Some sample pins share the same channel index. The channel mux mode decides which pin is used for an
 * indicated channel.
 *
 * param base ADC16 peripheral base address.
 * param mode Setting channel mux mode. See "adc16_channel_mux_mode_t".
 */
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

/*!
 * brief Configures the hardware compare mode.
 *
 * The hardware compare mode provides a way to process the conversion result automatically by using hardware. Only the
 * result
 * in the compare range is available. To compare the range, see "adc16_hardware_compare_mode_t" or the appopriate
 * reference
 * manual for more information.
 *
 * param base     ADC16 peripheral base address.
 * param config   Pointer to the "adc16_hardware_compare_config_t" structure. Passing "NULL" disables the feature.
 */
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
/*!
 * brief Sets the hardware average mode.
 *
 * The hardware average mode provides a way to process the conversion result automatically by using hardware. The
 * multiple
 * conversion results are accumulated and averaged internally making them easier to read.
 *
 * param base  ADC16 peripheral base address.
 * param mode  Setting the hardware average mode. See "adc16_hardware_average_mode_t".
 */
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
/*!
 * brief Configures the PGA for the converter's front end.
 *
 * param base    ADC16 peripheral base address.
 * param config  Pointer to the "adc16_pga_config_t" structure. Passing "NULL" disables the feature.
 */
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

/*!
 * brief  Gets the status flags of the converter.
 *
 * param  base ADC16 peripheral base address.
 *
 * return      Flags' mask if indicated flags are asserted. See "_adc16_status_flags".
 */
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

/*!
 * brief  Clears the status flags of the converter.
 *
 * param  base ADC16 peripheral base address.
 * param  mask Mask value for the cleared flags. See "_adc16_status_flags".
 */
void ADC16_ClearStatusFlags(ADC_Type *base, uint32_t mask)
{
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
    if (0U != (mask & kADC16_CalibrationFailedFlag))
    {
        base->SC3 |= ADC_SC3_CALF_MASK;
    }
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */
}

/*!
 * brief Configures the conversion channel.
 *
 * This operation triggers the conversion when in software trigger mode. When in hardware trigger mode, this API
 * configures the channel while the external trigger source helps to trigger the conversion.
 *
 * Note that the "Channel Group" has a detailed description.
 * To allow sequential conversions of the ADC to be triggered by internal peripherals, the ADC has more than one
 * group of status and control registers, one for each conversion. The channel group parameter indicates which group of
 * registers are used, for example, channel group 0 is for Group A registers and channel group 1 is for Group B
 * registers. The
 * channel groups are used in a "ping-pong" approach to control the ADC operation.  At any point, only one of
 * the channel groups is actively controlling ADC conversions. The channel group 0 is used for both software and
 * hardware
 * trigger modes. Channel group 1 and greater indicates multiple channel group registers for
 * use only in hardware trigger mode. See the chip configuration information in the appropriate MCU reference manual for
 * the
 * number of SC1n registers (channel groups) specific to this device.  Channel group 1 or greater are not used
 * for software trigger operation. Therefore, writing to these channel groups does not initiate a new conversion.
 * Updating the channel group 0 while a different channel group is actively controlling a conversion is allowed and
 * vice versa. Writing any of the channel group registers while that specific channel group is actively controlling a
 * conversion aborts the current conversion.
 *
 * param base          ADC16 peripheral base address.
 * param channelGroup  Channel group index.
 * param config        Pointer to the "adc16_channel_config_t" structure for the conversion channel.
 */
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

/*!
 * brief  Gets the status flags of channel.
 *
 * param  base         ADC16 peripheral base address.
 * param  channelGroup Channel group index.
 *
 * return              Flags' mask if indicated flags are asserted. See "_adc16_channel_status_flags".
 */
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
