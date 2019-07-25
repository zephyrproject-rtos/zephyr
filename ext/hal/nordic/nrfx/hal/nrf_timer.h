/*
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_TIMER_H__
#define NRF_TIMER_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_timer_hal TIMER HAL
 * @{
 * @ingroup nrf_timer
 * @brief   Hardware access layer for managing the TIMER peripheral.
 */

/**
 * @brief Macro for getting the maximum bit resolution of the specified timer instance.
 *
 * @param[in] id Index of the specified timer instance.
 *
 * @retval Maximum bit resolution of the specified timer instance.
 */
#define TIMER_MAX_SIZE(id)  NRFX_CONCAT_3(TIMER, id, _MAX_SIZE)

/**
 * @brief Macro for validating the correctness of the bit width resolution setting.
 *
 * @param[in] id        Index of the specified timer instance.
 * @param[in] bit_width Bit width resolution value to be checked.
 *
 * @retval true  Timer instance supports the specified bit width resolution value.
 * @retval false Timer instance does not support the specified bit width resolution value.
 */
#define TIMER_BIT_WIDTH_MAX(id, bit_width) \
    (TIMER_MAX_SIZE(id) == 8   ? (bit_width == NRF_TIMER_BIT_WIDTH_8)  :  \
    (TIMER_MAX_SIZE(id) == 16  ? (bit_width == NRF_TIMER_BIT_WIDTH_8)  || \
                                 (bit_width == NRF_TIMER_BIT_WIDTH_16)  : \
    (TIMER_MAX_SIZE(id) == 24  ? (bit_width == NRF_TIMER_BIT_WIDTH_8)  || \
                                 (bit_width == NRF_TIMER_BIT_WIDTH_16) || \
                                 (bit_width == NRF_TIMER_BIT_WIDTH_24) :  \
    (TIMER_MAX_SIZE(id) == 32  ? (bit_width == NRF_TIMER_BIT_WIDTH_8)  || \
                                 (bit_width == NRF_TIMER_BIT_WIDTH_16) || \
                                 (bit_width == NRF_TIMER_BIT_WIDTH_24) || \
                                 (bit_width == NRF_TIMER_BIT_WIDTH_32) :  \
    false))))

#if (TIMER_COUNT > 3) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Macro for checking correctness of bit width configuration for the specified timer.
 *
 * @param[in] p_reg     Timer instance register.
 * @param[in] bit_width Bit width resolution value to be checked.
 *
 * @retval true  Timer instance supports the specified bit width resolution value.
 * @retval false Timer instance does not support the specified bit width resolution value.
 */
#define NRF_TIMER_IS_BIT_WIDTH_VALID(p_reg, bit_width) (                \
       ((p_reg == NRF_TIMER0) && (TIMER_BIT_WIDTH_MAX(0, bit_width)))   \
    || ((p_reg == NRF_TIMER1) && (TIMER_BIT_WIDTH_MAX(1, bit_width)))   \
    || ((p_reg == NRF_TIMER2) && (TIMER_BIT_WIDTH_MAX(2, bit_width)))   \
    || ((p_reg == NRF_TIMER3) && (TIMER_BIT_WIDTH_MAX(3, bit_width)))   \
    || ((p_reg == NRF_TIMER4) && (TIMER_BIT_WIDTH_MAX(4, bit_width))) )

#else
#define NRF_TIMER_IS_BIT_WIDTH_VALID(p_reg, bit_width) (             \
       ((p_reg == NRF_TIMER0) && TIMER_BIT_WIDTH_MAX(0, bit_width))  \
    || ((p_reg == NRF_TIMER1) && TIMER_BIT_WIDTH_MAX(1, bit_width))  \
    || ((p_reg == NRF_TIMER2) && TIMER_BIT_WIDTH_MAX(2, bit_width)) )

#endif

/**
 * @brief Macro for getting the number of capture/compare channels available
 *        in a given timer instance.
 *
 * @param[in] id Index of the specified timer instance.
 */
