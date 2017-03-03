/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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

#include "fsl_dac.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for DAC module.
 *
 * @param base DAC peripheral base address
 */
static uint32_t DAC_GetInstance(DAC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to DAC bases for each instance. */
static DAC_Type *const s_dacBases[] = DAC_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to DAC clocks for each instance. */
static const clock_ip_name_t s_dacClocks[] = DAC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/
static uint32_t DAC_GetInstance(DAC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < FSL_FEATURE_SOC_DAC_COUNT; instance++)
    {
        if (s_dacBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < FSL_FEATURE_SOC_DAC_COUNT);

    return instance;
}

void DAC_Init(DAC_Type *base, const dac_config_t *config)
{
    assert(NULL != config);

    uint8_t tmp8;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_dacClocks[DAC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Configure. */
    /* DACx_C0. */
    tmp8 = base->C0 & ~(DAC_C0_DACRFS_MASK | DAC_C0_LPEN_MASK);
    if (kDAC_ReferenceVoltageSourceVref2 == config->referenceVoltageSource)
    {
        tmp8 |= DAC_C0_DACRFS_MASK;
    }
    if (config->enableLowPowerMode)
    {
        tmp8 |= DAC_C0_LPEN_MASK;
    }
    base->C0 = tmp8;

    /* DAC_Enable(base, true); */
    /* Tip: The DAC output can be enabled till then after user sets their own available data in application. */
}

void DAC_Deinit(DAC_Type *base)
{
    DAC_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_dacClocks[DAC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void DAC_GetDefaultConfig(dac_config_t *config)
{
    assert(NULL != config);

    config->referenceVoltageSource = kDAC_ReferenceVoltageSourceVref2;
    config->enableLowPowerMode = false;
}

void DAC_SetBufferConfig(DAC_Type *base, const dac_buffer_config_t *config)
{
    assert(NULL != config);

    uint8_t tmp8;

    /* DACx_C0. */
    tmp8 = base->C0 & ~(DAC_C0_DACTRGSEL_MASK);
    if (kDAC_BufferTriggerBySoftwareMode == config->triggerMode)
    {
        tmp8 |= DAC_C0_DACTRGSEL_MASK;
    }
    base->C0 = tmp8;

    /* DACx_C1. */
    tmp8 = base->C1 &
           ~(
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
               DAC_C1_DACBFWM_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */
               DAC_C1_DACBFMD_MASK);
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
    tmp8 |= DAC_C1_DACBFWM(config->watermark);
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */
    tmp8 |= DAC_C1_DACBFMD(config->workMode);
    base->C1 = tmp8;

    /* DACx_C2. */
    tmp8 = base->C2 & ~DAC_C2_DACBFUP_MASK;
    tmp8 |= DAC_C2_DACBFUP(config->upperLimit);
    base->C2 = tmp8;
}

void DAC_GetDefaultBufferConfig(dac_buffer_config_t *config)
{
    assert(NULL != config);

    config->triggerMode = kDAC_BufferTriggerBySoftwareMode;
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
    config->watermark = kDAC_BufferWatermark1Word;
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */
    config->workMode = kDAC_BufferWorkAsNormalMode;
    config->upperLimit = DAC_DATL_COUNT - 1U;
}

void DAC_SetBufferValue(DAC_Type *base, uint8_t index, uint16_t value)
{
    assert(index < DAC_DATL_COUNT);

    base->DAT[index].DATL = (uint8_t)(0xFFU & value);         /* Low 8-bit. */
    base->DAT[index].DATH = (uint8_t)((0xF00U & value) >> 8); /* High 4-bit. */
}

void DAC_SetBufferReadPointer(DAC_Type *base, uint8_t index)
{
    assert(index < DAC_DATL_COUNT);

    uint8_t tmp8 = base->C2 & ~DAC_C2_DACBFRP_MASK;

    tmp8 |= DAC_C2_DACBFRP(index);
    base->C2 = tmp8;
}

void DAC_EnableBufferInterrupts(DAC_Type *base, uint32_t mask)
{
    mask &= (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
        DAC_C0_DACBWIEN_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
        DAC_C0_DACBTIEN_MASK | DAC_C0_DACBBIEN_MASK);
    base->C0 |= ((uint8_t)mask); /* Write 1 to enable. */
}

void DAC_DisableBufferInterrupts(DAC_Type *base, uint32_t mask)
{
    mask &= (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
        DAC_C0_DACBWIEN_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
        DAC_C0_DACBTIEN_MASK | DAC_C0_DACBBIEN_MASK);
    base->C0 &= (uint8_t)(~((uint8_t)mask)); /* Write 0 to disable. */
}

uint32_t DAC_GetBufferStatusFlags(DAC_Type *base)
{
    return (uint32_t)(base->SR & (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
                                     DAC_SR_DACBFWMF_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
                                     DAC_SR_DACBFRPTF_MASK | DAC_SR_DACBFRPBF_MASK));
}

void DAC_ClearBufferStatusFlags(DAC_Type *base, uint32_t mask)
{
    mask &= (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
        DAC_SR_DACBFWMF_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
        DAC_SR_DACBFRPTF_MASK | DAC_SR_DACBFRPBF_MASK);
    base->SR &= (uint8_t)(~((uint8_t)mask)); /* Write 0 to clear flags. */
}
