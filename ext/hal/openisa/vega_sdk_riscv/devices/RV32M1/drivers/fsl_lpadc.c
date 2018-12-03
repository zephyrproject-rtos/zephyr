/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_lpadc.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for LPADC module.
 *
 * @param base LPADC peripheral base address
 */
static uint32_t LPADC_GetInstance(ADC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to LPADC bases for each instance. */
static ADC_Type *const s_lpadcBases[] = ADC_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to LPADC clocks for each instance. */
static const clock_ip_name_t s_lpadcClocks[] = LPADC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t LPADC_GetInstance(ADC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_lpadcBases); instance++)
    {
        if (s_lpadcBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_lpadcBases));

    return instance;
}

void LPADC_Init(ADC_Type *base, const lpadc_config_t *config)
{
    /* Check if the pointer is available. */
    assert(config != NULL);

    uint32_t tmp32 = 0U;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock for LPADC instance. */
    CLOCK_EnableClock(s_lpadcClocks[LPADC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset the module. */
    LPADC_DoResetConfig(base);
    LPADC_DoResetFIFO(base);

    /* Disable the module before setting configuration. */
    LPADC_Enable(base, false);

    /* Configure the module generally. */
    if (config->enableInDozeMode)
    {
        base->CTRL &= ~ADC_CTRL_DOZEN_MASK;
    }
    else
    {
        base->CTRL |= ADC_CTRL_DOZEN_MASK;
    }

/* ADCx_CFG. */
#if defined(FSL_FEATURE_LPADC_HAS_CFG_ADCKEN) && FSL_FEATURE_LPADC_HAS_CFG_ADCKEN
    if (config->enableInternalClock)
    {
        tmp32 |= ADC_CFG_ADCKEN_MASK;
    }
#endif /* FSL_FEATURE_LPADC_HAS_CFG_ADCKEN */
#if defined(FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG) && FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG
    if (config->enableVref1LowVoltage)
    {
        tmp32 |= ADC_CFG_VREF1RNG_MASK;
    }
#endif /* FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG */
    if (config->enableAnalogPreliminary)
    {
        tmp32 |= ADC_CFG_PWREN_MASK;
    }
    tmp32 |= ADC_CFG_PUDLY(config->powerUpDelay)                /* Power up delay. */
             | ADC_CFG_REFSEL(config->referenceVoltageSource)   /* Reference voltage. */
             | ADC_CFG_TPRICTRL(config->triggerPrioirtyPolicy); /* Trigger priority policy. */
    base->CFG = tmp32;

    /* ADCx_PAUSE. */
    if (config->enableConvPause)
    {
        base->PAUSE = ADC_PAUSE_PAUSEEN_MASK | ADC_PAUSE_PAUSEDLY(config->convPauseDelay);
    }
    else
    {
        base->PAUSE = 0U;
    }

    /* ADCx_FCTRL. */
    base->FCTRL = ADC_FCTRL_FWMARK(config->FIFOWatermark);

    /* Enable the module after setting configuration. */
    LPADC_Enable(base, true);
}

void LPADC_GetDefaultConfig(lpadc_config_t *config)
{
#if defined(FSL_FEATURE_LPADC_HAS_CFG_ADCKEN) && FSL_FEATURE_LPADC_HAS_CFG_ADCKEN
    config->enableInternalClock = false;
#endif /* FSL_FEATURE_LPADC_HAS_CFG_ADCKEN */
#if defined(FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG) && FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG
    config->enableVref1LowVoltage = false;
#endif /* FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG */
    config->enableInDozeMode = true;
    config->enableAnalogPreliminary = false;
    config->powerUpDelay = 0x80;
    config->referenceVoltageSource = kLPADC_ReferenceVoltageAlt1;
    config->powerLevelMode = kLPADC_PowerLevelAlt1;
    config->triggerPrioirtyPolicy = kLPADC_TriggerPriorityPreemptImmediately;
    config->enableConvPause = false;
    config->convPauseDelay = 0U;
    config->FIFOWatermark = 0U;
}

void LPADC_Deinit(ADC_Type *base)
{
    /* Disable the module. */
    LPADC_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the clock. */
    CLOCK_DisableClock(s_lpadcClocks[LPADC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

bool LPADC_GetConvResult(ADC_Type *base, lpadc_conv_result_t *result)
{
    assert(result != NULL); /* Check if the input pointer is available. */

    uint32_t tmp32;

    tmp32 = base->RESFIFO;

    if (0U == (ADC_RESFIFO_VALID_MASK & tmp32))
    {
        return false; /* FIFO is empty. Discard any read from RESFIFO. */
    }

    result->commandIdSource = (tmp32 & ADC_RESFIFO_CMDSRC_MASK) >> ADC_RESFIFO_CMDSRC_SHIFT;
    result->loopCountIndex = (tmp32 & ADC_RESFIFO_LOOPCNT_MASK) >> ADC_RESFIFO_LOOPCNT_SHIFT;
    result->triggerIdSource = (tmp32 & ADC_RESFIFO_TSRC_MASK) >> ADC_RESFIFO_TSRC_SHIFT;
    result->convValue = (uint16_t)(tmp32 & ADC_RESFIFO_D_MASK);

    return true;
}

void LPADC_SetConvTriggerConfig(ADC_Type *base, uint32_t triggerId, const lpadc_conv_trigger_config_t *config)
{
    assert(triggerId < ADC_TCTRL_COUNT); /* Check if the triggerId is available in this device. */
    assert(config != NULL);              /* Check if the input pointer is available. */

    uint32_t tmp32;

    tmp32 = ADC_TCTRL_TCMD(config->targetCommandId) /* Trigger command select. */
            | ADC_TCTRL_TDLY(config->delayPower)    /* Trigger delay select. */
            | ADC_TCTRL_TPRI(config->priority);     /* Trigger priority setting. */
    if (config->enableHardwareTrigger)
    {
        tmp32 |= ADC_TCTRL_HTEN_MASK;
    }

    base->TCTRL[triggerId] = tmp32;
}

void LPADC_GetDefaultConvTriggerConfig(lpadc_conv_trigger_config_t *config)
{
    assert(config != NULL); /* Check if the input pointer is available. */

    config->targetCommandId = 0U;
    config->delayPower = 0U;
    config->priority = 0U;
    config->enableHardwareTrigger = false;
}

void LPADC_SetConvCommandConfig(ADC_Type *base, uint32_t commandId, const lpadc_conv_command_config_t *config)
{
    assert(commandId < (ADC_CMDL_COUNT + 1U)); /* Check if the commandId is available on this device. */
    assert(config != NULL);                    /* Check if the input pointer is available. */

    uint32_t tmp32;

    commandId--; /* The available command number are 1-15, while the index of register group are 0-14. */

    /* ADCx_CMDL. */
    tmp32 = ADC_CMDL_ADCH(config->channelNumber); /* Channel number. */
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_CSCALE) && FSL_FEATURE_LPADC_HAS_CMDL_CSCALE
    tmp32 |= ADC_CMDL_CSCALE(config->sampleScaleMode); /* Full/Part scale input voltage. */
#endif                                                 /* FSL_FEATURE_LPADC_HAS_CMDL_CSCALE */
    switch (config->sampleChannelMode)                 /* Sample input. */
    {
        case kLPADC_SampleChannelSingleEndSideB:
            tmp32 |= ADC_CMDL_ABSEL_MASK;
            break;
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_DIFF) && FSL_FEATURE_LPADC_HAS_CMDL_DIFF
        case kLPADC_SampleChannelDiffBothSideAB:
            tmp32 |= ADC_CMDL_DIFF_MASK;
            break;
        case kLPADC_SampleChannelDiffBothSideBA:
            tmp32 |= ADC_CMDL_ABSEL_MASK | ADC_CMDL_DIFF_MASK;
            break;
#endif           /* FSL_FEATURE_LPADC_HAS_CMDL_DIFF */
        default: /* kLPADC_SampleChannelSingleEndSideA. */
            break;
    }
    base->CMD[commandId].CMDL = tmp32;

    /* ADCx_CMDH. */
    tmp32 = ADC_CMDH_NEXT(config->chainedNextCommandNumber) /* Next Command Select. */
            | ADC_CMDH_LOOP(config->loopCount)              /* Loop Count Select. */
            | ADC_CMDH_AVGS(config->hardwareAverageMode)    /* Hardware Average Select. */
            | ADC_CMDH_STS(config->sampleTimeMode)          /* Sample Time Select. */
            | ADC_CMDH_CMPEN(config->hardwareCompareMode);  /* Hardware compare enable. */
    if (config->enableAutoChannelIncrement)
    {
        tmp32 |= ADC_CMDH_LWI_MASK;
    }
    base->CMD[commandId].CMDH = tmp32;

    /* Hardware compare settings.
    * Not all Command Buffers have an associated Compare Value register. The compare function is only available on
    * Command Buffers that have a corresponding Compare Value register.
    */
    if (kLPADC_HardwareCompareDisabled != config->hardwareCompareMode)
    {
        /* Check if the hardware compare feature is available for indicated command buffer. */
        assert(commandId < ADC_CV_COUNT);

        /* Set CV register. */
        base->CV[commandId] = ADC_CV_CVH(config->hardwareCompareValueHigh)   /* Compare value high. */
                              | ADC_CV_CVL(config->hardwareCompareValueLow); /* Compare value low. */
    }
}

void LPADC_GetDefaultConvCommandConfig(lpadc_conv_command_config_t *config)
{
    assert(config != NULL); /* Check if the input pointer is available. */

#if defined(FSL_FEATURE_LPADC_HAS_CMDL_CSCALE) && FSL_FEATURE_LPADC_HAS_CMDL_CSCALE
    config->sampleScaleMode = kLPADC_SampleFullScale;
#endif /* FSL_FEATURE_LPADC_HAS_CMDL_CSCALE */
    config->sampleChannelMode = kLPADC_SampleChannelSingleEndSideA;
    config->channelNumber = 0U;
    config->chainedNextCommandNumber = 0U; /* No next command defined. */
    config->enableAutoChannelIncrement = false;
    config->loopCount = 0U;
    config->hardwareAverageMode = kLPADC_HardwareAverageCount1;
    config->sampleTimeMode = kLPADC_SampleTimeADCK3;
    config->hardwareCompareMode = kLPADC_HardwareCompareDisabled;
    config->hardwareCompareValueHigh = 0U; /* No used. */
    config->hardwareCompareValueLow = 0U;  /* No used. */
}

#if defined(FSL_FEATURE_LPADC_HAS_CFG_CALOFS) && FSL_FEATURE_LPADC_HAS_CFG_CALOFS
void LPADC_EnableCalibration(ADC_Type *base, bool enable)
{
    LPADC_Enable(base, false);
    if (enable)
    {
        base->CFG |= ADC_CFG_CALOFS_MASK;
    }
    else
    {
        base->CFG &= ~ADC_CFG_CALOFS_MASK;
    }
    LPADC_Enable(base, true);
}

#if defined(FSL_FEATURE_LPADC_HAS_OFSTRIM) && FSL_FEATURE_LPADC_HAS_OFSTRIM
void LPADC_DoAutoCalibration(ADC_Type *base)
{
    assert(0u == LPADC_GetConvResultCount(base));

    uint32_t mLpadcCMDL;
    uint32_t mLpadcCMDH;
    uint32_t mLpadcTrigger;
    lpadc_conv_trigger_config_t mLpadcTriggerConfigStruct;
    lpadc_conv_command_config_t mLpadcCommandConfigStruct;
    lpadc_conv_result_t mLpadcResultConfigStruct;

    /* Enable the calibration function. */
    LPADC_EnableCalibration(base, true);

    /* Keep the CMD and TRG state here and restore it later if the calibration completes.*/
    mLpadcCMDL = base->CMD[0].CMDL; /* CMD1L. */
    mLpadcCMDH = base->CMD[0].CMDH; /* CMD1H. */
    mLpadcTrigger = base->TCTRL[0]; /* Trigger0. */

    /* Set trigger0 configuration - for software trigger. */
    LPADC_GetDefaultConvTriggerConfig(&mLpadcTriggerConfigStruct);
    mLpadcTriggerConfigStruct.targetCommandId = 1U;                   /* CMD1 is executed. */
    LPADC_SetConvTriggerConfig(base, 0U, &mLpadcTriggerConfigStruct); /* Configurate the trigger0. */

    /* Set conversion CMD configuration. */
    LPADC_GetDefaultConvCommandConfig(&mLpadcCommandConfigStruct);
    mLpadcCommandConfigStruct.hardwareAverageMode = kLPADC_HardwareAverageCount128;
    LPADC_SetConvCommandConfig(base, 1U, &mLpadcCommandConfigStruct); /* Set CMD1 configuration. */

    /* Do calibration. */
    LPADC_DoSoftwareTrigger(base, 1U); /* 1U is trigger0 mask. */
    while (!LPADC_GetConvResult(base, &mLpadcResultConfigStruct))
    {
    }
    /* The valid bits of data are bits 14:3 in the RESFIFO register. */
    LPADC_SetOffsetValue(base, (mLpadcResultConfigStruct.convValue) >> 3U);
    /* Disable the calibration function. */
    LPADC_EnableCalibration(base, false);

    /* restore CMD and TRG registers. */
    base->CMD[0].CMDL = mLpadcCMDL; /* CMD1L. */
    base->CMD[0].CMDH = mLpadcCMDH; /* CMD1H. */
    base->TCTRL[0] = mLpadcTrigger; /* Trigger0. */
}
#endif /* FSL_FEATURE_LPADC_HAS_OFSTRIM */
#endif /* FSL_FEATURE_LPADC_HAS_CFG_CALOFS */