#define NRF_TIMER_CC_CHANNEL_COUNT(id)  NRFX_CONCAT_3(TIMER, id, _CC_NUM)


/** @brief Timer tasks. */
typedef enum
{
    NRF_TIMER_TASK_START    = offsetof(NRF_TIMER_Type, TASKS_START),      ///< Task for starting the timer.
    NRF_TIMER_TASK_STOP     = offsetof(NRF_TIMER_Type, TASKS_STOP),       ///< Task for stopping the timer.
    NRF_TIMER_TASK_COUNT    = offsetof(NRF_TIMER_Type, TASKS_COUNT),      ///< Task for incrementing the timer (in counter mode).
    NRF_TIMER_TASK_CLEAR    = offsetof(NRF_TIMER_Type, TASKS_CLEAR),      ///< Task for resetting the timer value.
    NRF_TIMER_TASK_SHUTDOWN = offsetof(NRF_TIMER_Type, TASKS_SHUTDOWN),   ///< Task for powering off the timer.
    NRF_TIMER_TASK_CAPTURE0 = offsetof(NRF_TIMER_Type, TASKS_CAPTURE[0]), ///< Task for capturing the timer value on channel 0.
    NRF_TIMER_TASK_CAPTURE1 = offsetof(NRF_TIMER_Type, TASKS_CAPTURE[1]), ///< Task for capturing the timer value on channel 1.
    NRF_TIMER_TASK_CAPTURE2 = offsetof(NRF_TIMER_Type, TASKS_CAPTURE[2]), ///< Task for capturing the timer value on channel 2.
    NRF_TIMER_TASK_CAPTURE3 = offsetof(NRF_TIMER_Type, TASKS_CAPTURE[3]), ///< Task for capturing the timer value on channel 3.
#if defined(TIMER_INTENSET_COMPARE4_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_TASK_CAPTURE4 = offsetof(NRF_TIMER_Type, TASKS_CAPTURE[4]), ///< Task for capturing the timer value on channel 4.
#endif
#if defined(TIMER_INTENSET_COMPARE5_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_TASK_CAPTURE5 = offsetof(NRF_TIMER_Type, TASKS_CAPTURE[5]), ///< Task for capturing the timer value on channel 5.
#endif
} nrf_timer_task_t;

/** @brief Timer events. */
typedef enum
{
    NRF_TIMER_EVENT_COMPARE0 = offsetof(NRF_TIMER_Type, EVENTS_COMPARE[0]), ///< Event from compare channel 0.
    NRF_TIMER_EVENT_COMPARE1 = offsetof(NRF_TIMER_Type, EVENTS_COMPARE[1]), ///< Event from compare channel 1.
    NRF_TIMER_EVENT_COMPARE2 = offsetof(NRF_TIMER_Type, EVENTS_COMPARE[2]), ///< Event from compare channel 2.
    NRF_TIMER_EVENT_COMPARE3 = offsetof(NRF_TIMER_Type, EVENTS_COMPARE[3]), ///< Event from compare channel 3.
#if defined(TIMER_INTENSET_COMPARE4_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_EVENT_COMPARE4 = offsetof(NRF_TIMER_Type, EVENTS_COMPARE[4]), ///< Event from compare channel 4.
#endif
#if defined(TIMER_INTENSET_COMPARE5_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_EVENT_COMPARE5 = offsetof(NRF_TIMER_Type, EVENTS_COMPARE[5]), ///< Event from compare channel 5.
#endif
} nrf_timer_event_t;

