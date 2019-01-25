/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_ctimer.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.ctimer"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address
 *
 * @param base Ctimer peripheral base address
 *
 * @return The Timer instance
 */
static uint32_t CTIMER_GetInstance(CTIMER_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to Timer bases for each instance. */
static CTIMER_Type *const s_ctimerBases[] = CTIMER_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to Timer clocks for each instance. */
static const clock_ip_name_t s_ctimerClocks[] = CTIMER_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_FEATURE_CTIMER_HAS_NO_RESET) && (FSL_FEATURE_CTIMER_HAS_NO_RESET))
#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
#if defined(FSL_FEATURE_CTIMER_WRITE_ZERO_ASSERT_RESET) && FSL_FEATURE_CTIMER_WRITE_ZERO_ASSERT_RESET
/*! @brief Pointers to Timer resets for each instance, writing a zero asserts the reset */
static const reset_ip_name_t s_ctimerResets[] = CTIMER_RSTS_N;
#else
/*! @brief Pointers to Timer resets for each instance, writing a one asserts the reset */
static const reset_ip_name_t s_ctimerResets[] = CTIMER_RSTS;
#endif
#endif
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

/*! @brief Pointers real ISRs installed by drivers for each instance. */
static ctimer_callback_t *s_ctimerCallback[FSL_FEATURE_SOC_CTIMER_COUNT] = {0};

/*! @brief Callback type installed by drivers for each instance. */
static ctimer_callback_type_t ctimerCallbackType[FSL_FEATURE_SOC_CTIMER_COUNT] = {kCTIMER_SingleCallback};

