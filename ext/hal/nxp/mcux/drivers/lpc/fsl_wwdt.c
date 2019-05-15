/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_wwdt.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.wwdt"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Gets the instance from the base address
 *
 * @param base WWDT peripheral base address
 *
 * @return The WWDT instance
 */
static uint32_t WWDT_GetInstance(WWDT_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to WWDT bases for each instance. */
static WWDT_Type *const s_wwdtBases[] = WWDT_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to WWDT clocks for each instance. */
static const clock_ip_name_t s_wwdtClocks[] = WWDT_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
#if !(defined(FSL_FEATURE_WWDT_HAS_NO_RESET) && FSL_FEATURE_WWDT_HAS_NO_RESET)
/*! @brief Pointers to WWDT resets for each instance. */
static const reset_ip_name_t s_wwdtResets[] = WWDT_RSTS;
#endif
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t WWDT_GetInstance(WWDT_Type *base)
{
    uint32_t instance;
    uint32_t wwdtArrayCount = (sizeof(s_wwdtBases) / sizeof(s_wwdtBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < wwdtArrayCount; instance++)
    {
        if (s_wwdtBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < wwdtArrayCount);

    return instance;
}

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Initializes WWDT configure structure.
 *
 * This function initializes the WWDT configure structure to default value. The default
 * value are:
 * code
 *  config->enableWwdt = true;
 *  config->enableWatchdogReset = false;
 *  config->enableWatchdogProtect = false;
 *  config->enableLockOscillator = false;
 *  config->windowValue = 0xFFFFFFU;
 *  config->timeoutValue = 0xFFFFFFU;
 *  config->warningValue = 0;
 * endcode
 *
 * param config Pointer to WWDT config structure.
 * see wwdt_config_t
 */
void WWDT_GetDefaultConfig(wwdt_config_t *config)
{
    assert(config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    /* Enable the watch dog */
    config->enableWwdt = true;
    /* Disable the watchdog timeout reset */
    config->enableWatchdogReset = false;
    /* Disable the watchdog protection for updating the timeout value */
    config->enableWatchdogProtect = false;
#if !(defined(FSL_FEATURE_WWDT_HAS_NO_OSCILLATOR_LOCK) && FSL_FEATURE_WWDT_HAS_NO_OSCILLATOR_LOCK)
    /* Do not lock the watchdog oscillator */
    config->enableLockOscillator = false;
#endif
    /* Windowing is not in effect */
    config->windowValue = 0xFFFFFFU;
    /* Set the timeout value to the max */
    config->timeoutValue = 0xFFFFFFU;
    /* No warning is provided */
    config->warningValue = 0;
    /* Set clock frequency. */
    config->clockFreq_Hz = 0U;
}

/*!
 * brief Initializes the WWDT.
 *
 * This function initializes the WWDT. When called, the WWDT runs according to the configuration.
 *
 * Example:
 * code
 *   wwdt_config_t config;
 *   WWDT_GetDefaultConfig(&config);
 *   config.timeoutValue = 0x7ffU;
 *   WWDT_Init(wwdt_base,&config);
 * endcode
 *
 * param base   WWDT peripheral base address
 * param config The configuration of WWDT
 */
void WWDT_Init(WWDT_Type *base, const wwdt_config_t *config)
{
    assert(config);
    /* The config->clockFreq_Hz must be set in order to config the delay time. */
    assert(config->clockFreq_Hz);

    uint32_t value = 0U;
    uint32_t timeDelay = 0U;

    timeDelay = (SystemCoreClock / config->clockFreq_Hz + 1) * 3;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the WWDT clock */
    CLOCK_EnableClock(s_wwdtClocks[WWDT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
#if !(defined(FSL_FEATURE_WWDT_HAS_NO_RESET) && FSL_FEATURE_WWDT_HAS_NO_RESET)
    /* Reset the module. */
    RESET_PeripheralReset(s_wwdtResets[WWDT_GetInstance(base)]);
#endif
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

#if !(defined(FSL_FEATURE_WWDT_HAS_NO_OSCILLATOR_LOCK) && FSL_FEATURE_WWDT_HAS_NO_OSCILLATOR_LOCK)
    value = WWDT_MOD_WDEN(config->enableWwdt) | WWDT_MOD_WDRESET(config->enableWatchdogReset) |
            WWDT_MOD_LOCK(config->enableLockOscillator);
#else
    value = WWDT_MOD_WDEN(config->enableWwdt) | WWDT_MOD_WDRESET(config->enableWatchdogReset);
#endif
    /* Set configuration */
    base->TC = WWDT_TC_COUNT(config->timeoutValue);
    base->MOD |= value;
    base->WINDOW = WWDT_WINDOW_WINDOW(config->windowValue);
    base->WARNINT = WWDT_WARNINT_WARNINT(config->warningValue);
    WWDT_Refresh(base);
    /*  This WDPROTECT bit can be set once by software and is only cleared by a reset */
    if ((base->MOD & WWDT_MOD_WDPROTECT_MASK) == 0U)
    {
        /* Set the WDPROTECT bit after the Feed Sequence (0xAA, 0x55) with 3 WDCLK delay */
        while (timeDelay--)
        {
            __NOP();
        }
        base->MOD |= WWDT_MOD_WDPROTECT(config->enableWatchdogProtect);
    }
}

/*!
 * brief Shuts down the WWDT.
 *
 * This function shuts down the WWDT.
 *
 * param base WWDT peripheral base address
 */
void WWDT_Deinit(WWDT_Type *base)
{
    WWDT_Disable(base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the WWDT clock */
    CLOCK_DisableClock(s_wwdtClocks[WWDT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Refreshes the WWDT timer.
 *
 * This function feeds the WWDT.
 * This function should be called before WWDT timer is in timeout. Otherwise, a reset is asserted.
 *
 * param base WWDT peripheral base address
 */
void WWDT_Refresh(WWDT_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupt to protect refresh sequence */
    primaskValue = DisableGlobalIRQ();
    base->FEED = WWDT_FIRST_WORD_OF_REFRESH;
    base->FEED = WWDT_SECOND_WORD_OF_REFRESH;
    EnableGlobalIRQ(primaskValue);
}

/*!
 * brief Clear WWDT flag.
 *
 * This function clears WWDT status flag.
 *
 * Example for clearing warning flag:
 * code
 *   WWDT_ClearStatusFlags(wwdt_base, kWWDT_WarningFlag);
 * endcode
 * param base WWDT peripheral base address
 * param mask The status flags to clear. This is a logical OR of members of the
 *             enumeration ::_wwdt_status_flags_t
 */
void WWDT_ClearStatusFlags(WWDT_Type *base, uint32_t mask)
{
    /* Clear the WDINT bit so that we don't accidentally clear it */
    uint32_t reg = (base->MOD & (~WWDT_MOD_WDINT_MASK));

    /* Clear timeout by writing a zero */
    if (mask & kWWDT_TimeoutFlag)
    {
        reg &= ~WWDT_MOD_WDTOF_MASK;
    }

    /* Clear warning interrupt flag by writing a one */
    if (mask & kWWDT_WarningFlag)
    {
        reg |= WWDT_MOD_WDINT_MASK;
    }

    base->MOD = reg;
}
