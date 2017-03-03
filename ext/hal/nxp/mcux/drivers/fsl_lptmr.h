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
#ifndef _FSL_LPTMR_H_
#define _FSL_LPTMR_H_

#include "fsl_common.h"

/*!
 * @addtogroup lptmr
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_LPTMR_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0 */
/*@}*/

/*! @brief LPTMR pin selection used in pulse counter mode.*/
typedef enum _lptmr_pin_select
{
    kLPTMR_PinSelectInput_0 = 0x0U, /*!< Pulse counter input 0 is selected */
    kLPTMR_PinSelectInput_1 = 0x1U, /*!< Pulse counter input 1 is selected */
    kLPTMR_PinSelectInput_2 = 0x2U, /*!< Pulse counter input 2 is selected */
    kLPTMR_PinSelectInput_3 = 0x3U  /*!< Pulse counter input 3 is selected */
} lptmr_pin_select_t;

/*! @brief LPTMR pin polarity used in pulse counter mode.*/
typedef enum _lptmr_pin_polarity
{
    kLPTMR_PinPolarityActiveHigh = 0x0U, /*!< Pulse Counter input source is active-high */
    kLPTMR_PinPolarityActiveLow = 0x1U   /*!< Pulse Counter input source is active-low */
} lptmr_pin_polarity_t;

/*! @brief LPTMR timer mode selection.*/
typedef enum _lptmr_timer_mode
{
    kLPTMR_TimerModeTimeCounter = 0x0U, /*!< Time Counter mode */
    kLPTMR_TimerModePulseCounter = 0x1U /*!< Pulse Counter mode */
} lptmr_timer_mode_t;

/*! @brief LPTMR prescaler/glitch filter values*/
typedef enum _lptmr_prescaler_glitch_value
{
    kLPTMR_Prescale_Glitch_0 = 0x0U,  /*!< Prescaler divide 2, glitch filter does not support this setting */
    kLPTMR_Prescale_Glitch_1 = 0x1U,  /*!< Prescaler divide 4, glitch filter 2 */
    kLPTMR_Prescale_Glitch_2 = 0x2U,  /*!< Prescaler divide 8, glitch filter 4 */
    kLPTMR_Prescale_Glitch_3 = 0x3U,  /*!< Prescaler divide 16, glitch filter 8 */
    kLPTMR_Prescale_Glitch_4 = 0x4U,  /*!< Prescaler divide 32, glitch filter 16 */
    kLPTMR_Prescale_Glitch_5 = 0x5U,  /*!< Prescaler divide 64, glitch filter 32 */
    kLPTMR_Prescale_Glitch_6 = 0x6U,  /*!< Prescaler divide 128, glitch filter 64 */
    kLPTMR_Prescale_Glitch_7 = 0x7U,  /*!< Prescaler divide 256, glitch filter 128 */
    kLPTMR_Prescale_Glitch_8 = 0x8U,  /*!< Prescaler divide 512, glitch filter 256 */
    kLPTMR_Prescale_Glitch_9 = 0x9U,  /*!< Prescaler divide 1024, glitch filter 512*/
    kLPTMR_Prescale_Glitch_10 = 0xAU, /*!< Prescaler divide 2048 glitch filter 1024 */
    kLPTMR_Prescale_Glitch_11 = 0xBU, /*!< Prescaler divide 4096, glitch filter 2048 */
    kLPTMR_Prescale_Glitch_12 = 0xCU, /*!< Prescaler divide 8192, glitch filter 4096 */
    kLPTMR_Prescale_Glitch_13 = 0xDU, /*!< Prescaler divide 16384, glitch filter 8192 */
    kLPTMR_Prescale_Glitch_14 = 0xEU, /*!< Prescaler divide 32768, glitch filter 16384 */
    kLPTMR_Prescale_Glitch_15 = 0xFU  /*!< Prescaler divide 65536, glitch filter 32768 */
} lptmr_prescaler_glitch_value_t;

/*!
 * @brief LPTMR prescaler/glitch filter clock select.
 * @note Clock connections are SoC-specific
 */
