/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_wdog32.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.wdog32"
#endif

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

/*!
 * brief Initializes the WDOG32 configuration structure.
 *
 * This function initializes the WDOG32 configuration structure to default values. The default
 * values are:
 * code
 *   wdog32Config->enableWdog32 = true;
 *   wdog32Config->clockSource = kWDOG32_ClockSource1;
 *   wdog32Config->prescaler = kWDOG32_ClockPrescalerDivide1;
 *   wdog32Config->workMode.enableWait = true;
 *   wdog32Config->workMode.enableStop = false;
 *   wdog32Config->workMode.enableDebug = false;
 *   wdog32Config->testMode = kWDOG32_TestModeDisabled;
 *   wdog32Config->enableUpdate = true;
 *   wdog32Config->enableInterrupt = false;
 *   wdog32Config->enableWindowMode = false;
 *   wdog32Config->windowValue = 0U;
 *   wdog32Config->timeoutValue = 0xFFFFU;
 * endcode
 *
 * param config Pointer to the WDOG32 configuration structure.
 * see wdog32_config_t
 */
void WDOG32_GetDefaultConfig(wdog32_config_t *config)
{
    assert(config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

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

    register uint32_t value = 0U;
    uint32_t primaskValue = 0U;
    register WDOG_Type *regBase = base;
    register const wdog32_config_t *regConfig = config;

    value = WDOG_CS_EN(regConfig->enableWdog32) | WDOG_CS_CLK(regConfig->clockSource) |
            WDOG_CS_INT(regConfig->enableInterrupt) | WDOG_CS_WIN(regConfig->enableWindowMode) |
            WDOG_CS_UPDATE(regConfig->enableUpdate) | WDOG_CS_DBG(regConfig->workMode.enableDebug) |
            WDOG_CS_STOP(regConfig->workMode.enableStop) | WDOG_CS_WAIT(regConfig->workMode.enableWait) |
            WDOG_CS_PRES(regConfig->prescaler) | WDOG_CS_CMD32EN(true) | WDOG_CS_TST(regConfig->testMode);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    if ((regBase->CS) & WDOG_CS_CMD32EN_MASK)
    {
        regBase->CNT = WDOG_UPDATE_KEY;
    }
    else
    {
        regBase->CNT = WDOG_FIRST_WORD_OF_UNLOCK;
        regBase->CNT = WDOG_SECOND_WORD_OF_UNLOCK;
    }
    regBase->WIN = regConfig->windowValue;
    regBase->TOVAL = regConfig->timeoutValue;
    regBase->CS = value;
    EnableGlobalIRQ(primaskValue);
}

/*!
 * brief De-initializes the WDOG32 module.
 *
 * This function shuts down the WDOG32.
 * Ensure that the WDOG_CS.UPDATE is 1, which means that the register update is enabled.
 *
 * param base   WDOG32 peripheral base address.
 */
void WDOG32_Deinit(WDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupts */
    primaskValue = DisableGlobalIRQ();
    WDOG32_Unlock(base);
    WDOG32_Disable(base);
    EnableGlobalIRQ(primaskValue);
}
