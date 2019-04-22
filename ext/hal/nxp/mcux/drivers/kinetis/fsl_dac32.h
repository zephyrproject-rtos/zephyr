/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_DAC32_H_
#define _FSL_DAC32_H_

#include "fsl_common.h"

/*!
 * @addtogroup dac32
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief DAC32 driver version. */
#define FSL_DAC32_DRIVER_VERSION (MAKE_VERSION(2, 0, 1)) /*!< Version 2.0.1. */

/*!
 * @brief DAC32 buffer flags.
 */
enum _dac32_buffer_status_flags
{
    kDAC32_BufferWatermarkFlag = DAC_STATCTRL_DACBFWMF_MASK,
    /*!< DAC32 Buffer Watermark Flag. */ /* FSL_FEATURE_DAC32_HAS_WATERMARK_DETECTION */
    kDAC32_BufferReadPointerTopPositionFlag =
        DAC_STATCTRL_DACBFRPTF_MASK, /*!< DAC32 Buffer Read Pointer Top Position Flag. */
    kDAC32_BufferReadPointerBottomPositionFlag =
        DAC_STATCTRL_DACBFRPBF_MASK, /*!< DAC32 Buffer Read Pointer Bottom Position
                                  Flag. */
};

/*!
 * @brief DAC32 buffer interrupts.
 */
enum _dac32_buffer_interrupt_enable
{
    kDAC32_BufferWatermarkInterruptEnable = DAC_STATCTRL_DACBWIEN_MASK, /*!< DAC32 Buffer Watermark Interrupt Enable. */
    kDAC32_BufferReadPointerTopInterruptEnable =
        DAC_STATCTRL_DACBTIEN_MASK, /*!< DAC32 Buffer Read Pointer Top Flag Interrupt
                                 Enable. */
    kDAC32_BufferReadPointerBottomInterruptEnable =
        DAC_STATCTRL_DACBBIEN_MASK, /*!< DAC32 Buffer Read Pointer Bottom Flag
                                 Interrupt Enable */
};

/*!
 * @brief DAC32 reference voltage source.
 */
typedef enum _dac32_reference_voltage_source
{
    kDAC32_ReferenceVoltageSourceVref1 = 0U, /*!< The DAC32 selects DACREF_1 as the reference voltage. */
    kDAC32_ReferenceVoltageSourceVref2 = 1U, /*!< The DAC32 selects DACREF_2 as the reference voltage. */
} dac32_reference_voltage_source_t;

/*!
 * @brief DAC32 buffer trigger mode.
 */
typedef enum _dac32_buffer_trigger_mode
{
    kDAC32_BufferTriggerByHardwareMode = 0U, /*!< The DAC32 hardware trigger is selected. */
    kDAC32_BufferTriggerBySoftwareMode = 1U, /*!< The DAC32 software trigger is selected. */
} dac32_buffer_trigger_mode_t;

/*!
 * @brief DAC32 buffer watermark.
 */
typedef enum _dac32_buffer_watermark
{
    kDAC32_BufferWatermark1Word = 0U, /*!< 1 word  away from the upper limit. */
    kDAC32_BufferWatermark2Word = 1U, /*!< 2 words away from the upper limit. */
    kDAC32_BufferWatermark3Word = 2U, /*!< 3 words away from the upper limit. */
    kDAC32_BufferWatermark4Word = 3U, /*!< 4 words away from the upper limit. */
} dac32_buffer_watermark_t;

/*!
 * @brief DAC32 buffer work mode.
 */
typedef enum _dac32_buffer_work_mode
{
    kDAC32_BufferWorkAsNormalMode = 0U,      /*!< Normal mode. */
    kDAC32_BufferWorkAsSwingMode = 1U,       /*!< Swing mode. */
    kDAC32_BufferWorkAsOneTimeScanMode = 2U, /*!< One-Time Scan mode. */
    kDAC32_BufferWorkAsFIFOMode = 3U,        /*!< FIFO mode. */
} dac32_buffer_work_mode_t;

/*!
 * @brief DAC32 module configuration.
 */
typedef struct _dac32_config
{
    dac32_reference_voltage_source_t referenceVoltageSource; /*!< Select the DAC32 reference voltage source. */
    bool enableLowPowerMode;                                 /*!< Enable the low power mode. */
} dac32_config_t;

/*!
 * @brief DAC32 buffer configuration.
 */
