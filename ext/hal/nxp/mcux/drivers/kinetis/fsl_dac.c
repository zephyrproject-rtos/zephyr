/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_dac.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.dac"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for DAC module.
 *
 * @param base DAC peripheral base address
 */
static uint32_t DAC_GetInstance(DAC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to DAC bases for each instance. */
static DAC_Type *const s_dacBases[] = DAC_BASE_PTRS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to DAC clocks for each instance. */
static const clock_ip_name_t s_dacClocks[] = DAC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Codes
 ******************************************************************************/
static uint32_t DAC_GetInstance(DAC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_dacBases); instance++)
    {
        if (s_dacBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_dacBases));

    return instance;
}

/*!
 * brief Initializes the DAC module.
 *
 * This function initializes the DAC module including the following operations.
 *  - Enabling the clock for DAC module.
 *  - Configuring the DAC converter with a user configuration.
 *  - Enabling the DAC module.
 *
 * param base DAC peripheral base address.
 * param config Pointer to the configuration structure. See "dac_config_t".
 */
void DAC_Init(DAC_Type *base, const dac_config_t *config)
{
    assert(NULL != config);

    uint8_t tmp8;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_dacClocks[DAC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Configure. */
    /* DACx_C0. */
    tmp8 = base->C0 & ~(DAC_C0_DACRFS_MASK | DAC_C0_LPEN_MASK);
    if (kDAC_ReferenceVoltageSourceVref2 == config->referenceVoltageSource)
    {
        tmp8 |= DAC_C0_DACRFS_MASK;
    }
    if (config->enableLowPowerMode)
    {
        tmp8 |= DAC_C0_LPEN_MASK;
    }
    base->C0 = tmp8;

    /* DAC_Enable(base, true); */
    /* Tip: The DAC output can be enabled till then after user sets their own available data in application. */
}

/*!
 * brief De-initializes the DAC module.
 *
 * This function de-initializes the DAC module including the following operations.
 *  - Disabling the DAC module.
 *  - Disabling the clock for the DAC module.
 *
 * param base DAC peripheral base address.
 */
void DAC_Deinit(DAC_Type *base)
{
    DAC_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_dacClocks[DAC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * brief Initializes the DAC user configuration structure.
 *
 * This function initializes the user configuration structure to a default value. The default values are as follows.
 * code
 *   config->referenceVoltageSource = kDAC_ReferenceVoltageSourceVref2;
 *   config->enableLowPowerMode = false;
 * endcode
 * param config Pointer to the configuration structure. See "dac_config_t".
 */
void DAC_GetDefaultConfig(dac_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->referenceVoltageSource = kDAC_ReferenceVoltageSourceVref2;
    config->enableLowPowerMode = false;
}

/*!
 * brief Configures the CMP buffer.
 *
 * param base   DAC peripheral base address.
 * param config Pointer to the configuration structure. See "dac_buffer_config_t".
 */
void DAC_SetBufferConfig(DAC_Type *base, const dac_buffer_config_t *config)
{
    assert(NULL != config);

    uint8_t tmp8;

    /* DACx_C0. */
    tmp8 = base->C0 & ~(DAC_C0_DACTRGSEL_MASK);
    if (kDAC_BufferTriggerBySoftwareMode == config->triggerMode)
    {
        tmp8 |= DAC_C0_DACTRGSEL_MASK;
    }
    base->C0 = tmp8;

    /* DACx_C1. */
    tmp8 = base->C1 &
           ~(
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
               DAC_C1_DACBFWM_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */
               DAC_C1_DACBFMD_MASK);
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
    tmp8 |= DAC_C1_DACBFWM(config->watermark);
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */
    tmp8 |= DAC_C1_DACBFMD(config->workMode);
    base->C1 = tmp8;

    /* DACx_C2. */
    tmp8 = base->C2 & ~DAC_C2_DACBFUP_MASK;
    tmp8 |= DAC_C2_DACBFUP(config->upperLimit);
    base->C2 = tmp8;
}

/*!
 * brief Initializes the DAC buffer configuration structure.
 *
 * This function initializes the DAC buffer configuration structure to default values. The default values are as
 * follows.
 * code
 *   config->triggerMode = kDAC_BufferTriggerBySoftwareMode;
 *   config->watermark   = kDAC_BufferWatermark1Word;
 *   config->workMode    = kDAC_BufferWorkAsNormalMode;
 *   config->upperLimit  = DAC_DATL_COUNT - 1U;
 * endcode
 * param config Pointer to the configuration structure. See "dac_buffer_config_t".
 */
void DAC_GetDefaultBufferConfig(dac_buffer_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->triggerMode = kDAC_BufferTriggerBySoftwareMode;
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
    config->watermark = kDAC_BufferWatermark1Word;
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */
    config->workMode = kDAC_BufferWorkAsNormalMode;
    config->upperLimit = DAC_DATL_COUNT - 1U;
}

/*!
 * brief Sets the value for  items in the buffer.
 *
 * param base  DAC peripheral base address.
 * param index Setting the index for items in the buffer. The available index should not exceed the size of the DAC
 * buffer.
 * param value Setting the value for items in the buffer. 12-bits are available.
 */
void DAC_SetBufferValue(DAC_Type *base, uint8_t index, uint16_t value)
{
    assert(index < DAC_DATL_COUNT);

    base->DAT[index].DATL = (uint8_t)(0xFFU & value);         /* Low 8-bit. */
    base->DAT[index].DATH = (uint8_t)((0xF00U & value) >> 8); /* High 4-bit. */
}

/*!
 * brief Sets the current read pointer of the DAC buffer.
 *
 * This function sets the current read pointer of the DAC buffer.
 * The current output value depends on the item indexed by the read pointer. It is updated either by a
 * software trigger or a hardware trigger. After the read pointer changes, the DAC output value also changes.
 *
 * param base  DAC peripheral base address.
 * param index Setting an index value for the pointer.
 */
void DAC_SetBufferReadPointer(DAC_Type *base, uint8_t index)
{
    assert(index < DAC_DATL_COUNT);

    uint8_t tmp8 = base->C2 & ~DAC_C2_DACBFRP_MASK;

    tmp8 |= DAC_C2_DACBFRP(index);
    base->C2 = tmp8;
}

/*!
 * brief Enables interrupts for the DAC buffer.
 *
 * param base DAC peripheral base address.
 * param mask Mask value for interrupts. See "_dac_buffer_interrupt_enable".
 */
void DAC_EnableBufferInterrupts(DAC_Type *base, uint32_t mask)
{
    mask &= (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
        DAC_C0_DACBWIEN_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
        DAC_C0_DACBTIEN_MASK | DAC_C0_DACBBIEN_MASK);
    base->C0 |= ((uint8_t)mask); /* Write 1 to enable. */
}

/*!
 * brief Disables interrupts for the DAC buffer.
 *
 * param base DAC peripheral base address.
 * param mask Mask value for interrupts. See  "_dac_buffer_interrupt_enable".
 */
void DAC_DisableBufferInterrupts(DAC_Type *base, uint32_t mask)
{
    mask &= (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
        DAC_C0_DACBWIEN_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
        DAC_C0_DACBTIEN_MASK | DAC_C0_DACBBIEN_MASK);
    base->C0 &= (uint8_t)(~((uint8_t)mask)); /* Write 0 to disable. */
}

/*!
 * brief Gets the flags of events for the DAC buffer.
 *
 * param  base DAC peripheral base address.
 *
 * return      Mask value for the asserted flags. See  "_dac_buffer_status_flags".
 */
uint32_t DAC_GetBufferStatusFlags(DAC_Type *base)
{
    return (uint32_t)(base->SR & (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
                                     DAC_SR_DACBFWMF_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
                                     DAC_SR_DACBFRPTF_MASK | DAC_SR_DACBFRPBF_MASK));
}

/*!
 * brief Clears the flags of events for the DAC buffer.
 *
 * param base DAC peripheral base address.
 * param mask Mask value for flags. See "_dac_buffer_status_flags_t".
 */
void DAC_ClearBufferStatusFlags(DAC_Type *base, uint32_t mask)
{
    mask &= (
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
        DAC_SR_DACBFWMF_MASK |
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
        DAC_SR_DACBFRPTF_MASK | DAC_SR_DACBFRPBF_MASK);
    base->SR &= (uint8_t)(~((uint8_t)mask)); /* Write 0 to clear flags. */
}
