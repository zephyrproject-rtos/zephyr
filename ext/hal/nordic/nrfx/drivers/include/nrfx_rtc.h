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

#ifndef NRFX_RTC_H__
#define NRFX_RTC_H__

#include <nrfx.h>
#include <hal/nrf_rtc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_rtc RTC driver
 * @{
 * @ingroup nrf_rtc
 * @brief   Real Timer Counter (RTC) peripheral driver.
 */

/** @brief Macro for converting microseconds into ticks. */
#define NRFX_RTC_US_TO_TICKS(us,freq) (((us) * (freq)) / 1000000U)

/** @brief RTC driver interrupt types. */
typedef enum
{
    NRFX_RTC_INT_COMPARE0 = 0, /**< Interrupt from COMPARE0 event. */
    NRFX_RTC_INT_COMPARE1 = 1, /**< Interrupt from COMPARE1 event. */
    NRFX_RTC_INT_COMPARE2 = 2, /**< Interrupt from COMPARE2 event. */
    NRFX_RTC_INT_COMPARE3 = 3, /**< Interrupt from COMPARE3 event. */
    NRFX_RTC_INT_TICK     = 4, /**< Interrupt from TICK event. */
    NRFX_RTC_INT_OVERFLOW = 5  /**< Interrupt from OVERFLOW event. */
} nrfx_rtc_int_type_t;

/** @brief RTC driver instance structure. */
typedef struct
{
    NRF_RTC_Type  * p_reg;            /**< Pointer to instance register set. */
    IRQn_Type       irq;              /**< Instance IRQ ID. */
    uint8_t         instance_id;      /**< Index of the driver instance. For internal use only. */
    uint8_t         cc_channel_count; /**< Number of capture/compare channels. */
} nrfx_rtc_t;

/** @brief Macro for creating an RTC driver instance. */
#define NRFX_RTC_INSTANCE(id)                                   \
{                                                               \
    .p_reg            = NRFX_CONCAT_2(NRF_RTC, id),             \
    .irq              = NRFX_CONCAT_3(RTC, id, _IRQn),          \
    .instance_id      = NRFX_CONCAT_3(NRFX_RTC, id, _INST_IDX), \
    .cc_channel_count = NRF_RTC_CC_CHANNEL_COUNT(id),           \
}

#ifndef __NRFX_DOXYGEN__
enum {
#if NRFX_CHECK(NRFX_RTC0_ENABLED)
    NRFX_RTC0_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_RTC1_ENABLED)
    NRFX_RTC1_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_RTC2_ENABLED)
    NRFX_RTC2_INST_IDX,
#endif
    NRFX_RTC_ENABLED_COUNT
};
#endif

/** @brief RTC driver instance configuration structure. */
typedef struct
{
    uint16_t prescaler;          /**< Prescaler. */
    uint8_t  interrupt_priority; /**< Interrupt priority. */
    uint8_t  tick_latency;       /**< Maximum length of the interrupt handler in ticks (maximum 7.7 ms). */
    bool     reliable;           /**< Reliable mode flag. */
} nrfx_rtc_config_t;

/** @brief RTC instance default configuration. */
#define NRFX_RTC_DEFAULT_CONFIG                                                     \
{                                                                                   \
    .prescaler          = RTC_FREQ_TO_PRESCALER(NRFX_RTC_DEFAULT_CONFIG_FREQUENCY), \
    .interrupt_priority = NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY,                     \
    .tick_latency       = NRFX_RTC_US_TO_TICKS(NRFX_RTC_MAXIMUM_LATENCY_US,         \
                                               NRFX_RTC_DEFAULT_CONFIG_FREQUENCY),  \
    .reliable           = NRFX_RTC_DEFAULT_CONFIG_RELIABLE,                         \
}

/** @brief RTC driver instance handler type. */
typedef void (*nrfx_rtc_handler_t)(nrfx_rtc_int_type_t int_type);

/**
 * @brief Function for initializing the RTC driver instance.
 *
 * After initialization, the instance is in power off state.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_config   Pointer to the structure with the initial configuration.
 * @param[in] handler    Event handler provided by the user.
 *                       Must not be NULL.
 *
 * @retval NRFX_SUCCESS             Successfully initialized.
 * @retval NRFX_ERROR_INVALID_STATE The instance is already initialized.
 */
nrfx_err_t nrfx_rtc_init(nrfx_rtc_t const * const  p_instance,
                         nrfx_rtc_config_t const * p_config,
                         nrfx_rtc_handler_t        handler);

/**
 * @brief Function for uninitializing the RTC driver instance.
 *
 * After uninitialization, the instance is in idle state. The hardware should return to the state
 * before initialization.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_rtc_uninit(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for enabling the RTC driver instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_rtc_enable(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for disabling the RTC driver instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_rtc_disable(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for setting a compare channel.
 *
 * The function powers on the instance if the instance was in power off state.
 *
 * The driver is not entering a critical section when configuring RTC, which means that it can be
 * preempted for a certain amount of time. When the driver was preempted and the value to be set
 * is short in time, there is a risk that the driver sets a compare value that is
 * behind. In this case, if the reliable mode is enabled for the specified instance,
 * the risk is handled.
 * However, to detect if the requested value is behind, this mode makes the following assumptions:
 *  -  The maximum preemption time in ticks (8-bit value) is known and is less than 7.7 ms
 *   (for prescaler = 0, RTC frequency 32 kHz).
 *  -  The requested absolute compare value is not bigger than (0x00FFFFFF)-tick_latency. It is
 *   the user's responsibility to ensure this.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] channel    One of the channels of the instance.
 * @param[in] val        Absolute value to be set in the compare register.
 * @param[in] enable_irq True to enable the interrupt. False to disable the interrupt.
 *
 * @retval NRFX_SUCCESS       The procedure is successful.
 * @retval NRFX_ERROR_TIMEOUT The compare is not set because the request value is behind the
 *                            current counter value. This error can only be reported
 *                            if the reliable mode is enabled.
 */
