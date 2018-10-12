/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_wdog.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.wdog01"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
static WDOG_Type *const s_wdogBases[] = WDOG_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of WDOG clock name. */
static const clock_ip_name_t s_wdogClock[] = WDOG_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t WDOG_GetInstance(WDOG_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_wdogBases); instance++)
    {
        if (s_wdogBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_wdogBases));

    return instance;
}

void WDOG_GetDefaultConfig(wdog_config_t *config)
{
    assert(config);

    config->enableWdog = true;
    config->workMode.enableWait = false;
    config->workMode.enableStop = false;
    config->workMode.enableDebug = false;
    config->enableInterrupt = false;
    config->softwareResetExtension = false;
    config->enablePowerDown = false;
    config->softwareAssertion = true;
    config->softwareResetSignal = true;
    config->timeoutValue = 0xffu;
    config->interruptTimeValue = 0x04u;
}

void WDOG_Init(WDOG_Type *base, const wdog_config_t *config)
{
    assert(config);

    uint16_t value = 0u;

    value = WDOG_WCR_WDE(config->enableWdog) | WDOG_WCR_WDW(config->workMode.enableWait) |
            WDOG_WCR_WDZST(config->workMode.enableStop) | WDOG_WCR_WDBG(config->workMode.enableDebug) |
            WDOG_WCR_SRE(config->softwareResetExtension) | WDOG_WCR_WT(config->timeoutValue) |
            WDOG_WCR_WDA(config->softwareAssertion) | WDOG_WCR_SRS(config->softwareResetSignal);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Set configruation */
    CLOCK_EnableClock(s_wdogClock[WDOG_GetInstance(base)]);
#endif

    base->WICR = WDOG_WICR_WICT(config->interruptTimeValue) | WDOG_WICR_WIE(config->enableInterrupt);
    base->WMCR = WDOG_WMCR_PDE(config->enablePowerDown);
    base->WCR = value;
}

void WDOG_Deinit(WDOG_Type *base)
{
    if (base->WCR & WDOG_WCR_WDBG_MASK)
    {
        WDOG_Disable(base);
    }
}

uint16_t WDOG_GetStatusFlags(WDOG_Type *base)
{
    uint16_t status_flag = 0U;

    status_flag |= (base->WCR & WDOG_WCR_WDE_MASK);
    status_flag |= (base->WRSR & WDOG_WRSR_POR_MASK);
    status_flag |= (base->WRSR & WDOG_WRSR_TOUT_MASK);
    status_flag |= (base->WRSR & WDOG_WRSR_SFTW_MASK);
    status_flag |= (base->WICR & WDOG_WICR_WTIS_MASK);

    return status_flag;
}

void WDOG_ClearInterruptStatus(WDOG_Type *base, uint16_t mask)
{
    if (mask & kWDOG_InterruptFlag)
    {
        base->WICR |= WDOG_WICR_WTIS_MASK;
    }
}

void WDOG_Refresh(WDOG_Type *base)
{
    base->WSR = WDOG_REFRESH_KEY & 0xFFFFU;
    base->WSR = (WDOG_REFRESH_KEY >> 16U) & 0xFFFFU;
}