/** @brief Types of timer shortcuts. */
typedef enum
{
    NRF_TIMER_SHORT_COMPARE0_STOP_MASK = TIMER_SHORTS_COMPARE0_STOP_Msk,   ///< Shortcut for stopping the timer based on compare 0.
    NRF_TIMER_SHORT_COMPARE1_STOP_MASK = TIMER_SHORTS_COMPARE1_STOP_Msk,   ///< Shortcut for stopping the timer based on compare 1.
    NRF_TIMER_SHORT_COMPARE2_STOP_MASK = TIMER_SHORTS_COMPARE2_STOP_Msk,   ///< Shortcut for stopping the timer based on compare 2.
    NRF_TIMER_SHORT_COMPARE3_STOP_MASK = TIMER_SHORTS_COMPARE3_STOP_Msk,   ///< Shortcut for stopping the timer based on compare 3.
#if defined(TIMER_INTENSET_COMPARE4_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_SHORT_COMPARE4_STOP_MASK = TIMER_SHORTS_COMPARE4_STOP_Msk,   ///< Shortcut for stopping the timer based on compare 4.
#endif
#if defined(TIMER_INTENSET_COMPARE5_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_SHORT_COMPARE5_STOP_MASK = TIMER_SHORTS_COMPARE5_STOP_Msk,   ///< Shortcut for stopping the timer based on compare 5.
#endif
    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK = TIMER_SHORTS_COMPARE0_CLEAR_Msk, ///< Shortcut for clearing the timer based on compare 0.
    NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK = TIMER_SHORTS_COMPARE1_CLEAR_Msk, ///< Shortcut for clearing the timer based on compare 1.
    NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK = TIMER_SHORTS_COMPARE2_CLEAR_Msk, ///< Shortcut for clearing the timer based on compare 2.
    NRF_TIMER_SHORT_COMPARE3_CLEAR_MASK = TIMER_SHORTS_COMPARE3_CLEAR_Msk, ///< Shortcut for clearing the timer based on compare 3.
#if defined(TIMER_INTENSET_COMPARE4_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_SHORT_COMPARE4_CLEAR_MASK = TIMER_SHORTS_COMPARE4_CLEAR_Msk, ///< Shortcut for clearing the timer based on compare 4.
#endif
#if defined(TIMER_INTENSET_COMPARE5_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_SHORT_COMPARE5_CLEAR_MASK = TIMER_SHORTS_COMPARE5_CLEAR_Msk, ///< Shortcut for clearing the timer based on compare 5.
#endif
} nrf_timer_short_mask_t;

/** @brief Timer modes. */
typedef enum
{
    NRF_TIMER_MODE_TIMER             = TIMER_MODE_MODE_Timer,           ///< Timer mode: timer.
    NRF_TIMER_MODE_COUNTER           = TIMER_MODE_MODE_Counter,         ///< Timer mode: counter.
#if defined(TIMER_MODE_MODE_LowPowerCounter) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_MODE_LOW_POWER_COUNTER = TIMER_MODE_MODE_LowPowerCounter, ///< Timer mode: low-power counter.
#endif
} nrf_timer_mode_t;

/** @brief Timer bit width. */
typedef enum
{
    NRF_TIMER_BIT_WIDTH_8  = TIMER_BITMODE_BITMODE_08Bit, ///< Timer bit width 8 bit.
    NRF_TIMER_BIT_WIDTH_16 = TIMER_BITMODE_BITMODE_16Bit, ///< Timer bit width 16 bit.
    NRF_TIMER_BIT_WIDTH_24 = TIMER_BITMODE_BITMODE_24Bit, ///< Timer bit width 24 bit.
    NRF_TIMER_BIT_WIDTH_32 = TIMER_BITMODE_BITMODE_32Bit  ///< Timer bit width 32 bit.
} nrf_timer_bit_width_t;

