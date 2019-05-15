/*
 * Copyright (c) 2018, NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "fsl_anactrl.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.anactrl"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for ANACTRL module.
 *
 * @param base ANACTRL peripheral base address
 */
static uint32_t ANACTRL_GetInstance(ANACTRL_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to ANACTRL bases for each instance. */
static ANACTRL_Type *const s_anactrlBases[] = ANACTRL_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to ANACTRL clocks for each instance. */
static const clock_ip_name_t s_anactrlClocks[] = ANALOGCTRL_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * brief Get the ANACTRL instance from peripheral base address.
 *
 * param base ANACTRL peripheral base address.
 * return ANACTRL instance.
 */
static uint32_t ANACTRL_GetInstance(ANACTRL_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_anactrlBases); instance++)
    {
        if (s_anactrlBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_anactrlBases));

    return instance;
}

/*!
 * @brief Enable the access to ANACTRL registers and initialize ANACTRL module.
 *
 * @param base ANACTRL peripheral base address.
 */
void ANACTRL_Init(ANACTRL_Type *base)
{
    assert(NULL != base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock for ANACTRL instance. */
    CLOCK_EnableClock(s_anactrlClocks[ANACTRL_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * @brief De-initialize ANACTRL module.
 *
 * @param base ANACTRL peripheral base address.
 */
void ANACTRL_Deinit(ANACTRL_Type *base)
{
    assert(NULL != base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock for ANACTRL instance. */
    CLOCK_DisableClock(s_anactrlClocks[ANACTRL_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * @brief Set the on-chip high-speed Free Running Oscillator.
 *
 * @param base ANACTRL peripheral base address.
 * @param config Pointer to FRO192M configuration structure. Refer to "anactrl_fro192M_config_t" structure.
 */
void ANACTRL_SetFro192M(ANACTRL_Type *base, anactrl_fro192M_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32 = 0;

    /* Set FRO trim values. */
    base->FRO192M_CTRL |= ANACTRL_FRO192M_CTRL_WRTRIM_MASK;
    tmp32 |= ANACTRL_FRO192M_CTRL_BIAS_TRIM(config->biasTrim) | ANACTRL_FRO192M_CTRL_TEMP_TRIM(config->tempTrim) |
             ANACTRL_FRO192M_CTRL_DAC_TRIM(config->dacTrim);

    if (config->enable12MHzClk)
    {
        tmp32 |= ANACTRL_FRO192M_CTRL_ENA_12MHZCLK_MASK;
    }
    if (config->enable48MhzClk)
    {
        tmp32 |= ANACTRL_FRO192M_CTRL_ENA_48MHZCLK_MASK;
    }
    if (config->enable96MHzClk)
    {
        tmp32 |= ANACTRL_FRO192M_CTRL_ENA_96MHZCLK_MASK;
    }

    if (config->enableAnalogTestBus)
    {
        tmp32 |= ANACTRL_FRO192M_CTRL_ATB_CTRL_MASK;
    }

    base->FRO192M_CTRL |= tmp32;
}

/*!
 * @brief Get the default configuration of FRO192M.
 * The default values are:
 * code
 *   config->biasTrim = 0x1AU;
 *   config->tempTrim = 0x20U;
 *   config->enable12MHzClk = true;
 *   config->enable48MhzClk = true;
 *   config->dacTrim = 0x80U;
 *   config->enableAnalogTestBus = false;
 *   config->enable96MHzClk = false;
 * encode
 * @param config Pointer to FRO192M configuration structure. Refer to "anactrl_fro192M_config_t" structure.
 */
void ANACTRL_GetDefaultFro192MConfig(anactrl_fro192M_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->biasTrim = 0x1AU;
    config->tempTrim = 0x20U;
    config->enable12MHzClk = true;
    config->enable48MhzClk = true;
    config->dacTrim = 0x80U;
    config->enableAnalogTestBus = false;
    config->enable96MHzClk = false;
}

/*!
 * @brief Set the 32 MHz Crystal oscillator.
 *
 * @param base ANACTRL peripheral base address.
 * @param config Pointer to XO32M configuration structure. Refer to "anactrl_xo32M_config_t" structure.
 */
void ANACTRL_SetXo32M(ANACTRL_Type *base, anactrl_xo32M_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32 = 0U;

    /* Set XO32M CTRL. */
    if (config->enableACBufferBypass)
    {
        tmp32 |= ANACTRL_XO32M_CTRL_ACBUF_PASS_ENABLE_MASK;
    }
    if (config->enablePllUsbOutput)
    {
        tmp32 |= ANACTRL_XO32M_CTRL_ENABLE_PLL_USB_OUT_MASK;
    }
    if (config->enableSysCLkOutput)
    {
        tmp32 |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;
    }
    base->XO32M_CTRL = tmp32;

    /* Set LDO XO32M. */
    tmp32 = ANACTRL_LDO_XO32M_HIGHZ(config->LDOOutputMode) | ANACTRL_LDO_XO32M_VOUT(config->LDOOutputLevel) |
            ANACTRL_LDO_XO32M_IBIAS(config->bias) | ANACTRL_LDO_XO32M_STABMODE(config->stability);
    if (config->enableLDOBypass)
    {
        tmp32 |= ANACTRL_LDO_XO32M_BYPASS_MASK;
    }

    base->LDO_XO32M = tmp32;
}

/*!
 * @brief Get the default configuration of XO32M.
 * The default values are:
 * code
 *   config->enableACBufferBypass = false;
 *   config->enablePllUsbOutput = false;
 *   config->enableSysCLkOutput = false;
 *   config->enableLDOBypass = false;
 *   config->LDOOutputMode = kANACTRL_LDOOutputHighNormalMode;
 *   config->LDOOutputLevel = kANACTRL_LDOOutputLevel4;
 *   config->bias = 2U;
 *   config->stability = 3U;
 * encode
 * @param config Pointer to XO32M configuration structure. Refer to "anactrl_xo32M_config_t" structure.
 */
void ANACTRL_GetDefaultXo32MConfig(anactrl_xo32M_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->enableACBufferBypass = false;
    config->enablePllUsbOutput = false;
    config->enableSysCLkOutput = false;
    config->enableLDOBypass = false;
    config->LDOOutputMode = kANACTRL_LDOOutputHighNormalMode;
    config->LDOOutputLevel = kANACTRL_LDOOutputLevel4;
    config->bias = 2U;
    config->stability = 3U;
}

/*!
 * @brief Set the ring oscillators.
 *
 * @param base ANACTRL peripheral base address.
 * @param config Pointer to ring osc configuration structure. Refer to "anactrl_ring_osc_config_t" structure.
 */
void ANACTRL_SetRingOsc(ANACTRL_Type *base, anactrl_ring_osc_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32 = 0U;

    /* Configure the first ring oscillator. */
    tmp32 = ANACTRL_RINGO0_CTRL_SL(config->ringOscSel) | ANACTRL_RINGO0_CTRL_FS(config->ringOscFreqOutputDiv) |
            ANACTRL_RINGO0_CTRL_SWN_SWP(config->pnRingOscMode) | ANACTRL_RINGO0_CTRL_E_ND0_MASK |
            ANACTRL_RINGO0_CTRL_E_ND1_MASK | ANACTRL_RINGO0_CTRL_E_NR0_MASK | ANACTRL_RINGO0_CTRL_E_NR1_MASK |
            ANACTRL_RINGO0_CTRL_E_IV0_MASK | ANACTRL_RINGO0_CTRL_E_IV1_MASK | ANACTRL_RINGO0_CTRL_E_PN0_MASK |
            ANACTRL_RINGO0_CTRL_E_PN1_MASK | ANACTRL_RINGO0_CTRL_DIV_UPDATE_REQ_MASK |
            ANACTRL_RINGO0_CTRL_DIVISOR(config->ringOscOutClkDiv);
    base->RINGO0_CTRL = tmp32;

    /* Configure the second and third ring oscillator. */
    tmp32 = ANACTRL_RINGO1_CTRL_S(config->ringOscSel) | ANACTRL_RINGO1_CTRL_FS(config->ringOscFreqOutputDiv) |
            ANACTRL_RINGO1_CTRL_E_R24_MASK | ANACTRL_RINGO1_CTRL_E_R35_MASK | ANACTRL_RINGO1_CTRL_E_M2_MASK |
            ANACTRL_RINGO1_CTRL_E_M3_MASK | ANACTRL_RINGO1_CTRL_E_M4_MASK | ANACTRL_RINGO1_CTRL_E_M5_MASK |
            ANACTRL_RINGO1_CTRL_DIV_UPDATE_REQ_MASK | ANACTRL_RINGO1_CTRL_DIVISOR(config->ringOscOutClkDiv);
    base->RINGO1_CTRL = tmp32;
    base->RINGO2_CTRL = tmp32;

    /* Ensure the Riongo module is enabled. */
    base->RINGO0_CTRL &= ~ANACTRL_RINGO0_CTRL_PD_MASK;
    base->RINGO1_CTRL &= ~ANACTRL_RINGO1_CTRL_PD_MASK;
    base->RINGO2_CTRL &= ~ANACTRL_RINGO2_CTRL_PD_MASK;
}

/*!
 * @brief Get the default configuration of ring oscillators.
 * The default values are:
 * code
 *   config->ringOscSel = kANACTRL_ShortRingOsc;
 *   config->ringOscFreqOutputDiv = kANACTRL_HighFreqOutput;
 *   config->pnRingOscMode = kANACTRL_NormalMode;
 *   config->ringOscOutClkDiv = 0U;
 * encode
 * @param config Pointer to ring oscillator configuration structure. Refer to "anactrl_ring_osc_config_t" structure.
 */
void ANACTRL_GetDefaultRingOscConfig(anactrl_ring_osc_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->ringOscSel = kANACTRL_ShortRingOsc;
    config->ringOscFreqOutputDiv = kANACTRL_HighFreqOutput;
    config->pnRingOscMode = kANACTRL_NormalMode;
    config->ringOscOutClkDiv = 0U;
}

/*!
 * @brief Measure Frequency
 *
 * This function measures target frequency according to a accurate reference frequency.The formula is:
 * Ftarget = (CAPVAL * Freference) / ((1<<SCALE)-1)
 *
 * @param base ANACTRL peripheral base address.
 * @scale Define the power of 2 count that ref counter counts to during measurement.
 * @refClkFreq frequency of the reference clock.
 * @return frequency of the target clock.
 *
 * @Note the minimum count (scale) is 2.
 */
uint32_t ANACTRL_MeasureFrequency(ANACTRL_Type *base, uint8_t scale, uint32_t refClkFreq)
{
    assert(scale >= 2U);

    uint32_t targetClkFreq = 0U;
    uint32_t capval = 0U;

    /* Init a measurement cycle. */
    base->FREQ_ME_CTRL = ANACTRL_FREQ_ME_CTRL_PROG_MASK + ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE(scale);
    while (ANACTRL_FREQ_ME_CTRL_PROG_MASK == (base->FREQ_ME_CTRL & ANACTRL_FREQ_ME_CTRL_PROG_MASK))
    {
    }

    /* Calculate the target clock frequency. */
    capval = (base->FREQ_ME_CTRL & ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE_MASK);
    targetClkFreq = (capval * refClkFreq) / ((1 << scale) - 1);

    return targetClkFreq;
}
