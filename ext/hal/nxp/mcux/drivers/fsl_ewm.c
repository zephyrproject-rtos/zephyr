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

#include "fsl_ewm.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

void EWM_Init(EWM_Type *base, const ewm_config_t *config)
{
    assert(config);

    uint32_t value = 0U;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Ewm0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    value = EWM_CTRL_EWMEN(config->enableEwm) | EWM_CTRL_ASSIN(config->setInputAssertLogic) |
            EWM_CTRL_INEN(config->enableEwmInput) | EWM_CTRL_INTEN(config->enableInterrupt);
#if defined(FSL_FEATURE_EWM_HAS_PRESCALER) && FSL_FEATURE_EWM_HAS_PRESCALER
    base->CLKPRESCALER = config->prescaler;
#endif /* FSL_FEATURE_EWM_HAS_PRESCALER */

#if defined(FSL_FEATURE_EWM_HAS_CLOCK_SELECT) && FSL_FEATURE_EWM_HAS_CLOCK_SELECT
    base->CLKCTRL = config->clockSource;
#endif /* FSL_FEATURE_EWM_HAS_CLOCK_SELECT*/

    base->CMPL = config->compareLowValue;
    base->CMPH = config->compareHighValue;
    base->CTRL = value;
}

void EWM_Deinit(EWM_Type *base)
{
    EWM_DisableInterrupts(base, kEWM_InterruptEnable);
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_Ewm0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void EWM_GetDefaultConfig(ewm_config_t *config)
{
    assert(config);

    config->enableEwm = true;
    config->enableEwmInput = false;
    config->setInputAssertLogic = false;
    config->enableInterrupt = false;
#if defined(FSL_FEATURE_EWM_HAS_CLOCK_SELECT) && FSL_FEATURE_EWM_HAS_CLOCK_SELECT
    config->clockSource = kEWM_LpoClockSource0;
#endif /* FSL_FEATURE_EWM_HAS_CLOCK_SELECT*/
#if defined(FSL_FEATURE_EWM_HAS_PRESCALER) && FSL_FEATURE_EWM_HAS_PRESCALER
    config->prescaler = 0U;
#endif /* FSL_FEATURE_EWM_HAS_PRESCALER */
    config->compareLowValue = 0U;
    config->compareHighValue = 0xFEU;
}

void EWM_Refresh(EWM_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupt to protect refresh sequence */
    primaskValue = DisableGlobalIRQ();
    base->SERV = (uint8_t)0xB4U;
    base->SERV = (uint8_t)0x2CU;
    EnableGlobalIRQ(primaskValue);
}
