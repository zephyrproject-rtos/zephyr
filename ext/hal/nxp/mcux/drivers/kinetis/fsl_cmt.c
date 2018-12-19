/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_cmt.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.cmt"
#endif

/* The standard intermediate frequency (IF). */
#define CMT_INTERMEDIATEFREQUENCY_8MHZ (8000000U)
/* CMT data modulate mask. */
#define CMT_MODULATE_COUNT_WIDTH (8U)
/* CMT diver 1. */
#define CMT_CMTDIV_ONE (1)
/* CMT diver 2. */
#define CMT_CMTDIV_TWO (2)
/* CMT diver 4. */
#define CMT_CMTDIV_FOUR (4)
/* CMT diver 8. */
#define CMT_CMTDIV_EIGHT (8)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get instance number for CMT module.
 *
 * @param base CMT peripheral base address.
 */
static uint32_t CMT_GetInstance(CMT_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to cmt clocks for each instance. */
static const clock_ip_name_t s_cmtClock[FSL_FEATURE_SOC_CMT_COUNT] = CMT_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief Pointers to cmt bases for each instance. */
static CMT_Type *const s_cmtBases[] = CMT_BASE_PTRS;

/*! @brief Pointers to cmt IRQ number for each instance. */
static const IRQn_Type s_cmtIrqs[] = CMT_IRQS;

/*******************************************************************************
 * Codes
 ******************************************************************************/

static uint32_t CMT_GetInstance(CMT_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_cmtBases); instance++)
    {
        if (s_cmtBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_cmtBases));

    return instance;
}

/*!
 * brief Gets the CMT default configuration structure. This API
 * gets the default configuration structure for the CMT_Init().
 * Use the initialized structure unchanged in CMT_Init() or modify
 * fields of the structure before calling the CMT_Init().
 *
 * param config The CMT configuration structure pointer.
 */
