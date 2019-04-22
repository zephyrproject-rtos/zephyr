/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_acmp.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.acmp"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the ACMP instance from the peripheral base address.
 *
 * @param base ACMP peripheral base address.
 * @return ACMP instance.
 */
static uint32_t ACMP_GetInstance(CMP_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Array of ACMP peripheral base address. */
static CMP_Type *const s_acmpBases[] = CMP_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Clock name of ACMP. */
static const clock_ip_name_t s_acmpClock[] = CMP_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/
static uint32_t ACMP_GetInstance(CMP_Type *base)
{
    uint32_t instance = 0U;
    uint32_t acmpArrayCount = (sizeof(s_acmpBases) / sizeof(s_acmpBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < acmpArrayCount; instance++)
    {
        if (s_acmpBases[instance] == base)
        {
            break;
        }
    }

    return instance;
}

/*!
 * brief Initializes the ACMP.
 *
 * The default configuration can be got by calling ACMP_GetDefaultConfig().
 *
 * param base ACMP peripheral base address.
 * param config Pointer to ACMP configuration structure.
 */
void ACMP_Init(CMP_Type *base, const acmp_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Open clock gate. */
    CLOCK_EnableClock(s_acmpClock[ACMP_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Disable the module before configuring it. */
    ACMP_Enable(base, false);

    /* CMPx_C0
     * Set control bit. Avoid clearing status flags at the same time.
     */
    tmp32 = (base->C0 & (~(CMP_C0_PMODE_MASK | CMP_C0_INVT_MASK | CMP_C0_COS_MASK | CMP_C0_OPE_MASK |
                           CMP_C0_HYSTCTR_MASK | CMP_C0_CFx_MASK)));
#if defined(FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT) && (FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT == 1U)
    tmp32 &= ~CMP_C0_OFFSET_MASK;
#endif /* FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT */
    if (config->enableHighSpeed)
    {
        tmp32 |= CMP_C0_PMODE_MASK;
    }
    if (config->enableInvertOutput)
    {
        tmp32 |= CMP_C0_INVT_MASK;
    }
    if (config->useUnfilteredOutput)
    {
        tmp32 |= CMP_C0_COS_MASK;
    }
    if (config->enablePinOut)
    {
        tmp32 |= CMP_C0_OPE_MASK;
    }
    tmp32 |= CMP_C0_HYSTCTR(config->hysteresisMode);
#if defined(FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT) && (FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT == 1U)
    tmp32 |= CMP_C0_OFFSET(config->offsetMode);
#endif /* FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT */
    base->C0 = tmp32;
}

/*!
 * brief Deinitializes the ACMP.
 *
 * param base ACMP peripheral base address.
 */
void ACMP_Deinit(CMP_Type *base)
{
    /* Disable the module. */
    ACMP_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable clock gate. */
    CLOCK_DisableClock(s_acmpClock[ACMP_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Gets the default configuration for ACMP.
 *
 * This function initializes the user configuration structure to default value. The default value are:
 *
 * Example:
   code
   config->enableHighSpeed = false;
   config->enableInvertOutput = false;
   config->useUnfilteredOutput = false;
   config->enablePinOut = false;
   config->enableHysteresisBothDirections = false;
   config->hysteresisMode = kACMP_hysteresisMode0;
   endcode
 *
 * param config Pointer to ACMP configuration structure.
 */
void ACMP_GetDefaultConfig(acmp_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    /* Fill default configuration */
    config->enableHighSpeed = false;
    config->enableInvertOutput = false;
    config->useUnfilteredOutput = false;
    config->enablePinOut = false;
#if defined(FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT) && (FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT == 1U)
    config->offsetMode = kACMP_OffsetLevel0;
#endif /* FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT */
    config->hysteresisMode = kACMP_HysteresisLevel0;
}

/*!
 * brief Enables or disables the ACMP.
 *
 * param base ACMP peripheral base address.
 * param enable True to enable the ACMP.
 */
void ACMP_Enable(CMP_Type *base, bool enable)
{
    /* CMPx_C0
     * Set control bit. Avoid clearing status flags at the same time.
     */
    if (enable)
    {
        base->C0 = ((base->C0 | CMP_C0_EN_MASK) & ~CMP_C0_CFx_MASK);
    }
    else
    {
        base->C0 &= ~(CMP_C0_EN_MASK | CMP_C0_CFx_MASK);
    }
}

#if defined(FSL_FEATURE_ACMP_HAS_C0_LINKEN_BIT) && (FSL_FEATURE_ACMP_HAS_C0_LINKEN_BIT == 1U)
/*!
 * brief Enables the link from CMP to DAC enable.
 *
 * When this bit is set, the DAC enable/disable is controlled by the bit CMP_C0[EN] instead of CMP_C1[DACEN].
 *
 * param base ACMP peripheral base address.
 * param enable Enable the feature or not.
 */
void ACMP_EnableLinkToDAC(CMP_Type *base, bool enable)
{
    /* CMPx_C0_LINKEN
     * Set control bit. Avoid clearing status flags at the same time.
     */
    if (enable)
    {
        base->C0 = ((base->C0 | CMP_C0_LINKEN_MASK) & ~CMP_C0_CFx_MASK);
    }
    else
    {
        base->C0 &= ~(CMP_C0_LINKEN_MASK | CMP_C0_CFx_MASK);
    }
}
#endif /* FSL_FEATURE_ACMP_HAS_C0_LINKEN_BIT */

/*!
 * brief Sets the channel configuration.
 *
 * Note that the plus/minus mux's setting is only valid when the positive/negative port's input isn't from DAC but
 * from channel mux.
 *
 * Example:
   code
   acmp_channel_config_t configStruct = {0};
   configStruct.positivePortInput = kACMP_PortInputFromDAC;
   configStruct.negativePortInput = kACMP_PortInputFromMux;
   configStruct.minusMuxInput = 1U;
   ACMP_SetChannelConfig(CMP0, &configStruct);
   endcode
 *
 * param base ACMP peripheral base address.
 * param config Pointer to channel configuration structure.
 */
void ACMP_SetChannelConfig(CMP_Type *base, const acmp_channel_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32 = (base->C1 & (~(CMP_C1_PSEL_MASK | CMP_C1_MSEL_MASK)));

/* CMPx_C1
 * Set the input of CMP's positive port.
 */
#if (defined(FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT == 1U))
    tmp32 &= ~CMP_C1_INPSEL_MASK;
    tmp32 |= CMP_C1_INPSEL(config->positivePortInput);
#endif /* FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT */

#if (defined(FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT == 1U))
    tmp32 &= ~CMP_C1_INNSEL_MASK;
    tmp32 |= CMP_C1_INNSEL(config->negativePortInput);
#endif /* FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT */

    tmp32 |= CMP_C1_PSEL(config->plusMuxInput) | CMP_C1_MSEL(config->minusMuxInput);

    base->C1 = tmp32;
}

/*!
 * brief Enables or disables DMA.
 *
 * param base ACMP peripheral base address.
 * param enable True to enable DMA.
 */
void ACMP_EnableDMA(CMP_Type *base, bool enable)
{
    /* CMPx_C0
     * Set control bit. Avoid clearing status flags at the same time.
     */
    if (enable)
    {
        base->C0 = ((base->C0 | CMP_C0_DMAEN_MASK) & ~CMP_C0_CFx_MASK);
    }
    else
    {
        base->C0 &= ~(CMP_C0_DMAEN_MASK | CMP_C0_CFx_MASK);
    }
}

/*!
 * brief Enables or disables window mode.
 *
 * param base ACMP peripheral base address.
 * param enable True to enable window mode.
 */
void ACMP_EnableWindowMode(CMP_Type *base, bool enable)
{
    /* CMPx_C0
     * Set control bit. Avoid clearing status flags at the same time.
     */
    if (enable)
    {
        base->C0 = ((base->C0 | CMP_C0_WE_MASK) & ~CMP_C0_CFx_MASK);
    }
    else
    {
        base->C0 &= ~(CMP_C0_WE_MASK | CMP_C0_CFx_MASK);
    }
}

/*!
 * brief Configures the filter.
 *
 * The filter can be enabled when the filter count is bigger than 1, the filter period is greater than 0 and the sample
 * clock is from divided bus clock or the filter is bigger than 1 and the sample clock is from external clock. Detailed
 * usage can be got from the reference manual.
 *
 * Example:
   code
   acmp_filter_config_t configStruct = {0};
   configStruct.filterCount = 5U;
   configStruct.filterPeriod = 200U;
   configStruct.enableSample = false;
   ACMP_SetFilterConfig(CMP0, &configStruct);
   endcode
 *
 * param base ACMP peripheral base address.
 * param config Pointer to filter configuration structure.
 */
void ACMP_SetFilterConfig(CMP_Type *base, const acmp_filter_config_t *config)
{
    assert(NULL != config);

    /* CMPx_C0
     * Set control bit. Avoid clearing status flags at the same time.
     */
    uint32_t tmp32 = (base->C0 & (~(CMP_C0_FILTER_CNT_MASK | CMP_C0_FPR_MASK | CMP_C0_SE_MASK | CMP_C0_CFx_MASK)));

    if (config->enableSample)
    {
        tmp32 |= CMP_C0_SE_MASK;
    }
    tmp32 |= (CMP_C0_FILTER_CNT(config->filterCount) | CMP_C0_FPR(config->filterPeriod));
    base->C0 = tmp32;
}

/*!
 * brief Configures the internal DAC.
 *
 * Example:
   code
   acmp_dac_config_t configStruct = {0};
   configStruct.referenceVoltageSource = kACMP_VrefSourceVin1;
   configStruct.DACValue = 20U;
   configStruct.enableOutput = false;
   configStruct.workMode = kACMP_DACWorkLowSpeedMode;
   ACMP_SetDACConfig(CMP0, &configStruct);
   endcode
 *
 * param base ACMP peripheral base address.
 * param config Pointer to DAC configuration structure. "NULL" is for disabling the feature.
 */
void ACMP_SetDACConfig(CMP_Type *base, const acmp_dac_config_t *config)
{
    uint32_t tmp32;

    /* CMPx_C1
     * NULL configuration means to disable the feature.
     */
    if (NULL == config)
    {
        base->C1 &= ~CMP_C1_DACEN_MASK;
        return;
    }

    tmp32 = (base->C1 & (~(CMP_C1_VRSEL_MASK | CMP_C1_VOSEL_MASK)));
    /* Set configuration and enable the feature. */
    tmp32 |= (CMP_C1_VRSEL(config->referenceVoltageSource) | CMP_C1_VOSEL(config->DACValue) | CMP_C1_DACEN_MASK);

#if defined(FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT) && (FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT == 1U)
    tmp32 &= ~CMP_C1_DACOE_MASK;
    if (config->enableOutput)
    {
        tmp32 |= CMP_C1_DACOE_MASK;
    }
#endif /* FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT */

#if defined(FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT) && (FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT == 1U)
    switch (config->workMode)
    {
        case kACMP_DACWorkLowSpeedMode:
            tmp32 &= ~CMP_C1_DMODE_MASK;
            break;
        case kACMP_DACWorkHighSpeedMode:
            tmp32 |= CMP_C1_DMODE_MASK;
            break;
        default:
            break;
    }
#endif /* FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT */

    base->C1 = tmp32;
}

/*!
 * brief Configures the round robin mode.
 *
 * Example:
   code
   acmp_round_robin_config_t configStruct = {0};
   configStruct.fixedPort = kACMP_FixedPlusPort;
   configStruct.fixedChannelNumber = 3U;
   configStruct.checkerChannelMask = 0xF7U;
   configStruct.sampleClockCount = 0U;
   configStruct.delayModulus = 0U;
   ACMP_SetRoundRobinConfig(CMP0, &configStruct);
   endcode
 * param base ACMP peripheral base address.
 * param config Pointer to round robin mode configuration structure. "NULL" is for disabling the feature.
 */
void ACMP_SetRoundRobinConfig(CMP_Type *base, const acmp_round_robin_config_t *config)
{
    uint32_t tmp32;

    /* CMPx_C2
     * Set control bit. Avoid clearing status flags at the same time.
     * NULL configuration means to disable the feature.
     */
    if (NULL == config)
    {
        tmp32 = CMP_C2_CHnF_MASK;
#if defined(FSL_FEATURE_ACMP_HAS_C2_RRE_BIT) && (FSL_FEATURE_ACMP_HAS_C2_RRE_BIT == 1U)
        tmp32 |= CMP_C2_RRE_MASK;
#endif /* FSL_FEATURE_ACMP_HAS_C2_RRE_BIT */
        base->C2 &= ~(tmp32);
        return;
    }

    /* CMPx_C1
     * Set all channel's round robin checker enable mask.
     */
    tmp32 = (base->C1 & ~(CMP_C1_CHNn_MASK));
    tmp32 |= ((config->checkerChannelMask) << CMP_C1_CHN0_SHIFT);
    base->C1 = tmp32;

    /* CMPx_C2
     * Set configuration and enable the feature.
     */
    tmp32 = (base->C2 &
             (~(CMP_C2_FXMP_MASK | CMP_C2_FXMXCH_MASK | CMP_C2_NSAM_MASK | CMP_C2_INITMOD_MASK | CMP_C2_CHnF_MASK)));
    tmp32 |= (CMP_C2_FXMP(config->fixedPort) | CMP_C2_FXMXCH(config->fixedChannelNumber) |
              CMP_C2_NSAM(config->sampleClockCount) | CMP_C2_INITMOD(config->delayModulus));
#if defined(FSL_FEATURE_ACMP_HAS_C2_RRE_BIT) && (FSL_FEATURE_ACMP_HAS_C2_RRE_BIT == 1U)
    tmp32 |= CMP_C2_RRE_MASK;
#endif /* FSL_FEATURE_ACMP_HAS_C2_RRE_BIT */
    base->C2 = tmp32;
}

/*!
 * brief Defines the pre-set state of channels in round robin mode.
 *
 * Note: The pre-state has different circuit with get-round-robin-result in the SOC even though they are same bits.
 * So get-round-robin-result can't return the same value as the value are set by pre-state.
 *
 * param base ACMP peripheral base address.
 * param mask Mask of round robin channel index. Available range is channel0:0x01 to channel7:0x80.
 */
void ACMP_SetRoundRobinPreState(CMP_Type *base, uint32_t mask)
{
    /* CMPx_C2
     * Set control bit. Avoid clearing status flags at the same time.
     */
    uint32_t tmp32 = (base->C2 & ~(CMP_C2_ACOn_MASK | CMP_C2_CHnF_MASK));

    tmp32 |= (mask << CMP_C2_ACOn_SHIFT);
    base->C2 = tmp32;
}

/*!
 * brief Clears the channel input changed flags in round robin mode.
 *
 * param base ACMP peripheral base address.
 * param mask Mask of channel index. Available range is channel0:0x01 to channel7:0x80.
 */
void ACMP_ClearRoundRobinStatusFlags(CMP_Type *base, uint32_t mask)
{
    /* CMPx_C2 */
    uint32_t tmp32 = (base->C2 & (~CMP_C2_CHnF_MASK));

    tmp32 |= (mask << CMP_C2_CH0F_SHIFT);
    base->C2 = tmp32;
}

/*!
 * brief Enables interrupts.
 *
 * param base ACMP peripheral base address.
 * param mask Interrupts mask. See "_acmp_interrupt_enable".
 */
void ACMP_EnableInterrupts(CMP_Type *base, uint32_t mask)
{
    uint32_t tmp32;

    /* CMPx_C0
     * Set control bit. Avoid clearing status flags at the same time.
     * Set CMP interrupt enable flag.
     */
    tmp32 = base->C0 & ~CMP_C0_CFx_MASK; /* To protect the W1C flags. */
    if (kACMP_OutputRisingInterruptEnable == (mask & kACMP_OutputRisingInterruptEnable))
    {
        tmp32 = ((tmp32 | CMP_C0_IER_MASK) & ~CMP_C0_CFx_MASK);
    }
    if (kACMP_OutputFallingInterruptEnable == (mask & kACMP_OutputFallingInterruptEnable))
    {
        tmp32 = ((tmp32 | CMP_C0_IEF_MASK) & ~CMP_C0_CFx_MASK);
    }
    base->C0 = tmp32;

    /* CMPx_C2
     * Set round robin interrupt enable flag.
     */
    if (kACMP_RoundRobinInterruptEnable == (mask & kACMP_RoundRobinInterruptEnable))
    {
        tmp32 = base->C2;
        /* Set control bit. Avoid clearing status flags at the same time. */
        tmp32 = ((tmp32 | CMP_C2_RRIE_MASK) & ~CMP_C2_CHnF_MASK);
        base->C2 = tmp32;
    }
}

/*!
 * brief Disables interrupts.
 *
 * param base ACMP peripheral base address.
 * param mask Interrupts mask. See "_acmp_interrupt_enable".
 */
void ACMP_DisableInterrupts(CMP_Type *base, uint32_t mask)
{
    uint32_t tmp32;

    /* CMPx_C0
     * Set control bit. Avoid clearing status flags at the same time.
     * Clear CMP interrupt enable flag.
     */
    tmp32 = base->C0;
    if (kACMP_OutputRisingInterruptEnable == (mask & kACMP_OutputRisingInterruptEnable))
    {
        tmp32 &= ~(CMP_C0_IER_MASK | CMP_C0_CFx_MASK);
    }
    if (kACMP_OutputFallingInterruptEnable == (mask & kACMP_OutputFallingInterruptEnable))
    {
        tmp32 &= ~(CMP_C0_IEF_MASK | CMP_C0_CFx_MASK);
    }
    base->C0 = tmp32;

    /* CMPx_C2
     * Clear round robin interrupt enable flag.
     */
    if (kACMP_RoundRobinInterruptEnable == (mask & kACMP_RoundRobinInterruptEnable))
    {
        tmp32 = base->C2;
        /* Set control bit. Avoid clearing status flags at the same time. */
        tmp32 &= ~(CMP_C2_RRIE_MASK | CMP_C2_CHnF_MASK);
        base->C2 = tmp32;
    }
}

/*!
 * brief Gets status flags.
 *
 * param base ACMP peripheral base address.
 * return Status flags asserted mask. See "_acmp_status_flags".
 */
uint32_t ACMP_GetStatusFlags(CMP_Type *base)
{
    uint32_t status = 0U;
    uint32_t tmp32 = base->C0;

    /* CMPx_C0
     * Check if each flag is set.
     */
    if (CMP_C0_CFR_MASK == (tmp32 & CMP_C0_CFR_MASK))
    {
        status |= kACMP_OutputRisingEventFlag;
    }
    if (CMP_C0_CFF_MASK == (tmp32 & CMP_C0_CFF_MASK))
    {
        status |= kACMP_OutputFallingEventFlag;
    }
    if (CMP_C0_COUT_MASK == (tmp32 & CMP_C0_COUT_MASK))
    {
        status |= kACMP_OutputAssertEventFlag;
    }

    return status;
}

/*!
 * brief Clears status flags.
 *
 * param base ACMP peripheral base address.
 * param mask Status flags mask. See "_acmp_status_flags".
 */
void ACMP_ClearStatusFlags(CMP_Type *base, uint32_t mask)
{
    /* CMPx_C0 */
    uint32_t tmp32 = (base->C0 & (~(CMP_C0_CFR_MASK | CMP_C0_CFF_MASK)));

    /* Clear flag according to mask. */
    if (kACMP_OutputRisingEventFlag == (mask & kACMP_OutputRisingEventFlag))
    {
        tmp32 |= CMP_C0_CFR_MASK;
    }
    if (kACMP_OutputFallingEventFlag == (mask & kACMP_OutputFallingEventFlag))
    {
        tmp32 |= CMP_C0_CFF_MASK;
    }
    base->C0 = tmp32;
}

#if defined(FSL_FEATURE_ACMP_HAS_C3_REG) && (FSL_FEATURE_ACMP_HAS_C3_REG == 1U)
/*!
 * brief Configure the discrete mode.
 *
 * Configure the discrete mode when supporting 3V domain with 1.8V core.
 *
 * param base ACMP peripheral base address.
 * param config Pointer to configuration structure. See "acmp_discrete_mode_config_t".
 */
void ACMP_SetDiscreteModeConfig(CMP_Type *base, const acmp_discrete_mode_config_t *config)
{
    uint32_t tmp32 = 0U;

    if (!config->enablePositiveChannelDiscreteMode)
    {
        tmp32 |= CMP_C3_PCHCTEN_MASK;
    }
    if (!config->enableNegativeChannelDiscreteMode)
    {
        tmp32 |= CMP_C3_NCHCTEN_MASK;
    }
    if (config->enableResistorDivider)
    {
        tmp32 |= CMP_C3_RDIVE_MASK;
    }

    tmp32 |= CMP_C3_DMCS(config->clockSource)      /* Select the clock. */
             | CMP_C3_ACSAT(config->sampleTime)    /* Sample time period. */
             | CMP_C3_ACPH1TC(config->phase1Time)  /* Phase 1 sample time. */
             | CMP_C3_ACPH2TC(config->phase2Time); /* Phase 2 sample time. */

    base->C3 = tmp32;
}

/*!
 * brief Get the default configuration for discrete mode setting.
 *
 * param config Pointer to configuration structure to be restored with the setting values.
 */
void ACMP_GetDefaultDiscreteModeConfig(acmp_discrete_mode_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->enablePositiveChannelDiscreteMode = false;
    config->enableNegativeChannelDiscreteMode = false;
    config->enableResistorDivider = false;
    config->clockSource = kACMP_DiscreteClockSlow;
    config->sampleTime = kACMP_DiscreteSampleTimeAs1T;
    config->phase1Time = kACMP_DiscretePhaseTimeAlt0;
    config->phase2Time = kACMP_DiscretePhaseTimeAlt0;
}

#endif /* FSL_FEATURE_ACMP_HAS_C3_REG */
