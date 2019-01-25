/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_adc.h"
#include "fsl_clock.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.lpc_adc"
#endif

static ADC_Type *const s_adcBases[] = ADC_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
static const clock_ip_name_t s_adcClocks[] = ADC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

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

/*!
 * brief Initialize the ADC module.
 *
 * param base ADC peripheral base address.
 * param config Pointer to configuration structure, see to #adc_config_t.
 */
void ADC_Init(ADC_Type *base, const adc_config_t *config)
{
    assert(config != NULL);

    uint32_t tmp32 = 0U;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable clock. */
    CLOCK_EnableClock(s_adcClocks[ADC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Disable the interrupts. */
    base->INTEN = 0U; /* Quickly disable all the interrupts. */

    /* Configure the ADC block. */
    tmp32 = ADC_CTRL_CLKDIV(config->clockDividerNumber);

#if defined(FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE) & FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE
    /* Async or Sync clock mode. */
    switch (config->clockMode)
    {
        case kADC_ClockAsynchronousMode:
            tmp32 |= ADC_CTRL_ASYNMODE_MASK;
            break;
        default: /* kADC_ClockSynchronousMode */
            break;
    }
#endif /* FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE. */

#if defined(FSL_FEATURE_ADC_HAS_CTRL_RESOL) & FSL_FEATURE_ADC_HAS_CTRL_RESOL
    /* Resolution. */
    tmp32 |= ADC_CTRL_RESOL(config->resolution);
#endif /* FSL_FEATURE_ADC_HAS_CTRL_RESOL. */

#if defined(FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL) & FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL
    /* Bypass calibration. */
    if (config->enableBypassCalibration)
    {
        tmp32 |= ADC_CTRL_BYPASSCAL_MASK;
    }
#endif /* FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL. */

#if defined(FSL_FEATURE_ADC_HAS_CTRL_TSAMP) & FSL_FEATURE_ADC_HAS_CTRL_TSAMP
    /* Sample time clock count. */
    tmp32 |= ADC_CTRL_TSAMP(config->sampleTimeNumber);
#endif /* FSL_FEATURE_ADC_HAS_CTRL_TSAMP. */

#if defined(FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE) & FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE
    if (config->enableLowPowerMode)
    {
        tmp32 |= ADC_CTRL_LPWRMODE_MASK;
    }
#endif /* FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE. */

    base->CTRL = tmp32;

#if defined(FSL_FEATURE_ADC_HAS_TRIM_REG) & FSL_FEATURE_ADC_HAS_TRIM_REG
    base->TRM &= ~ADC_TRM_VRANGE_MASK;
    base->TRM |= ADC_TRM_VRANGE(config->voltageRange);
#endif /* FSL_FEATURE_ADC_HAS_TRIM_REG. */
}

/*!
 * brief Gets an available pre-defined settings for initial configuration.
 *
 * This function initializes the initial configuration structure with an available settings. The default values are:
 * code
 *   config->clockMode = kADC_ClockSynchronousMode;
 *   config->clockDividerNumber = 0U;
 *   config->resolution = kADC_Resolution12bit;
 *   config->enableBypassCalibration = false;
 *   config->sampleTimeNumber = 0U;
 * endcode
 * param config Pointer to configuration structure.
 */
void ADC_GetDefaultConfig(adc_config_t *config)
{
    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

#if defined(FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE) & FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE

    config->clockMode = kADC_ClockSynchronousMode;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE. */

    config->clockDividerNumber = 0U;
#if defined(FSL_FEATURE_ADC_HAS_CTRL_RESOL) & FSL_FEATURE_ADC_HAS_CTRL_RESOL
    config->resolution = kADC_Resolution12bit;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_RESOL. */
#if defined(FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL) & FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL
    config->enableBypassCalibration = false;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL. */
#if defined(FSL_FEATURE_ADC_HAS_CTRL_TSAMP) & FSL_FEATURE_ADC_HAS_CTRL_TSAMP
    config->sampleTimeNumber = 0U;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_TSAMP. */
#if defined(FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE) & FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE
    config->enableLowPowerMode = false;
#endif /* FSL_FEATURE_ADC_HAS_CTRL_LPWRMODE. */
#if defined(FSL_FEATURE_ADC_HAS_TRIM_REG) & FSL_FEATURE_ADC_HAS_TRIM_REG
    config->voltageRange = kADC_HighVoltageRange;
#endif /* FSL_FEATURE_ADC_HAS_TRIM_REG. */
}

/*!
 * brief Deinitialize the ADC module.
 *
 * param base ADC peripheral base address.
 */
void ADC_Deinit(ADC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_adcClocks[ADC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

#if !(defined(FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC) && FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC)
#if defined(FSL_FEATURE_ADC_HAS_CALIB_REG) & FSL_FEATURE_ADC_HAS_CALIB_REG
/*!
 * brief Do the self calibration. To calibrate the ADC, set the ADC clock to 500 kHz.
 *        In order to achieve the specified ADC accuracy, the A/D converter must be recalibrated, at a minimum,
 *        following every chip reset before initiating normal ADC operation.
 *
 * param base ADC peripheral base address.
 * param frequency The ststem clock frequency to ADC.
 * retval true  Calibration succeed.
 * retval false Calibration failed.
 */
bool ADC_DoSelfCalibration(ADC_Type *base)
{
    uint32_t i;

    /* Enable the converter. */
    /* This bit acn only be set 1 by software. It is cleared automatically whenever the ADC is powered down.
       This bit should be set after at least 10 ms after the ADC is powered on. */
    base->STARTUP = ADC_STARTUP_ADC_ENA_MASK;
    for (i = 0U; i < 0x10; i++) /* Wait a few clocks to startup up. */
    {
        __ASM("NOP");
    }
    if (!(base->STARTUP & ADC_STARTUP_ADC_ENA_MASK))
    {
        return false; /* ADC is not powered up. */
    }

    /* If not in by-pass mode, do the calibration. */
    if ((ADC_CALIB_CALREQD_MASK == (base->CALIB & ADC_CALIB_CALREQD_MASK)) &&
        (0U == (base->CTRL & ADC_CTRL_BYPASSCAL_MASK)))
    {
        /* Calibration is needed, do it now. */
        base->CALIB = ADC_CALIB_CALIB_MASK;
        i = 0xF0000;
        while ((ADC_CALIB_CALIB_MASK == (base->CALIB & ADC_CALIB_CALIB_MASK)) && (--i))
        {
        }
        if (i == 0U)
        {
            return false; /* Calibration timeout. */
        }
    }

    /* A dummy conversion cycle will be performed. */
    base->STARTUP |= ADC_STARTUP_ADC_INIT_MASK;
    i = 0x7FFFF;
    while ((ADC_STARTUP_ADC_INIT_MASK == (base->STARTUP & ADC_STARTUP_ADC_INIT_MASK)) && (--i))
    {
    }
    if (i == 0U)
    {
        return false;
    }

    return true;
}
#else
/*!
 * brief Do the self calibration. To calibrate the ADC, set the ADC clock to 500 kHz.
 *        In order to achieve the specified ADC accuracy, the A/D converter must be recalibrated, at a minimum,
 *        following every chip reset before initiating normal ADC operation.
 *
 * param base ADC peripheral base address.
 * param frequency The ststem clock frequency to ADC.
 * retval true  Calibration succeed.
 * retval false Calibration failed.
 */
bool ADC_DoSelfCalibration(ADC_Type *base, uint32_t frequency)
{
    uint32_t tmp32;
    uint32_t i = 0xF0000;

    /* Store the current contents of the ADC CTRL register. */
    tmp32 = base->CTRL;

    /* Start ADC self-calibration. */
    base->CTRL |= ADC_CTRL_CALMODE_MASK;

    /* Divide the system clock to yield an ADC clock of about 500 kHz. */
    base->CTRL &= ~ADC_CTRL_CLKDIV_MASK;
    base->CTRL |= ADC_CTRL_CLKDIV((frequency / 500000U) - 1U);

    /* Clear the LPWR bit. */
    base->CTRL &= ~ADC_CTRL_LPWRMODE_MASK;

    /* Wait for the completion of calibration. */
    while ((ADC_CTRL_CALMODE_MASK == (base->CTRL & ADC_CTRL_CALMODE_MASK)) && (--i))
    {
    }
    /* Restore the contents of the ADC CTRL register. */
    base->CTRL = tmp32;

    /* Judge whether the calibration is overtime.  */
    if (i == 0U)
    {
        return false; /* Calibration timeout. */
    }

    return true;
}
#endif /* FSL_FEATURE_ADC_HAS_CALIB_REG */
#endif /* FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC*/

/*!
 * brief Configure the conversion sequence A.
 *
 * param base ADC peripheral base address.
 * param config Pointer to configuration structure, see to #adc_conv_seq_config_t.
 */
void ADC_SetConvSeqAConfig(ADC_Type *base, const adc_conv_seq_config_t *config)
{
    assert(config != NULL);

    uint32_t tmp32;

    tmp32 = ADC_SEQ_CTRL_CHANNELS(config->channelMask)   /* Channel mask. */
            | ADC_SEQ_CTRL_TRIGGER(config->triggerMask); /* Trigger mask. */

    /* Polarity for tirgger signal. */
    switch (config->triggerPolarity)
    {
        case kADC_TriggerPolarityPositiveEdge:
            tmp32 |= ADC_SEQ_CTRL_TRIGPOL_MASK;
            break;
        default: /* kADC_TriggerPolarityNegativeEdge */
            break;
    }

    /* Bypass the clock Sync. */
    if (config->enableSyncBypass)
    {
        tmp32 |= ADC_SEQ_CTRL_SYNCBYPASS_MASK;
    }

    /* Interrupt point. */
    switch (config->interruptMode)
    {
        case kADC_InterruptForEachSequence:
            tmp32 |= ADC_SEQ_CTRL_MODE_MASK;
            break;
        default: /* kADC_InterruptForEachConversion */
            break;
    }

    /* One trigger for a conversion, or for a sequence. */
    if (config->enableSingleStep)
    {
        tmp32 |= ADC_SEQ_CTRL_SINGLESTEP_MASK;
    }

    base->SEQ_CTRL[0] = tmp32;
}

/*!
 * brief Configure the conversion sequence B.
 *
 * param base ADC peripheral base address.
 * param config Pointer to configuration structure, see to #adc_conv_seq_config_t.
 */
void ADC_SetConvSeqBConfig(ADC_Type *base, const adc_conv_seq_config_t *config)
{
    assert(config != NULL);

    uint32_t tmp32;

    tmp32 = ADC_SEQ_CTRL_CHANNELS(config->channelMask)   /* Channel mask. */
            | ADC_SEQ_CTRL_TRIGGER(config->triggerMask); /* Trigger mask. */

    /* Polarity for tirgger signal. */
    switch (config->triggerPolarity)
    {
        case kADC_TriggerPolarityPositiveEdge:
            tmp32 |= ADC_SEQ_CTRL_TRIGPOL_MASK;
            break;
        default: /* kADC_TriggerPolarityPositiveEdge */
            break;
    }

    /* Bypass the clock Sync. */
    if (config->enableSyncBypass)
    {
        tmp32 |= ADC_SEQ_CTRL_SYNCBYPASS_MASK;
    }

    /* Interrupt point. */
    switch (config->interruptMode)
    {
        case kADC_InterruptForEachSequence:
            tmp32 |= ADC_SEQ_CTRL_MODE_MASK;
            break;
        default: /* kADC_InterruptForEachConversion */
            break;
    }

    /* One trigger for a conversion, or for a sequence. */
    if (config->enableSingleStep)
    {
        tmp32 |= ADC_SEQ_CTRL_SINGLESTEP_MASK;
    }

    base->SEQ_CTRL[1] = tmp32;
}

/*!
 * brief Get the global ADC conversion infomation of sequence A.
 *
 * param base ADC peripheral base address.
 * param info Pointer to information structure, see to #adc_result_info_t;
 * retval true  The conversion result is ready.
 * retval false The conversion result is not ready yet.
 */
bool ADC_GetConvSeqAGlobalConversionResult(ADC_Type *base, adc_result_info_t *info)
{
    assert(info != NULL);

    uint32_t tmp32 = base->SEQ_GDAT[0]; /* Read to clear the status. */

    if (0U == (ADC_SEQ_GDAT_DATAVALID_MASK & tmp32))
    {
        return false;
    }

    info->result = (tmp32 & ADC_SEQ_GDAT_RESULT_MASK) >> ADC_SEQ_GDAT_RESULT_SHIFT;
    info->thresholdCompareStatus =
        (adc_threshold_compare_status_t)((tmp32 & ADC_SEQ_GDAT_THCMPRANGE_MASK) >> ADC_SEQ_GDAT_THCMPRANGE_SHIFT);
    info->thresholdCorssingStatus =
        (adc_threshold_crossing_status_t)((tmp32 & ADC_SEQ_GDAT_THCMPCROSS_MASK) >> ADC_SEQ_GDAT_THCMPCROSS_SHIFT);
    info->channelNumber = (tmp32 & ADC_SEQ_GDAT_CHN_MASK) >> ADC_SEQ_GDAT_CHN_SHIFT;
    info->overrunFlag = ((tmp32 & ADC_SEQ_GDAT_OVERRUN_MASK) == ADC_SEQ_GDAT_OVERRUN_MASK);

    return true;
}

/*!
 * brief Get the global ADC conversion infomation of sequence B.
 *
 * param base ADC peripheral base address.
 * param info Pointer to information structure, see to #adc_result_info_t;
 * retval true  The conversion result is ready.
 * retval false The conversion result is not ready yet.
 */
bool ADC_GetConvSeqBGlobalConversionResult(ADC_Type *base, adc_result_info_t *info)
{
    assert(info != NULL);

    uint32_t tmp32 = base->SEQ_GDAT[1]; /* Read to clear the status. */

    if (0U == (ADC_SEQ_GDAT_DATAVALID_MASK & tmp32))
    {
        return false;
    }

    info->result = (tmp32 & ADC_SEQ_GDAT_RESULT_MASK) >> ADC_SEQ_GDAT_RESULT_SHIFT;
    info->thresholdCompareStatus =
        (adc_threshold_compare_status_t)((tmp32 & ADC_SEQ_GDAT_THCMPRANGE_MASK) >> ADC_SEQ_GDAT_THCMPRANGE_SHIFT);
    info->thresholdCorssingStatus =
        (adc_threshold_crossing_status_t)((tmp32 & ADC_SEQ_GDAT_THCMPCROSS_MASK) >> ADC_SEQ_GDAT_THCMPCROSS_SHIFT);
    info->channelNumber = (tmp32 & ADC_SEQ_GDAT_CHN_MASK) >> ADC_SEQ_GDAT_CHN_SHIFT;
    info->overrunFlag = ((tmp32 & ADC_SEQ_GDAT_OVERRUN_MASK) == ADC_SEQ_GDAT_OVERRUN_MASK);

    return true;
}

/*!
 * brief Get the channel's ADC conversion completed under each conversion sequence.
 *
 * param base ADC peripheral base address.
 * param channel The indicated channel number.
 * param info Pointer to information structure, see to #adc_result_info_t;
 * retval true  The conversion result is ready.
 * retval false The conversion result is not ready yet.
 */
bool ADC_GetChannelConversionResult(ADC_Type *base, uint32_t channel, adc_result_info_t *info)
{
    assert(info != NULL);
    assert(channel < ADC_DAT_COUNT);

    uint32_t tmp32 = base->DAT[channel]; /* Read to clear the status. */

    if (0U == (ADC_DAT_DATAVALID_MASK & tmp32))
    {
        return false;
    }

    info->result = (tmp32 & ADC_DAT_RESULT_MASK) >> ADC_DAT_RESULT_SHIFT;
    info->thresholdCompareStatus =
        (adc_threshold_compare_status_t)((tmp32 & ADC_DAT_THCMPRANGE_MASK) >> ADC_DAT_THCMPRANGE_SHIFT);
    info->thresholdCorssingStatus =
        (adc_threshold_crossing_status_t)((tmp32 & ADC_DAT_THCMPCROSS_MASK) >> ADC_DAT_THCMPCROSS_SHIFT);
    info->channelNumber = (tmp32 & ADC_DAT_CHANNEL_MASK) >> ADC_DAT_CHANNEL_SHIFT;
    info->overrunFlag = ((tmp32 & ADC_DAT_OVERRUN_MASK) == ADC_DAT_OVERRUN_MASK);

    return true;
}
