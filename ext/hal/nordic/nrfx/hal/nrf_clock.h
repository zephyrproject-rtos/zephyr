/*
 * Copyright (c) 2015 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_CLOCK_H__
#define NRF_CLOCK_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_clock_hal Clock HAL
 * @{
 * @ingroup nrf_clock
 * @brief   Hardware access layer for managing the CLOCK peripheral.
 *
 * This code can be used to managing low-frequency clock (LFCLK) and the high-frequency clock
 * (HFCLK) settings.
 */

#define NRF_CLOCK_TASK_TRIGGER (1UL)
#define NRF_CLOCK_EVENT_CLEAR  (0UL)

#if defined(NRF52810_XXAA) || \
    defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
    defined(NRF52840_XXAA)
// Enable support for external LFCLK sources. Read more in the Product Specification.
#define NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES
#endif

/**
 * @brief Low-frequency clock sources.
 * @details Used by LFCLKSRC, LFCLKSTAT, and LFCLKSRCCOPY registers.
 */
typedef enum
{
    NRF_CLOCK_LFCLK_RC    = CLOCK_LFCLKSRC_SRC_RC,    /**< Internal 32 kHz RC oscillator. */
    NRF_CLOCK_LFCLK_Xtal  = CLOCK_LFCLKSRC_SRC_Xtal,  /**< External 32 kHz crystal. */
    NRF_CLOCK_LFCLK_Synth = CLOCK_LFCLKSRC_SRC_Synth, /**< Internal 32 kHz synthesizer from HFCLK system clock. */
#if defined(NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES) || defined(__NRFX_DOXYGEN__)
    /**
     * External 32 kHz low swing signal. Used only with the LFCLKSRC register.
     * For the others @ref NRF_CLOCK_LFCLK_Xtal is returned for this setting.
     */
    NRF_CLOCK_LFCLK_Xtal_Low_Swing = (CLOCK_LFCLKSRC_SRC_Xtal |
        (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos)),
    /**
     * External 32 kHz full swing signal. Used only with the LFCLKSRC register.
     * For the others @ref NRF_CLOCK_LFCLK_Xtal is returned for this setting.
     */
    NRF_CLOCK_LFCLK_Xtal_Full_Swing = (CLOCK_LFCLKSRC_SRC_Xtal |
        (CLOCK_LFCLKSRC_BYPASS_Enabled   << CLOCK_LFCLKSRC_BYPASS_Pos) |
        (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos)),
#endif // defined(NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES) || defined(__NRFX_DOXYGEN__)
} nrf_clock_lfclk_t;

/**
 * @brief High-frequency clock sources.
 */
typedef enum
{
    NRF_CLOCK_HFCLK_LOW_ACCURACY  = CLOCK_HFCLKSTAT_SRC_RC,  /**< Internal 16 MHz RC oscillator. */
    NRF_CLOCK_HFCLK_HIGH_ACCURACY = CLOCK_HFCLKSTAT_SRC_Xtal /**< External 16 MHz/32 MHz crystal oscillator. */
} nrf_clock_hfclk_t;

/**
 * @brief Trigger status of task LFCLKSTART/HFCLKSTART.
 * @details Used by LFCLKRUN and HFCLKRUN registers.
 */
typedef enum
{
    NRF_CLOCK_START_TASK_NOT_TRIGGERED = CLOCK_LFCLKRUN_STATUS_NotTriggered, /**< Task LFCLKSTART/HFCLKSTART has not been triggered. */
    NRF_CLOCK_START_TASK_TRIGGERED     = CLOCK_LFCLKRUN_STATUS_Triggered     /**< Task LFCLKSTART/HFCLKSTART has been triggered. */
} nrf_clock_start_task_status_t;

/**
 * @brief Interrupts.
 */
typedef enum
{
    NRF_CLOCK_INT_HF_STARTED_MASK = CLOCK_INTENSET_HFCLKSTARTED_Msk, /**< Interrupt on HFCLKSTARTED event. */
    NRF_CLOCK_INT_LF_STARTED_MASK = CLOCK_INTENSET_LFCLKSTARTED_Msk, /**< Interrupt on LFCLKSTARTED event. */
    NRF_CLOCK_INT_DONE_MASK       = CLOCK_INTENSET_DONE_Msk,         /**< Interrupt on DONE event. */
    NRF_CLOCK_INT_CTTO_MASK       = CLOCK_INTENSET_CTTO_Msk          /**< Interrupt on CTTO event. */
} nrf_clock_int_mask_t;

