/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
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

#include "fsl_cmp.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for CMP module.
 *
 * @param base CMP peripheral base address
 */
static uint32_t CMP_GetInstance(CMP_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to CMP bases for each instance. */
static CMP_Type *const s_cmpBases[] = CMP_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to CMP clocks for each instance. */
static const clock_ip_name_t s_cmpClocks[] = CMP_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/
static uint32_t CMP_GetInstance(CMP_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_cmpBases); instance++)
    {
        if (s_cmpBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_cmpBases));

    return instance;
}

void CMP_Init(CMP_Type *base, const cmp_config_t *config)
{
    assert(NULL != config);

    uint8_t tmp8;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_cmpClocks[CMP_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Configure. */
    CMP_Enable(base, false); /* Disable the CMP module during configuring. */
    /* CMPx_CR1. */
    tmp8 = base->CR1 & ~(CMP_CR1_PMODE_MASK | CMP_CR1_INV_MASK | CMP_CR1_COS_MASK | CMP_CR1_OPE_MASK);
    if (config->enableHighSpeed)
    {
        tmp8 |= CMP_CR1_PMODE_MASK;
    }
    if (config->enableInvertOutput)
    {
        tmp8 |= CMP_CR1_INV_MASK;
    }
    if (config->useUnfilteredOutput)
    {
        tmp8 |= CMP_CR1_COS_MASK;
    }
    if (config->enablePinOut)
    {
        tmp8 |= CMP_CR1_OPE_MASK;
    }
#if defined(FSL_FEATURE_CMP_HAS_TRIGGER_MODE) && FSL_FEATURE_CMP_HAS_TRIGGER_MODE
    if (config->enableTriggerMode)
    {
        tmp8 |= CMP_CR1_TRIGM_MASK;
    }
    else
    {
        tmp8 &= ~CMP_CR1_TRIGM_MASK;
    }
#endif /* FSL_FEATURE_CMP_HAS_TRIGGER_MODE */
    base->CR1 = tmp8;

    /* CMPx_CR0. */
    tmp8 = base->CR0 & ~CMP_CR0_HYSTCTR_MASK;
    tmp8 |= CMP_CR0_HYSTCTR(config->hysteresisMode);
    base->CR0 = tmp8;

    CMP_Enable(base, config->enableCmp); /* Enable the CMP module after configured or not. */
}

void CMP_Deinit(CMP_Type *base)
{
    /* Disable the CMP module. */
    CMP_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_cmpClocks[CMP_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void CMP_GetDefaultConfig(cmp_config_t *config)
{
    assert(NULL != config);

    config->enableCmp = true; /* Enable the CMP module after initialization. */
    config->hysteresisMode = kCMP_HysteresisLevel0;
    config->enableHighSpeed = false;
    config->enableInvertOutput = false;
    config->useUnfilteredOutput = false;
    config->enablePinOut = false;
#if defined(FSL_FEATURE_CMP_HAS_TRIGGER_MODE) && FSL_FEATURE_CMP_HAS_TRIGGER_MODE
    config->enableTriggerMode = false;
#endif /* FSL_FEATURE_CMP_HAS_TRIGGER_MODE */
}

void CMP_SetInputChannels(CMP_Type *base, uint8_t positiveChannel, uint8_t negativeChannel)
{
    uint8_t tmp8 = base->MUXCR;

    tmp8 &= ~(CMP_MUXCR_PSEL_MASK | CMP_MUXCR_MSEL_MASK);
    tmp8 |= CMP_MUXCR_PSEL(positiveChannel) | CMP_MUXCR_MSEL(negativeChannel);
    base->MUXCR = tmp8;
}

#if defined(FSL_FEATURE_CMP_HAS_DMA) && FSL_FEATURE_CMP_HAS_DMA
void CMP_EnableDMA(CMP_Type *base, bool enable)
{
    uint8_t tmp8 = base->SCR & ~(CMP_SCR_CFR_MASK | CMP_SCR_CFF_MASK); /* To avoid change the w1c bits. */

    if (enable)
    {
        tmp8 |= CMP_SCR_DMAEN_MASK;
    }
    else
    {
        tmp8 &= ~CMP_SCR_DMAEN_MASK;
    }
    base->SCR = tmp8;
}
#endif /* FSL_FEATURE_CMP_HAS_DMA */

void CMP_SetFilterConfig(CMP_Type *base, const cmp_filter_config_t *config)
{
    assert(NULL != config);

    uint8_t tmp8;

#if defined(FSL_FEATURE_CMP_HAS_EXTERNAL_SAMPLE_SUPPORT) && FSL_FEATURE_CMP_HAS_EXTERNAL_SAMPLE_SUPPORT
    /* Choose the clock source for sampling. */
    if (config->enableSample)
    {
        base->CR1 |= CMP_CR1_SE_MASK; /* Choose the external SAMPLE clock. */
    }
    else
    {
        base->CR1 &= ~CMP_CR1_SE_MASK; /* Choose the internal divided bus clock. */
    }
#endif /* FSL_FEATURE_CMP_HAS_EXTERNAL_SAMPLE_SUPPORT */
    /* Set the filter count. */
    tmp8 = base->CR0 & ~CMP_CR0_FILTER_CNT_MASK;
    tmp8 |= CMP_CR0_FILTER_CNT(config->filterCount);
    base->CR0 = tmp8;
    /* Set the filter period. It is used as the divider to bus clock. */
    base->FPR = CMP_FPR_FILT_PER(config->filterPeriod);
}

void CMP_SetDACConfig(CMP_Type *base, const cmp_dac_config_t *config)
{
    uint8_t tmp8 = 0U;

    if (NULL == config)
    {
        /* Passing "NULL" as input parameter means no available configuration. So the DAC feature is disabled.*/
        base->DACCR = 0U;
        return;
    }
    /* CMPx_DACCR. */
    tmp8 |= CMP_DACCR_DACEN_MASK; /* Enable the internal DAC. */
    if (kCMP_VrefSourceVin2 == config->referenceVoltageSource)
    {
        tmp8 |= CMP_DACCR_VRSEL_MASK;
    }
    tmp8 |= CMP_DACCR_VOSEL(config->DACValue);

    base->DACCR = tmp8;
}

void CMP_EnableInterrupts(CMP_Type *base, uint32_t mask)
{
    uint8_t tmp8 = base->SCR & ~(CMP_SCR_CFR_MASK | CMP_SCR_CFF_MASK); /* To avoid change the w1c bits. */

    if (0U != (kCMP_OutputRisingInterruptEnable & mask))
    {
        tmp8 |= CMP_SCR_IER_MASK;
    }
    if (0U != (kCMP_OutputFallingInterruptEnable & mask))
    {
        tmp8 |= CMP_SCR_IEF_MASK;
    }
    base->SCR = tmp8;
}

void CMP_DisableInterrupts(CMP_Type *base, uint32_t mask)
{
    uint8_t tmp8 = base->SCR & ~(CMP_SCR_CFR_MASK | CMP_SCR_CFF_MASK); /* To avoid change the w1c bits. */

    if (0U != (kCMP_OutputRisingInterruptEnable & mask))
    {
        tmp8 &= ~CMP_SCR_IER_MASK;
    }
    if (0U != (kCMP_OutputFallingInterruptEnable & mask))
    {
        tmp8 &= ~CMP_SCR_IEF_MASK;
    }
    base->SCR = tmp8;
}

uint32_t CMP_GetStatusFlags(CMP_Type *base)
{
    uint32_t ret32 = 0U;

    if (0U != (CMP_SCR_CFR_MASK & base->SCR))
    {
        ret32 |= kCMP_OutputRisingEventFlag;
    }
    if (0U != (CMP_SCR_CFF_MASK & base->SCR))
    {
        ret32 |= kCMP_OutputFallingEventFlag;
    }
    if (0U != (CMP_SCR_COUT_MASK & base->SCR))
    {
        ret32 |= kCMP_OutputAssertEventFlag;
    }
    return ret32;
}

void CMP_ClearStatusFlags(CMP_Type *base, uint32_t mask)
{
    uint8_t tmp8 = base->SCR & ~(CMP_SCR_CFR_MASK | CMP_SCR_CFF_MASK); /* To avoid change the w1c bits. */

    if (0U != (kCMP_OutputRisingEventFlag & mask))
    {
        tmp8 |= CMP_SCR_CFR_MASK;
    }
    if (0U != (kCMP_OutputFallingEventFlag & mask))
    {
        tmp8 |= CMP_SCR_CFF_MASK;
    }
    base->SCR = tmp8;
}