typedef struct _dac32_buffer_config
{
    dac32_buffer_trigger_mode_t triggerMode; /*!< Select the buffer's trigger mode. */
    dac32_buffer_watermark_t watermark;      /*!< Select the buffer's watermark. */
    dac32_buffer_work_mode_t workMode;       /*!< Select the buffer's work mode. */
    uint32_t upperLimit;                     /*!< Set the upper limit for buffer index.
                                                 Normally, 0-15 is available for buffer with 16 item. */
} dac32_buffer_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitiailzation
 * @{
 */

/*!
 * @brief Initializes the DAC32 module.
 *
 * This function initializes the DAC32 module, including:
 *  - Enabling the clock for DAC32 module.
 *  - Configuring the DAC32 converter with a user configuration.
 *  - Enabling the DAC32 module.
 *
 * @param base DAC32 peripheral base address.
 * @param config Pointer to the configuration structure. See "dac32_config_t".
 */
void DAC32_Init(DAC_Type *base, const dac32_config_t *config);

/*!
 * @brief De-initializes the DAC32 module.
 *
 * This function de-initializes the DAC32 module, including:
 *  - Disabling the DAC32 module.
 *  - Disabling the clock for the DAC32 module.
 *
 * @param base DAC32 peripheral base address.
 */
void DAC32_Deinit(DAC_Type *base);

/*!
 * @brief Initializes the DAC32 user configuration structure.
 *
 * This function initializes the user configuration structure to a default value. The default values are:
 * @code
 *   config->referenceVoltageSource = kDAC32_ReferenceVoltageSourceVref2;
 *   config->enableLowPowerMode = false;
 * @endcode
 * @param config Pointer to the configuration structure. See "dac32_config_t".
 */
void DAC32_GetDefaultConfig(dac32_config_t *config);

/*!
 * @brief Enables the DAC32 module.
 *
 * @param base DAC32 peripheral base address.
 * @param enable Enables the feature or not.
 */
static inline void DAC32_Enable(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->STATCTRL |= DAC_STATCTRL_DACEN_MASK;
    }
    else
    {
        base->STATCTRL &= ~DAC_STATCTRL_DACEN_MASK;
    }
}

/* @} */

/*!
 * @name Buffer
 * @{
 */

/*!
 * @brief Enables the DAC32 buffer.
 *
 * @param base DAC32 peripheral base address.
 * @param enable Enables the feature or not.
 */
static inline void DAC32_EnableBuffer(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->STATCTRL |= DAC_STATCTRL_DACBFEN_MASK;
    }
    else
    {
        base->STATCTRL &= ~DAC_STATCTRL_DACBFEN_MASK;
    }
}

/*!
 * @brief Configures the DAC32 buffer.
 *
 * @param base   DAC32 peripheral base address.
 * @param config Pointer to the configuration structure. See "dac32_buffer_config_t".
 */
void DAC32_SetBufferConfig(DAC_Type *base, const dac32_buffer_config_t *config);

/*!
 * @brief Initializes the DAC32 buffer configuration structure.
 *
 * This function initializes the DAC32 buffer configuration structure to a default value. The default values are:
 * @code
 *   config->triggerMode = kDAC32_BufferTriggerBySoftwareMode;
 *   config->watermark   = kDAC32_BufferWatermark1Word;
 *   config->workMode    = kDAC32_BufferWorkAsNormalMode;
 *   config->upperLimit  = DAC_DAT_COUNT * 2U - 1U; // Full buffer is used.
 * @endcode
 * @param config Pointer to the configuration structure. See "dac32_buffer_config_t".
 */
void DAC32_GetDefaultBufferConfig(dac32_buffer_config_t *config);

/*!
 * @brief Enables the DMA for DAC32 buffer.
 *
 * @param base DAC32 peripheral base address.
 * @param enable Enables the feature or not.
 */
static inline void DAC32_EnableBufferDMA(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->STATCTRL |= DAC_STATCTRL_DMAEN_MASK;
    }
    else
    {
        base->STATCTRL &= ~DAC_STATCTRL_DMAEN_MASK;
    }
}

/*!
 * @brief Sets the value for  items in the buffer.
 *
 * @param base  DAC32 peripheral base address.
 * @param index Setting index for items in the buffer. The available index should not exceed the size of the DAC32
 * buffer.
 * @param value Setting value for items in the buffer. 12-bits are available.
 */
void DAC32_SetBufferValue(DAC_Type *base, uint32_t index, uint32_t value);

/*!
 * @brief Triggers the buffer by software and updates the read pointer of the DAC32 buffer.
 *
 * This function triggers the function by software. The read pointer of the DAC32 buffer is updated with one step
 * after this function is called. Changing the read pointer depends on the buffer's work mode.
 *
 * @param base DAC32 peripheral base address.
 */
