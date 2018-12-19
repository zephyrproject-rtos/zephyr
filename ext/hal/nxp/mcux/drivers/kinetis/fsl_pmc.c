/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "fsl_pmc.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.pmc"
#endif

#if (defined(FSL_FEATURE_PMC_HAS_PARAM) && FSL_FEATURE_PMC_HAS_PARAM)
/*!
 * brief Gets the PMC parameter.
 *
 * This function gets the PMC parameter including the VLPO enable and the HVD enable.
 *
 * param base PMC peripheral base address.
 * param param         Pointer to PMC param structure.
 */
void PMC_GetParam(PMC_Type *base, pmc_param_t *param)
{
    uint32_t reg = base->PARAM;
    ;
    param->vlpoEnable = (bool)(reg & PMC_PARAM_VLPOE_MASK);
    param->hvdEnable = (bool)(reg & PMC_PARAM_HVDE_MASK);
}
#endif /* FSL_FEATURE_PMC_HAS_PARAM */

/*!
 * brief Configures the low-voltage detect setting.
 *
 * This function configures the low-voltage detect setting, including the trip
 * point voltage setting, enables or disables the interrupt, enables or disables the system reset.
 *
 * param base PMC peripheral base address.
 * param config  Low-voltage detect configuration structure.
 */
void PMC_ConfigureLowVoltDetect(PMC_Type *base, const pmc_low_volt_detect_config_t *config)
{
    base->LVDSC1 = (0U |
#if (defined(FSL_FEATURE_PMC_HAS_LVDV) && FSL_FEATURE_PMC_HAS_LVDV)
                    ((uint32_t)config->voltSelect << PMC_LVDSC1_LVDV_SHIFT) |
#endif
                    ((uint32_t)config->enableInt << PMC_LVDSC1_LVDIE_SHIFT) |
                    ((uint32_t)config->enableReset << PMC_LVDSC1_LVDRE_SHIFT)
                    /* Clear the Low Voltage Detect Flag with previouse power detect setting */
                    | PMC_LVDSC1_LVDACK_MASK);
}

/*!
 * brief Configures the low-voltage warning setting.
 *
 * This function configures the low-voltage warning setting, including the trip
 * point voltage setting and enabling or disabling the interrupt.
 *
 * param base PMC peripheral base address.
 * param config  Low-voltage warning configuration structure.
 */
void PMC_ConfigureLowVoltWarning(PMC_Type *base, const pmc_low_volt_warning_config_t *config)
{
    base->LVDSC2 = (0U |
#if (defined(FSL_FEATURE_PMC_HAS_LVWV) && FSL_FEATURE_PMC_HAS_LVWV)
                    ((uint32_t)config->voltSelect << PMC_LVDSC2_LVWV_SHIFT) |
#endif
                    ((uint32_t)config->enableInt << PMC_LVDSC2_LVWIE_SHIFT)
                    /* Clear the Low Voltage Warning Flag with previouse power detect setting */
                    | PMC_LVDSC2_LVWACK_MASK);
}

#if (defined(FSL_FEATURE_PMC_HAS_HVDSC1) && FSL_FEATURE_PMC_HAS_HVDSC1)
/*!
 * brief Configures the high-voltage detect setting.
 *
 * This function configures the high-voltage detect setting, including the trip
 * point voltage setting, enabling or disabling the interrupt, enabling or disabling the system reset.
 *
 * param base PMC peripheral base address.
 * param config  High-voltage detect configuration structure.
 */
void PMC_ConfigureHighVoltDetect(PMC_Type *base, const pmc_high_volt_detect_config_t *config)
{
    base->HVDSC1 = (((uint32_t)config->voltSelect << PMC_HVDSC1_HVDV_SHIFT) |
                    ((uint32_t)config->enableInt << PMC_HVDSC1_HVDIE_SHIFT) |
                    ((uint32_t)config->enableReset << PMC_HVDSC1_HVDRE_SHIFT)
                    /* Clear the High Voltage Detect Flag with previouse power detect setting */
                    | PMC_HVDSC1_HVDACK_MASK);
}
#endif /* FSL_FEATURE_PMC_HAS_HVDSC1 */

#if ((defined(FSL_FEATURE_PMC_HAS_BGBE) && FSL_FEATURE_PMC_HAS_BGBE) || \
     (defined(FSL_FEATURE_PMC_HAS_BGEN) && FSL_FEATURE_PMC_HAS_BGEN) || \
     (defined(FSL_FEATURE_PMC_HAS_BGBDS) && FSL_FEATURE_PMC_HAS_BGBDS))
/*!
 * brief Configures the PMC bandgap.
 *
 * This function configures the PMC bandgap, including the drive select and
 * behavior in low-power mode.
 *
 * param base PMC peripheral base address.
 * param config Pointer to the configuration structure
 */
void PMC_ConfigureBandgapBuffer(PMC_Type *base, const pmc_bandgap_buffer_config_t *config)
{
    base->REGSC = (0U
#if (defined(FSL_FEATURE_PMC_HAS_BGBE) && FSL_FEATURE_PMC_HAS_BGBE)
                   | ((uint32_t)config->enable << PMC_REGSC_BGBE_SHIFT)
#endif /* FSL_FEATURE_PMC_HAS_BGBE */
#if (defined(FSL_FEATURE_PMC_HAS_BGEN) && FSL_FEATURE_PMC_HAS_BGEN)
                   | (((uint32_t)config->enableInLowPowerMode << PMC_REGSC_BGEN_SHIFT))
#endif /* FSL_FEATURE_PMC_HAS_BGEN */
#if (defined(FSL_FEATURE_PMC_HAS_BGBDS) && FSL_FEATURE_PMC_HAS_BGBDS)
                   | ((uint32_t)config->drive << PMC_REGSC_BGBDS_SHIFT)
#endif /* FSL_FEATURE_PMC_HAS_BGBDS */
                       );
}
#endif