/** @brief Timer prescalers. */
typedef enum
{
    NRF_TIMER_FREQ_16MHz = 0, ///< Timer frequency 16 MHz.
    NRF_TIMER_FREQ_8MHz,      ///< Timer frequency 8 MHz.
    NRF_TIMER_FREQ_4MHz,      ///< Timer frequency 4 MHz.
    NRF_TIMER_FREQ_2MHz,      ///< Timer frequency 2 MHz.
    NRF_TIMER_FREQ_1MHz,      ///< Timer frequency 1 MHz.
    NRF_TIMER_FREQ_500kHz,    ///< Timer frequency 500 kHz.
    NRF_TIMER_FREQ_250kHz,    ///< Timer frequency 250 kHz.
    NRF_TIMER_FREQ_125kHz,    ///< Timer frequency 125 kHz.
    NRF_TIMER_FREQ_62500Hz,   ///< Timer frequency 62500 Hz.
    NRF_TIMER_FREQ_31250Hz    ///< Timer frequency 31250 Hz.
} nrf_timer_frequency_t;

/** @brief Timer capture/compare channels. */
typedef enum
{
    NRF_TIMER_CC_CHANNEL0 = 0, ///< Timer capture/compare channel 0.
    NRF_TIMER_CC_CHANNEL1,     ///< Timer capture/compare channel 1.
    NRF_TIMER_CC_CHANNEL2,     ///< Timer capture/compare channel 2.
    NRF_TIMER_CC_CHANNEL3,     ///< Timer capture/compare channel 3.
#if defined(TIMER_INTENSET_COMPARE4_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_CC_CHANNEL4,     ///< Timer capture/compare channel 4.
#endif
#if defined(TIMER_INTENSET_COMPARE5_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_CC_CHANNEL5,     ///< Timer capture/compare channel 5.
#endif
} nrf_timer_cc_channel_t;

/** @brief Timer interrupts. */
typedef enum
{
    NRF_TIMER_INT_COMPARE0_MASK = TIMER_INTENSET_COMPARE0_Msk, ///< Timer interrupt from compare event on channel 0.
    NRF_TIMER_INT_COMPARE1_MASK = TIMER_INTENSET_COMPARE1_Msk, ///< Timer interrupt from compare event on channel 1.
    NRF_TIMER_INT_COMPARE2_MASK = TIMER_INTENSET_COMPARE2_Msk, ///< Timer interrupt from compare event on channel 2.
    NRF_TIMER_INT_COMPARE3_MASK = TIMER_INTENSET_COMPARE3_Msk, ///< Timer interrupt from compare event on channel 3.
#if defined(TIMER_INTENSET_COMPARE4_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_INT_COMPARE4_MASK = TIMER_INTENSET_COMPARE4_Msk, ///< Timer interrupt from compare event on channel 4.
#endif
#if defined(TIMER_INTENSET_COMPARE5_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_TIMER_INT_COMPARE5_MASK = TIMER_INTENSET_COMPARE5_Msk, ///< Timer interrupt from compare event on channel 5.
#endif
} nrf_timer_int_mask_t;


/**
 * @brief Function for activating the specified timer task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
__STATIC_INLINE void nrf_timer_task_trigger(NRF_TIMER_Type * p_reg,
                                            nrf_timer_task_t task);

/**
 * @brief Function for getting the address of the specified timer task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  The specified task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t * nrf_timer_task_address_get(NRF_TIMER_Type * p_reg,
                                                      nrf_timer_task_t task);

/**
 * @brief Function for clearing the specified timer event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_timer_event_clear(NRF_TIMER_Type *  p_reg,
                                           nrf_timer_event_t event);

/**
 * @brief Function for retrieving the state of the TIMER event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
__STATIC_INLINE bool nrf_timer_event_check(NRF_TIMER_Type *  p_reg,
                                           nrf_timer_event_t event);

/**
 * @brief Function for getting the address of the specified timer event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event The specified event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t * nrf_timer_event_address_get(NRF_TIMER_Type *  p_reg,
                                                       nrf_timer_event_t event);

/**
 * @brief Function for enabling the specified shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be enabled.
 */
__STATIC_INLINE void nrf_timer_shorts_enable(NRF_TIMER_Type * p_reg,
                                             uint32_t         mask);

/**
 * @brief Function for disabling the specified shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be disabled.
 */
