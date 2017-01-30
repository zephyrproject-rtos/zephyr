/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
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

#ifndef _FSL_DAC_H_
#define _FSL_DAC_H_

#include "fsl_common.h"

/*!
 * @addtogroup dac
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief DAC driver version 2.0.0. */
#define FSL_DAC_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
 * @brief DAC buffer flags.
 */
enum _dac_buffer_status_flags
{
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
    kDAC_BufferWatermarkFlag = DAC_SR_DACBFWMF_MASK,                  /*!< DAC Buffer Watermark Flag. */
#endif                                                                /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
    kDAC_BufferReadPointerTopPositionFlag = DAC_SR_DACBFRPTF_MASK,    /*!< DAC Buffer Read Pointer Top Position Flag. */
    kDAC_BufferReadPointerBottomPositionFlag = DAC_SR_DACBFRPBF_MASK, /*!< DAC Buffer Read Pointer Bottom Position
                                                                           Flag. */
};

/*!
 * @brief DAC buffer interrupts.
 */
enum _dac_buffer_interrupt_enable
{
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION
    kDAC_BufferWatermarkInterruptEnable = DAC_C0_DACBWIEN_MASK,         /*!< DAC Buffer Watermark Interrupt Enable. */
#endif                                                                  /* FSL_FEATURE_DAC_HAS_WATERMARK_DETECTION */
    kDAC_BufferReadPointerTopInterruptEnable = DAC_C0_DACBTIEN_MASK,    /*!< DAC Buffer Read Pointer Top Flag Interrupt
                                                                             Enable. */
    kDAC_BufferReadPointerBottomInterruptEnable = DAC_C0_DACBBIEN_MASK, /*!< DAC Buffer Read Pointer Bottom Flag
                                                                             Interrupt Enable */
};

/*!
 * @brief DAC reference voltage source.
 */
typedef enum _dac_reference_voltage_source
{
    kDAC_ReferenceVoltageSourceVref1 = 0U, /*!< The DAC selects DACREF_1 as the reference voltage. */
    kDAC_ReferenceVoltageSourceVref2 = 1U, /*!< The DAC selects DACREF_2 as the reference voltage. */
} dac_reference_voltage_source_t;

/*!
 * @brief DAC buffer trigger mode.
 */
typedef enum _dac_buffer_trigger_mode
{
    kDAC_BufferTriggerByHardwareMode = 0U, /*!< The DAC hardware trigger is selected. */
    kDAC_BufferTriggerBySoftwareMode = 1U, /*!< The DAC software trigger is selected. */
} dac_buffer_trigger_mode_t;

#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
/*!
 * @brief DAC buffer watermark.
 */
typedef enum _dac_buffer_watermark
{
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_1_WORD) && FSL_FEATURE_DAC_HAS_WATERMARK_1_WORD
    kDAC_BufferWatermark1Word = 0U, /*!< 1 word  away from the upper limit. */
#endif                              /* FSL_FEATURE_DAC_HAS_WATERMARK_1_WORD */
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_2_WORDS) && FSL_FEATURE_DAC_HAS_WATERMARK_2_WORDS
    kDAC_BufferWatermark2Word = 1U, /*!< 2 words away from the upper limit. */
#endif                              /* FSL_FEATURE_DAC_HAS_WATERMARK_2_WORDS */
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_3_WORDS) && FSL_FEATURE_DAC_HAS_WATERMARK_3_WORDS
    kDAC_BufferWatermark3Word = 2U, /*!< 3 words away from the upper limit. */
#endif                              /* FSL_FEATURE_DAC_HAS_WATERMARK_3_WORDS */
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_4_WORDS) && FSL_FEATURE_DAC_HAS_WATERMARK_4_WORDS
    kDAC_BufferWatermark4Word = 3U, /*!< 4 words away from the upper limit. */
#endif                              /* FSL_FEATURE_DAC_HAS_WATERMARK_4_WORDS */
} dac_buffer_watermark_t;
#endif /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */

/*!
 * @brief DAC buffer work mode.
 */