static inline void DAC32_DoSoftwareTriggerBuffer(DAC_Type *base)
{
    base->STATCTRL |= DAC_STATCTRL_DACSWTRG_MASK;
}

/*!
 * @brief Gets the current read pointer of the DAC32 buffer.
 *
 * This function gets the current read pointer of the DAC32 buffer.
 * The current output value depends on the item indexed by the read pointer. It is updated
 * by software trigger or hardware trigger.
 *
 * @param  base DAC32 peripheral base address.
 *
 * @return      Current read pointer of DAC32 buffer.
 */
static inline uint32_t DAC32_GetBufferReadPointer(DAC_Type *base)
{
    return ((base->STATCTRL & DAC_STATCTRL_DACBFRP_MASK) >> DAC_STATCTRL_DACBFRP_SHIFT);
}

/*!
 * @brief Sets the current read pointer of the DAC32 buffer.
 *
 * This function sets the current read pointer of the DAC32 buffer.
 * The current output value depends on the item indexed by the read pointer. It is updated by
 * software trigger or hardware trigger. After the read pointer changes, the DAC32 output value also changes.
 *
 * @param base  DAC32 peripheral base address.
 * @param index Setting index value for the pointer.
 */
static inline void DAC32_SetBufferReadPointer(DAC_Type *base, uint32_t index)
{
    assert(index < (DAC_DAT_COUNT * 2U));

    base->STATCTRL = (base->STATCTRL & ~DAC_STATCTRL_DACBFRP_MASK) | DAC_STATCTRL_DACBFRP(index);
}

/*!
 * @brief Enables interrupts for the DAC32 buffer.
 *
 * @param base DAC32 peripheral base address.
 * @param mask Mask value for interrupts. See "_dac32_buffer_interrupt_enable".
 */
static inline void DAC32_EnableBufferInterrupts(DAC_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~(DAC_STATCTRL_DACBWIEN_MASK | DAC_STATCTRL_DACBTIEN_MASK | DAC_STATCTRL_DACBBIEN_MASK)));

    base->STATCTRL |= mask;
}

/*!
 * @brief Disables interrupts for the DAC32 buffer.
 *
 * @param base DAC32 peripheral base address.
 * @param mask Mask value for interrupts. See  "_dac32_buffer_interrupt_enable".
 */
static inline void DAC32_DisableBufferInterrupts(DAC_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~(DAC_STATCTRL_DACBWIEN_MASK | DAC_STATCTRL_DACBTIEN_MASK | DAC_STATCTRL_DACBBIEN_MASK)));

    base->STATCTRL &= ~mask;
}

/*!
 * @brief Gets the flags of events for the DAC32 buffer.
 *
 * @param  base DAC32 peripheral base address.
 *
 * @return      Mask value for the asserted flags. See  "_dac32_buffer_status_flags".
 */
static inline uint32_t DAC32_GetBufferStatusFlags(DAC_Type *base)
{
    return (base->STATCTRL & (DAC_STATCTRL_DACBFWMF_MASK | DAC_STATCTRL_DACBFRPTF_MASK | DAC_STATCTRL_DACBFRPBF_MASK));
}

/*!
 * @brief Clears the flags of events for the DAC32 buffer.
 *
 * @param base DAC32 peripheral base address.
 * @param mask Mask value for flags. See "_dac32_buffer_status_flags_t".
 */
static inline void DAC32_ClearBufferStatusFlags(DAC_Type *base, uint32_t mask)
{
    base->STATCTRL &= ~mask;
}

/*!
 * @brief Enable the buffer output.
 *
 * @param base DAC32 peripheral base address.
 * @param enable Enable the buffer output or not.
 */
static inline void DAC32_EnableBufferOutput(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->STATCTRL |= DAC_STATCTRL_BFOUTEN_MASK;
    }
    else
    {
        base->STATCTRL &= ~DAC_STATCTRL_BFOUTEN_MASK;
    }
}

/*!
 * @brief Enable the test output.
 *
 * @param base DAC32 peripheral base address.
 * @param enable Enable the test output or not.
 */
static inline void DAC32_EnableTestOutput(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->STATCTRL |= DAC_STATCTRL_TESTOUTEN_MASK;
    }
    else
    {
        base->STATCTRL &= ~DAC_STATCTRL_TESTOUTEN_MASK;
    }
}

/* @} */

#if defined(__cplusplus)
}
#endif
/*!
 * @}
 */
#endif /* _FSL_DAC32_H_ */
