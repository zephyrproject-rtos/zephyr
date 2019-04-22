/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_lpit.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.lpit"
#endif

/*******************************************************************************
* Variables
******************************************************************************/

/*! @brief Array to map LPIT instance number to base pointer. */
static LPIT_Type *const s_lpitBases[] = LPIT_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Clock array name */
static const clock_ip_name_t s_lpitClock[] = LPIT_CLOCKS;

#if defined(LPIT_PERIPH_CLOCKS)
/* Array of LPIT functional clock name. */
static const clock_ip_name_t s_lpitPeriphClocks[] = LPIT_PERIPH_CLOCKS;
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get the instance for LPIT module.
 *
 * @param base LPIT base address
 */
uint32_t LPIT_GetInstance(LPIT_Type *base);

/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t LPIT_GetInstance(LPIT_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_lpitBases); instance++)
    {
        if (s_lpitBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_lpitBases));

    return instance;
}

/*!
 * brief Ungates the LPIT clock and configures the peripheral for a basic operation.
 *
 * This function issues a software reset to reset all channels and registers except the Module
 * Control register.
 *
 * note This API should be called at the beginning of the application using the LPIT driver.
 *
 * param base   LPIT peripheral base address.
 * param config Pointer to the user configuration structure.
 */
void LPIT_Init(LPIT_Type *base, const lpit_config_t *config)
{
    assert(config);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)

    uint32_t instance = LPIT_GetInstance(base);

    /* Enable the clock */
    CLOCK_EnableClock(s_lpitClock[instance]);
#if defined(LPIT_PERIPH_CLOCKS)
    CLOCK_EnableClock(s_lpitPeriphClocks[instance]);
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset the timer channels and registers except the MCR register */
    LPIT_Reset(base);

    /* Setup timer operation in debug and doze modes and enable the module */
    base->MCR =
        (LPIT_MCR_DBG_EN(config->enableRunInDebug) | LPIT_MCR_DOZE_EN(config->enableRunInDoze) | LPIT_MCR_M_CEN_MASK);
}

/*!
 * brief Disables the module and gates the LPIT clock.
 *
 * param base LPIT peripheral base address.
 */
void LPIT_Deinit(LPIT_Type *base)
{
    /* Disable the module */
    base->MCR &= ~LPIT_MCR_M_CEN_MASK;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)

    uint32_t instance = LPIT_GetInstance(base);

    /* Disable the clock */
    CLOCK_DisableClock(s_lpitClock[instance]);
#if defined(LPIT_PERIPH_CLOCKS)
    CLOCK_DisableClock(s_lpitPeriphClocks[instance]);
#endif

#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Fills in the LPIT configuration structure with default settings.
 *
 * The default values are:
 * code
 *    config->enableRunInDebug = false;
 *    config->enableRunInDoze = false;
 * endcode
 * param config Pointer to the user configuration structure.
 */
void LPIT_GetDefaultConfig(lpit_config_t *config)
{
    assert(config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    /* Timers are stopped in debug mode */
    config->enableRunInDebug = false;
    /* Timers are stopped in doze mode */
    config->enableRunInDoze = false;
}

/*!
 * brief Sets up an LPIT channel based on the user's preference.
 *
 * This function sets up the operation mode to one of the options available in the
 * enumeration ::lpit_timer_modes_t. It sets the trigger source as either internal or external,
 * trigger selection and the timers behaviour when a timeout occurs. It also chains
 * the timer if a prior timer if requested by the user.
 *
 * param base      LPIT peripheral base address.
 * param channel   Channel that is being configured.
 * param chnlSetup Configuration parameters.
 */
status_t LPIT_SetupChannel(LPIT_Type *base, lpit_chnl_t channel, const lpit_chnl_params_t *chnlSetup)
{
    assert(chnlSetup);

    uint32_t reg = 0;

    /* Cannot assert the chain bit for channel 0 */
    if ((channel == kLPIT_Chnl_0) && (chnlSetup->chainChannel == true))
    {
        return kStatus_Fail;
    }

    /* Setup the channel counters operation mode, trigger operation, chain mode */
    reg = (LPIT_TCTRL_MODE(chnlSetup->timerMode) | LPIT_TCTRL_TRG_SRC(chnlSetup->triggerSource) |
           LPIT_TCTRL_TRG_SEL(chnlSetup->triggerSelect) | LPIT_TCTRL_TROT(chnlSetup->enableReloadOnTrigger) |
           LPIT_TCTRL_TSOI(chnlSetup->enableStopOnTimeout) | LPIT_TCTRL_TSOT(chnlSetup->enableStartOnTrigger) |
           LPIT_TCTRL_CHAIN(chnlSetup->chainChannel));

    base->CHANNEL[channel].TCTRL = reg;

    return kStatus_Success;
}
