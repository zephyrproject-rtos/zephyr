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

#ifndef NRF_RTC_H
#define NRF_RTC_H

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_rtc_hal RTC HAL
 * @{
 * @ingroup nrf_rtc
 * @brief   Hardware access layer for managing the Real Time Counter (RTC) peripheral.
 */

/** @brief Macro for getting the number of compare channels available in a given RTC instance. */
#define NRF_RTC_CC_CHANNEL_COUNT(id)  NRFX_CONCAT_3(RTC, id, _CC_NUM)

/** @brief Input frequency of the RTC instance. */
#define RTC_INPUT_FREQ 32768

/** @brief Macro for converting expected frequency to prescaler setting. */
#define RTC_FREQ_TO_PRESCALER(FREQ) (uint16_t)(((RTC_INPUT_FREQ) / (FREQ)) - 1)

/** @brief Macro for trimming values to the RTC bit width. */
#define RTC_WRAP(val) ((val) & RTC_COUNTER_COUNTER_Msk)

/** @brief Macro for creating the interrupt bitmask for the specified compare channel. */
#define RTC_CHANNEL_INT_MASK(ch)    ((uint32_t)(NRF_RTC_INT_COMPARE0_MASK) << (ch))

/** @brief Macro for obtaining the compare event for the specified channel. */
#define RTC_CHANNEL_EVENT_ADDR(ch)  (nrf_rtc_event_t)((NRF_RTC_EVENT_COMPARE_0) + (ch) * sizeof(uint32_t))


/** @brief RTC tasks. */
typedef enum
{
    NRF_RTC_TASK_START            = offsetof(NRF_RTC_Type,TASKS_START),     /**< Start. */
    NRF_RTC_TASK_STOP             = offsetof(NRF_RTC_Type,TASKS_STOP),      /**< Stop. */
    NRF_RTC_TASK_CLEAR            = offsetof(NRF_RTC_Type,TASKS_CLEAR),     /**< Clear. */
    NRF_RTC_TASK_TRIGGER_OVERFLOW = offsetof(NRF_RTC_Type,TASKS_TRIGOVRFLW),/**< Trigger overflow. */
} nrf_rtc_task_t;

/** @brief RTC events. */
typedef enum
{
    NRF_RTC_EVENT_TICK        = offsetof(NRF_RTC_Type,EVENTS_TICK),       /**< Tick event. */
    NRF_RTC_EVENT_OVERFLOW    = offsetof(NRF_RTC_Type,EVENTS_OVRFLW),     /**< Overflow event. */
    NRF_RTC_EVENT_COMPARE_0   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[0]), /**< Compare 0 event. */
    NRF_RTC_EVENT_COMPARE_1   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[1]), /**< Compare 1 event. */
    NRF_RTC_EVENT_COMPARE_2   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[2]), /**< Compare 2 event. */
    NRF_RTC_EVENT_COMPARE_3   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[3])  /**< Compare 3 event. */
} nrf_rtc_event_t;

/** @brief RTC interrupts. */
typedef enum
{
    NRF_RTC_INT_TICK_MASK     = RTC_INTENSET_TICK_Msk,     /**< RTC interrupt from tick event. */
    NRF_RTC_INT_OVERFLOW_MASK = RTC_INTENSET_OVRFLW_Msk,   /**< RTC interrupt from overflow event. */
    NRF_RTC_INT_COMPARE0_MASK = RTC_INTENSET_COMPARE0_Msk, /**< RTC interrupt from compare event on channel 0. */
    NRF_RTC_INT_COMPARE1_MASK = RTC_INTENSET_COMPARE1_Msk, /**< RTC interrupt from compare event on channel 1. */
    NRF_RTC_INT_COMPARE2_MASK = RTC_INTENSET_COMPARE2_Msk, /**< RTC interrupt from compare event on channel 2. */
    NRF_RTC_INT_COMPARE3_MASK = RTC_INTENSET_COMPARE3_Msk  /**< RTC interrupt from compare event on channel 3. */
} nrf_rtc_int_t;


/**
 * @brief Function for setting a compare value for a channel.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] ch     Channel.
 * @param[in] cc_val Compare value to be set.
 */
__STATIC_INLINE  void nrf_rtc_cc_set(NRF_RTC_Type * p_reg, uint32_t ch, uint32_t cc_val);

/**
 * @brief Function for returning the compare value for a channel.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] ch    Channel.
 *
 * @return COMPARE[ch] value.
 */
__STATIC_INLINE  uint32_t nrf_rtc_cc_get(NRF_RTC_Type * p_reg, uint32_t ch);

/**
 * @brief Function for enabling interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Interrupt mask to be enabled.
 */
