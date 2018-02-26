/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
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

#include "fsl_gpc.h"

void GPC_EnableIRQ(GPC_Type *base, uint32_t irqId)
{
    uint32_t irqRegNum = irqId / 32U;
    uint32_t irqRegShiftNum = irqId % 32U;

    assert(irqRegNum > 0U);
    assert(irqRegNum <= GPC_IMR_COUNT);

#if ((defined FSL_FEATURE_GPC_HAS_IRQ_0_31) && FSL_FEATURE_GPC_HAS_IRQ_0_31)
    if (irqRegNum == GPC_IMR_COUNT)
    {
        base->IMR5 &= ~(1U << irqRegShiftNum);
    }
    else
    {
        base->IMR[irqRegNum] &= ~(1U << irqRegShiftNum);
    }
#else
    base->IMR[irqRegNum - 1U] &= ~(1U << irqRegShiftNum);
#endif /* FSL_FEATURE_GPC_HAS_IRQ_0_31 */
}

void GPC_DisableIRQ(GPC_Type *base, uint32_t irqId)
{
    uint32_t irqRegNum = irqId / 32U;
    uint32_t irqRegShiftNum = irqId % 32U;

    assert(irqRegNum > 0U);
    assert(irqRegNum <= GPC_IMR_COUNT);

#if ((defined FSL_FEATURE_GPC_HAS_IRQ_0_31) && FSL_FEATURE_GPC_HAS_IRQ_0_31)
    if (irqRegNum == GPC_IMR_COUNT)
    {
        base->IMR5 |= (1U << irqRegShiftNum);
    }
    else
    {
        base->IMR[irqRegNum] |= (1U << irqRegShiftNum);
    }
#else
    base->IMR[irqRegNum - 1U] |= (1U << irqRegShiftNum);
#endif /* FSL_FEATURE_GPC_HAS_IRQ_0_31 */
}

bool GPC_GetIRQStatusFlag(GPC_Type *base, uint32_t irqId)
{
    uint32_t irqRegNum = irqId / 32U;
    uint32_t irqRegShiftNum = irqId % 32U;
    uint32_t ret;

    assert(irqRegNum > 0U);
    assert(irqRegNum <= GPC_IMR_COUNT);

#if ((defined FSL_FEATURE_GPC_HAS_IRQ_0_31) && FSL_FEATURE_GPC_HAS_IRQ_0_31)
    if (irqRegNum == GPC_IMR_COUNT)
    {
        ret = base->ISR5 & (1U << irqRegShiftNum);
    }
    else
    {
        ret = base->ISR[irqRegNum] & (1U << irqRegShiftNum);
    }
#else
    ret = base->ISR[irqRegNum - 1U] & (1U << irqRegShiftNum);
#endif /* FSL_FEATURE_GPC_HAS_IRQ_0_31 */

    return (1U << irqRegShiftNum) == ret;
}
