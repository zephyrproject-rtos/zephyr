/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_ostimer.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.ostimer"
#endif

/* Typedef for interrupt handler. */
typedef void (*ostimer_isr_t)(OSTIMER_Type *base, ostimer_callback_t cb);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address
 *
 * @param base OSTIMER peripheral base address
 *
 * @return The OSTIMER instance
 */
static uint32_t OSTIMER_GetInstance(OSTIMER_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Array of OSTIMER handle. */
static ostimer_callback_t s_ostimerHandle[FSL_FEATURE_SOC_OSTIMER_COUNT];
/* Array of OSTIMER peripheral base address. */
static OSTIMER_Type *const s_ostimerBases[] = OSTIMER_BASE_PTRS;
/* Array of OSTIMER IRQ number. */
static const IRQn_Type s_ostimerIRQ[] = OSTIMER_IRQS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of OSTIMER clock name. */
static const clock_ip_name_t s_ostimerClock[] = OSTIMER_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/* Array of OSTIMER reset name. */
static const reset_ip_name_t s_ostimerReset[] = OSTIMER_RSTS;

/* OSTIMER ISR for transactional APIs. */
static ostimer_isr_t s_ostimerIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/

/* @brief Function for getting the instance number of OS timer. */
static uint32_t OSTIMER_GetInstance(OSTIMER_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_ostimerBases); instance++)
    {
        if (s_ostimerBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_ostimerBases));

    return instance;
}

/* @brief Translate the value from gray-code to decimal. */
static uint64_t OSTIMER_GrayToDecimal(uint64_t gray)
{
    uint64_t temp = gray;
    while (temp)
    {
        temp >>= 1U;
        gray ^= temp;
    }

    return gray;
}

/* @brief Translate the value from decimal to gray-code. */
static uint64_t OSTIMER_DecimalToGray(uint64_t dec)
{
    return (dec ^ (dec >> 1U));
}