/*! @brief Array to map timer instance to IRQ number. */
static const IRQn_Type s_ctimerIRQ[] = CTIMER_IRQS;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t CTIMER_GetInstance(CTIMER_Type *base)
{
    uint32_t instance;
    uint32_t ctimerArrayCount = (sizeof(s_ctimerBases) / sizeof(s_ctimerBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ctimerArrayCount; instance++)
    {
        if (s_ctimerBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ctimerArrayCount);

    return instance;
}

/*!
 * brief Ungates the clock and configures the peripheral for basic operation.
 *
 * note This API should be called at the beginning of the application before using the driver.
 *
 * param base   Ctimer peripheral base address
 * param config Pointer to the user configuration structure.
 */
void CTIMER_Init(CTIMER_Type *base, const ctimer_config_t *config)
{
    assert(config);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the timer clock*/
    CLOCK_EnableClock(s_ctimerClocks[CTIMER_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
/* Reset the module. */
#if !(defined(FSL_FEATURE_CTIMER_HAS_NO_RESET) && (FSL_FEATURE_CTIMER_HAS_NO_RESET))
    RESET_PeripheralReset(s_ctimerResets[CTIMER_GetInstance(base)]);
#endif
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

/* Setup the cimer mode and count select */
#if !defined(FSL_FEATURE_CTIMER_HAS_NO_INPUT_CAPTURE) && FSL_FEATURE_CTIMER_HAS_NO_INPUT_CAPTURE
    base->CTCR = CTIMER_CTCR_CTMODE(config->mode) | CTIMER_CTCR_CINSEL(config->input);
#endif
    /* Setup the timer prescale value */
    base->PR = CTIMER_PR_PRVAL(config->prescale);
}

/*!
 * brief Gates the timer clock.
 *
 * param base Ctimer peripheral base address
 */
void CTIMER_Deinit(CTIMER_Type *base)
{
    uint32_t index = CTIMER_GetInstance(base);
    /* Stop the timer */
    base->TCR &= ~CTIMER_TCR_CEN_MASK;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the timer clock*/
    CLOCK_DisableClock(s_ctimerClocks[index]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Disable IRQ at NVIC Level */
    DisableIRQ(s_ctimerIRQ[index]);
}

/*!
 * brief  Fills in the timers configuration structure with the default settings.
 *
 * The default values are:
 * code
 *   config->mode = kCTIMER_TimerMode;
 *   config->input = kCTIMER_Capture_0;
 *   config->prescale = 0;
 * endcode
 * param config Pointer to the user configuration structure.
 */
void CTIMER_GetDefaultConfig(ctimer_config_t *config)
{
    assert(config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    /* Run as a timer */
    config->mode = kCTIMER_TimerMode;
    /* This field is ignored when mode is timer */
    config->input = kCTIMER_Capture_0;
    /* Timer counter is incremented on every APB bus clock */
    config->prescale = 0;
}

/*!
 * brief Configures the PWM signal parameters.
 *
 * Enables PWM mode on the match channel passed in and will then setup the match value
 * and other match parameters to generate a PWM signal.
 * This function will assign match channel 3 to set the PWM cycle.
 *
 * note When setting PWM output from multiple output pins, all should use the same PWM
 * frequency. Please use CTIMER_SetupPwmPeriod to set up the PWM with high resolution.
 *
 * param base             Ctimer peripheral base address
 * param matchChannel     Match pin to be used to output the PWM signal
 * param dutyCyclePercent PWM pulse width; the value should be between 0 to 100
 * param pwmFreq_Hz       PWM signal frequency in Hz
 * param srcClock_Hz      Timer counter clock in Hz
 * param enableInt        Enable interrupt when the timer value reaches the match value of the PWM pulse,
 *                         if it is 0 then no interrupt is generated
 *
 * return kStatus_Success on success
 *         kStatus_Fail If matchChannel passed in is 3; this channel is reserved to set the PWM cycle
 */
status_t CTIMER_SetupPwm(CTIMER_Type *base,
                         ctimer_match_t matchChannel,
                         uint8_t dutyCyclePercent,
                         uint32_t pwmFreq_Hz,
                         uint32_t srcClock_Hz,
                         bool enableInt)
{
    assert(pwmFreq_Hz > 0);

    uint32_t reg;
    uint32_t period, pulsePeriod = 0;
    uint32_t timerClock = srcClock_Hz / (base->PR + 1);
    uint32_t index = CTIMER_GetInstance(base);

    if (matchChannel == kCTIMER_Match_3)
    {
        return kStatus_Fail;
    }

    /* Enable PWM mode on the channel */
    base->PWMC |= (1U << matchChannel);

    /* Clear the stop, reset and interrupt bits for this channel */
    reg = base->MCR;
    reg &= ~((CTIMER_MCR_MR0R_MASK | CTIMER_MCR_MR0S_MASK | CTIMER_MCR_MR0I_MASK) << (matchChannel * 3));

    /* If call back function is valid then enable match interrupt for the channel */
    if (enableInt)
    {
        reg |= (CTIMER_MCR_MR0I_MASK << (CTIMER_MCR_MR0I_SHIFT + (matchChannel * 3)));
    }

    /* Reset the counter when match on channel 3 */
    reg |= CTIMER_MCR_MR3R_MASK;

    base->MCR = reg;

    /* Calculate PWM period match value */
    period = (timerClock / pwmFreq_Hz) - 1;

    /* Calculate pulse width match value */
    if (dutyCyclePercent == 0)
    {
        pulsePeriod = period + 1;
    }
    else
    {
        pulsePeriod = (period * (100 - dutyCyclePercent)) / 100;
    }

    /* Match on channel 3 will define the PWM period */
    base->MR[kCTIMER_Match_3] = period;

    /* This will define the PWM pulse period */
    base->MR[matchChannel] = pulsePeriod;
    /* Clear status flags */
    CTIMER_ClearStatusFlags(base, CTIMER_IR_MR0INT_MASK << matchChannel);
    /* If call back function is valid then enable interrupt and update the call back function */
    if (enableInt)
    {
        EnableIRQ(s_ctimerIRQ[index]);
    }

    return kStatus_Success;
}

/*!
 * brief Configures the PWM signal parameters.
 *
 * Enables PWM mode on the match channel passed in and will then setup the match value
 * and other match parameters to generate a PWM signal.
 * This function will assign match channel 3 to set the PWM cycle.
 *
 * note When setting PWM output from multiple output pins, all should use the same PWM
 * period
 *
 * param base             Ctimer peripheral base address
 * param matchChannel     Match pin to be used to output the PWM signal
 * param pwmPeriod        PWM period match value
 * param pulsePeriod      Pulse width match value
 * param enableInt        Enable interrupt when the timer value reaches the match value of the PWM pulse,
 *                         if it is 0 then no interrupt is generated
 *
 * return kStatus_Success on success
 *         kStatus_Fail If matchChannel passed in is 3; this channel is reserved to set the PWM period
 */
status_t CTIMER_SetupPwmPeriod(
    CTIMER_Type *base, ctimer_match_t matchChannel, uint32_t pwmPeriod, uint32_t pulsePeriod, bool enableInt)
{
/* Some CTimers only have 16bits , so the value is limited*/
#if defined(FSL_FEATURE_SOC_CTIMER16B) && FSL_FEATURE_SOC_CTIMER16B
    assert(!((FSL_FEATURE_CTIMER_BIT_SIZEn(base) < 32) && (pulsePeriod > 0xFFFFU)));
#endif

    uint32_t reg;
    uint32_t index = CTIMER_GetInstance(base);

    if (matchChannel == kCTIMER_Match_3)
    {
        return kStatus_Fail;
    }

    /* Enable PWM mode on the channel */
    base->PWMC |= (1U << matchChannel);

    /* Clear the stop, reset and interrupt bits for this channel */
    reg = base->MCR;
    reg &= ~((CTIMER_MCR_MR0R_MASK | CTIMER_MCR_MR0S_MASK | CTIMER_MCR_MR0I_MASK) << (matchChannel * 3));

    /* If call back function is valid then enable match interrupt for the channel */
    if (enableInt)
    {
        reg |= (CTIMER_MCR_MR0I_MASK << (CTIMER_MCR_MR0I_SHIFT + (matchChannel * 3)));
    }

    /* Reset the counter when match on channel 3 */
    reg |= CTIMER_MCR_MR3R_MASK;

    base->MCR = reg;

    /* Match on channel 3 will define the PWM period */
    base->MR[kCTIMER_Match_3] = pwmPeriod;

    /* This will define the PWM pulse period */
    base->MR[matchChannel] = pulsePeriod;
    /* Clear status flags */
    CTIMER_ClearStatusFlags(base, CTIMER_IR_MR0INT_MASK << matchChannel);
    /* If call back function is valid then enable interrupt and update the call back function */
    if (enableInt)
    {
        EnableIRQ(s_ctimerIRQ[index]);
    }

    return kStatus_Success;
}

/*!
 * brief Updates the duty cycle of an active PWM signal.
 *
 * note Please use CTIMER_UpdatePwmPulsePeriod to update the PWM with high resolution.
 *
 * param base             Ctimer peripheral base address
 * param matchChannel     Match pin to be used to output the PWM signal
 * param dutyCyclePercent New PWM pulse width; the value should be between 0 to 100
 */
void CTIMER_UpdatePwmDutycycle(CTIMER_Type *base, ctimer_match_t matchChannel, uint8_t dutyCyclePercent)
{
    uint32_t pulsePeriod = 0, period;

    /* Match channel 3 defines the PWM period */
    period = base->MR[kCTIMER_Match_3];

    /* Calculate pulse width match value */
    pulsePeriod = (period * dutyCyclePercent) / 100;

    /* For 0% dutycyle, make pulse period greater than period so the event will never occur */
    if (dutyCyclePercent == 0)
    {
        pulsePeriod = period + 1;
    }
    else
    {
        pulsePeriod = (period * (100 - dutyCyclePercent)) / 100;
    }

    /* Update dutycycle */
    base->MR[matchChannel] = pulsePeriod;
}

/*!
 * brief Setup the match register.
 *
 * User configuration is used to setup the match value and action to be taken when a match occurs.
 *
 * param base         Ctimer peripheral base address
 * param matchChannel Match register to configure
 * param config       Pointer to the match configuration structure
 */
void CTIMER_SetupMatch(CTIMER_Type *base, ctimer_match_t matchChannel, const ctimer_match_config_t *config)
{
/* Some CTimers only have 16bits , so the value is limited*/
#if defined(FSL_FEATURE_SOC_CTIMER16B) && FSL_FEATURE_SOC_CTIMER16B
    assert(!(FSL_FEATURE_CTIMER_BIT_SIZEn(base) < 32 && config->matchValue > 0xFFFFU));
#endif
    uint32_t reg;
    uint32_t index = CTIMER_GetInstance(base);

    /* Set the counter operation when a match on this channel occurs */
    reg = base->MCR;
    reg &= ~((CTIMER_MCR_MR0R_MASK | CTIMER_MCR_MR0S_MASK | CTIMER_MCR_MR0I_MASK) << (matchChannel * 3));
    reg |= (uint32_t)((uint32_t)(config->enableCounterReset) << (CTIMER_MCR_MR0R_SHIFT + (matchChannel * 3)));
    reg |= (uint32_t)((uint32_t)(config->enableCounterStop) << (CTIMER_MCR_MR0S_SHIFT + (matchChannel * 3)));
    reg |= (uint32_t)((uint32_t)(config->enableInterrupt) << (CTIMER_MCR_MR0I_SHIFT + (matchChannel * 3)));
    base->MCR = reg;

    reg = base->EMR;
    /* Set the match output operation when a match on this channel occurs */
    reg &= ~(CTIMER_EMR_EMC0_MASK << (matchChannel * 2));
    reg |= (uint32_t)config->outControl << (CTIMER_EMR_EMC0_SHIFT + (matchChannel * 2));

    /* Set the initial state of the EM bit/output */
    reg &= ~(CTIMER_EMR_EM0_MASK << matchChannel);
    reg |= (uint32_t)config->outPinInitState << matchChannel;
    base->EMR = reg;

    /* Set the match value */
    base->MR[matchChannel] = config->matchValue;
    /* Clear status flags */
    CTIMER_ClearStatusFlags(base, CTIMER_IR_MR0INT_MASK << matchChannel);
    /* If interrupt is enabled then enable interrupt and update the call back function */
    if (config->enableInterrupt)
    {
        EnableIRQ(s_ctimerIRQ[index]);
    }
}

#if !defined(FSL_FEATURE_CTIMER_HAS_NO_INPUT_CAPTURE) && FSL_FEATURE_CTIMER_HAS_NO_INPUT_CAPTURE
/*!
 * brief Setup the capture.
 *
 * param base      Ctimer peripheral base address
 * param capture   Capture channel to configure
 * param edge      Edge on the channel that will trigger a capture
 * param enableInt Flag to enable channel interrupts, if enabled then the registered call back
 *                  is called upon capture
 */
void CTIMER_SetupCapture(CTIMER_Type *base,
                         ctimer_capture_channel_t capture,
                         ctimer_capture_edge_t edge,
                         bool enableInt)
{
    uint32_t reg = base->CCR;
    uint32_t index = CTIMER_GetInstance(base);

    /* Set the capture edge */
    reg &= ~((CTIMER_CCR_CAP0RE_MASK | CTIMER_CCR_CAP0FE_MASK | CTIMER_CCR_CAP0I_MASK) << (capture * 3));
    reg |= (uint32_t)edge << (CTIMER_CCR_CAP0RE_SHIFT + (capture * 3));
    /* Clear status flags */
    CTIMER_ClearStatusFlags(base, (kCTIMER_Capture0Flag << capture));
    /* If call back function is valid then enable capture interrupt for the channel and update the call back function */
    if (enableInt)
    {
        reg |= CTIMER_CCR_CAP0I_MASK << (capture * 3);
        EnableIRQ(s_ctimerIRQ[index]);
    }
    base->CCR = reg;
}
#endif

/*!
 * brief Register callback.
 *
 * param base      Ctimer peripheral base address
 * param cb_func   callback function
 * param cb_type   callback function type, singular or multiple
 */
void CTIMER_RegisterCallBack(CTIMER_Type *base, ctimer_callback_t *cb_func, ctimer_callback_type_t cb_type)
{
    uint32_t index = CTIMER_GetInstance(base);
    s_ctimerCallback[index] = cb_func;
    ctimerCallbackType[index] = cb_type;
}

void CTIMER_GenericIRQHandler(uint32_t index)
{
    uint32_t int_stat, i, mask;
    /* Get Interrupt status flags */
    int_stat = CTIMER_GetStatusFlags(s_ctimerBases[index]);
    /* Clear the status flags that were set */
    CTIMER_ClearStatusFlags(s_ctimerBases[index], int_stat);
    if (ctimerCallbackType[index] == kCTIMER_SingleCallback)
    {
        if (s_ctimerCallback[index][0])
        {
            s_ctimerCallback[index][0](int_stat);
        }
    }
    else
    {
#if defined(FSL_FEATURE_CTIMER_HAS_NO_INPUT_CAPTURE) && FSL_FEATURE_CTIMER_HAS_NO_INPUT_CAPTURE
        for (i = 0; i <= CTIMER_IR_MR3INT_SHIFT; i++)
#else
#if defined(FSL_FEATURE_CTIMER_HAS_IR_CR3INT) && FSL_FEATURE_CTIMER_HAS_IR_CR3INT
        for (i = 0; i <= CTIMER_IR_CR3INT_SHIFT; i++)
#else
        for (i = 0; i <= CTIMER_IR_CR2INT_SHIFT; i++)
#endif /* FSL_FEATURE_CTIMER_HAS_IR_CR3INT */
#endif
        {
            mask = 0x01 << i;
            /* For each status flag bit that was set call the callback function if it is valid */
            if ((int_stat & mask) && (s_ctimerCallback[index][i]))
            {
                s_ctimerCallback[index][i](int_stat);
            }
        }
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/* IRQ handler functions overloading weak symbols in the startup */
#if defined(CTIMER0)
void CTIMER0_DriverIRQHandler(void)
{
    CTIMER_GenericIRQHandler(0);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CTIMER1)
void CTIMER1_DriverIRQHandler(void)
{
    CTIMER_GenericIRQHandler(1);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CTIMER2)
void CTIMER2_DriverIRQHandler(void)
{
    CTIMER_GenericIRQHandler(2);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CTIMER3)
void CTIMER3_DriverIRQHandler(void)
{
    CTIMER_GenericIRQHandler(3);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CTIMER4)
void CTIMER4_DriverIRQHandler(void)
{
    CTIMER_GenericIRQHandler(4);
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
