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

#ifndef __GPT_H__
#define __GPT_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup gpt_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Clock source. */
enum _gpt_clock_source
{
    gptClockSourceNone    = 0U, /*!< No source selected.*/
    gptClockSourcePeriph  = 1U, /*!< Use peripheral module clock.*/
    gptClockSourceLowFreq = 4U, /*!< Use 32 K clock.*/
    gptClockSourceOsc     = 5U, /*!< Use 24 M OSC clock.*/
};

/*! @brief Input capture channel number. */
enum _gpt_input_capture_channel
{
    gptInputCaptureChannel1 = 0U, /*!< Input Capture Channel1.*/
    gptInputCaptureChannel2 = 1U, /*!< Input Capture Channel2.*/
};

/*! @brief Input capture operation mode. */
enum _gpt_input_operation_mode
{
    gptInputOperationDisabled = 0U, /*!< Don't capture.*/
    gptInputOperationRiseEdge = 1U, /*!< Capture on rising edge of input pin.*/
    gptInputOperationFallEdge = 2U, /*!< Capture on falling edge of input pin.*/
    gptInputOperationBothEdge = 3U, /*!< Capture on both edges of input pin.*/
};

/*! @brief Output compare channel number. */
enum _gpt_output_compare_channel
{
    gptOutputCompareChannel1 = 0U, /*!< Output Compare Channel1.*/
    gptOutputCompareChannel2 = 1U, /*!< Output Compare Channel2.*/
    gptOutputCompareChannel3 = 2U, /*!< Output Compare Channel3.*/
};

/*! @brief Output compare operation mode. */
enum _gpt_output_operation_mode
{
    gptOutputOperationDisconnected = 0U, /*!< Don't change output pin.*/
    gptOutputOperationToggle       = 1U, /*!< Toggle output pin.*/
    gptOutputOperationClear        = 2U, /*!< Set output pin low.*/
    gptOutputOperationSet          = 3U, /*!< Set output pin high.*/
    gptOutputOperationActivelow    = 4U, /*!< Generate a active low pulse on output pin.*/
};

/*! @brief Status flag. */
enum _gpt_status_flag
{
    gptStatusFlagOutputCompare1 = 1U << 0, /*!< Output compare channel 1 event.*/
    gptStatusFlagOutputCompare2 = 1U << 1, /*!< Output compare channel 2 event.*/
    gptStatusFlagOutputCompare3 = 1U << 2, /*!< Output compare channel 3 event.*/
    gptStatusFlagInputCapture1  = 1U << 3, /*!< Capture channel 1 event.*/
    gptStatusFlagInputCapture2  = 1U << 4, /*!< Capture channel 2 event.*/
    gptStatusFlagRollOver       = 1U << 5, /*!< Counter reaches maximum value and rolled over to 0 event.*/
};

