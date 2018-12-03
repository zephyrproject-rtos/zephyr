/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_lptmr.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if defined(LPTMR_CLOCKS)
/*!
 * @brief Gets the instance from the base address to be used to gate or ungate the module clock
 *
 * @param base LPTMR peripheral base address
 *
 * @return The LPTMR instance
 */
static uint32_t LPTMR_GetInstance(LPTMR_Type *base);
#endif /* LPTMR_CLOCKS */

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if defined(LPTMR_CLOCKS)
/*! @brief Pointers to LPTMR bases for each instance. */
static LPTMR_Type *const s_lptmrBases[] = LPTMR_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to LPTMR clocks for each instance. */
static const clock_ip_name_t s_lptmrClocks[] = LPTMR_CLOCKS;

#if defined(LPTMR_PERIPH_CLOCKS)
/* Array of LPTMR functional clock name. */
static const clock_ip_name_t s_lptmrPeriphClocks[] = LPTMR_PERIPH_CLOCKS;
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
#endif /* LPTMR_CLOCKS */

/*******************************************************************************
 * Code
 ******************************************************************************/
#if defined(LPTMR_CLOCKS)
static uint32_t LPTMR_GetInstance(LPTMR_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_lptmrBases); instance++)
    {
        if (s_lptmrBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_lptmrBases));

    return instance;
}
#endif /* LPTMR_CLOCKS */

void LPTMR_Init(LPTMR_Type *base, const lptmr_config_t *config)
{
    assert(config);

#if defined(LPTMR_CLOCKS)
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    
    uint32_t instance = LPTMR_GetInstance(base);

    /* Ungate the LPTMR clock*/
    CLOCK_EnableClock(s_lptmrClocks[instance]);
#if defined(LPTMR_PERIPH_CLOCKS)
    CLOCK_EnableClock(s_lptmrPeriphClocks[instance]);
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
#endif /* LPTMR_CLOCKS */

    /* Configure the timers operation mode and input pin setup */
    base->CSR = (LPTMR_CSR_TMS(config->timerMode) | LPTMR_CSR_TFC(config->enableFreeRunning) |
                 LPTMR_CSR_TPP(config->pinPolarity) | LPTMR_CSR_TPS(config->pinSelect));

    /* Configure the prescale value and clock source */
    base->PSR = (LPTMR_PSR_PRESCALE(config->value) | LPTMR_PSR_PBYP(config->bypassPrescaler) |
                 LPTMR_PSR_PCS(config->prescalerClockSource));
}

void LPTMR_Deinit(LPTMR_Type *base)
{
    /* Disable the LPTMR and reset the internal logic */
    base->CSR &= ~LPTMR_CSR_TEN_MASK;

#if defined(LPTMR_CLOCKS)
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)

    uint32_t instance = LPTMR_GetInstance(base);

    /* Gate the LPTMR clock*/
    CLOCK_DisableClock(s_lptmrClocks[instance]);
#if defined(LPTMR_PERIPH_CLOCKS)
    CLOCK_DisableClock(s_lptmrPeriphClocks[instance]);
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
#endif /* LPTMR_CLOCKS */
}

void LPTMR_GetDefaultConfig(lptmr_config_t *config)
{
    assert(config);

    /* Use time counter mode */
    config->timerMode = kLPTMR_TimerModeTimeCounter;
    /* Use input 0 as source in pulse counter mode */
    config->pinSelect = kLPTMR_PinSelectInput_0;
    /* Pulse input pin polarity is active-high */
    config->pinPolarity = kLPTMR_PinPolarityActiveHigh;
    /* Counter resets whenever TCF flag is set */
    config->enableFreeRunning = false;
    /* Bypass the prescaler */
    config->bypassPrescaler = true;
    /* LPTMR clock source */
    config->prescalerClockSource = kLPTMR_PrescalerClock_1;
    /* Divide the prescaler clock by 2 */
    config->value = kLPTMR_Prescale_Glitch_0;
}