/**
 * @brief Tasks.
 *
 * @details The NRF_CLOCK_TASK_LFCLKSTOP task cannot be set when the low-frequency clock is not running.
 * The NRF_CLOCK_TASK_HFCLKSTOP task cannot be set when the high-frequency clock is not running.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_CLOCK_TASK_HFCLKSTART = offsetof(NRF_CLOCK_Type, TASKS_HFCLKSTART), /**< Start HFCLK clock source.*/
    NRF_CLOCK_TASK_HFCLKSTOP  = offsetof(NRF_CLOCK_Type, TASKS_HFCLKSTOP),  /**< Stop HFCLK clock source.*/
    NRF_CLOCK_TASK_LFCLKSTART = offsetof(NRF_CLOCK_Type, TASKS_LFCLKSTART), /**< Start LFCLK clock source.*/
    NRF_CLOCK_TASK_LFCLKSTOP  = offsetof(NRF_CLOCK_Type, TASKS_LFCLKSTOP),  /**< Stop LFCLK clock source.*/
    NRF_CLOCK_TASK_CAL        = offsetof(NRF_CLOCK_Type, TASKS_CAL),        /**< Start calibration of LFCLK RC oscillator.*/
    NRF_CLOCK_TASK_CTSTART    = offsetof(NRF_CLOCK_Type, TASKS_CTSTART),    /**< Start calibration timer.*/
    NRF_CLOCK_TASK_CTSTOP     = offsetof(NRF_CLOCK_Type, TASKS_CTSTOP)      /**< Stop calibration timer.*/
} nrf_clock_task_t;                                                         /*lint -restore */

/**
 * @brief Events.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_CLOCK_EVENT_HFCLKSTARTED = offsetof(NRF_CLOCK_Type, EVENTS_HFCLKSTARTED), /**< HFCLK oscillator started.*/
    NRF_CLOCK_EVENT_LFCLKSTARTED = offsetof(NRF_CLOCK_Type, EVENTS_LFCLKSTARTED), /**< LFCLK oscillator started.*/
    NRF_CLOCK_EVENT_DONE         = offsetof(NRF_CLOCK_Type, EVENTS_DONE),         /**< Calibration of LFCLK RC oscillator completed.*/
    NRF_CLOCK_EVENT_CTTO         = offsetof(NRF_CLOCK_Type, EVENTS_CTTO)          /**< Calibration timer time-out.*/
} nrf_clock_event_t;                                                               /*lint -restore */

/**
 * @brief Function for enabling a specific interrupt.
 *
 * @param[in]  int_mask         Interrupt.
 */
__STATIC_INLINE void nrf_clock_int_enable(uint32_t int_mask);

/**
 * @brief Function for disabling a specific interrupt.
 *
 * @param[in]  int_mask         Interrupt.
 */
__STATIC_INLINE void nrf_clock_int_disable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of a specific interrupt.
 *
 * @param[in]  int_mask         Interrupt.
 *
 * @retval     true                   If the interrupt is enabled.
 * @retval     false                  If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_clock_int_enable_check(nrf_clock_int_mask_t int_mask);

/**
 * @brief Function for retrieving the address of a specific task.
 * @details This function can be used by the PPI module.
 *
 * @param[in]  task             Task.
 *
 * @return     Address of the requested task register.
 */
__STATIC_INLINE uint32_t nrf_clock_task_address_get(nrf_clock_task_t task);

/**
 * @brief Function for setting a specific task.
 *
 * @param[in]  task             Task.
 */
__STATIC_INLINE void nrf_clock_task_trigger(nrf_clock_task_t task);

/**
 * @brief Function for retrieving the address of a specific event.
 * @details This function can be used by the PPI module.
 *
 * @param[in]  event       Event.
 *
 * @return     Address of the requested event register.
 */
__STATIC_INLINE uint32_t nrf_clock_event_address_get(nrf_clock_event_t event);

/**
 * @brief Function for clearing a specific event.
 *
 * @param[in]  event       Event.
 */
__STATIC_INLINE void nrf_clock_event_clear(nrf_clock_event_t event);

/**
 * @brief Function for retrieving the state of a specific event.
 *
 * @param[in]  event       Event.
 *
 * @retval     true              If the event is set.
 * @retval     false             If the event is not set.
 */
