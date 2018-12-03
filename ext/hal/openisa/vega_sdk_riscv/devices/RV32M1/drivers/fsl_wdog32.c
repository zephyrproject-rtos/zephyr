/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_wdog32.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

void WDOG32_ClearStatusFlags(WDOG_Type *base, uint32_t mask)
{
    if (mask & kWDOG32_InterruptFlag)
    {
        base->CS |= WDOG_CS_FLG_MASK;
    }
}

void WDOG32_GetDefaultConfig(wdog32_config_t *config)
{
    assert(config);

    config->enableWdog32 = true;
    config->clockSource = kWDOG32_ClockSource1;
    config->prescaler = kWDOG32_ClockPrescalerDivide1;
    config->workMode.enableWait = true;
    config->workMode.enableStop = false;
    config->workMode.enableDebug = false;
    config->testMode = kWDOG32_TestModeDisabled;
    config->enableUpdate = true;
    config->enableInterrupt = false;
    config->enableWindowMode = false;
    config->windowValue = 0U;
    config->timeoutValue = 0xFFFFU;
}

void WDOG32_Init(WDOG_Type *base, const wdog32_config_t *config)
{
    assert(config);

    uint32_t value = 0U;
    uint32_t primaskValue = 0U;

    value = WDOG_CS_EN(config->enableWdog32) | WDOG_CS_CLK(config->clockSource) | WDOG_CS_INT(config->enableInterrupt) |
            WDOG_CS_WIN(config->enableWindowMode) | WDOG_CS_UPDATE(config->enableUpdate) |
            WDOG_CS_DBG(config->workMode.enableDebug) | WDOG_CS_STOP(config->workMode.enableStop) |
            WDOG_CS_WAIT(config->workMode.enableWait) | WDOG_CS_PRES(config->prescaler) | WDOG_CS_CMD32EN(true) |
            WDOG_CS_TST(config->testMode);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    WDOG32_Unlock(base);
    base->WIN = config->windowValue;
    base->TOVAL = config->timeoutValue;
    base->CS = value;
    EnableGlobalIRQ(primaskValue);
}

void WDOG32_Deinit(WDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupts */
    primaskValue = DisableGlobalIRQ();
    WDOG32_Unlock(base);
    WDOG32_Disable(base);
    EnableGlobalIRQ(primaskValue);
}
