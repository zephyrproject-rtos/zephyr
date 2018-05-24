/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
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

#include "fsl_cmt.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

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

void CMT_GetDefaultConfig(cmt_config_t *config)
{
    assert(config);

    /* Default infrared output is enabled and set with high active, the divider is set to 1. */
    config->isInterruptEnabled = false;
    config->isIroEnabled = true;
    config->iroPolarity = kCMT_IROActiveHigh;
    config->divider = kCMT_SecondClkDiv1;
}

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
        mscReg &= ~ (CMT_MSC_FSK_MASK | CMT_MSC_BASE_MASK);
        mscReg |= mode;
    }
    else
    {
        mscReg &= ~CMT_MSC_MCGEN_MASK;
    }
    /* Set the CMT mode. */
    base->MSC = mscReg;
}

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

void CMT_SetModulateMarkSpace(CMT_Type *base, uint32_t markCount, uint32_t spaceCount)
{
    /* Set modulate mark. */
    base->CMD1 = (markCount >> CMT_MODULATE_COUNT_WIDTH) & CMT_CMD1_MB_MASK;
    base->CMD2 = (markCount & CMT_CMD2_MB_MASK);
    /* Set modulate space. */
    base->CMD3 = (spaceCount >> CMT_MODULATE_COUNT_WIDTH) & CMT_CMD3_SB_MASK;
    base->CMD4 = spaceCount & CMT_CMD4_SB_MASK;
}

void CMT_SetIroState(CMT_Type *base, cmt_infrared_output_state_t state)
{
    uint8_t ocReg = base->OC;

    ocReg &= ~CMT_OC_IROL_MASK;
    ocReg |= CMT_OC_IROL(state);

    /* Set the infrared output signal control. */
    base->OC = ocReg;
}
