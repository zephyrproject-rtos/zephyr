/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_pdb.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.pdb"
#endif

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

/*!
 * brief Initializes the PDB module.
 *
 * This function initializes the PDB module. The operations included are as follows.
 *  - Enable the clock for PDB instance.
 *  - Configure the PDB module.
 *  - Enable the PDB module.
 *
 * param base PDB peripheral base address.
 * param config Pointer to the configuration structure. See "pdb_config_t".
 */
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

/*!
 * brief De-initializes the PDB module.
 *
 * param base PDB peripheral base address.
 */
void PDB_Deinit(PDB_Type *base)
{
    PDB_Enable(base, false); /* Disable the PDB module. */

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_pdbClocks[PDB_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Initializes the PDB user configuration structure.
 *
 * This function initializes the user configuration structure to a default value. The default values are as follows.
 * code
 *   config->loadValueMode = kPDB_LoadValueImmediately;
 *   config->prescalerDivider = kPDB_PrescalerDivider1;
 *   config->dividerMultiplicationFactor = kPDB_DividerMultiplicationFactor1;
 *   config->triggerInputSource = kPDB_TriggerSoftware;
 *   config->enableContinuousMode = false;
 * endcode
 * param config Pointer to configuration structure. See "pdb_config_t".
 */
void PDB_GetDefaultConfig(pdb_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->loadValueMode = kPDB_LoadValueImmediately;
    config->prescalerDivider = kPDB_PrescalerDivider1;
    config->dividerMultiplicationFactor = kPDB_DividerMultiplicationFactor1;
    config->triggerInputSource = kPDB_TriggerSoftware;
    config->enableContinuousMode = false;
}

#if defined(FSL_FEATURE_PDB_HAS_DAC) && FSL_FEATURE_PDB_HAS_DAC
/*!
 * brief Configures the DAC trigger in the PDB module.
 *
 * param base    PDB peripheral base address.
 * param channel Channel index for DAC instance.
 * param config  Pointer to the configuration structure. See "pdb_dac_trigger_config_t".
 */
void PDB_SetDACTriggerConfig(PDB_Type *base, pdb_dac_trigger_channel_t channel, pdb_dac_trigger_config_t *config)
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