typedef enum _lptmr_prescaler_clock_select
{
    kLPTMR_PrescalerClock_0 = 0x0U, /*!< Prescaler/glitch filter clock 0 selected. */
    kLPTMR_PrescalerClock_1 = 0x1U, /*!< Prescaler/glitch filter clock 1 selected. */
    kLPTMR_PrescalerClock_2 = 0x2U, /*!< Prescaler/glitch filter clock 2 selected. */
    kLPTMR_PrescalerClock_3 = 0x3U, /*!< Prescaler/glitch filter clock 3 selected. */
} lptmr_prescaler_clock_select_t;

/*! @brief List of the LPTMR interrupts */
typedef enum _lptmr_interrupt_enable
{
    kLPTMR_TimerInterruptEnable = LPTMR_CSR_TIE_MASK, /*!< Timer interrupt enable */
} lptmr_interrupt_enable_t;

/*! @brief List of the LPTMR status flags */
typedef enum _lptmr_status_flags
{
    kLPTMR_TimerCompareFlag = LPTMR_CSR_TCF_MASK, /*!< Timer compare flag */
} lptmr_status_flags_t;

/*!
 * @brief LPTMR config structure
 *
 * This structure holds the configuration settings for the LPTMR peripheral. To initialize this
 * structure to reasonable defaults, call the LPTMR_GetDefaultConfig() function and pass a
 * pointer to your configuration structure instance.
 *
 * The configuration struct can be made constant so it resides in flash.
 */
typedef struct _lptmr_config
{
    lptmr_timer_mode_t timerMode;     /*!< Time counter mode or pulse counter mode */
    lptmr_pin_select_t pinSelect;     /*!< LPTMR pulse input pin select; used only in pulse counter mode */
    lptmr_pin_polarity_t pinPolarity; /*!< LPTMR pulse input pin polarity; used only in pulse counter mode */
    bool enableFreeRunning;           /*!< True: enable free running, counter is reset on overflow
                                           False: counter is reset when the compare flag is set */
    bool bypassPrescaler;             /*!< True: bypass prescaler; false: use clock from prescaler */
    lptmr_prescaler_clock_select_t prescalerClockSource; /*!< LPTMR clock source */
    lptmr_prescaler_glitch_value_t value;                /*!< Prescaler or glitch filter value */
} lptmr_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Ungates the LPTMR clock and configures the peripheral for a basic operation.
 *
 * @note This API should be called at the beginning of the application using the LPTMR driver.
 *
 * @param base   LPTMR peripheral base address
 * @param config A pointer to the LPTMR configuration structure.
 */
void LPTMR_Init(LPTMR_Type* base, const lptmr_config_t* config);

/*!
 * @brief Gates the LPTMR clock.
 *
 * @param base LPTMR peripheral base address
 */
void LPTMR_Deinit(LPTMR_Type* base);

/*!
 * @brief Fills in the LPTMR configuration structure with default settings.
 *
 * The default values are as follows.
 * @code
 *    config->timerMode = kLPTMR_TimerModeTimeCounter;
 *    config->pinSelect = kLPTMR_PinSelectInput_0;
 *    config->pinPolarity = kLPTMR_PinPolarityActiveHigh;
 *    config->enableFreeRunning = false;
 *    config->bypassPrescaler = true;
 *    config->prescalerClockSource = kLPTMR_PrescalerClock_1;
 *    config->value = kLPTMR_Prescale_Glitch_0;
 * @endcode
 * @param config A pointer to the LPTMR configuration structure.
 */
void LPTMR_GetDefaultConfig(lptmr_config_t* config);

/*! @}*/

/*!
 * @name Interrupt Interface
 * @{
 */

/*!
 * @brief Enables the selected LPTMR interrupts.
 *
 * @param base LPTMR peripheral base address
 * @param mask The interrupts to enable. This is a logical OR of members of the
 *             enumeration ::lptmr_interrupt_enable_t
 */
static inline void LPTMR_EnableInterrupts(LPTMR_Type* base, uint32_t mask)
{
    uint32_t reg = base->CSR;

    /* Clear the TCF bit so that we don't clear this w1c bit when writing back */
    reg &= ~(LPTMR_CSR_TCF_MASK);
    reg |= mask;
    base->CSR = reg;
}

/*!
 * @brief Disables the selected LPTMR interrupts.
 *
 * @param base LPTMR peripheral base address
 * @param mask The interrupts to disable. This is a logical OR of members of the
 *             enumeration ::lptmr_interrupt_enable_t.
 */