nrfx_err_t nrfx_rtc_cc_set(nrfx_rtc_t const * const p_instance,
                           uint32_t                 channel,
                           uint32_t                 val,
                           bool                     enable_irq);

/**
 * @brief Function for disabling a channel.
 *
 * This function disables channel events and channel interrupts.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] channel    One of the channels of the instance.
 *
 * @retval NRFX_SUCCESS       The procedure is successful.
 * @retval NRFX_ERROR_TIMEOUT Interrupt is pending on the requested channel.
 */
nrfx_err_t nrfx_rtc_cc_disable(nrfx_rtc_t const * const p_instance, uint32_t channel);

/**
 * @brief Function for enabling the TICK event.
 *
 * This function enables the tick event and optionally the interrupt.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] enable_irq True to enable the interrupt. False to disable the interrupt.
 */
void nrfx_rtc_tick_enable(nrfx_rtc_t const * const p_instance, bool enable_irq);

/**
 * @brief Function for disabling the TICK event.
 *
 * This function disables the TICK event and interrupt.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_rtc_tick_disable(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for enabling overflow.
 *
 * This function enables the overflow event and optionally the interrupt.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] enable_irq True to enable the interrupt. False to disable the interrupt.
 */
void nrfx_rtc_overflow_enable(nrfx_rtc_t const * const p_instance, bool enable_irq);

/**
 * @brief Function for disabling overflow.
 *
 * This function disables the overflow event and interrupt.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_rtc_overflow_disable(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for getting the maximum relative tick value that can be set in the compare channel.
 *
 * When a stack (for example SoftDevice) is used and it occupies high priority interrupts,
 * the application code can be interrupted at any moment for a certain period of time.
 * If the reliable mode is enabled, the provided maximum latency is taken into account
 * and the return value is smaller than the RTC counter resolution.
 * If the reliable mode is disabled, the return value equals the counter resolution.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return Maximum ticks value.
 */
uint32_t nrfx_rtc_max_ticks_get(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for disabling all instance interrupts.
  *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_mask     Pointer to the location where the mask is filled.
 */
__STATIC_INLINE void nrfx_rtc_int_disable(nrfx_rtc_t const * const p_instance,
                                          uint32_t               * p_mask);

/**
 * @brief Function for enabling instance interrupts.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] mask       Mask of interrupts to enable.
 */
__STATIC_INLINE void nrfx_rtc_int_enable(nrfx_rtc_t const * const p_instance, uint32_t mask);

/**
 * @brief Function for retrieving the current counter value.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return Counter value.
 */
__STATIC_INLINE uint32_t nrfx_rtc_counter_get(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for clearing the counter value.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
__STATIC_INLINE void nrfx_rtc_counter_clear(nrfx_rtc_t const * const p_instance);

/**
 * @brief Function for returning a requested task address for the RTC driver instance.
 *
 * The task address can be used by the PPI module.
 *
 * @param[in] p_instance Pointer to the instance.
 * @param[in] task       One of the peripheral tasks.
 *
 * @return Address of task register.
 */
__STATIC_INLINE uint32_t nrfx_rtc_task_address_get(nrfx_rtc_t const * const p_instance,
                                                   nrf_rtc_task_t           task);

/**
 * @brief Function for returning a requested event address for the RTC driver instance.
 *
 * The event address can be used by the PPI module.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] event      One of the peripheral events.
 *
 * @return Address of event register.
 */
__STATIC_INLINE uint32_t nrfx_rtc_event_address_get(nrfx_rtc_t const * const p_instance,
                                                    nrf_rtc_event_t          event);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrfx_rtc_int_disable(nrfx_rtc_t const * const p_instance,
                                          uint32_t               * p_mask)
{
    *p_mask = nrf_rtc_int_get(p_instance->p_reg);
    nrf_rtc_int_disable(p_instance->p_reg, NRF_RTC_INT_TICK_MASK |
                                           NRF_RTC_INT_OVERFLOW_MASK |
                                           NRF_RTC_INT_COMPARE0_MASK |
                                           NRF_RTC_INT_COMPARE1_MASK |
                                           NRF_RTC_INT_COMPARE2_MASK |
                                           NRF_RTC_INT_COMPARE3_MASK);
}

__STATIC_INLINE void nrfx_rtc_int_enable(nrfx_rtc_t const * const p_instance, uint32_t mask)
{
    nrf_rtc_int_enable(p_instance->p_reg, mask);
}

__STATIC_INLINE uint32_t nrfx_rtc_counter_get(nrfx_rtc_t const * const p_instance)
{
    return nrf_rtc_counter_get(p_instance->p_reg);
}

__STATIC_INLINE void nrfx_rtc_counter_clear(nrfx_rtc_t const * const p_instance)
{
    nrf_rtc_task_trigger(p_instance->p_reg, NRF_RTC_TASK_CLEAR);
}

__STATIC_INLINE uint32_t nrfx_rtc_task_address_get(nrfx_rtc_t const * const p_instance,
                                                   nrf_rtc_task_t           task)
{
    return nrf_rtc_task_address_get(p_instance->p_reg, task);
}

__STATIC_INLINE uint32_t nrfx_rtc_event_address_get(nrfx_rtc_t const * const p_instance,
                                                    nrf_rtc_event_t          event)
{
    return nrf_rtc_event_address_get(p_instance->p_reg, event);
}
#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */


void nrfx_rtc_0_irq_handler(void);
void nrfx_rtc_1_irq_handler(void);
void nrfx_rtc_2_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif // NRFX_RTC_H__