/*!
* @brief Initializes an OSTIMER by turning it's clock on.
*
*/
void OSTIMER_Init(OSTIMER_Type *base)
{
    assert(base);

    uint32_t instance = OSTIMER_GetInstance(base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the OSTIMER 32k clock in PMC module. */
    PMC->OSTIMERr |= PMC_OSTIMER_CLOCKENABLE_MASK;
    PMC->OSTIMERr &= ~PMC_OSTIMER_OSC32KPD_MASK;
    /* Enable clock for OSTIMER. */
    CLOCK_EnableClock(s_ostimerClock[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_FEATURE_OSTIMER_HAS_NO_RESET) && FSL_FEATURE_OSTIMER_HAS_NO_RESET)
    /* Reset the OSTIMER. */
    RESET_PeripheralReset(s_ostimerReset[instance]);
#endif
}

/*!
 * @brief Deinitializes a OSTIMER instance.
 *
 * This function shuts down OSTIMER clock
 *
 * @param base OSTIMER peripheral base address.
 */
void OSTIMER_Deinit(OSTIMER_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable clock for OSTIMER. */
    CLOCK_DisableClock(s_ostimerClock[OSTIMER_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * @brief Get OSTIMER status Flags.
 *
 * This returns the status flag.
 * Currently, only match interrupt flag can be got.
 *
 * @param base OSTIMER peripheral base address.
 * @return status register value
 */
uint32_t OSTIMER_GetStatusFlags(OSTIMER_Type *base)
{
    return base->OSEVENT_CTRL & OSTIMER_OSEVENT_CTRL_OSTIMER_INTRFLAG_MASK;
}

/*!
 * @brief Clear Status Interrupt Flags.
 *
 * This clears intr status flag.
 * Currently, only match interrupt flag can be cleared.
 *
 * @param base OSTIMER peripheral base address.
 * @return none
 */
void OSTIMER_ClearStatusFlags(OSTIMER_Type *base, uint32_t mask)
{
    base->OSEVENT_CTRL |= mask;
}

/*!
 * @brief Set the match raw value for OSTIMER.
 *
 * This function will set a match value for OSTIMER with an optional callback. And this callback
 * will be called while the data in dedicated pair match register is equals to the value of central EVTIMER.
 * Please note that, the data format is gray-code, if decimal data was desired, please using OSTIMER_SetMatchValue().
 *
 * @param base   OSTIMER peripheral base address.
 * @param count  OSTIMER timer match value.(Value is gray-code format)
 *
 * @param cb     OSTIMER callback (can be left as NULL if none, otherwise should be a void func(void)).
 * @return       none
 */
void OSTIMER_SetMatchRawValue(OSTIMER_Type *base, uint64_t count, ostimer_callback_t cb)
{
    uint64_t tmp = count;
    uint32_t instance = OSTIMER_GetInstance(base);

    s_ostimerIsr = OSTIMER_HandleIRQ;
    s_ostimerHandle[instance] = cb;

    /* Set the match value. */
    base->MATCHN_L = tmp;
    base->MATCHN_H = tmp >> 32U;

    /*
     * Enable deep sleep IRQ directly for some times the OS timer may run in deep sleep mode.
     * Please note that while enabling deep sleep IRQ, the NVIC will be also enabled.
     */
    base->OSEVENT_CTRL |= OSTIMER_OSEVENT_CTRL_OSTIMER_INTENA_MASK;
    PMC->OSTIMERr |= PMC_OSTIMER_DPDWAKEUPENABLE_MASK;
    EnableDeepSleepIRQ(s_ostimerIRQ[instance]);
}

/*!
 * @brief Set the match value for OSTIMER.
 *
 * This function will set a match value for OSTIMER with an optional callback. And this callback
 * will be called while the data in dedicated pair match register is equals to the value of central EVTIMER.
 *
 * @param base   OSTIMER peripheral base address.
 * @param count  OSTIMER timer match value.(Value is decimal format, and this value will be translate to Gray code in
 * API. )
 * @param cb  OSTIMER callback (can be left as NULL if none, otherwise should be a void func(void)).
 * @return none
 */
void OSTIMER_SetMatchValue(OSTIMER_Type *base, uint64_t count, ostimer_callback_t cb)
{
    uint64_t tmp = OSTIMER_DecimalToGray(count);

    OSTIMER_SetMatchRawValue(base, tmp, cb);
}

/*!
 * @brief Get current timer count value from OSTIMER.
 *
 * This function will get a decimal timer count value.
 * The RAW value of timer count is gray code format, will be translated to decimal data internally.
 *
 * @param base   OSTIMER peripheral base address.
 * @return Value of OSTIMER which will formated to decimal value.
 */
uint64_t OSTIMER_GetCurrentTimerValue(OSTIMER_Type *base)
{
    uint64_t tmp = 0U;

    tmp = OSTIMER_GetCurrentTimerRawValue(base);

    return OSTIMER_GrayToDecimal(tmp);
}

/*!
 * @brief Get the capture value from OSTIMER.
 *
 * This function will get a capture decimal-value from OSTIMER.
 * The RAW value of timer capture is gray code format, will be translated to decimal data internally.
 *
 * @param base   OSTIMER peripheral base address.
 * @return Value of capture register, data format is decimal.
 */
uint64_t OSTIMER_GetCaptureValue(OSTIMER_Type *base)
{
    uint64_t tmp = 0U;

    tmp = OSTIMER_GetCaptureRawValue(base);

    return OSTIMER_GrayToDecimal(tmp);
}

void OSTIMER_HandleIRQ(OSTIMER_Type *base, ostimer_callback_t cb)
{
    /* Clear the match interrupt flag. */
    OSTIMER_ClearStatusFlags(base, kOSTIMER_MatchInterruptFlag);

    if (cb)
    {
        cb();
    }
}

#if defined(OSTIMER)
void OS_EVENT_DriverIRQHandler(void)
{
    s_ostimerIsr(OSTIMER, s_ostimerHandle[0]);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