static inline void LPTMR_DisableInterrupts(LPTMR_Type* base, uint32_t mask)
{
    uint32_t reg = base->CSR;

    /* Clear the TCF bit so that we don't clear this w1c bit when writing back */
    reg &= ~(LPTMR_CSR_TCF_MASK);
    reg &= ~mask;
    base->CSR = reg;
}

/*!
 * @brief Gets the enabled LPTMR interrupts.
 *
 * @param base LPTMR peripheral base address
 *
 * @return The enabled interrupts. This is the logical OR of members of the
 *         enumeration ::lptmr_interrupt_enable_t
 */
static inline uint32_t LPTMR_GetEnabledInterrupts(LPTMR_Type* base)
{
    return (base->CSR & LPTMR_CSR_TIE_MASK);
}

/*! @}*/

/*!
 * @name Status Interface
 * @{
 */

/*!
 * @brief Gets the LPTMR status flags.
 *
 * @param base LPTMR peripheral base address
 *
 * @return The status flags. This is the logical OR of members of the
 *         enumeration ::lptmr_status_flags_t
 */
static inline uint32_t LPTMR_GetStatusFlags(LPTMR_Type* base)
{
    return (base->CSR & LPTMR_CSR_TCF_MASK);
}

/*!
 * @brief  Clears the LPTMR status flags.
 *
 * @param base LPTMR peripheral base address
 * @param mask The status flags to clear. This is a logical OR of members of the
 *             enumeration ::lptmr_status_flags_t.
 */
static inline void LPTMR_ClearStatusFlags(LPTMR_Type* base, uint32_t mask)
{
    base->CSR |= mask;
}

/*! @}*/

/*!
 * @name Read and write the timer period
 * @{
 */

/*!
 * @brief Sets the timer period in units of count.
 *
 * Timers counts from 0 until it equals the count value set here. The count value is written to
 * the CMR register.
 *
 * @note
 * 1. The TCF flag is set with the CNR equals the count provided here and then increments.
 * 2. Call the utility macros provided in the fsl_common.h to convert to ticks.
 *
 * @param base  LPTMR peripheral base address
 * @param ticks A timer period in units of ticks, which should be equal or greater than 1.
 */
static inline void LPTMR_SetTimerPeriod(LPTMR_Type* base, uint16_t ticks)
{
    base->CMR = ticks - 1;
}

/*!
 * @brief Reads the current timer counting value.
 *
 * This function returns the real-time timer counting value in a range from 0 to a
 * timer period.
 *
 * @note Call the utility macros provided in the fsl_common.h to convert ticks to usec or msec.
 *
 * @param base LPTMR peripheral base address
 *
 * @return The current counter value in ticks
 */
static inline uint16_t LPTMR_GetCurrentTimerCount(LPTMR_Type* base)
{
    /* Must first write any value to the CNR. This synchronizes and registers the current value
     * of the CNR into a temporary register which can then be read
     */
    base->CNR = 0U;
    return (uint16_t)base->CNR;
}

/*! @}*/

/*!
 * @name Timer Start and Stop
 * @{
 */

/*!
 * @brief Starts the timer.
 *
 * After calling this function, the timer counts up to the CMR register value.
 * Each time the timer reaches the CMR value and then increments, it generates a
 * trigger pulse and sets the timeout interrupt flag. An interrupt is also
 * triggered if the timer interrupt is enabled.
 *
 * @param base LPTMR peripheral base address
 */
static inline void LPTMR_StartTimer(LPTMR_Type* base)
{
    uint32_t reg = base->CSR;

    /* Clear the TCF bit to avoid clearing the w1c bit when writing back. */
    reg &= ~(LPTMR_CSR_TCF_MASK);
    reg |= LPTMR_CSR_TEN_MASK;
    base->CSR = reg;
}

/*!
 * @brief Stops the timer.
 *
 * This function stops the timer and resets the timer's counter register.
 *
 * @param base LPTMR peripheral base address
 */
static inline void LPTMR_StopTimer(LPTMR_Type* base)
{
    uint32_t reg = base->CSR;

    /* Clear the TCF bit to avoid clearing the w1c bit when writing back. */
    reg &= ~(LPTMR_CSR_TCF_MASK);
    reg &= ~LPTMR_CSR_TEN_MASK;
    base->CSR = reg;
}

/*! @}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_LPTMR_H_ */
