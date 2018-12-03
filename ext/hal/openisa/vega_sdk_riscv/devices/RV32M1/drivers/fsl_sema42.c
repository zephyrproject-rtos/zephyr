/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_sema42.h"

/******************************************************************************
 * Definitions
 *****************************************************************************/
/* The first number write to RSTGDP when reset SEMA42 gate. */
#define SEMA42_GATE_RESET_PATTERN_1 (0xE2U)
/* The second number write to RSTGDP when reset SEMA42 gate. */
#define SEMA42_GATE_RESET_PATTERN_2 (0x1DU)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get instance number for SEMA42 module.
 *
 * @param base SEMA42 peripheral base address.
 */
uint32_t SEMA42_GetInstance(SEMA42_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Pointers to sema42 bases for each instance. */
static SEMA42_Type *const s_sema42Bases[] = SEMA42_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to sema42 clocks for each instance. */
static const clock_ip_name_t s_sema42Clocks[] = SEMA42_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/******************************************************************************
 * CODE
 *****************************************************************************/

uint32_t SEMA42_GetInstance(SEMA42_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_sema42Bases); instance++)
    {
        if (s_sema42Bases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_sema42Bases));

    return instance;
}

void SEMA42_Init(SEMA42_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(s_sema42Clocks[SEMA42_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void SEMA42_Deinit(SEMA42_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(s_sema42Clocks[SEMA42_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

status_t SEMA42_TryLock(SEMA42_Type *base, uint8_t gateNum, uint8_t procNum)
{
    assert(gateNum < FSL_FEATURE_SEMA42_GATE_COUNT);

    ++procNum;

    /* Try to lock. */
    SEMA42_GATEn(base, gateNum) = procNum;

    /* Check locked or not. */
    if (procNum != SEMA42_GATEn(base, gateNum))
    {
        return kStatus_SEMA42_Busy;
    }

    return kStatus_Success;
}

void SEMA42_Lock(SEMA42_Type *base, uint8_t gateNum, uint8_t procNum)
{
    assert(gateNum < FSL_FEATURE_SEMA42_GATE_COUNT);

    ++procNum;

    while (procNum != SEMA42_GATEn(base, gateNum))
    {
        /* Wait for unlocked status. */
        while (SEMA42_GATEn(base, gateNum))
        {
        }

        /* Lock the gate. */
        SEMA42_GATEn(base, gateNum) = procNum;
    }
}

status_t SEMA42_ResetGate(SEMA42_Type *base, uint8_t gateNum)
{
    /*
     * Reset all gates if gateNum >= SEMA42_GATE_NUM_RESET_ALL
     * Reset specific gate if gateNum < FSL_FEATURE_SEMA42_GATE_COUNT
     */
    assert(!((gateNum < SEMA42_GATE_NUM_RESET_ALL) && (gateNum >= FSL_FEATURE_SEMA42_GATE_COUNT)));

    /* Check whether some reset is ongoing. */
    if (base->RSTGT_R & SEMA42_RSTGT_R_RSTGSM_MASK)
    {
        return kStatus_SEMA42_Reseting;
    }

    /* First step. */
    base->RSTGT_W = SEMA42_RSTGT_W_RSTGDP(SEMA42_GATE_RESET_PATTERN_1);
    /* Second step. */
    base->RSTGT_W = SEMA42_RSTGT_W_RSTGDP(SEMA42_GATE_RESET_PATTERN_2) | SEMA42_RSTGT_W_RSTGTN(gateNum);

    return kStatus_Success;
}
