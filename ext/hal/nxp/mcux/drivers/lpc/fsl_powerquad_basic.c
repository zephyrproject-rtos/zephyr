/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_powerquad.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.powerquad_basic"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
void PQ_GetDefaultConfig(pq_config_t *config)
{
    config->inputAFormat = kPQ_Float;
    config->inputAPrescale = 0;
    config->inputBFormat = kPQ_Float;
    config->inputBPrescale = 0;
    config->outputFormat = kPQ_Float;
    config->outputPrescale = 0;
    config->tmpFormat = kPQ_Float;
    config->tmpPrescale = 0;
    config->machineFormat = kPQ_Float;
    config->tmpBase = (void *)0xE0000000U;
}

void PQ_SetConfig(POWERQUAD_Type *base, const pq_config_t *config)
{
    assert(config);

    base->TMPBASE = (uint32_t)config->tmpBase;
    base->INAFORMAT =
        ((uint32_t)config->inputAPrescale << 8U) | ((uint32_t)config->inputAFormat << 4U) | config->machineFormat;
    base->INBFORMAT =
        ((uint32_t)config->inputBPrescale << 8U) | ((uint32_t)config->inputBFormat << 4U) | config->machineFormat;
    base->TMPFORMAT =
        ((uint32_t)config->tmpPrescale << 8U) | ((uint32_t)config->tmpFormat << 4U) | config->machineFormat;
    base->OUTFORMAT =
        ((uint32_t)config->outputPrescale << 8U) | ((uint32_t)config->outputFormat << 4U) | config->machineFormat;
}

void PQ_Init(POWERQUAD_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_PowerQuad);
#endif
#if !(defined(FSL_SDK_DISABLE_DRIVER_RESET_CONTROL) && FSL_SDK_DISABLE_DRIVER_RESET_CONTROL)
    RESET_PeripheralReset(kPOWERQUAD_RST_SHIFT_RSTn);
#endif

    /* Enable event used for WFE. */
    base->EVENTEN = POWERQUAD_EVENTEN_EVENT_OFLOW_MASK | POWERQUAD_EVENTEN_EVENT_NAN_MASK |
                    POWERQUAD_EVENTEN_EVENT_FIXED_MASK | POWERQUAD_EVENTEN_EVENT_UFLOW_MASK |
                    POWERQUAD_EVENTEN_EVENT_BERR_MASK | POWERQUAD_EVENTEN_EVENT_COMP_MASK;
}

void PQ_Deinit(POWERQUAD_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_PowerQuad);
#endif
}

void PQ_SetFormat(POWERQUAD_Type *base, pq_computationengine_t engine, pq_format_t format)
{
    pq_config_t config;

    PQ_GetDefaultConfig(&config);

    /* 32-bit Float point */
    if (kPQ_Float == format)
    {
        config.inputAFormat = kPQ_Float;
        config.inputAPrescale = 0;
        config.inputBFormat = kPQ_Float;
        config.inputBPrescale = 0;
        config.outputFormat = kPQ_Float;
        config.outputPrescale = 0;
        config.tmpFormat = kPQ_Float;
        config.tmpPrescale = 0;
    }
    /* 32-bit Fixed point */
    if (kPQ_32Bit == format)
    {
        config.inputAFormat = kPQ_32Bit;
        config.inputAPrescale = 0;
        config.inputBFormat = kPQ_32Bit;
        config.inputBPrescale = 0;
        config.outputFormat = kPQ_32Bit;
        config.outputPrescale = 0;
        config.tmpFormat = kPQ_Float;
        config.tmpPrescale = 0;
    }
    /* 16-bit Fixed point */
    if (kPQ_16Bit == format)
    {
        config.inputAFormat = kPQ_16Bit;
        config.inputAPrescale = 0;
        config.inputBFormat = kPQ_16Bit;
        config.inputBPrescale = 0;
        config.outputFormat = kPQ_16Bit;
        config.outputPrescale = 0;
        config.tmpFormat = kPQ_Float;
        config.tmpPrescale = 0;
    }

    if (CP_FFT == engine)
    {
        config.machineFormat = kPQ_32Bit;
    }
    else
    {
        config.machineFormat = kPQ_Float;
    }

    PQ_SetConfig(base, &config);
}
