/*
 * Copyright (c) 2018, NXP Semiconductors, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_plu.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.plu"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Gets the instance from the base address
 *
 * @param base PLU peripheral base address
 *
 * @return The PLU instance
 */
static uint32_t PLU_GetInstance(PLU_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to PLU bases for each instance. */
static PLU_Type *const s_pluBases[] = PLU_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to PLU clocks for each instance. */
static const clock_ip_name_t s_pluClocks[] = PLU_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
/*! @brief Pointers to PLU resets for each instance. */
static const reset_ip_name_t s_lpuResets[] = PLU_RSTS_N;
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t PLU_GetInstance(PLU_Type *base)
{
    uint32_t instance;
    uint32_t pluArrayCount = (sizeof(s_pluBases) / sizeof(s_pluBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < pluArrayCount; instance++)
    {
        if (s_pluBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < pluArrayCount);

    return instance;
}

/*!
 * brief Ungates the PLU clock and reset the module.
 *
 * note This API should be called at the beginning of the application using the PLU driver.
 *
 * param base PLU peripheral base address
 */
void PLU_Init(PLU_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the PLU peripheral clock */
    CLOCK_EnableClock(s_pluClocks[PLU_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    /* Reset the module. */
    RESET_PeripheralReset(s_lpuResets[PLU_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_RESET_CONTROL */
}

/*!
 * brief Gate the PLU clock
 *
 * param base PLU peripheral base address
 */
void PLU_Deinit(PLU_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the module clock */
    CLOCK_DisableClock((s_pluClocks[PLU_GetInstance(base)]));
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}