/*! @brief Structure to configure the running mode. */
typedef struct _gpt_init_config
{
    bool freeRun;    /*!< true: FreeRun mode, false: Restart mode. */
    bool waitEnable; /*!< GPT enabled in wait mode. */
    bool stopEnable; /*!< GPT enabled in stop mode. */
    bool dozeEnable; /*!< GPT enabled in doze mode. */
    bool dbgEnable;  /*!< GPT enabled in debug mode. */
    bool enableMode; /*!< true: counter reset to 0 when enabled, false: counter retain its value when enabled. */
} gpt_init_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name GPT State Control
 * @{
 */

/*!
 * @brief Initialize GPT to reset state and initialize running mode.
 *
 * @param base GPT base pointer.
 * @param initConfig GPT mode setting configuration.
 */
void GPT_Init(GPT_Type* base, const gpt_init_config_t* initConfig);

/*!
 * @brief Software reset of GPT module.
 *
 * @param base GPT base pointer.
 */
static inline void GPT_SoftReset(GPT_Type* base)
{
    base->CR |= GPT_CR_SWR_MASK;
    /* Wait reset finished. */
    while (base->CR & GPT_CR_SWR_MASK) {};
}

/*!
 * @brief Set clock source of GPT.
 *
 * @param base GPT base pointer.
 * @param source Clock source (see @ref _gpt_clock_source enumeration).
 */
void GPT_SetClockSource(GPT_Type* base, uint32_t source);

/*!
 * @brief Get clock source of GPT.
 *
 * @param base GPT base pointer.
 * @return clock source (see @ref _gpt_clock_source enumeration).
 */
static inline uint32_t GPT_GetClockSource(GPT_Type* base)
{
    return (base->CR & GPT_CR_CLKSRC_MASK) >> GPT_CR_CLKSRC_SHIFT;
}

/*!
 * @brief Set pre scaler of GPT.
 *
 * @param base GPT base pointer.
 * @param prescaler Pre-scaler of GPT (0-4095, divider = prescaler + 1).
 */
static inline void GPT_SetPrescaler(GPT_Type* base, uint32_t prescaler)
{
    assert(prescaler <= GPT_PR_PRESCALER_MASK);

    base->PR = (base->PR & ~GPT_PR_PRESCALER_MASK) | GPT_PR_PRESCALER(prescaler);
}

/*!
 * @brief Get pre scaler of GPT.
 *
 * @param base GPT base pointer.
 * @return pre scaler of GPT (0-4095).
 */
static inline uint32_t GPT_GetPrescaler(GPT_Type* base)
{
    return (base->PR & GPT_PR_PRESCALER_MASK) >> GPT_PR_PRESCALER_SHIFT;
}

/*!
 * @brief OSC 24M pre-scaler before selected by clock source.
 *
 * @param base GPT base pointer.
 * @param prescaler OSC pre-scaler(0-15, divider = prescaler + 1).
 */
static inline void GPT_SetOscPrescaler(GPT_Type* base, uint32_t prescaler)
{
    assert(prescaler <= (GPT_PR_PRESCALER24M_MASK >> GPT_PR_PRESCALER24M_SHIFT));

    base->PR = (base->PR & ~GPT_PR_PRESCALER24M_MASK) | GPT_PR_PRESCALER24M(prescaler);
}

/*!
 * @brief Get pre-scaler of GPT.
 *
 * @param base GPT base pointer.
 * @return OSC pre scaler of GPT (0-15).
 */
static inline uint32_t GPT_GetOscPrescaler(GPT_Type* base)
{
    return (base->PR & GPT_PR_PRESCALER24M_MASK) >> GPT_PR_PRESCALER24M_SHIFT;
}

/*!
 * @brief Enable GPT module.
 *
 * @param base GPT base pointer.
 */
static inline void GPT_Enable(GPT_Type* base)
{
    base->CR |= GPT_CR_EN_MASK;
}

/*!
 * @brief Disable GPT module.
 *
 * @param base GPT base pointer.
 */
static inline void GPT_Disable(GPT_Type* base)
{
    base->CR &= ~GPT_CR_EN_MASK;
}

/*!
 * @brief Get GPT counter value.
 *
 * @param base GPT base pointer.
 * @return GPT counter value.
 */
static inline uint32_t GPT_ReadCounter(GPT_Type* base)
{
    return base->CNT;
}

/*@}*/

/*!
 * @name GPT Input/Output Signal Control
 * @{
 */

/*!
 * @brief Set GPT operation mode of input capture channel.
 *
 * @param base GPT base pointer.
 * @param channel GPT capture channel (see @ref _gpt_input_capture_channel enumeration).
 * @param mode GPT input capture operation mode (see @ref _gpt_input_operation_mode enumeration).
 */
static inline void GPT_SetInputOperationMode(GPT_Type* base, uint32_t channel, uint32_t mode)
{
    assert (channel <= gptInputCaptureChannel2);

    base->CR = (base->CR & ~(GPT_CR_IM1_MASK << (channel * 2))) | (GPT_CR_IM1(mode) << (channel * 2));
}

/*!
 * @brief Get GPT operation mode of input capture channel.
 *
 * @param base GPT base pointer.
 * @param channel GPT capture channel (see @ref _gpt_input_capture_channel enumeration).
 * @return GPT input capture operation mode (see @ref _gpt_input_operation_mode enumeration).
 */
static inline uint32_t GPT_GetInputOperationMode(GPT_Type* base, uint32_t channel)
{
    assert (channel <= gptInputCaptureChannel2);

    return (base->CR >> (GPT_CR_IM1_SHIFT + channel * 2)) & (GPT_CR_IM1_MASK >> GPT_CR_IM1_SHIFT);
}

/*!
 * @brief Get GPT input capture value of certain channel.
 *
 * @param base GPT base pointer.
 * @param channel GPT capture channel (see @ref _gpt_input_capture_channel enumeration).
 * @return GPT input capture value.
 */
static inline uint32_t GPT_GetInputCaptureValue(GPT_Type* base, uint32_t channel)
{
    assert (channel <= gptInputCaptureChannel2);

    return *(&base->ICR1 + channel);
}

/*!
 * @brief Set GPT operation mode of output compare channel.
 *
 * @param base GPT base pointer.
 * @param channel GPT output compare channel (see @ref _gpt_output_compare_channel enumeration).
 * @param mode GPT output operation mode (see @ref _gpt_output_operation_mode enumeration).
 */
static inline void GPT_SetOutputOperationMode(GPT_Type* base, uint32_t channel, uint32_t mode)
{
    assert (channel <= gptOutputCompareChannel3);

    base->CR = (base->CR & ~(GPT_CR_OM1_MASK << (channel * 3))) | (GPT_CR_OM1(mode) << (channel * 3));
}

/*!
 * @brief Get GPT operation mode of output compare channel.
 *
 * @param base GPT base pointer.
 * @param channel GPT output compare channel (see @ref _gpt_output_compare_channel enumeration).
 * @return GPT output operation mode (see @ref _gpt_output_operation_mode enumeration).
 */
static inline uint32_t GPT_GetOutputOperationMode(GPT_Type* base, uint32_t channel)
{
    assert (channel <= gptOutputCompareChannel3);

    return (base->CR >> (GPT_CR_OM1_SHIFT + channel * 3)) & (GPT_CR_OM1_MASK >> GPT_CR_OM1_SHIFT);
}

/*!
 * @brief Set GPT output compare value of output compare channel.
 *
 * @param base GPT base pointer.
 * @param channel GPT output compare channel (see @ref _gpt_output_compare_channel enumeration).
 * @param value GPT output compare value.
 */
static inline void GPT_SetOutputCompareValue(GPT_Type* base, uint32_t channel, uint32_t value)
{
    assert (channel <= gptOutputCompareChannel3);

    *(&base->OCR1 + channel) = value;
}

/*!
 * @brief Get GPT output compare value of output compare channel.
 *
 * @param base GPT base pointer.
 * @param channel GPT output compare channel (see @ref _gpt_output_compare_channel enumeration).
 * @return GPT output compare value.
 */
static inline uint32_t GPT_GetOutputCompareValue(GPT_Type* base, uint32_t channel)
{
    assert (channel <= gptOutputCompareChannel3);

    return *(&base->OCR1 + channel);
}

/*!
 * @brief Force GPT output action on output compare channel, ignoring comparator.
 *
 * @param base GPT base pointer.
 * @param channel GPT output compare channel (see @ref _gpt_output_compare_channel enumeration).
 */
static inline void GPT_ForceOutput(GPT_Type* base, uint32_t channel)
{
    assert (channel <= gptOutputCompareChannel3);

    base->CR |= (GPT_CR_FO1_MASK << channel);
}

/*@}*/

/*!
 * @name GPT Interrupt and Status Control
 * @{
 */

/*!
 * @brief Get GPT status flag.
 *
 * @param base GPT base pointer.
 * @param flags GPT status flag mask (see @ref _gpt_status_flag for bit definition).
 * @return GPT status, each bit represents one status flag.
 */
static inline uint32_t GPT_GetStatusFlag(GPT_Type* base, uint32_t flags)
{
    return base->SR & flags;
}

/*!
 * @brief Clear one or more GPT status flag.
 *
 * @param base GPT base pointer.
 * @param flags GPT status flag mask (see @ref _gpt_status_flag for bit definition).
 */
static inline void GPT_ClearStatusFlag(GPT_Type* base, uint32_t flags)
{
    base->SR = flags;
}

/*!
 * @brief Enable or Disable GPT interrupts.
 *
 * @param base GPT base pointer.
 * @param flags GPT status flag mask (see @ref _gpt_status_flag for bit definition).
 * @param enable Enable/Disable GPT interrupts.
 *               -true: Enable GPT interrupts.
 *               -false: Disable GPT interrupts.
 */
void GPT_SetIntCmd(GPT_Type* base, uint32_t flags, bool enable);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __GPT_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
