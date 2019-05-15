/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_utick.h"
#include "fsl_power.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.utick"
#endif

/* Typedef for interrupt handler. */
typedef void (*utick_isr_t)(UTICK_Type *base, utick_callback_t cb);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address
 *
 * @param base UTICK peripheral base address
 *
 * @return The UTICK instance
 */
static uint32_t UTICK_GetInstance(UTICK_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Array of UTICK handle. */
static utick_callback_t s_utickHandle[FSL_FEATURE_SOC_UTICK_COUNT];
/* Array of UTICK peripheral base address. */
static UTICK_Type *const s_utickBases[] = UTICK_BASE_PTRS;
/* Array of UTICK IRQ number. */
static const IRQn_Type s_utickIRQ[] = UTICK_IRQS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of UTICK clock name. */
static const clock_ip_name_t s_utickClock[] = UTICK_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_FEATURE_UTICK_HAS_NO_RESET) && FSL_FEATURE_UTICK_HAS_NO_RESET)
/*! @brief Pointers to UTICK resets for each instance. */
static const reset_ip_name_t s_utickResets[] = UTICK_RSTS;
#endif

/* UTICK ISR for transactional APIs. */
static utick_isr_t s_utickIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t UTICK_GetInstance(UTICK_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_utickBases); instance++)
    {
        if (s_utickBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_utickBases));

    return instance;
}

/*!
 * brief Starts UTICK.
 *
 * This function starts a repeat/onetime countdown with an optional callback
 *
 * param base   UTICK peripheral base address.
 * param mode  UTICK timer mode (ie kUTICK_onetime or kUTICK_repeat)
 * param count  UTICK timer mode (ie kUTICK_onetime or kUTICK_repeat)
 * param cb  UTICK callback (can be left as NULL if none, otherwise should be a void func(void))
 * return none
 */
void UTICK_SetTick(UTICK_Type *base, utick_mode_t mode, uint32_t count, utick_callback_t cb)
{
    uint32_t instance;

    /* Get instance from peripheral base address. */
    instance = UTICK_GetInstance(base);

    /* Save the handle in global variables to support the double weak mechanism. */
    s_utickHandle[instance] = cb;
    EnableDeepSleepIRQ(s_utickIRQ[instance]);
    base->CTRL = count | UTICK_CTRL_REPEAT(mode);
}

/*!
* brief Initializes an UTICK by turning its bus clock on
*
*/
void UTICK_Init(UTICK_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable utick clock */
    CLOCK_EnableClock(s_utickClock[UTICK_GetInstance(base)]);
#endif

#if !(defined(FSL_FEATURE_UTICK_HAS_NO_RESET) && FSL_FEATURE_UTICK_HAS_NO_RESET)
    RESET_PeripheralReset(s_utickResets[UTICK_GetInstance(base)]);
#endif

#if !(defined(FSL_FEATURE_UTICK_HAS_NO_PDCFG) && FSL_FEATURE_UTICK_HAS_NO_PDCFG)
    /* Power up Watchdog oscillator*/
    POWER_DisablePD(kPDRUNCFG_PD_WDT_OSC);
#endif

    s_utickIsr = UTICK_HandleIRQ;
}

/*!
 * brief Deinitializes a UTICK instance.
 *
 * This function shuts down Utick bus clock
 *
 * param base UTICK peripheral base address.
 */
void UTICK_Deinit(UTICK_Type *base)
{
    /* Turn off utick */
    base->CTRL = 0;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable utick clock */
    CLOCK_DisableClock(s_utickClock[UTICK_GetInstance(base)]);
#endif
}

/*!
 * brief Get Status Flags.
 *
 * This returns the status flag
 *
 * param base UTICK peripheral base address.
 * return status register value
 */
uint32_t UTICK_GetStatusFlags(UTICK_Type *base)
{
    return (base->STAT);
}

/*!
 * brief Clear Status Interrupt Flags.
 *
 * This clears intr status flag
 *
 * param base UTICK peripheral base address.
 * return none
 */
void UTICK_ClearStatusFlags(UTICK_Type *base)
{
    base->STAT = UTICK_STAT_INTR_MASK;
}

/*!
 * brief UTICK Interrupt Service Handler.
 *
 * This function handles the interrupt and refers to the callback array in the driver to callback user (as per request
 * in UTICK_SetTick()).
 * if no user callback is scheduled, the interrupt will simply be cleared.
 *
 * param base   UTICK peripheral base address.
 * param cb  callback scheduled for this instance of UTICK
 * return none
 */
void UTICK_HandleIRQ(UTICK_Type *base, utick_callback_t cb)
{
    UTICK_ClearStatusFlags(base);
    if (cb)
    {
        cb();
    }
}

#if defined(UTICK0)
void UTICK0_DriverIRQHandler(void)
{
    s_utickIsr(UTICK0, s_utickHandle[0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
#if defined(UTICK1)
void UTICK1_DriverIRQHandler(void)
{
    s_utickIsr(UTICK1, s_utickHandle[1]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
#if defined(UTICK2)
void UTICK2_DriverIRQHandler(void)
{
    s_utickIsr(UTICK2, s_utickHandle[2]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