typedef enum _dac_buffer_work_mode
{
    kDAC_BufferWorkAsNormalMode = 0U, /*!< Normal mode. */
#if defined(FSL_FEATURE_DAC_HAS_BUFFER_SWING_MODE) && FSL_FEATURE_DAC_HAS_BUFFER_SWING_MODE
    kDAC_BufferWorkAsSwingMode,       /*!< Swing mode. */
#endif                                /* FSL_FEATURE_DAC_HAS_BUFFER_SWING_MODE */
    kDAC_BufferWorkAsOneTimeScanMode, /*!< One-Time Scan mode. */
#if defined(FSL_FEATURE_DAC_HAS_BUFFER_FIFO_MODE) && FSL_FEATURE_DAC_HAS_BUFFER_FIFO_MODE
    kDAC_BufferWorkAsFIFOMode, /*!< FIFO mode. */
#endif                         /* FSL_FEATURE_DAC_HAS_BUFFER_FIFO_MODE */
} dac_buffer_work_mode_t;

/*!
 * @brief DAC module configuration.
 */
typedef struct _dac_config
{
    dac_reference_voltage_source_t referenceVoltageSource; /*!< Select the DAC reference voltage source. */
    bool enableLowPowerMode;                               /*!< Enable the low power mode. */
} dac_config_t;

/*!
 * @brief DAC buffer configuration.
 */
typedef struct _dac_buffer_config
{
    dac_buffer_trigger_mode_t triggerMode; /*!< Select the buffer's trigger mode. */
#if defined(FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION) && FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION
    dac_buffer_watermark_t watermark; /*!< Select the buffer's watermark. */
#endif                                /* FSL_FEATURE_DAC_HAS_WATERMARK_SELECTION */
    dac_buffer_work_mode_t workMode;  /*!< Select the buffer's work mode. */
    uint8_t upperLimit;               /*!< Set the upper limit for buffer index.
                                           Normally, 0-15 is available for buffer with 16 item. */
} dac_buffer_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization
 * @{
 */

/*!
 * @brief Initializes the DAC module.
 *
 * This function initializes the DAC module, including:
 *  - Enabling the clock for DAC module.
 *  - Configuring the DAC converter with a user configuration.
 *  - Enabling the DAC module.
 *
 * @param base DAC peripheral base address.
 * @param config Pointer to the configuration structure. See "dac_config_t".
 */
void DAC_Init(DAC_Type *base, const dac_config_t *config);

/*!
 * @brief De-initializes the DAC module.
 *
 * This function de-initializes the DAC module, including:
 *  - Disabling the DAC module.
 *  - Disabling the clock for the DAC module.
 *
 * @param base DAC peripheral base address.
 */
void DAC_Deinit(DAC_Type *base);

/*!
 * @brief Initializes the DAC user configuration structure.
 *
 * This function initializes the user configuration structure to a default value. The default values are:
 * @code
 *   config->referenceVoltageSource = kDAC_ReferenceVoltageSourceVref2;
 *   config->enableLowPowerMode = false;
 * @endcode
 * @param config Pointer to the configuration structure. See "dac_config_t".
 */
void DAC_GetDefaultConfig(dac_config_t *config);

/*!
 * @brief Enables the DAC module.
 *
 * @param base DAC peripheral base address.
 * @param enable Enables the feature or not.
 */
static inline void DAC_Enable(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->C0 |= DAC_C0_DACEN_MASK;
    }
    else
    {
        base->C0 &= ~DAC_C0_DACEN_MASK;
    }
}

/* @} */

/*!
 * @name Buffer
 * @{
 */

/*!
 * @brief Enables the DAC buffer.
 *
 * @param base DAC peripheral base address.
 * @param enable Enables the feature or not.
 */
static inline void DAC_EnableBuffer(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->C1 |= DAC_C1_DACBFEN_MASK;
    }
    else
    {
        base->C1 &= ~DAC_C1_DACBFEN_MASK;
    }
}

/*!
 * @brief Configures the CMP buffer.
 *
 * @param base   DAC peripheral base address.
 * @param config Pointer to the configuration structure. See "dac_buffer_config_t".
 */
void DAC_SetBufferConfig(DAC_Type *base, const dac_buffer_config_t *config);