__STATIC_INLINE bool nrf_clock_event_check(nrf_clock_event_t event);

/**
 * @brief Function for changing the low-frequency clock source.
 * @details This function cannot be called when the low-frequency clock is running.
 *
 * @param[in]  source            New low-frequency clock source.
 *
 */
__STATIC_INLINE void nrf_clock_lf_src_set(nrf_clock_lfclk_t source);

/**
 * @brief Function for retrieving the selected source for the low-frequency clock.
 *
 * @retval     NRF_CLOCK_LFCLK_RC     If the internal 32 kHz RC oscillator is the selected source for the low-frequency clock.
 * @retval     NRF_CLOCK_LFCLK_Xtal   If an external 32 kHz crystal oscillator is the selected source for the low-frequency clock.
 * @retval     NRF_CLOCK_LFCLK_Synth  If the internal 32 kHz synthesizer from the HFCLK is the selected source for the low-frequency clock.
 */
__STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_src_get(void);

/**
 * @brief Function for retrieving the active source of the low-frequency clock.
 *
 * @retval     NRF_CLOCK_LFCLK_RC     If the internal 32 kHz RC oscillator is the active source of the low-frequency clock.
 * @retval     NRF_CLOCK_LFCLK_Xtal   If an external 32 kHz crystal oscillator is the active source of the low-frequency clock.
 * @retval     NRF_CLOCK_LFCLK_Synth  If the internal 32 kHz synthesizer from the HFCLK is the active source of the low-frequency clock.
 */
__STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_actv_src_get(void);

/**
 * @brief Function for retrieving the clock source for the LFCLK clock when the task LKCLKSTART is triggered.
 *
 * @retval     NRF_CLOCK_LFCLK_RC     If the internal 32 kHz RC oscillator is running and generating the LFCLK clock.
 * @retval     NRF_CLOCK_LFCLK_Xtal   If an external 32 kHz crystal oscillator is running and generating the LFCLK clock.
 * @retval     NRF_CLOCK_LFCLK_Synth  If the internal 32 kHz synthesizer from the HFCLK is running and generating the LFCLK clock.
 */
__STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_srccopy_get(void);

/**
 * @brief Function for retrieving the state of the LFCLK clock.
 *
 * @retval     false                     If the LFCLK clock is not running.
 * @retval     true                      If the LFCLK clock is running.
 */
__STATIC_INLINE bool nrf_clock_lf_is_running(void);

/**
 * @brief Function for retrieving the trigger status of the task LFCLKSTART.
 *
 * @retval     NRF_CLOCK_START_TASK_NOT_TRIGGERED     If the task LFCLKSTART has not been triggered.
 * @retval     NRF_CLOCK_START_TASK_TRIGGERED         If the task LFCLKSTART has been triggered.
 */
__STATIC_INLINE nrf_clock_start_task_status_t nrf_clock_lf_start_task_status_get(void);

/**
 * @brief Function for retrieving the active source of the high-frequency clock.
 *
 * @retval     NRF_CLOCK_HFCLK_LOW_ACCURACY   If the internal 16 MHz RC oscillator is the active source of the high-frequency clock.
 * @retval     NRF_CLOCK_HFCLK_HIGH_ACCURACY  If an external 16 MHz/32 MHz crystal oscillator is the active source of the high-frequency clock.
 */
__STATIC_INLINE nrf_clock_hfclk_t nrf_clock_hf_src_get(void);

/**
 * @brief Function for retrieving the state of the HFCLK clock.
 *
 * @param[in]  clk_src                   Clock source to be checked.
 *
 * @retval     false                     If the HFCLK clock is not running.
 * @retval     true                      If the HFCLK clock is running.
 */
__STATIC_INLINE bool nrf_clock_hf_is_running(nrf_clock_hfclk_t clk_src);

/**
 * @brief Function for retrieving the trigger status of the task HFCLKSTART.
 *
 * @retval     NRF_CLOCK_START_TASK_NOT_TRIGGERED     If the task HFCLKSTART has not been triggered.
 * @retval     NRF_CLOCK_START_TASK_TRIGGERED         If the task HFCLKSTART has been triggered.
 */
__STATIC_INLINE nrf_clock_start_task_status_t nrf_clock_hf_start_task_status_get(void);

/**
 * @brief Function for changing the calibration timer interval.
 *
 * @param[in]  interval             New calibration timer interval in 0.25 s resolution (range: 0.25 seconds to 31.75 seconds).
 */
