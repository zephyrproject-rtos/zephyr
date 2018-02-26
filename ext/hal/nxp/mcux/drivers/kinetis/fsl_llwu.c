/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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

#include "fsl_llwu.h"

#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN)
void LLWU_SetExternalWakeupPinMode(LLWU_Type *base, uint32_t pinIndex, llwu_external_pin_mode_t pinMode)
{
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
    volatile uint32_t *regBase;
    uint32_t regOffset;
    uint32_t reg;

    switch (pinIndex >> 4U)
    {
        case 0U:
            regBase = &base->PE1;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16))
        case 1U:
            regBase = &base->PE2;
            break;
#endif
        default:
            regBase = NULL;
            break;
    }
#else
    volatile uint8_t *regBase;
    uint8_t regOffset;
    uint8_t reg;
    switch (pinIndex >> 2U)
    {
        case 0U:
            regBase = &base->PE1;
            break;
        case 1U:
            regBase = &base->PE2;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 8))
        case 2U:
            regBase = &base->PE3;
            break;
#endif
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 12))
        case 3U:
            regBase = &base->PE4;
            break;
#endif
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16))
        case 4U:
            regBase = &base->PE5;
            break;
#endif
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 20))
        case 5U:
            regBase = &base->PE6;
            break;
#endif
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 24))
        case 6U:
            regBase = &base->PE7;
            break;
#endif
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 28))
        case 7U:
            regBase = &base->PE8;
            break;
#endif
        default:
            regBase = NULL;
            break;
    }
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH == 32 */

    if (regBase)
    {
        reg = *regBase;
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
        regOffset = ((pinIndex & 0x0FU) << 1U);
#else
        regOffset = ((pinIndex & 0x03U) << 1U);
#endif
        reg &= ~(0x3U << regOffset);
        reg |= ((uint32_t)pinMode << regOffset);
        *regBase = reg;
    }
}

bool LLWU_GetExternalWakeupPinFlag(LLWU_Type *base, uint32_t pinIndex)
{
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
    return (bool)(base->PF & (1U << pinIndex));
#else
    volatile uint8_t *regBase;

    switch (pinIndex >> 3U)
    {
#if (defined(FSL_FEATURE_LLWU_HAS_PF) && FSL_FEATURE_LLWU_HAS_PF)
        case 0U:
            regBase = &base->PF1;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 8))
        case 1U:
            regBase = &base->PF2;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16))
        case 2U:
            regBase = &base->PF3;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 24))
        case 3U:
            regBase = &base->PF4;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#else
        case 0U:
            regBase = &base->F1;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 8))
        case 1U:
            regBase = &base->F2;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16))
        case 2U:
            regBase = &base->F3;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 24))
        case 3U:
            regBase = &base->F4;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#endif /* FSL_FEATURE_LLWU_HAS_PF */
        default:
            regBase = NULL;
            break;
    }

    if (regBase)
    {
        return (bool)(*regBase & (1U << pinIndex % 8));
    }
    else
    {
        return false;
    }
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH */
}

void LLWU_ClearExternalWakeupPinFlag(LLWU_Type *base, uint32_t pinIndex)
{
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
    base->PF = (1U << pinIndex);
#else
    volatile uint8_t *regBase;
    switch (pinIndex >> 3U)
    {
#if (defined(FSL_FEATURE_LLWU_HAS_PF) && FSL_FEATURE_LLWU_HAS_PF)
        case 0U:
            regBase = &base->PF1;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 8))
        case 1U:
            regBase = &base->PF2;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16))
        case 2U:
            regBase = &base->PF3;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 24))
        case 3U:
            regBase = &base->PF4;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#else
        case 0U:
            regBase = &base->F1;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 8))
        case 1U:
            regBase = &base->F2;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16))
        case 2U:
            regBase = &base->F3;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 24))
        case 3U:
            regBase = &base->F4;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#endif /* FSL_FEATURE_LLWU_HAS_PF */
        default:
            regBase = NULL;
            break;
    }
    if (regBase)
    {
        *regBase = (1U << pinIndex % 8U);
    }
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH */
}
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */

#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && FSL_FEATURE_LLWU_HAS_PIN_FILTER)
void LLWU_SetPinFilterMode(LLWU_Type *base, uint32_t filterIndex, llwu_external_pin_filter_mode_t filterMode)
{
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
    uint32_t reg;

    reg = base->FILT;
    reg &= ~((LLWU_FILT_FILTSEL1_MASK | LLWU_FILT_FILTE1_MASK) << (filterIndex * 8U - 1U));
    reg |= (((filterMode.pinIndex << LLWU_FILT_FILTSEL1_SHIFT) | (filterMode.filterMode << LLWU_FILT_FILTE1_SHIFT)
             /* Clear the Filter Detect Flag */
             | LLWU_FILT_FILTF1_MASK)
            << (filterIndex * 8U - 1U));
    base->FILT = reg;
#else
    volatile uint8_t *regBase;
    uint8_t reg;

    switch (filterIndex)
    {
        case 1:
            regBase = &base->FILT1;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 1))
        case 2:
            regBase = &base->FILT2;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 2))
        case 3:
            regBase = &base->FILT3;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 3))
        case 4:
            regBase = &base->FILT4;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
        default:
            regBase = NULL;
            break;
    }

    if (regBase)
    {
        reg = *regBase;
        reg &= ~(LLWU_FILT1_FILTSEL_MASK | LLWU_FILT1_FILTE_MASK);
        reg |= ((uint32_t)filterMode.pinIndex << LLWU_FILT1_FILTSEL_SHIFT);
        reg |= ((uint32_t)filterMode.filterMode << LLWU_FILT1_FILTE_SHIFT);
        /* Clear the Filter Detect Flag */
        reg |= LLWU_FILT1_FILTF_MASK;
        *regBase = reg;
    }
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH */
}

bool LLWU_GetPinFilterFlag(LLWU_Type *base, uint32_t filterIndex)
{
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
    return (bool)(base->FILT & (1U << (filterIndex * 8U - 1)));
#else
    bool status = false;

    switch (filterIndex)
    {
        case 1:
            status = (base->FILT1 & LLWU_FILT1_FILTF_MASK);
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 1))
        case 2:
            status = (base->FILT2 & LLWU_FILT2_FILTF_MASK);
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 2))
        case 3:
            status = (base->FILT3 & LLWU_FILT3_FILTF_MASK);
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 3))
        case 4:
            status = (base->FILT4 & LLWU_FILT4_FILTF_MASK);
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
        default:
            break;
    }

    return status;
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH */
}

void LLWU_ClearPinFilterFlag(LLWU_Type *base, uint32_t filterIndex)
{
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
    uint32_t reg;

    reg = base->FILT;
    switch (filterIndex)
    {
        case 1:
            reg |= LLWU_FILT_FILTF1_MASK;
            break;
        case 2:
            reg |= LLWU_FILT_FILTF2_MASK;
            break;
        case 3:
            reg |= LLWU_FILT_FILTF3_MASK;
            break;
        case 4:
            reg |= LLWU_FILT_FILTF4_MASK;
            break;
        default:
            break;
    }
    base->FILT = reg;
#else
    volatile uint8_t *regBase;
    uint8_t reg;

    switch (filterIndex)
    {
        case 1:
            regBase = &base->FILT1;
            break;
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 1))
        case 2:
            regBase = &base->FILT2;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 2))
        case 3:
            regBase = &base->FILT3;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && (FSL_FEATURE_LLWU_HAS_PIN_FILTER > 3))
        case 4:
            regBase = &base->FILT4;
            break;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */
        default:
            regBase = NULL;
            break;
    }

    if (regBase)
    {
        reg = *regBase;
        reg |= LLWU_FILT1_FILTF_MASK;
        *regBase = reg;
    }
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH */
}
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */

#if (defined(FSL_FEATURE_LLWU_HAS_RESET_ENABLE) && FSL_FEATURE_LLWU_HAS_RESET_ENABLE)
void LLWU_SetResetPinMode(LLWU_Type *base, bool pinEnable, bool enableInLowLeakageMode)
{
    uint8_t reg;

    reg = base->RST;
    reg &= ~(LLWU_RST_LLRSTE_MASK | LLWU_RST_RSTFILT_MASK);
    reg |=
        (((uint32_t)pinEnable << LLWU_RST_LLRSTE_SHIFT) | ((uint32_t)enableInLowLeakageMode << LLWU_RST_RSTFILT_SHIFT));
    base->RST = reg;
}
#endif /* FSL_FEATURE_LLWU_HAS_RESET_ENABLE */
