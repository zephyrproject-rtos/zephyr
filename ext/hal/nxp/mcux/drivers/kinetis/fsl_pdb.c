/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_pdb.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for PDB module.
 *
 * @param base PDB peripheral base address
 */
static uint32_t PDB_GetInstance(PDB_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to PDB bases for each instance. */
static PDB_Type *const s_pdbBases[] = PDB_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to PDB clocks for each instance. */
static const clock_ip_name_t s_pdbClocks[] = PDB_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/
static uint32_t PDB_GetInstance(PDB_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_pdbBases); instance++)
    {
        if (s_pdbBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_pdbBases));

    return instance;
}

void PDB_Init(PDB_Type *base, const pdb_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_pdbClocks[PDB_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Configure. */
    /* PDBx_SC. */
    tmp32 = base->SC &
            ~(PDB_SC_LDMOD_MASK | PDB_SC_PRESCALER_MASK | PDB_SC_TRGSEL_MASK | PDB_SC_MULT_MASK | PDB_SC_CONT_MASK);

    tmp32 |= PDB_SC_LDMOD(config->loadValueMode) | PDB_SC_PRESCALER(config->prescalerDivider) |
             PDB_SC_TRGSEL(config->triggerInputSource) | PDB_SC_MULT(config->dividerMultiplicationFactor);
    if (config->enableContinuousMode)
    {
        tmp32 |= PDB_SC_CONT_MASK;
    }
    base->SC = tmp32;

    PDB_Enable(base, true); /* Enable the PDB module. */
}

void PDB_Deinit(PDB_Type *base)
{
    PDB_Enable(base, false); /* Disable the PDB module. */

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_pdbClocks[PDB_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void PDB_GetDefaultConfig(pdb_config_t *config)
{
    assert(NULL != config);

    config->loadValueMode = kPDB_LoadValueImmediately;
    config->prescalerDivider = kPDB_PrescalerDivider1;
    config->dividerMultiplicationFactor = kPDB_DividerMultiplicationFactor1;
    config->triggerInputSource = kPDB_TriggerSoftware;
    config->enableContinuousMode = false;
}

#if defined(FSL_FEATURE_PDB_HAS_DAC) && FSL_FEATURE_PDB_HAS_DAC
void PDB_SetDACTriggerConfig(PDB_Type *base, uint32_t channel, pdb_dac_trigger_config_t *config)
{
    assert(channel < PDB_INTC_COUNT);
    assert(NULL != config);

    uint32_t tmp32 = 0U;

    /* PDBx_DACINTC. */
    if (config->enableExternalTriggerInput)
    {
        tmp32 |= PDB_INTC_EXT_MASK;
    }
    if (config->enableIntervalTrigger)
    {
        tmp32 |= PDB_INTC_TOE_MASK;
    }
    base->DAC[channel].INTC = tmp32;
}
#endif /* FSL_FEATURE_PDB_HAS_DAC */
