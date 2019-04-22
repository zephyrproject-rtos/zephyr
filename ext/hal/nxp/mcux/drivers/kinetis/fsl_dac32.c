/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_dac32.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.dac32"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for DAC32 module.
 *
 * @param base DAC32 peripheral base address
 */
static uint32_t DAC32_GetInstance(DAC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to DAC32 bases for each instance. */
static DAC_Type *const s_dac32Bases[] = DAC_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to DAC32 clocks for each instance. */
static const clock_ip_name_t s_dac32Clocks[] = DAC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/
static uint32_t DAC32_GetInstance(DAC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_dac32Bases); instance++)
    {
        if (s_dac32Bases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_dac32Bases));

    return instance;
}

/*!
 * brief Initializes the DAC32 module.
 *
 * This function initializes the DAC32 module, including:
 *  - Enabling the clock for DAC32 module.
 *  - Configuring the DAC32 converter with a user configuration.
 *  - Enabling the DAC32 module.
 *
 * param base DAC32 peripheral base address.
 * param config Pointer to the configuration structure. See "dac32_config_t".
 */
void DAC32_Init(DAC_Type *base, const dac32_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock before any operation to DAC32 registers.*/
    CLOCK_EnableClock(s_dac32Clocks[DAC32_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Configure. */
    tmp32 = base->STATCTRL & ~(DAC_STATCTRL_DACRFS_MASK | DAC_STATCTRL_LPEN_MASK);
    if (kDAC32_ReferenceVoltageSourceVref2 == config->referenceVoltageSource)
    {
        tmp32 |= DAC_STATCTRL_DACRFS_MASK;
    }
    if (config->enableLowPowerMode)
    {
        tmp32 |= DAC_STATCTRL_LPEN_MASK;
    }
    base->STATCTRL = tmp32;

    /* DAC32_Enable(base, true); */
    /* Move this function to application, so that users can enable it when they will. */
}

/*!
 * brief De-initializes the DAC32 module.
 *
 * This function de-initializes the DAC32 module, including:
 *  - Disabling the DAC32 module.
 *  - Disabling the clock for the DAC32 module.
 *
 * param base DAC32 peripheral base address.
 */
void DAC32_Deinit(DAC_Type *base)
{
    DAC32_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_dac32Clocks[DAC32_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Initializes the DAC32 user configuration structure.
 *
 * This function initializes the user configuration structure to a default value. The default values are:
 * code
 *   config->referenceVoltageSource = kDAC32_ReferenceVoltageSourceVref2;
 *   config->enableLowPowerMode = false;
 * endcode
 * param config Pointer to the configuration structure. See "dac32_config_t".
 */
void DAC32_GetDefaultConfig(dac32_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->referenceVoltageSource = kDAC32_ReferenceVoltageSourceVref2;
    config->enableLowPowerMode = false;
}

/*!
 * brief Configures the DAC32 buffer.
 *
 * param base   DAC32 peripheral base address.
 * param config Pointer to the configuration structure. See "dac32_buffer_config_t".
 */
void DAC32_SetBufferConfig(DAC_Type *base, const dac32_buffer_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

    tmp32 = base->STATCTRL &
            ~(DAC_STATCTRL_DACTRGSEL_MASK | DAC_STATCTRL_DACBFMD_MASK | DAC_STATCTRL_DACBFUP_MASK |
              DAC_STATCTRL_DACBFWM_MASK);
    if (kDAC32_BufferTriggerBySoftwareMode == config->triggerMode)
    {
        tmp32 |= DAC_STATCTRL_DACTRGSEL_MASK;
    }
    tmp32 |= (DAC_STATCTRL_DACBFWM(config->watermark) | DAC_STATCTRL_DACBFMD(config->workMode) |
              DAC_STATCTRL_DACBFUP(config->upperLimit));
    base->STATCTRL = tmp32;
}

/*!
 * brief Initializes the DAC32 buffer configuration structure.
 *
 * This function initializes the DAC32 buffer configuration structure to a default value. The default values are:
 * code
 *   config->triggerMode = kDAC32_BufferTriggerBySoftwareMode;
 *   config->watermark   = kDAC32_BufferWatermark1Word;
 *   config->workMode    = kDAC32_BufferWorkAsNormalMode;
 *   config->upperLimit  = DAC_DAT_COUNT * 2U - 1U; // Full buffer is used.
 * endcode
 * param config Pointer to the configuration structure. See "dac32_buffer_config_t".
 */
void DAC32_GetDefaultBufferConfig(dac32_buffer_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->triggerMode = kDAC32_BufferTriggerBySoftwareMode;
    config->watermark = kDAC32_BufferWatermark1Word;
    config->workMode = kDAC32_BufferWorkAsNormalMode;
    config->upperLimit = DAC_DAT_COUNT * 2U - 1U;
}

/*!
 * brief Sets the value for  items in the buffer.
 *
 * param base  DAC32 peripheral base address.
 * param index Setting index for items in the buffer. The available index should not exceed the size of the DAC32
 * buffer.
 * param value Setting value for items in the buffer. 12-bits are available.
 */
void DAC32_SetBufferValue(DAC_Type *base, uint32_t index, uint32_t value)
{
    assert(index < (DAC_DAT_COUNT * 2U));

    if (0U == (index % 2U))
    {
        index = index / 2U; /* Register index. */
        base->DAT[index] = (base->DAT[index] & ~(DAC_DAT_DATA0_MASK)) | DAC_DAT_DATA0(value);
    }
    else
    {
        index = index / 2U; /* Register index. */
        base->DAT[index] = (base->DAT[index] & ~(DAC_DAT_DATA1_MASK)) | DAC_DAT_DATA1(value);
    }
}
