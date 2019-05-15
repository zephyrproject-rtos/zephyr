/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016, NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "fsl_reset.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.reset"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

#if (defined(FSL_FEATURE_SOC_SYSCON_COUNT) && (FSL_FEATURE_SOC_SYSCON_COUNT > 0))

/*!
 * brief Assert reset to peripheral.
 *
 * Asserts reset signal to specified peripheral module.
 *
 * param peripheral Assert reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_SetPeripheralReset(reset_ip_name_t peripheral)
{
    const uint32_t regIndex = ((uint32_t)peripheral & 0xFFFF0000u) >> 16;
    const uint32_t bitPos = ((uint32_t)peripheral & 0x0000FFFFu);
    const uint32_t bitMask = 1u << bitPos;

    assert(bitPos < 32u);

    /* reset register is in SYSCON */
    /* set bit */
    SYSCON->PRESETCTRLSET[regIndex] = bitMask;
    /* wait until it reads 0b1 */
    while (0u == (SYSCON->PRESETCTRLX[regIndex] & bitMask))
    {
    }
}

/*!
 * brief Clear reset to peripheral.
 *
 * Clears reset signal to specified peripheral module, allows it to operate.
 *
 * param peripheral Clear reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_ClearPeripheralReset(reset_ip_name_t peripheral)
{
    const uint32_t regIndex = ((uint32_t)peripheral & 0xFFFF0000u) >> 16;
    const uint32_t bitPos = ((uint32_t)peripheral & 0x0000FFFFu);
    const uint32_t bitMask = 1u << bitPos;

    assert(bitPos < 32u);

    /* reset register is in SYSCON */

    /* clear bit */
    SYSCON->PRESETCTRLCLR[regIndex] = bitMask;
    /* wait until it reads 0b0 */
    while (bitMask == (SYSCON->PRESETCTRLX[regIndex] & bitMask))
    {
    }
}

/*!
 * brief Reset peripheral module.
 *
 * Reset peripheral module.
 *
 * param peripheral Peripheral to reset. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_PeripheralReset(reset_ip_name_t peripheral)
{
    RESET_SetPeripheralReset(peripheral);
    RESET_ClearPeripheralReset(peripheral);
}

#endif /* FSL_FEATURE_SOC_SYSCON_COUNT || FSL_FEATURE_SOC_ASYNC_SYSCON_COUNT */
