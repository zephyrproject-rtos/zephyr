/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_rtwdog.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.rtwdog"
#endif


/*******************************************************************************
 * Code
 ******************************************************************************/

void RTWDOG_ClearStatusFlags(RTWDOG_Type *base, uint32_t mask)
{
    if (mask & kRTWDOG_InterruptFlag)
    {
        base->CS |= RTWDOG_CS_FLG_MASK;
    }
}

void RTWDOG_GetDefaultConfig(rtwdog_config_t *config)
{
    assert(config);

    config->enableRtwdog = true;
    config->clockSource = kRTWDOG_ClockSource1;
    config->prescaler = kRTWDOG_ClockPrescalerDivide1;
    config->workMode.enableWait = true;
    config->workMode.enableStop = false;
    config->workMode.enableDebug = false;
    config->testMode = kRTWDOG_TestModeDisabled;
    config->enableUpdate = true;
    config->enableInterrupt = false;
    config->enableWindowMode = false;
    config->windowValue = 0U;
    config->timeoutValue = 0xFFFFU;
}

void RTWDOG_Init(RTWDOG_Type *base, const rtwdog_config_t *config)
{
    assert(config);

    uint32_t value = 0U;
    uint32_t primaskValue = 0U;
 
    value = RTWDOG_CS_EN(config->enableRtwdog) | RTWDOG_CS_CLK(config->clockSource) | RTWDOG_CS_INT(config->enableInterrupt) |
            RTWDOG_CS_WIN(config->enableWindowMode) | RTWDOG_CS_UPDATE(config->enableUpdate) |
            RTWDOG_CS_DBG(config->workMode.enableDebug) | RTWDOG_CS_STOP(config->workMode.enableStop) |
            RTWDOG_CS_WAIT(config->workMode.enableWait) | RTWDOG_CS_PRES(config->prescaler) | RTWDOG_CS_CMD32EN(true) |
            RTWDOG_CS_TST(config->testMode);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    RTWDOG_Unlock(base);
    base->WIN = config->windowValue;
    base->TOVAL = config->timeoutValue;
    base->CS = value;
    EnableGlobalIRQ(primaskValue);
}

void RTWDOG_Deinit(RTWDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupts */
    primaskValue = DisableGlobalIRQ();
    RTWDOG_Unlock(base);
    RTWDOG_Disable(base);
    EnableGlobalIRQ(primaskValue);
}
