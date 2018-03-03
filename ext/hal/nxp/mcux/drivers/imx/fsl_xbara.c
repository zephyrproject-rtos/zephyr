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

#include "fsl_xbara.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get the XBARA instance from peripheral base address.
 *
 * @param base XBARA peripheral base address.
 * @return XBARA instance.
 */
static uint32_t XBARA_GetInstance(XBARA_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Array of XBARA peripheral base address. */
static XBARA_Type *const s_xbaraBases[] = XBARA_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of XBARA clock name. */
static const clock_ip_name_t s_xbaraClock[] = XBARA_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint32_t XBARA_GetInstance(XBARA_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_xbaraBases); instance++)
    {
        if (s_xbaraBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_xbaraBases));

    return instance;
}

void XBARA_Init(XBARA_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable XBARA module clock. */
    CLOCK_EnableClock(s_xbaraClock[XBARA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void XBARA_Deinit(XBARA_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable XBARA module clock. */
    CLOCK_DisableClock(s_xbaraClock[XBARA_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void XBARA_SetSignalsConnection(XBARA_Type *base, xbar_input_signal_t input, xbar_output_signal_t output)
{
    XBARA_WR_SELx_SELx(base, (((uint16_t)input) & 0xFFU), (((uint16_t)output) & 0xFFU));
}

uint32_t XBARA_GetStatusFlags(XBARA_Type *base)
{
    uint32_t status_flag;

    status_flag = ((base->CTRL0 & (XBARA_CTRL0_STS0_MASK | XBARA_CTRL0_STS1_MASK)) |
                   ((base->CTRL1 & (XBARA_CTRL1_STS2_MASK | XBARA_CTRL1_STS3_MASK)) << 16U));

    return status_flag;
}

void XBARA_ClearStatusFlags(XBARA_Type *base, uint32_t mask)
{
    uint16_t regVal;

    /* Assign regVal to CTRL0 register's value */
    regVal = (base->CTRL0);
    /* Perform this command to avoid writing 1 into interrupt flag bits */
    regVal &= (uint16_t)(~(XBARA_CTRL0_STS0_MASK | XBARA_CTRL0_STS1_MASK));
    /* Write 1 to interrupt flag bits corresponding to mask */
    regVal |= (uint16_t)(mask & (XBARA_CTRL0_STS0_MASK | XBARA_CTRL0_STS1_MASK));
    /* Write regVal value into CTRL0 register */
    base->CTRL0 = regVal;

    /* Assign regVal to CTRL1 register's value */
    regVal = (base->CTRL1);
    /* Perform this command to avoid writing 1 into interrupt flag bits */
    regVal &= (uint16_t)(~(XBARA_CTRL1_STS2_MASK | XBARA_CTRL1_STS3_MASK));
    /* Write 1 to interrupt flag bits corresponding to mask */
    regVal |= (uint16_t)((mask >> 16U) & (XBARA_CTRL1_STS2_MASK | XBARA_CTRL1_STS3_MASK));
    /* Write regVal value into CTRL1 register */
    base->CTRL1 = regVal;
}

void XBARA_SetOutputSignalConfig(XBARA_Type *base,
                                 xbar_output_signal_t output,
                                 const xbara_control_config_t *controlConfig)
{
    uint16_t regVal;
    /* Set active edge for edge detection, set interrupt or DMA function. */
    switch ((uint16_t)output)
    {
#if defined(FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_30) && FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_30 
        case kXBARA1_OutputDmaChMuxReq30:
#else
        case kXBARA_OutputDmamux18:
#endif
            /* Assign regVal to CTRL0 register's value */
            regVal = (base->CTRL0);
            /* Perform this command to avoid writing 1 into interrupt flag bits and clears bit DEN0, IEN0 */
            regVal &= (uint16_t)(
                ~(XBARA_CTRL0_STS0_MASK | XBARA_CTRL0_STS1_MASK | XBARA_CTRL0_DEN0_MASK | XBARA_CTRL0_IEN0_MASK));
            /* Configure edge and request type */
            regVal |= (uint16_t)(XBARA_CTRL0_EDGE0(controlConfig->activeEdge) |
                                 ((controlConfig->requestType) << XBARA_CTRL0_DEN0_SHIFT));
            /* Write regVal value into CTRL0 register */
            base->CTRL0 = regVal;
            break;
#if defined(FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_31) && FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_31 
        case kXBARA1_OutputDmaChMuxReq31:
#else
        case kXBARA_OutputDmamux19:
#endif
            /* Assign regVal to CTRL0 register's value */
            regVal = (base->CTRL0);
            /* Perform this command to avoid writing 1 into interrupt flag bits and clears bit DEN1, IEN1 */
            regVal &= (uint16_t)(
                ~(XBARA_CTRL0_STS0_MASK | XBARA_CTRL0_STS1_MASK | XBARA_CTRL0_DEN1_MASK | XBARA_CTRL0_IEN1_MASK));
            /* Configure edge and request type */
            regVal |= (uint16_t)(XBARA_CTRL0_EDGE1(controlConfig->activeEdge) |
                                 ((controlConfig->requestType) << XBARA_CTRL0_DEN1_SHIFT));
            /* Write regVal value into CTRL0 register */
            base->CTRL0 = regVal;
            break;
#if defined(FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_94) && FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_94 
        case kXBARA1_OutputDmaChMuxReq94:
#else
        case kXBARA_OutputDmamux20:
#endif
            /* Assign regVal to CTRL1 register's value */
            regVal = (base->CTRL1);
            /* Perform this command to avoid writing 1 into interrupt flag bits and clears bit DEN2, IEN2 */
            regVal &= (uint16_t)(
                ~(XBARA_CTRL1_STS2_MASK | XBARA_CTRL1_STS3_MASK | XBARA_CTRL1_DEN2_MASK | XBARA_CTRL1_IEN2_MASK));
            /* Configure edge and request type */
            regVal |= (uint16_t)(XBARA_CTRL1_EDGE2(controlConfig->activeEdge) |
                                 ((controlConfig->requestType) << XBARA_CTRL1_DEN2_SHIFT));
            /* Write regVal value into CTRL1 register */
            base->CTRL1 = regVal;
            break;
#if defined(FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_95) && FSL_FEATURE_XBARA_OUTPUT_DMA_CH_MUX_REQ_95 
        case kXBARA1_OutputDmaChMuxReq95:
#else
             case kXBARA_OutputDmamux21:
#endif
            /* Assign regVal to CTRL1 register's value */
            regVal = (base->CTRL1);
            /* Perform this command to avoid writing 1 into interrupt flag bits and clears bit DEN3, IEN3 */
            regVal &= (uint16_t)(
                ~(XBARA_CTRL1_STS2_MASK | XBARA_CTRL1_STS3_MASK | XBARA_CTRL1_DEN3_MASK | XBARA_CTRL1_IEN3_MASK));
            /* Configure edge and request type */
            regVal |= (uint16_t)(XBARA_CTRL1_EDGE3(controlConfig->activeEdge) |
                                 ((controlConfig->requestType) << XBARA_CTRL1_DEN3_SHIFT));
            /* Write regVal value into CTRL1 register */
            base->CTRL1 = regVal;
            break;
        default:
            break;
    }
}
