/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_mrt.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.mrt"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address
 *
 * @param base Multi-Rate timer peripheral base address
 *
 * @return The MRT instance
 */
static uint32_t MRT_GetInstance(MRT_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to MRT bases for each instance. */
static MRT_Type *const s_mrtBases[] = MRT_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to MRT clocks for each instance. */
static const clock_ip_name_t s_mrtClocks[] = MRT_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
#if defined(FSL_FEATURE_MRT_WRITE_ZERO_ASSERT_RESET) && FSL_FEATURE_MRT_WRITE_ZERO_ASSERT_RESET
/*! @brief Pointers to MRT resets for each instance, writing a zero asserts the reset */
static const reset_ip_name_t s_mrtResets[] = MRT_RSTS_N;
#else
/*! @brief Pointers to MRT resets for each instance, writing a one asserts the reset */
static const reset_ip_name_t s_mrtResets[] = MRT_RSTS;
#endif
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t MRT_GetInstance(MRT_Type *base)
{
    uint32_t instance;
    uint32_t mrtArrayCount = (sizeof(s_mrtBases) / sizeof(s_mrtBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < mrtArrayCount; instance++)
    {
        if (s_mrtBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < mrtArrayCount);

    return instance;
}

/*!
 * brief Ungates the MRT clock and configures the peripheral for basic operation.
 *
 * note This API should be called at the beginning of the application using the MRT driver.
 *
 * param base   Multi-Rate timer peripheral base address
 * param config Pointer to user's MRT config structure. If MRT has  MULTITASK bit field in
 *               MODCFG reigster, param config is useless.
 */
void MRT_Init(MRT_Type *base, const mrt_config_t *config)
{
    assert(config);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate the MRT clock */
    CLOCK_EnableClock(s_mrtClocks[MRT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    /* Reset the module. */
    RESET_PeripheralReset(s_mrtResets[MRT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

#if !(defined(FSL_FEATURE_MRT_HAS_NO_MODCFG_MULTITASK) && FSL_FEATURE_MRT_HAS_NO_MODCFG_MULTITASK)
    /* Set timer operating mode */
    base->MODCFG = MRT_MODCFG_MULTITASK(config->enableMultiTask);
#endif
}

/*!
 * brief Gate the MRT clock
 *
 * param base Multi-Rate timer peripheral base address
 */
void MRT_Deinit(MRT_Type *base)
{
    /* Stop all the timers */
    MRT_StopTimer(base, kMRT_Channel_0);
    MRT_StopTimer(base, kMRT_Channel_1);
#if (FSL_FEATURE_MRT_NUMBER_OF_CHANNELS > 2U)
    MRT_StopTimer(base, kMRT_Channel_2);
#endif
#if (FSL_FEATURE_MRT_NUMBER_OF_CHANNELS > 3U)
    MRT_StopTimer(base, kMRT_Channel_3);
#endif

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the MRT clock*/
    CLOCK_DisableClock(s_mrtClocks[MRT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Used to update the timer period in units of count.
 *
 * The new value will be immediately loaded or will be loaded at the end of the current time
 * interval. For one-shot interrupt mode the new value will be immediately loaded.
 *
 * note User can call the utility macros provided in fsl_common.h to convert to ticks
 *
 * param base          Multi-Rate timer peripheral base address
 * param channel       Timer channel number
 * param count         Timer period in units of ticks
 * param immediateLoad true: Load the new value immediately into the TIMER register;
 *                      false: Load the new value at the end of current timer interval
 */
void MRT_UpdateTimerPeriod(MRT_Type *base, mrt_chnl_t channel, uint32_t count, bool immediateLoad)
{
    assert(channel < FSL_FEATURE_MRT_NUMBER_OF_CHANNELS);

    uint32_t newValue = count;
    if (((base->CHANNEL[channel].CTRL & MRT_CHANNEL_CTRL_MODE_MASK) == kMRT_OneShotMode) || (immediateLoad))
    {
        /* For one-shot interrupt mode, load the new value immediately even if user forgot to enable */
        newValue |= MRT_CHANNEL_INTVAL_LOAD_MASK;
    }

    /* Update the timer interval value */
    base->CHANNEL[channel].INTVAL = newValue;
}
