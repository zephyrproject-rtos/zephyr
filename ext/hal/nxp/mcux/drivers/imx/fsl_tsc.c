/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_tsc.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.tsc"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for TSC module.
 *
 * @param base TSC peripheral base address
 */
static uint32_t TSC_GetInstance(TSC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to TSC bases for each instance. */
static TSC_Type *const s_tscBases[] = TSC_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to ADC clocks for each instance. */
static const clock_ip_name_t s_tscClocks[] = TSC_CLOCKS;
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t TSC_GetInstance(TSC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_tscBases); instance++)
    {
        if (s_tscBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_tscBases));

    return instance;
}

void TSC_Init(TSC_Type *base, const tsc_config_t *config)
{
    assert(NULL != config);
    assert(config->measureDelayTime <= 0xFFFFFFU);

    uint32_t tmp32;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the TSC clock. */
    CLOCK_EnableClock(s_tscClocks[TSC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    /* Configure TSC_BASIC_SETTING register. */
    tmp32 = TSC_BASIC_SETTING_MEASURE_DELAY_TIME(config->measureDelayTime) |
            TSC_BASIC_SETTING__4_5_WIRE(config->detectionMode);
    if (config->enableAutoMeasure)
    {
        tmp32 |= TSC_BASIC_SETTING_AUTO_MEASURE_MASK;
    }
    base->BASIC_SETTING = tmp32;
    /* Configure TSC_PS_INPUT_BUFFER_ADDR register. */
    base->PRE_CHARGE_TIME = TSC_PRE_CHARGE_TIME_PRE_CHARGE_TIME(config->prechargeTime);
}

void TSC_Deinit(TSC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the TSC clcok. */
    CLOCK_DisableClock(s_tscClocks[TSC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void TSC_GetDefaultConfig(tsc_config_t *config)
{
    config->enableAutoMeasure = false;
    config->measureDelayTime = 0xFFFFU;
    config->prechargeTime = 0xFFFFU;
    config->detectionMode = kTSC_Detection4WireMode;
}

uint32_t TSC_GetMeasureValue(TSC_Type *base, tsc_corrdinate_value_selection_t selection)
{
    uint32_t tmp32 = 0;

    if (selection == kTSC_XCoordinateValueSelection)
    {
        tmp32 = ((base->MEASEURE_VALUE) & TSC_MEASEURE_VALUE_X_VALUE_MASK) >> TSC_MEASEURE_VALUE_X_VALUE_SHIFT;
    }
    else if (selection == kTSC_YCoordinateValueSelection)
    {
        tmp32 = ((base->MEASEURE_VALUE) & TSC_MEASEURE_VALUE_Y_VALUE_MASK) >> TSC_MEASEURE_VALUE_Y_VALUE_SHIFT;
    }
    else
    {
    }
    return tmp32;
}

void TSC_DebugTriggerSignalToADC(TSC_Type *base, tsc_trigger_signal_t hwts, bool enable)
{
    if (enable)
    {
        /* TSC_DEBUG_MODE_EXT_HWTS field should be writed before writing TSC_DEBUG_MODE_TRIGGER field.
           If the two fields are writed at the same time, the trigger couldn't work as expect. */
        base->DEBUG_MODE &= ~TSC_DEBUG_MODE_EXT_HWTS_MASK;
        base->DEBUG_MODE |= TSC_DEBUG_MODE_EXT_HWTS(hwts);
        base->DEBUG_MODE |= TSC_DEBUG_MODE_TRIGGER_MASK;
    }
    else
    {
        base->DEBUG_MODE &= ~TSC_DEBUG_MODE_TRIGGER_MASK;
    }
}

void TSC_DebugEnableDetection(TSC_Type *base, tsc_detection_mode_t detectionMode, bool enable)
{
    if (detectionMode == kTSC_Detection4WireMode)
    {
        if (enable)
        {
            base->DEBUG_MODE2 |= TSC_DEBUG_MODE2_DETECT_ENABLE_FOUR_WIRE_MASK;
        }
        else
        {
            base->DEBUG_MODE2 &= ~TSC_DEBUG_MODE2_DETECT_ENABLE_FOUR_WIRE_MASK;
        }
    }
    else if (detectionMode == kTSC_Detection5WireMode)
    {
        if (enable)
        {
            base->DEBUG_MODE2 |= TSC_DEBUG_MODE2_DETECT_ENABLE_FIVE_WIRE_MASK;
        }
        else
        {
            base->DEBUG_MODE2 &= ~TSC_DEBUG_MODE2_DETECT_ENABLE_FIVE_WIRE_MASK;
        }
    }
    else
    {
    }
}

void TSC_DebugSetPortMode(TSC_Type *base, tsc_port_source_t port, tsc_port_mode_t mode)
{
    uint32_t tmp32;

    tmp32 = base->DEBUG_MODE2;
    switch (port)
    {
        case kTSC_WiperPortSource:
            tmp32 &= ~(TSC_DEBUG_MODE2_WIPER_200K_PULL_UP_MASK | TSC_DEBUG_MODE2_WIPER_PULL_UP_MASK |
                       TSC_DEBUG_MODE2_WIPER_PULL_DOWN_MASK);
            tmp32 |= ((uint32_t)mode << TSC_DEBUG_MODE2_WIPER_PULL_DOWN_SHIFT);
            break;
        case kTSC_YnlrPortSource:
            tmp32 &= ~(TSC_DEBUG_MODE2_YNLR_200K_PULL_UP_MASK | TSC_DEBUG_MODE2_YNLR_PULL_UP_MASK |
                       TSC_DEBUG_MODE2_YNLR_PULL_DOWN_MASK);
            tmp32 |= ((uint32_t)mode << TSC_DEBUG_MODE2_YNLR_PULL_DOWN_SHIFT);
            break;
        case kTSC_YpllPortSource:
            tmp32 &= ~(TSC_DEBUG_MODE2_YPLL_200K_PULL_UP_MASK | TSC_DEBUG_MODE2_YPLL_PULL_UP_MASK |
                       TSC_DEBUG_MODE2_YPLL_PULL_DOWN_MASK);
            tmp32 |= ((uint32_t)mode << TSC_DEBUG_MODE2_YPLL_PULL_DOWN_SHIFT);
            break;
        case kTSC_XnurPortSource:
            tmp32 &= ~(TSC_DEBUG_MODE2_XNUR_200K_PULL_UP_MASK | TSC_DEBUG_MODE2_XNUR_PULL_UP_MASK |
                       TSC_DEBUG_MODE2_XNUR_PULL_DOWN_MASK);
            tmp32 |= ((uint32_t)mode << TSC_DEBUG_MODE2_XNUR_PULL_DOWN_SHIFT);
            break;
        case kTSC_XpulPortSource:
            tmp32 &= ~(TSC_DEBUG_MODE2_XPUL_200K_PULL_UP_MASK | TSC_DEBUG_MODE2_XPUL_PULL_UP_MASK |
                       TSC_DEBUG_MODE2_XPUL_PULL_DOWN_MASK);
            tmp32 |= ((uint32_t)mode << TSC_DEBUG_MODE2_XPUL_PULL_DOWN_SHIFT);
            break;
        default:
            break;
    }
    base->DEBUG_MODE2 = tmp32;
}
