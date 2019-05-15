/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_rng.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.rng_1"
#endif

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*******************************************************************************
 * Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

void RNG_Init(RNG_Type *base)
{
    /* Clear ring oscilator disable bit*/
    PMC->PDRUNCFGCLR0 = PMC_PDRUNCFG0_PDEN_RNG_MASK;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Rng);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    /* Clear POWERDOWN bit to enable RNG */
    base->POWERDOWN &= ~RNG_POWERDOWN_POWERDOWN_MASK;
}

void RNG_Deinit(RNG_Type *base)
{
    /* Set ring oscilator disable bit*/
    PMC->PDRUNCFGSET0 = PMC_PDRUNCFG0_PDEN_RNG_MASK;
    /* Set POWERDOWN bit to disable RNG */
    base->POWERDOWN |= RNG_POWERDOWN_POWERDOWN_MASK;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_Rng);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

status_t RNG_GetRandomData(RNG_Type *base, void *data, size_t dataSize)
{
    status_t result = kStatus_Fail;
    uint32_t random32;
    uint32_t randomSize;
    uint8_t *pRandom;
    uint8_t *pData = (uint8_t *)data;
    uint32_t i;

    /* Check input parameters.*/
    if (!(base && data && dataSize))
    {
        result = kStatus_InvalidArgument;
    }
    else
    {
        /* Check that ring oscilator is enabled */
        if (!(PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_RNG_MASK))
        {
            do
            {
                /* Read Entropy.*/
                random32 = base->RANDOM_NUMBER;
                pRandom = (uint8_t *)&random32;

                if (dataSize < sizeof(random32))
                {
                    randomSize = dataSize;
                }
                else
                {
                    randomSize = sizeof(random32);
                }

                for (i = 0; i < randomSize; i++)
                {
                    *pData++ = *pRandom++;
                }

                dataSize -= randomSize;
            } while (dataSize > 0);

            result = kStatus_Success;
        }
    }

    return result;
}
