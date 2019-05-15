/*
 * Copyright 2018 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "fsl_cmp.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.cmp_1"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
void CMP_Init(cmp_config_t *config)
{
    assert(NULL != config);

/*enable the clock to the register interface*/
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Comp);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*Clear reset to peripheral cmp*/
#if !(defined(FSL_FEATURE_CMP_HAS_NO_RESET) && FSL_TEATURE_CMP_HAS_NO_RESET)
    RESET_ClearPeripheralReset(kCMP_RST_SHIFT_RSTn);
#endif

    /*clear COMP bits*/
    PMC->COMP &= ~(PMC_COMP_LOWPOWER_MASK | PMC_COMP_HYST_MASK | PMC_COMP_PMUX_MASK | PMC_COMP_NMUX_MASK);

    PMC->COMP |= (config->enLowPower << PMC_COMP_LOWPOWER_SHIFT) /*Select if enter low power mode*/
                 | (config->enHysteris << PMC_COMP_HYST_SHIFT)   /*select if enable hysteresis*/
                 | config->pmuxInput                             /*pmux input source select*/
                 | config->nmuxInput;                            /*nmux input source select */
}

void CMP_Deinit(void)
{
/*disable the clock to the register interface*/
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_Comp);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}
