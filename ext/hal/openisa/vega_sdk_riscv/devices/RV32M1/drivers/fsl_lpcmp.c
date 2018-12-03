/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_lpcmp.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if defined(LPCMP_CLOCKS)
/*!
 * @brief Get instance number for LPCMP module.
 *
 * @param base LPCMP peripheral base address
 */
static uint32_t LPCMP_GetInstance(LPCMP_Type *base);
#endif /* LPCMP_CLOCKS */

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if defined(LPCMP_CLOCKS)
/*! @brief Pointers to LPCMP bases for each instance. */
static LPCMP_Type *const s_lpcmpBases[] = LPCMP_BASE_PTRS;
/*! @brief Pointers to LPCMP clocks for each instance. */
static const clock_ip_name_t s_lpcmpClocks[] = LPCMP_CLOCKS;
#endif /* LPCMP_CLOCKS */

/*******************************************************************************
 * Codes
 ******************************************************************************/
#if defined(LPCMP_CLOCKS)
static uint32_t LPCMP_GetInstance(LPCMP_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_lpcmpBases); instance++)
    {
        if (s_lpcmpBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_lpcmpBases));

    return instance;
}
#endif /* LPCMP_CLOCKS */

void LPCMP_Init(LPCMP_Type *base, const lpcmp_config_t *config)
{
    assert(config != NULL);

    uint32_t tmp32;

#if defined(LPCMP_CLOCKS)
    /* Enable LPCMP clock. */
    CLOCK_EnableClock(s_lpcmpClocks[LPCMP_GetInstance(base)]);
#endif /* LPCMP_CLOCKS */

    /* Configure. */
    LPCMP_Enable(base, false);
    /* CCR0 register. */
    if (config->enableStopMode)
    {
        base->CCR0 |= LPCMP_CCR0_CMP_STOP_EN_MASK;
    }
    else
    {
        base->CCR0 &= ~LPCMP_CCR0_CMP_STOP_EN_MASK;
    }
    /* CCR1 register. */
    tmp32 = base->CCR1 & ~(LPCMP_CCR1_COUT_PEN_MASK | LPCMP_CCR1_COUT_SEL_MASK | LPCMP_CCR1_COUT_INV_MASK);
    if (config->enableOutputPin)
    {
        tmp32 |= LPCMP_CCR1_COUT_PEN_MASK;
    }
    if (config->useUnfilteredOutput)
    {
        tmp32 |= LPCMP_CCR1_COUT_SEL_MASK;
    }
    if (config->enableInvertOutput)
    {
        tmp32 |= LPCMP_CCR1_COUT_INV_MASK;
    }
    base->CCR1 = tmp32;
    /* CCR2 register. */
    tmp32 = base->CCR2 & ~(LPCMP_CCR2_HYSTCTR_MASK | LPCMP_CCR2_CMP_NPMD_MASK | LPCMP_CCR2_CMP_HPMD_MASK);
    tmp32 |= LPCMP_CCR2_HYSTCTR(config->hysteresisMode);
    tmp32 |= ((uint32_t)(config->powerMode) << LPCMP_CCR2_CMP_HPMD_SHIFT);
    base->CCR2 = tmp32;

    LPCMP_Enable(base, true); /* Enable the LPCMP module. */
}

void LPCMP_Deinit(LPCMP_Type *base)
{
    /* Disable the LPCMP module. */
    LPCMP_Enable(base, false);
#if defined(LPCMP_CLOCKS)
    /* Disable the clock for LPCMP. */
    CLOCK_DisableClock(s_lpcmpClocks[LPCMP_GetInstance(base)]);
#endif /* LPCMP_CLOCKS */
}

void LPCMP_GetDefaultConfig(lpcmp_config_t *config)
{
    config->enableStopMode = false;
    config->enableOutputPin = false;
    config->useUnfilteredOutput = false;
    config->enableInvertOutput = false;
    config->hysteresisMode = kLPCMP_HysteresisLevel0;
    config->powerMode = kLPCMP_LowSpeedPowerMode;
}

void LPCMP_SetInputChannels(LPCMP_Type *base, uint32_t positiveChannel, uint32_t negativeChannel)
{
    uint32_t tmp32;

    tmp32 = base->CCR2 & ~(LPCMP_CCR2_PSEL_MASK | LPCMP_CCR2_MSEL_MASK);
    tmp32 |= LPCMP_CCR2_PSEL(positiveChannel) | LPCMP_CCR2_MSEL(negativeChannel);
    base->CCR2 = tmp32;
}

void LPCMP_SetFilterConfig(LPCMP_Type *base, const lpcmp_filter_config_t *config)
{
    assert(config != NULL);

    uint32_t tmp32;

    tmp32 = base->CCR1 & ~(LPCMP_CCR1_FILT_PER_MASK | LPCMP_CCR1_FILT_CNT_MASK | LPCMP_CCR1_SAMPLE_EN_MASK);
    if (config->enableSample)
    {
        tmp32 |= LPCMP_CCR1_SAMPLE_EN_MASK;
    }
    tmp32 |= LPCMP_CCR1_FILT_PER(config->filterSamplePeriod) | LPCMP_CCR1_FILT_CNT(config->filterSampleCount);
    base->CCR1 = tmp32;
}

void LPCMP_SetDACConfig(LPCMP_Type *base, const lpcmp_dac_config_t *config)
{
    uint32_t tmp32;
    if (config == NULL)
    {
        tmp32 = 0U; /* Disable internal DAC. */
    }
    else
    {
        tmp32 = LPCMP_DCR_VRSEL(config->referenceVoltageSource) | LPCMP_DCR_DAC_DATA(config->DACValue);
        if (config->enableLowPowerMode)
        {
            tmp32 |= LPCMP_DCR_DAC_HPMD_MASK;
        }
        tmp32 |= LPCMP_DCR_DAC_EN_MASK;
    }
    base->DCR = tmp32;
}