void CMT_GetDefaultConfig(cmt_config_t *config)
{
    assert(config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    /* Default infrared output is enabled and set with high active, the divider is set to 1. */
    config->isInterruptEnabled = false;
    config->isIroEnabled = true;
    config->iroPolarity = kCMT_IROActiveHigh;
    config->divider = kCMT_SecondClkDiv1;
}

/*!
 * brief Initializes the CMT module.
 *
 * This function ungates the module clock and sets the CMT internal clock,
 * interrupt, and infrared output signal for the CMT module.
 *
 * param base            CMT peripheral base address.
 * param config          The CMT basic configuration structure.
 * param busClock_Hz     The CMT module input clock - bus clock frequency.
 */
void CMT_Init(CMT_Type *base, const cmt_config_t *config, uint32_t busClock_Hz)
{
    assert(config);
    assert(busClock_Hz >= CMT_INTERMEDIATEFREQUENCY_8MHZ);

    uint8_t divider;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate clock. */
    CLOCK_EnableClock(s_cmtClock[CMT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Sets clock divider. The divider set in pps should be set
       to make sycClock_Hz/divder = 8MHz */
    base->PPS = CMT_PPS_PPSDIV(busClock_Hz / CMT_INTERMEDIATEFREQUENCY_8MHZ - 1);
    divider = base->MSC;
    divider &= ~CMT_MSC_CMTDIV_MASK;
    divider |= CMT_MSC_CMTDIV(config->divider);
    base->MSC = divider;

    /* Set the IRO signal. */
    base->OC = CMT_OC_CMTPOL(config->iroPolarity) | CMT_OC_IROPEN(config->isIroEnabled);

    /* Set interrupt. */
    if (config->isInterruptEnabled)
    {
        CMT_EnableInterrupts(base, kCMT_EndOfCycleInterruptEnable);
        EnableIRQ(s_cmtIrqs[CMT_GetInstance(base)]);
    }
}

/*!
 * brief Disables the CMT module and gate control.
 *
 * This function disables CMT modulator, interrupts, and gates the
 * CMT clock control. CMT_Init must be called  to use the CMT again.
 *
 * param base   CMT peripheral base address.
 */
void CMT_Deinit(CMT_Type *base)
{
    /*Disable the CMT modulator. */
    base->MSC = 0;

    /* Disable the interrupt. */
    CMT_DisableInterrupts(base, kCMT_EndOfCycleInterruptEnable);
    DisableIRQ(s_cmtIrqs[CMT_GetInstance(base)]);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the clock. */
    CLOCK_DisableClock(s_cmtClock[CMT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Selects the mode for CMT.
 *
 * param base   CMT peripheral base address.
 * param mode   The CMT feature mode enumeration. See "cmt_mode_t".
 * param modulateConfig  The carrier generation and modulator configuration.
 */
void CMT_SetMode(CMT_Type *base, cmt_mode_t mode, cmt_modulate_config_t *modulateConfig)
{
    uint8_t mscReg = base->MSC;

    /* Judge the mode. */
    if (mode != kCMT_DirectIROCtl)
    {
        assert(modulateConfig);

        /* Set carrier generator. */
        CMT_SetCarrirGenerateCountOne(base, modulateConfig->highCount1, modulateConfig->lowCount1);
        if (mode == kCMT_FSKMode)
        {
            CMT_SetCarrirGenerateCountTwo(base, modulateConfig->highCount2, modulateConfig->lowCount2);
        }

        /* Set carrier modulator. */
        CMT_SetModulateMarkSpace(base, modulateConfig->markCount, modulateConfig->spaceCount);
        mscReg &= ~(CMT_MSC_FSK_MASK | CMT_MSC_BASE_MASK);
        mscReg |= mode;
    }
    else
    {
        mscReg &= ~CMT_MSC_MCGEN_MASK;
    }
    /* Set the CMT mode. */
    base->MSC = mscReg;
}

/*!
 * brief Gets the mode of the CMT module.
 *
 * param base   CMT peripheral base address.
 * return The CMT mode.
 *     kCMT_DirectIROCtl     Carrier modulator is disabled; the IRO signal is directly in software control.
 *     kCMT_TimeMode         Carrier modulator is enabled in time mode.
 *     kCMT_FSKMode          Carrier modulator is enabled in FSK mode.
 *     kCMT_BasebandMode     Carrier modulator is enabled in baseband mode.
 */
cmt_mode_t CMT_GetMode(CMT_Type *base)
{
    uint8_t mode = base->MSC;

    if (!(mode & CMT_MSC_MCGEN_MASK))
    { /* Carrier modulator disabled and the IRO signal is in direct software control. */
        return kCMT_DirectIROCtl;
    }
    else
    {
        /* Carrier modulator is enabled. */
        if (mode & CMT_MSC_BASE_MASK)
        {
            /* Base band mode. */
            return kCMT_BasebandMode;
        }
        else if (mode & CMT_MSC_FSK_MASK)
        {
            /* FSK mode. */
            return kCMT_FSKMode;
        }
        else
        {
            /* Time mode. */
            return kCMT_TimeMode;
        }
    }
}

/*!
 * brief Gets the actual CMT clock frequency.
 *
 * param base        CMT peripheral base address.
 * param busClock_Hz CMT module input clock - bus clock frequency.
 * return The CMT clock frequency.
 */
uint32_t CMT_GetCMTFrequency(CMT_Type *base, uint32_t busClock_Hz)
{
    uint32_t frequency;
    uint32_t divider;

    /* Get intermediate frequency. */
    frequency = busClock_Hz / ((base->PPS & CMT_PPS_PPSDIV_MASK) + 1);

    /* Get the second divider. */
    divider = ((base->MSC & CMT_MSC_CMTDIV_MASK) >> CMT_MSC_CMTDIV_SHIFT);
    /* Get CMT frequency. */
    switch ((cmt_second_clkdiv_t)divider)
    {
        case kCMT_SecondClkDiv1:
            frequency = frequency / CMT_CMTDIV_ONE;
            break;
        case kCMT_SecondClkDiv2:
            frequency = frequency / CMT_CMTDIV_TWO;
            break;
        case kCMT_SecondClkDiv4:
            frequency = frequency / CMT_CMTDIV_FOUR;
            break;
        case kCMT_SecondClkDiv8:
            frequency = frequency / CMT_CMTDIV_EIGHT;
            break;
        default:
            frequency = frequency / CMT_CMTDIV_ONE;
            break;
    }

    return frequency;
}

/*!
 * brief Sets the modulation mark and space time period for the CMT modulator.
 *
 * This function sets the mark time period of the CMT modulator counter
 * to control the mark time of the output modulated signal from the carrier generator output signal.
 * If the CMT clock frequency is Fcmt and the carrier out signal frequency is fcg:
 *      - In Time and Baseband mode: The mark period of the generated signal equals (markCount + 1) / (Fcmt/8).
 *                                   The space period of the generated signal equals spaceCount / (Fcmt/8).
 *      - In FSK mode: The mark period of the generated signal equals (markCount + 1)/fcg.
 *                     The space period of the generated signal equals spaceCount / fcg.
 *
 * param base Base address for current CMT instance.
 * param markCount The number of clock period for CMT modulator signal mark period,
 *                   in the range of 0 ~ 0xFFFF.
 * param spaceCount The number of clock period for CMT modulator signal space period,
 *                   in the range of the 0 ~ 0xFFFF.
 */
void CMT_SetModulateMarkSpace(CMT_Type *base, uint32_t markCount, uint32_t spaceCount)
{
    /* Set modulate mark. */
    base->CMD1 = (markCount >> CMT_MODULATE_COUNT_WIDTH) & CMT_CMD1_MB_MASK;
    base->CMD2 = (markCount & CMT_CMD2_MB_MASK);
    /* Set modulate space. */
    base->CMD3 = (spaceCount >> CMT_MODULATE_COUNT_WIDTH) & CMT_CMD3_SB_MASK;
    base->CMD4 = spaceCount & CMT_CMD4_SB_MASK;
}

/*!
 * brief Sets the IRO (infrared output) signal state.
 *
 * Changes the states of the IRO signal when the kCMT_DirectIROMode mode is set
 * and the IRO signal is enabled.
 *
 * param base   CMT peripheral base address.
 * param state  The control of the IRO signal. See "cmt_infrared_output_state_t"
 */
void CMT_SetIroState(CMT_Type *base, cmt_infrared_output_state_t state)
{
    uint8_t ocReg = base->OC;

    ocReg &= ~CMT_OC_IROL_MASK;
    ocReg |= CMT_OC_IROL(state);

    /* Set the infrared output signal control. */
    base->OC = ocReg;
}