/*!
 * @brief Initializes the DAC buffer configuration structure.
 *
 * This function initializes the DAC buffer configuration structure to a default value. The default values are:
 * @code
 *   config->triggerMode = kDAC_BufferTriggerBySoftwareMode;
 *   config->watermark   = kDAC_BufferWatermark1Word;
 *   config->workMode    = kDAC_BufferWorkAsNormalMode;
 *   config->upperLimit  = DAC_DATL_COUNT - 1U;
 * @endcode
 * @param config Pointer to the configuration structure. See "dac_buffer_config_t".
 */
void DAC_GetDefaultBufferConfig(dac_buffer_config_t *config);

/*!
 * @brief Enables the DMA for DAC buffer.
 *
 * @param base DAC peripheral base address.
 * @param enable Enables the feature or not.
 */
static inline void DAC_EnableBufferDMA(DAC_Type *base, bool enable)
{
    if (enable)
    {
        base->C1 |= DAC_C1_DMAEN_MASK;
    }
    else
    {
        base->C1 &= ~DAC_C1_DMAEN_MASK;
    }
}

/*!
 * @brief Sets the value for  items in the buffer.
 *
 * @param base  DAC peripheral base address.
 * @param index Setting index for items in the buffer. The available index should not exceed the size of the DAC buffer.
 * @param value Setting value for items in the buffer. 12-bits are available.
 */
void DAC_SetBufferValue(DAC_Type *base, uint8_t index, uint16_t value);

/*!
 * @brief Triggers the buffer by software and updates the read pointer of the DAC buffer.
 *
 * This function triggers the function by software. The read pointer of the DAC buffer is updated with one step
 * after this function is called. Changing the read pointer depends on the buffer's work mode.
 *
 * @param base DAC peripheral base address.
 */
static inline void DAC_DoSoftwareTriggerBuffer(DAC_Type *base)
{
    base->C0 |= DAC_C0_DACSWTRG_MASK;
}

/*!
 * @brief Gets the current read pointer of the DAC buffer.
 *
 * This function gets the current read pointer of the DAC buffer.
 * The current output value depends on the item indexed by the read pointer. It is updated
 * by software trigger or hardware trigger.
 *
 * @param  base DAC peripheral base address.
 *
 * @return      Current read pointer of DAC buffer.
 */
static inline uint8_t DAC_GetBufferReadPointer(DAC_Type *base)
{
    return ((base->C2 & DAC_C2_DACBFRP_MASK) >> DAC_C2_DACBFRP_SHIFT);
}

/*!
 * @brief Sets the current read pointer of the DAC buffer.
 *
 * This function sets the current read pointer of the DAC buffer.
 * The current output value depends on the item indexed by the read pointer. It is updated by
 * software trigger or hardware trigger. After the read pointer changes, the DAC output value also changes.
 *
 * @param base  DAC peripheral base address.
 * @param index Setting index value for the pointer.
 */
void DAC_SetBufferReadPointer(DAC_Type *base, uint8_t index);

/*!
 * @brief Enables interrupts for the DAC buffer.
 *
 * @param base DAC peripheral base address.
 * @param mask Mask value for interrupts. See "_dac_buffer_interrupt_enable".
 */
void DAC_EnableBufferInterrupts(DAC_Type *base, uint32_t mask);

/*!
 * @brief Disables interrupts for the DAC buffer.
 *
 * @param base DAC peripheral base address.
 * @param mask Mask value for interrupts. See  "_dac_buffer_interrupt_enable".
 */
void DAC_DisableBufferInterrupts(DAC_Type *base, uint32_t mask);

/*!
 * @brief Gets the flags of events for the DAC buffer.
 *
 * @param  base DAC peripheral base address.
 *
 * @return      Mask value for the asserted flags. See  "_dac_buffer_status_flags".
 */
uint32_t DAC_GetBufferStatusFlags(DAC_Type *base);

/*!
 * @brief Clears the flags of events for the DAC buffer.
 *
 * @param base DAC peripheral base address.
 * @param mask Mask value for flags. See "_dac_buffer_status_flags_t".
 */
void DAC_ClearBufferStatusFlags(DAC_Type *base, uint32_t mask);

/* @} */

#if defined(__cplusplus)
}
#endif
/*!
 * @}
 */
#endif /* _FSL_DAC_H_ */