__STATIC_INLINE void nrf_timer_shorts_disable(NRF_TIMER_Type * p_reg,
                                              uint32_t         mask);

/**
 * @brief Function for enabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
__STATIC_INLINE void nrf_timer_int_enable(NRF_TIMER_Type * p_reg,
                                          uint32_t         mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
__STATIC_INLINE void nrf_timer_int_disable(NRF_TIMER_Type * p_reg,
                                           uint32_t         mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] timer_int Interrupt to be checked.
 *
 * @retval true  The interrupt is enabled.
 * @retval false The interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_timer_int_enable_check(NRF_TIMER_Type * p_reg,
                                                uint32_t         timer_int);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        TIMER task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_timer_subscribe_set(NRF_TIMER_Type * p_reg,
                                             nrf_timer_task_t task,
                                             uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        TIMER task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_timer_subscribe_clear(NRF_TIMER_Type * p_reg,
                                               nrf_timer_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        TIMER event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
__STATIC_INLINE void nrf_timer_publish_set(NRF_TIMER_Type *  p_reg,
                                           nrf_timer_event_t event,
                                           uint8_t           channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        TIMER event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
__STATIC_INLINE void nrf_timer_publish_clear(NRF_TIMER_Type *  p_reg,
                                             nrf_timer_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for setting the timer mode.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mode  Timer mode.
 */
__STATIC_INLINE void nrf_timer_mode_set(NRF_TIMER_Type * p_reg,
                                        nrf_timer_mode_t mode);

/**
 * @brief Function for retrieving the timer mode.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Timer mode.
 */
__STATIC_INLINE nrf_timer_mode_t nrf_timer_mode_get(NRF_TIMER_Type * p_reg);

/**
 * @brief Function for setting the timer bit width.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] bit_width Timer bit width.
 */
__STATIC_INLINE void nrf_timer_bit_width_set(NRF_TIMER_Type *      p_reg,
                                             nrf_timer_bit_width_t bit_width);

/**
 * @brief Function for retrieving the timer bit width.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Timer bit width.
 */
__STATIC_INLINE nrf_timer_bit_width_t nrf_timer_bit_width_get(NRF_TIMER_Type * p_reg);

/**
 * @brief Function for setting the timer frequency.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] frequency Timer frequency.
 */
__STATIC_INLINE void nrf_timer_frequency_set(NRF_TIMER_Type *      p_reg,
                                             nrf_timer_frequency_t frequency);

/**
 * @brief Function for retrieving the timer frequency.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Timer frequency.
 */
__STATIC_INLINE nrf_timer_frequency_t nrf_timer_frequency_get(NRF_TIMER_Type * p_reg);

/**
 * @brief Function for writing the capture/compare register for the specified channel.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] cc_channel The specified capture/compare channel.
 * @param[in] cc_value   Value to write to the capture/compare register.
 */
__STATIC_INLINE void nrf_timer_cc_write(NRF_TIMER_Type *       p_reg,
                                        nrf_timer_cc_channel_t cc_channel,
                                        uint32_t               cc_value);

/**
 * @brief Function for retrieving the capture/compare value for a specified channel.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] cc_channel The specified capture/compare channel.
 *
 * @return Value from the specified capture/compare register.
 */
__STATIC_INLINE uint32_t nrf_timer_cc_read(NRF_TIMER_Type *       p_reg,
                                           nrf_timer_cc_channel_t cc_channel);

/**
 * @brief Function for getting the specified timer capture task.
 *
 * @param[in] channel Capture channel.
 *
 * @return Capture task.
 */
__STATIC_INLINE nrf_timer_task_t nrf_timer_capture_task_get(uint32_t channel);

/**
 * @brief Function for getting the specified timer compare event.
 *
 * @param[in] channel Compare channel.
 *
 * @return Compare event.
 */
__STATIC_INLINE nrf_timer_event_t nrf_timer_compare_event_get(uint32_t channel);