__STATIC_INLINE void nrf_clock_cal_timer_timeout_set(uint32_t interval);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_clock_int_enable(uint32_t int_mask)
{
    NRF_CLOCK->INTENSET = int_mask;
}

__STATIC_INLINE void nrf_clock_int_disable(uint32_t int_mask)
{
    NRF_CLOCK->INTENCLR = int_mask;
}

__STATIC_INLINE bool nrf_clock_int_enable_check(nrf_clock_int_mask_t int_mask)
{
    return (bool)(NRF_CLOCK->INTENCLR & int_mask);
}

__STATIC_INLINE uint32_t nrf_clock_task_address_get(nrf_clock_task_t task)
{
    return ((uint32_t )NRF_CLOCK + task);
}

__STATIC_INLINE void nrf_clock_task_trigger(nrf_clock_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_CLOCK + task)) = NRF_CLOCK_TASK_TRIGGER;
}

__STATIC_INLINE uint32_t nrf_clock_event_address_get(nrf_clock_event_t event)
{
    return ((uint32_t)NRF_CLOCK + event);
}

__STATIC_INLINE void nrf_clock_event_clear(nrf_clock_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_CLOCK + event)) = NRF_CLOCK_EVENT_CLEAR;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_CLOCK + event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_clock_event_check(nrf_clock_event_t event)
{
    return (bool)*((volatile uint32_t *)((uint8_t *)NRF_CLOCK + event));
}

__STATIC_INLINE void nrf_clock_lf_src_set(nrf_clock_lfclk_t source)
{
    NRF_CLOCK->LFCLKSRC = (uint32_t)(source);
}

__STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_src_get(void)
{
    return (nrf_clock_lfclk_t)(NRF_CLOCK->LFCLKSRC);
}

__STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_actv_src_get(void)
{
    return (nrf_clock_lfclk_t)((NRF_CLOCK->LFCLKSTAT &
                                CLOCK_LFCLKSTAT_SRC_Msk) >> CLOCK_LFCLKSTAT_SRC_Pos);
}

__STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_srccopy_get(void)
{
    return (nrf_clock_lfclk_t)((NRF_CLOCK->LFCLKSRCCOPY &
                                CLOCK_LFCLKSRCCOPY_SRC_Msk) >> CLOCK_LFCLKSRCCOPY_SRC_Pos);
}

__STATIC_INLINE bool nrf_clock_lf_is_running(void)
{
    return ((NRF_CLOCK->LFCLKSTAT &
             CLOCK_LFCLKSTAT_STATE_Msk) >> CLOCK_LFCLKSTAT_STATE_Pos);
}

__STATIC_INLINE nrf_clock_start_task_status_t nrf_clock_lf_start_task_status_get(void)
{
    return (nrf_clock_start_task_status_t)((NRF_CLOCK->LFCLKRUN &
                                            CLOCK_LFCLKRUN_STATUS_Msk) >>
                                           CLOCK_LFCLKRUN_STATUS_Pos);
}

__STATIC_INLINE nrf_clock_hfclk_t nrf_clock_hf_src_get(void)
{
    return (nrf_clock_hfclk_t)((NRF_CLOCK->HFCLKSTAT &
                                CLOCK_HFCLKSTAT_SRC_Msk) >> CLOCK_HFCLKSTAT_SRC_Pos);
}

__STATIC_INLINE bool nrf_clock_hf_is_running(nrf_clock_hfclk_t clk_src)
{
    return (NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_STATE_Msk | CLOCK_HFCLKSTAT_SRC_Msk)) ==
            (CLOCK_HFCLKSTAT_STATE_Msk | (clk_src << CLOCK_HFCLKSTAT_SRC_Pos));
}

__STATIC_INLINE nrf_clock_start_task_status_t nrf_clock_hf_start_task_status_get(void)
{
    return (nrf_clock_start_task_status_t)((NRF_CLOCK->HFCLKRUN &
                                            CLOCK_HFCLKRUN_STATUS_Msk) >>
                                           CLOCK_HFCLKRUN_STATUS_Pos);
}

__STATIC_INLINE void nrf_clock_cal_timer_timeout_set(uint32_t interval)
{
    NRF_CLOCK->CTIV = ((interval << CLOCK_CTIV_CTIV_Pos) & CLOCK_CTIV_CTIV_Msk);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_CLOCK_H__
