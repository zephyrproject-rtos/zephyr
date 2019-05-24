/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_trgmux.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.trgmux"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Configures the trigger source of the appointed peripheral.
 *
 * The function configures the trigger source of the appointed peripheral.
 * Example:
   code
   TRGMUX_SetTriggerSource(TRGMUX0, kTRGMUX_Trgmux0Dmamux0, kTRGMUX_TriggerInput0, kTRGMUX_SourcePortPin);
   endcode
 * param base        TRGMUX peripheral base address.
 * param index       The index of the TRGMUX register, see the enum trgmux_device_t
 *                    defined in <SOC>.h.
 * param input       The MUX select for peripheral trigger input
 * param trigger_src The trigger inputs for various peripherals. See the enum trgmux_source_t
 *                    defined in <SOC>.h.
 * retval  kStatus_Success  Configured successfully.
 * retval  kStatus_TRGMUX_Locked   Configuration failed because the register is locked.
 */
status_t TRGMUX_SetTriggerSource(TRGMUX_Type *base, uint32_t index, trgmux_trigger_input_t input, uint32_t trigger_src)
{
    uint32_t value;

    value = base->TRGCFG[index];
    if (value & TRGMUX_TRGCFG_LK_MASK)
    {
        return kStatus_TRGMUX_Locked;
    }
    else
    {
        /* Since all SEL bitfileds in TRGCFG register have the same length, SEL0's mask is used to access other SEL
         * bitfileds. */
        value = (value & ~((uint32_t)(TRGMUX_TRGCFG_SEL0_MASK << input))) |
                ((uint32_t)(((uint32_t)(trigger_src & TRGMUX_TRGCFG_SEL0_MASK)) << input));
        base->TRGCFG[index] = value;
        return kStatus_Success;
    }
}