__STATIC_INLINE void nrf_rtc_int_enable(NRF_RTC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Interrupt mask to be disabled.
 */
__STATIC_INLINE void nrf_rtc_int_disable(NRF_RTC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupt flags to be checked.
 *
 * @return Mask with enabled interrupts.
 */
__STATIC_INLINE uint32_t nrf_rtc_int_is_enabled(NRF_RTC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for returning the status of currently enabled interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Value in INTEN register.
 */
__STATIC_INLINE uint32_t nrf_rtc_int_get(NRF_RTC_Type * p_reg);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        RTC task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_rtc_subscribe_set(NRF_RTC_Type * p_reg,
                                           nrf_rtc_task_t task,
                                           uint8_t        channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        RTC task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_rtc_subscribe_clear(NRF_RTC_Type * p_reg,
                                             nrf_rtc_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        RTC event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
__STATIC_INLINE void nrf_rtc_publish_set(NRF_RTC_Type *  p_reg,
                                         nrf_rtc_event_t event,
                                         uint8_t         channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        RTC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
__STATIC_INLINE void nrf_rtc_publish_clear(NRF_RTC_Type *  p_reg,
                                           nrf_rtc_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for checking if an event is pending.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Address of the event.
 *
 * @return Mask of pending events.
 */
__STATIC_INLINE uint32_t nrf_rtc_event_pending(NRF_RTC_Type * p_reg, nrf_rtc_event_t event);

/**
 * @brief Function for clearing an event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be cleared.
 */
__STATIC_INLINE void nrf_rtc_event_clear(NRF_RTC_Type * p_reg, nrf_rtc_event_t event);

/**
 * @brief Function for returning a counter value.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Counter value.
 */
__STATIC_INLINE uint32_t nrf_rtc_counter_get(NRF_RTC_Type * p_reg);

/**
 * @brief Function for setting a prescaler value.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] val   Value to set the prescaler to.
 */
__STATIC_INLINE void nrf_rtc_prescaler_set(NRF_RTC_Type * p_reg, uint32_t val);

/**
 * @brief Function for returning the address of an event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Requested event.
 *
 * @return Address of the requested event register.
 */
__STATIC_INLINE uint32_t nrf_rtc_event_address_get(NRF_RTC_Type * p_reg, nrf_rtc_event_t event);

/**
 * @brief Function for returning the address of a task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Requested task.
 *
 * @return Address of the requested task register.
 */
__STATIC_INLINE uint32_t nrf_rtc_task_address_get(NRF_RTC_Type * p_reg, nrf_rtc_task_t task);

/**
 * @brief Function for starting a task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Requested task.
 */
__STATIC_INLINE void nrf_rtc_task_trigger(NRF_RTC_Type * p_reg, nrf_rtc_task_t task);

/**
 * @brief Function for enabling events.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of event flags to be enabled.
 */
__STATIC_INLINE void nrf_rtc_event_enable(NRF_RTC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling an event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Requested event.
 */
__STATIC_INLINE void nrf_rtc_event_disable(NRF_RTC_Type * p_reg, uint32_t event);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE  void nrf_rtc_cc_set(NRF_RTC_Type * p_reg, uint32_t ch, uint32_t cc_val)
{
    p_reg->CC[ch] = cc_val;
}

__STATIC_INLINE  uint32_t nrf_rtc_cc_get(NRF_RTC_Type * p_reg, uint32_t ch)
{
    return p_reg->CC[ch];
}

__STATIC_INLINE void nrf_rtc_int_enable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

__STATIC_INLINE void nrf_rtc_int_disable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

__STATIC_INLINE uint32_t nrf_rtc_int_is_enabled(NRF_RTC_Type * p_reg, uint32_t mask)
{
    return (p_reg->INTENSET & mask);
}

__STATIC_INLINE uint32_t nrf_rtc_int_get(NRF_RTC_Type * p_reg)
{
    return p_reg->INTENSET;
}

#if defined(DPPI_PRESENT)
__STATIC_INLINE void nrf_rtc_subscribe_set(NRF_RTC_Type * p_reg,
                                           nrf_rtc_task_t task,
                                           uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | RTC_SUBSCRIBE_START_EN_Msk);
}

__STATIC_INLINE void nrf_rtc_subscribe_clear(NRF_RTC_Type * p_reg,
                                             nrf_rtc_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_rtc_publish_set(NRF_RTC_Type *  p_reg,
                                         nrf_rtc_event_t event,
                                         uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | RTC_PUBLISH_TICK_EN_Msk);
}

__STATIC_INLINE void nrf_rtc_publish_clear(NRF_RTC_Type *  p_reg,
                                           nrf_rtc_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

__STATIC_INLINE uint32_t nrf_rtc_event_pending(NRF_RTC_Type * p_reg, nrf_rtc_event_t event)
{
    return *(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE void nrf_rtc_event_clear(NRF_RTC_Type * p_reg, nrf_rtc_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE uint32_t nrf_rtc_counter_get(NRF_RTC_Type * p_reg)
{
     return p_reg->COUNTER;
}

__STATIC_INLINE void nrf_rtc_prescaler_set(NRF_RTC_Type * p_reg, uint32_t val)
{
    NRFX_ASSERT(val <= (RTC_PRESCALER_PRESCALER_Msk >> RTC_PRESCALER_PRESCALER_Pos));
    p_reg->PRESCALER = val;
}
__STATIC_INLINE uint32_t rtc_prescaler_get(NRF_RTC_Type * p_reg)
{
    return p_reg->PRESCALER;
}

__STATIC_INLINE uint32_t nrf_rtc_event_address_get(NRF_RTC_Type * p_reg, nrf_rtc_event_t event)
{
    return (uint32_t)p_reg + event;
}

__STATIC_INLINE uint32_t nrf_rtc_task_address_get(NRF_RTC_Type * p_reg, nrf_rtc_task_t task)
{
    return (uint32_t)p_reg + task;
}

__STATIC_INLINE void nrf_rtc_task_trigger(NRF_RTC_Type * p_reg, nrf_rtc_task_t task)
{
    *(__IO uint32_t *)((uint32_t)p_reg + task) = 1;
}

__STATIC_INLINE void nrf_rtc_event_enable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->EVTENSET = mask;
}
__STATIC_INLINE void nrf_rtc_event_disable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->EVTENCLR = mask;
}
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* NRF_RTC_H */
