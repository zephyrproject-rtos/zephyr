/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_ewm.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

void EWM_Init(EWM_Type *base, const ewm_config_t *config)
{
    assert(config);

    uint32_t value = 0U;

#if !((defined(FSL_FEATURE_SOC_PCC_COUNT) && FSL_FEATURE_SOC_PCC_COUNT) && \
    (defined(FSL_FEATURE_PCC_SUPPORT_EWM_CLOCK_REMOVE) && FSL_FEATURE_PCC_SUPPORT_EWM_CLOCK_REMOVE))
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Ewm0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
#endif
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
#if !((defined(FSL_FEATURE_SOC_PCC_COUNT) && FSL_FEATURE_SOC_PCC_COUNT) && \
    (defined(FSL_FEATURE_PCC_SUPPORT_EWM_CLOCK_REMOVE) && FSL_FEATURE_PCC_SUPPORT_EWM_CLOCK_REMOVE))
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_Ewm0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
#endif /* FSL_FEATURE_PCC_SUPPORT_EWM_CLOCK_REMOVE */
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