/**
 * @brief Function for getting the specified timer compare interrupt.
 *
 * @param[in] channel Compare channel.
 *
 * @return Compare interrupt.
 */
__STATIC_INLINE nrf_timer_int_mask_t nrf_timer_compare_int_get(uint32_t channel);

/**
 * @brief Function for calculating the number of timer ticks for a given time
 *        (in microseconds) and timer frequency.
 *
 * @param[in] time_us   Time in microseconds.
 * @param[in] frequency Timer frequency.
 *
 * @return Number of timer ticks.
 */
__STATIC_INLINE uint32_t nrf_timer_us_to_ticks(uint32_t              time_us,
                                               nrf_timer_frequency_t frequency);

/**
 * @brief Function for calculating the number of timer ticks for a given time
 *        (in milliseconds) and timer frequency.
 *
 * @param[in] time_ms   Time in milliseconds.
 * @param[in] frequency Timer frequency.
 *
 * @return Number of timer ticks.
 */
__STATIC_INLINE uint32_t nrf_timer_ms_to_ticks(uint32_t              time_ms,
                                               nrf_timer_frequency_t frequency);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_timer_task_trigger(NRF_TIMER_Type * p_reg,
                                            nrf_timer_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t * nrf_timer_task_address_get(NRF_TIMER_Type * p_reg,
                                                      nrf_timer_task_t task)
{
    return (uint32_t *)((uint8_t *)p_reg + (uint32_t)task);
}

__STATIC_INLINE void nrf_timer_event_clear(NRF_TIMER_Type * p_reg,
                                           nrf_timer_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_timer_event_check(NRF_TIMER_Type * p_reg,
                                           nrf_timer_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE uint32_t * nrf_timer_event_address_get(NRF_TIMER_Type * p_reg,
                                                       nrf_timer_event_t event)
{
    return (uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE void nrf_timer_shorts_enable(NRF_TIMER_Type * p_reg,
                                             uint32_t mask)
{
    p_reg->SHORTS |= mask;
}

__STATIC_INLINE void nrf_timer_shorts_disable(NRF_TIMER_Type * p_reg,
                                              uint32_t mask)
{
    p_reg->SHORTS &= ~(mask);
}

__STATIC_INLINE void nrf_timer_int_enable(NRF_TIMER_Type * p_reg,
                                          uint32_t mask)
{
    p_reg->INTENSET = mask;
}

__STATIC_INLINE void nrf_timer_int_disable(NRF_TIMER_Type * p_reg,
                                           uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

__STATIC_INLINE bool nrf_timer_int_enable_check(NRF_TIMER_Type * p_reg,
                                                uint32_t timer_int)
{
    return (bool)(p_reg->INTENSET & timer_int);
}

#if defined(DPPI_PRESENT)
__STATIC_INLINE void nrf_timer_subscribe_set(NRF_TIMER_Type * p_reg,
                                             nrf_timer_task_t task,
                                             uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | TIMER_SUBSCRIBE_START_EN_Msk);
}

__STATIC_INLINE void nrf_timer_subscribe_clear(NRF_TIMER_Type * p_reg,
                                               nrf_timer_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_timer_publish_set(NRF_TIMER_Type *  p_reg,
                                           nrf_timer_event_t event,
                                           uint8_t           channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | TIMER_PUBLISH_COMPARE_EN_Msk);
}

__STATIC_INLINE void nrf_timer_publish_clear(NRF_TIMER_Type *  p_reg,
                                             nrf_timer_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

__STATIC_INLINE void nrf_timer_mode_set(NRF_TIMER_Type * p_reg,
                                        nrf_timer_mode_t mode)
{
    p_reg->MODE = (p_reg->MODE & ~TIMER_MODE_MODE_Msk) |
                    ((mode << TIMER_MODE_MODE_Pos) & TIMER_MODE_MODE_Msk);
}

__STATIC_INLINE nrf_timer_mode_t nrf_timer_mode_get(NRF_TIMER_Type * p_reg)
{
    return (nrf_timer_mode_t)(p_reg->MODE);
}

__STATIC_INLINE void nrf_timer_bit_width_set(NRF_TIMER_Type * p_reg,
                                             nrf_timer_bit_width_t bit_width)
{
    p_reg->BITMODE = (p_reg->BITMODE & ~TIMER_BITMODE_BITMODE_Msk) |
                       ((bit_width << TIMER_BITMODE_BITMODE_Pos) &
                            TIMER_BITMODE_BITMODE_Msk);
}

__STATIC_INLINE nrf_timer_bit_width_t nrf_timer_bit_width_get(NRF_TIMER_Type * p_reg)
{
    return (nrf_timer_bit_width_t)(p_reg->BITMODE);
}

__STATIC_INLINE void nrf_timer_frequency_set(NRF_TIMER_Type * p_reg,
                                             nrf_timer_frequency_t frequency)
{
    p_reg->PRESCALER = (p_reg->PRESCALER & ~TIMER_PRESCALER_PRESCALER_Msk) |
                         ((frequency << TIMER_PRESCALER_PRESCALER_Pos) &
                              TIMER_PRESCALER_PRESCALER_Msk);
}

__STATIC_INLINE nrf_timer_frequency_t nrf_timer_frequency_get(NRF_TIMER_Type * p_reg)
{
    return (nrf_timer_frequency_t)(p_reg->PRESCALER);
}

__STATIC_INLINE void nrf_timer_cc_write(NRF_TIMER_Type * p_reg,
                                        nrf_timer_cc_channel_t cc_channel,
                                        uint32_t               cc_value)
{
    p_reg->CC[cc_channel] = cc_value;
}

__STATIC_INLINE uint32_t nrf_timer_cc_read(NRF_TIMER_Type * p_reg,
                                           nrf_timer_cc_channel_t cc_channel)
{
    return (uint32_t)p_reg->CC[cc_channel];
}

__STATIC_INLINE nrf_timer_task_t nrf_timer_capture_task_get(uint32_t channel)
{
    return (nrf_timer_task_t)NRFX_OFFSETOF(NRF_TIMER_Type, TASKS_CAPTURE[channel]);
}

__STATIC_INLINE nrf_timer_event_t nrf_timer_compare_event_get(uint32_t channel)
{
    return (nrf_timer_event_t)NRFX_OFFSETOF(NRF_TIMER_Type, EVENTS_COMPARE[channel]);
}

__STATIC_INLINE nrf_timer_int_mask_t nrf_timer_compare_int_get(uint32_t channel)
{
    return (nrf_timer_int_mask_t)
        ((uint32_t)NRF_TIMER_INT_COMPARE0_MASK << channel);
}

__STATIC_INLINE uint32_t nrf_timer_us_to_ticks(uint32_t              time_us,
                                               nrf_timer_frequency_t frequency)
{
    // The "frequency" parameter here is actually the prescaler value, and the
    // timer runs at the following frequency: f = 16 MHz / 2^prescaler.
    uint32_t prescaler = (uint32_t)frequency;
    uint64_t ticks = ((time_us * 16ULL) >> prescaler);
    NRFX_ASSERT(ticks <= UINT32_MAX);
    return (uint32_t)ticks;
}

__STATIC_INLINE uint32_t nrf_timer_ms_to_ticks(uint32_t              time_ms,
                                               nrf_timer_frequency_t frequency)
{
    // The "frequency" parameter here is actually the prescaler value, and the
    // timer runs at the following frequency: f = 16000 kHz / 2^prescaler.
    uint32_t prescaler = (uint32_t)frequency;
    uint64_t ticks = ((time_ms * 16000ULL) >> prescaler);
    NRFX_ASSERT(ticks <= UINT32_MAX);
    return (uint32_t)ticks;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_TIMER_H__
