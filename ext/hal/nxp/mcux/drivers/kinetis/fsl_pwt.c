/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_pwt.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.pwt"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address
 *
 * @param base PWT peripheral base address
 *
 * @return The PWT instance
 */
static uint32_t PWT_GetInstance(PWT_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to PWT bases for each instance. */
static PWT_Type *const s_pwtBases[] = PWT_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to PWT clocks for each instance. */
static const clock_ip_name_t s_pwtClocks[] = PWT_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t PWT_GetInstance(PWT_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_pwtBases); instance++)
    {
        if (s_pwtBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_pwtBases));

    return instance;
}

/*!
 * brief Ungates the PWT clock and configures the peripheral for basic operation.
 *
 * note This API should be called at the beginning of the application using the PWT driver.
 *
 * param base   PWT peripheral base address
 * param config Pointer to the user configuration structure.
 */
void PWT_Init(PWT_Type *base, const pwt_config_t *config)
{
    assert(config);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate the PWT module clock*/
    CLOCK_EnableClock(s_pwtClocks[PWT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset the module */
    PWT_Reset(base);

    /*
     * Clear all interrupt, disable the counter, config the first counter load enable bit,
     * enable module interrupt bit.
     */
    base->CS = PWT_CS_FCTLE(config->enableFirstCounterLoad) | PWT_CS_PWTIE_MASK;

    /* Set clock source, prescale and input source */
    base->CR = PWT_CR_PCLKS(config->clockSource) | PWT_CR_PRE(config->prescale) | PWT_CR_PINSEL(config->inputSelect);
}

/*!
 * brief Gates the PWT clock.
 *
 * param base PWT peripheral base address
 */
void PWT_Deinit(PWT_Type *base)
{
    /* Disable the counter */
    base->CS &= ~PWT_CS_PWTEN_MASK;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the PWT clock */
    CLOCK_DisableClock(s_pwtClocks[PWT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief  Fills in the PWT configuration structure with the default settings.
 *
 * The default values are:
 * code
 *   config->clockSource = kPWT_BusClock;
 *   config->prescale = kPWT_Prescale_Divide_1;
 *   config->inputSelect = kPWT_InputPort_0;
 *   config->enableFirstCounterLoad = false;
 * endcode
 * param config Pointer to the user configuration structure.
 */
void PWT_GetDefaultConfig(pwt_config_t *config)
{
    assert(config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    /* Use the IP Bus clock as source clock for the PWT submodule */
    config->clockSource = kPWT_BusClock;
    /* Clock source prescale is set to divide by 1*/
    config->prescale = kPWT_Prescale_Divide_1;
    /* PWT input signal coming from Port 0 */
    config->inputSelect = kPWT_InputPort_0;
    /* Do not load the first counter values to the corresponding registers */
    config->enableFirstCounterLoad = false;
}
