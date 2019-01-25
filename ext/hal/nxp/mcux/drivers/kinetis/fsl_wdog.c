/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_wdog.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.wdog"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Initializes the WDOG configuration sturcture.
 *
 * This function initializes the WDOG configuration structure to default values. The default
 * values are as follows.
 * code
 *   wdogConfig->enableWdog = true;
 *   wdogConfig->clockSource = kWDOG_LpoClockSource;
 *   wdogConfig->prescaler = kWDOG_ClockPrescalerDivide1;
 *   wdogConfig->workMode.enableWait = true;
 *   wdogConfig->workMode.enableStop = false;
 *   wdogConfig->workMode.enableDebug = false;
 *   wdogConfig->enableUpdate = true;
 *   wdogConfig->enableInterrupt = false;
 *   wdogConfig->enableWindowMode = false;
 *   wdogConfig->windowValue = 0;
 *   wdogConfig->timeoutValue = 0xFFFFU;
 * endcode
 *
 * param config Pointer to the WDOG configuration structure.
 * see wdog_config_t
 */
void WDOG_GetDefaultConfig(wdog_config_t *config)
{
    assert(config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->enableWdog = true;
    config->clockSource = kWDOG_LpoClockSource;
    config->prescaler = kWDOG_ClockPrescalerDivide1;
#if defined(FSL_FEATURE_WDOG_HAS_WAITEN) && FSL_FEATURE_WDOG_HAS_WAITEN
    config->workMode.enableWait = true;
#endif /* FSL_FEATURE_WDOG_HAS_WAITEN */
    config->workMode.enableStop = false;
    config->workMode.enableDebug = false;
    config->enableUpdate = true;
    config->enableInterrupt = false;
    config->enableWindowMode = false;
    config->windowValue = 0U;
    config->timeoutValue = 0xFFFFU;
}

/*!
 * brief Initializes the WDOG.
 *
 * This function initializes the WDOG. When called, the WDOG runs according to the configuration.
 * To reconfigure WDOG without forcing a reset first, enableUpdate must be set to true
 * in the configuration.
 *
 * This is an example.
 * code
 *   wdog_config_t config;
 *   WDOG_GetDefaultConfig(&config);
 *   config.timeoutValue = 0x7ffU;
 *   config.enableUpdate = true;
 *   WDOG_Init(wdog_base,&config);
 * endcode
 *
 * param base   WDOG peripheral base address
 * param config The configuration of WDOG
 */
void WDOG_Init(WDOG_Type *base, const wdog_config_t *config)
{
    assert(config);

    uint32_t value = 0U;
    uint32_t primaskValue = 0U;

    value = WDOG_STCTRLH_WDOGEN(config->enableWdog) | WDOG_STCTRLH_CLKSRC(config->clockSource) |
            WDOG_STCTRLH_IRQRSTEN(config->enableInterrupt) | WDOG_STCTRLH_WINEN(config->enableWindowMode) |
            WDOG_STCTRLH_ALLOWUPDATE(config->enableUpdate) | WDOG_STCTRLH_DBGEN(config->workMode.enableDebug) |
            WDOG_STCTRLH_STOPEN(config->workMode.enableStop) |
#if defined(FSL_FEATURE_WDOG_HAS_WAITEN) && FSL_FEATURE_WDOG_HAS_WAITEN
            WDOG_STCTRLH_WAITEN(config->workMode.enableWait) |
#endif /* FSL_FEATURE_WDOG_HAS_WAITEN */
            WDOG_STCTRLH_DISTESTWDOG(1U);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    WDOG_Unlock(base);
    /* Wait one bus clock cycle */
    base->RSTCNT = 0U;
    /* Set configruation */
    base->PRESC = WDOG_PRESC_PRESCVAL(config->prescaler);
    base->WINH = (uint16_t)((config->windowValue >> 16U) & 0xFFFFU);
    base->WINL = (uint16_t)((config->windowValue) & 0xFFFFU);
    base->TOVALH = (uint16_t)((config->timeoutValue >> 16U) & 0xFFFFU);
    base->TOVALL = (uint16_t)((config->timeoutValue) & 0xFFFFU);
    base->STCTRLH = value;
    EnableGlobalIRQ(primaskValue);
}

/*!
 * brief Shuts down the WDOG.
 *
 * This function shuts down the WDOG.
 * Ensure that the WDOG_STCTRLH.ALLOWUPDATE is 1 which indicates that the register update is enabled.
 */
void WDOG_Deinit(WDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupts */
    primaskValue = DisableGlobalIRQ();
    WDOG_Unlock(base);
    /* Wait one bus clock cycle */
    base->RSTCNT = 0U;
    WDOG_Disable(base);
    EnableGlobalIRQ(primaskValue);
    WDOG_ClearResetCount(base);
}

/*!
 * brief Configures the WDOG functional test.
 *
 * This function is used to configure the WDOG functional test. When called, the WDOG goes into test mode
 * and runs according to the configuration.
 * Ensure that the WDOG_STCTRLH.ALLOWUPDATE is 1 which means that the register update is enabled.
 *
 * This is an example.
 * code
 *   wdog_test_config_t test_config;
 *   test_config.testMode = kWDOG_QuickTest;
 *   test_config.timeoutValue = 0xfffffu;
 *   WDOG_SetTestModeConfig(wdog_base, &test_config);
 * endcode
 * param base   WDOG peripheral base address
 * param config The functional test configuration of WDOG
 */
void WDOG_SetTestModeConfig(WDOG_Type *base, wdog_test_config_t *config)
{
    assert(config);

    uint32_t value = 0U;
    uint32_t primaskValue = 0U;

    value = WDOG_STCTRLH_DISTESTWDOG(0U) | WDOG_STCTRLH_TESTWDOG(1U) | WDOG_STCTRLH_TESTSEL(config->testMode) |
            WDOG_STCTRLH_BYTESEL(config->testedByte) | WDOG_STCTRLH_IRQRSTEN(0U) | WDOG_STCTRLH_WDOGEN(1U) |
            WDOG_STCTRLH_ALLOWUPDATE(1U);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    WDOG_Unlock(base);
    /* Wait one bus clock cycle */
    base->RSTCNT = 0U;
    /* Set configruation */
    base->TOVALH = (uint16_t)((config->timeoutValue >> 16U) & 0xFFFFU);
    base->TOVALL = (uint16_t)((config->timeoutValue) & 0xFFFFU);
    base->STCTRLH = value;
    EnableGlobalIRQ(primaskValue);
}

/*!
 * brief Gets the WDOG all status flags.
 *
 * This function gets all status flags.
 *
 * This is an example for getting the Running Flag.
 * code
 *   uint32_t status;
 *   status = WDOG_GetStatusFlags (wdog_base) & kWDOG_RunningFlag;
 * endcode
 * param base        WDOG peripheral base address
 * return            State of the status flag: asserted (true) or not-asserted (false).see _wdog_status_flags_t
 *                    - true: a related status flag has been set.
 *                    - false: a related status flag is not set.
 */
uint32_t WDOG_GetStatusFlags(WDOG_Type *base)
{
    uint32_t status_flag = 0U;

    status_flag |= (base->STCTRLH & WDOG_STCTRLH_WDOGEN_MASK);
    status_flag |= (base->STCTRLL & WDOG_STCTRLL_INTFLG_MASK);

    return status_flag;
}

/*!
 * brief Clears the WDOG flag.
 *
 * This function clears the WDOG status flag.
 *
 * This is an example for clearing the timeout (interrupt) flag.
 * code
 *   WDOG_ClearStatusFlags(wdog_base,kWDOG_TimeoutFlag);
 * endcode
 * param base        WDOG peripheral base address
 * param mask        The status flags to clear.
 *                    The parameter could be any combination of the following values.
 *                    kWDOG_TimeoutFlag
 */
void WDOG_ClearStatusFlags(WDOG_Type *base, uint32_t mask)
{
    if (mask & kWDOG_TimeoutFlag)
    {
        base->STCTRLL |= WDOG_STCTRLL_INTFLG_MASK;
    }
}

/*!
 * brief Refreshes the WDOG timer.
 *
 * This function feeds the WDOG.
 * This function should be called before the WDOG timer is in timeout. Otherwise, a reset is asserted.
 *
 * param base WDOG peripheral base address
 */
void WDOG_Refresh(WDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupt to protect refresh sequence */
    primaskValue = DisableGlobalIRQ();
    base->REFRESH = WDOG_FIRST_WORD_OF_REFRESH;
    base->REFRESH = WDOG_SECOND_WORD_OF_REFRESH;
    EnableGlobalIRQ(primaskValue);
}
