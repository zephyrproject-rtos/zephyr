/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_fmeas.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.fmeas"
#endif

/*! @brief Target clock counter value.
 * According to user manual, 2 has to be subtracted from captured value (CAPVAL). */
#define TARGET_CLOCK_COUNT(base) \
    ((uint32_t)(                 \
        ((((SYSCON_Type *)base)->FREQMECTRL & SYSCON_FREQMECTRL_CAPVAL_MASK) >> SYSCON_FREQMECTRL_CAPVAL_SHIFT) - 2))

/*! @brief Reference clock counter value. */
#define REFERENCE_CLOCK_COUNT ((uint32_t)((SYSCON_FREQMECTRL_CAPVAL_MASK >> SYSCON_FREQMECTRL_CAPVAL_SHIFT) + 1))

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief    Returns the computed value for a frequency measurement cycle
 *
 * param    base         : SYSCON peripheral base address.
 * param    refClockRate : Reference clock rate used during the frequency measurement cycle.
 *
 * return   Frequency in Hz.
 */
uint32_t FMEAS_GetFrequency(SYSCON_Type *base, uint32_t refClockRate)
{
    uint32_t targetClockCount = TARGET_CLOCK_COUNT(base);
    uint64_t clkrate = 0;

    if (targetClockCount > 0)
    {
        clkrate = (((uint64_t)targetClockCount) * (uint64_t)refClockRate) / REFERENCE_CLOCK_COUNT;
    }

    return (uint32_t)clkrate;
}
