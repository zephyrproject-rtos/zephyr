/*
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_WDT_H__
#define NRF_WDT_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_wdt_hal WDT HAL
 * @{
 * @ingroup nrf_wdt
 * @brief   Hardware access layer for managing the Watchdog Timer (WDT) peripheral.
 */

/** @brief Number of WDT channels. */
#define NRF_WDT_CHANNEL_NUMBER 0x8UL

/** @brief WDT register reload value. */
#define NRF_WDT_RR_VALUE       0x6E524635UL /* Fixed value; should not be modified. */


/** @brief WDT tasks. */
typedef enum
{
    NRF_WDT_TASK_START = offsetof(NRF_WDT_Type, TASKS_START), /**< Task for starting WDT. */
} nrf_wdt_task_t;

/** @brief WDT events. */
typedef enum
{
    NRF_WDT_EVENT_TIMEOUT = offsetof(NRF_WDT_Type, EVENTS_TIMEOUT), /**< Event from WDT time-out. */
} nrf_wdt_event_t;

/** @brief WDT behavior in the SLEEP or HALT CPU modes. */
typedef enum
{
    NRF_WDT_BEHAVIOUR_RUN_SLEEP        = WDT_CONFIG_SLEEP_Msk,                       /**< WDT will run when CPU is in SLEEP mode. */
    NRF_WDT_BEHAVIOUR_RUN_HALT         = WDT_CONFIG_HALT_Msk,                        /**< WDT will run when CPU is in HALT mode. */
    NRF_WDT_BEHAVIOUR_RUN_SLEEP_HALT   = WDT_CONFIG_SLEEP_Msk | WDT_CONFIG_HALT_Msk, /**< WDT will run when CPU is in SLEEP or HALT mode. */
    NRF_WDT_BEHAVIOUR_PAUSE_SLEEP_HALT = 0,                                          /**< WDT will be paused when CPU is in SLEEP or HALT mode. */
} nrf_wdt_behaviour_t;

/** @brief WDT reload request registers. */
typedef enum
{
    NRF_WDT_RR0 = 0, /**< Reload request register 0. */
    NRF_WDT_RR1,     /**< Reload request register 1. */
    NRF_WDT_RR2,     /**< Reload request register 2. */
    NRF_WDT_RR3,     /**< Reload request register 3. */
    NRF_WDT_RR4,     /**< Reload request register 4. */
    NRF_WDT_RR5,     /**< Reload request register 5. */
    NRF_WDT_RR6,     /**< Reload request register 6. */
    NRF_WDT_RR7      /**< Reload request register 7. */
} nrf_wdt_rr_register_t;

/** @brief WDT interrupts. */
typedef enum
{
    NRF_WDT_INT_TIMEOUT_MASK = WDT_INTENSET_TIMEOUT_Msk, /**< WDT interrupt from time-out event. */
} nrf_wdt_int_mask_t;


/**
 * @brief Function for configuring the watchdog behavior when the CPU is sleeping or halted.
 *
 * @param behaviour Watchdog behavior when CPU is in SLEEP or HALT mode.
 */
__STATIC_INLINE void nrf_wdt_behaviour_set(nrf_wdt_behaviour_t behaviour);

/**
 * @brief Function for starting the WDT task.
 *
 * @param[in] task Task.
 */
__STATIC_INLINE void nrf_wdt_task_trigger(nrf_wdt_task_t task);

/**
 * @brief Function for clearing the WDT event register.
 *
 * @param[in] event Event.
 */
__STATIC_INLINE void nrf_wdt_event_clear(nrf_wdt_event_t event);

/**
 * @brief Function for retrieving the state of the WDT event.
 *
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
__STATIC_INLINE bool nrf_wdt_event_check(nrf_wdt_event_t event);

/**
 * @brief Function for enabling the specified interrupt.
 *
 * @param[in] int_mask Interrupt.
 */
__STATIC_INLINE void nrf_wdt_int_enable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of given interrupt.
 *
 * @param[in] int_mask Interrupt.
 *
 * @retval true  Interrupt is enabled.
 * @retval false Interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_wdt_int_enable_check(uint32_t int_mask);

/**
 * @brief Function for disabling a specific interrupt.
 *
 * @param[in] int_mask Interrupt.
 */
__STATIC_INLINE void nrf_wdt_int_disable(uint32_t int_mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        WDT task.
 *
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_wdt_subscribe_set(nrf_wdt_task_t task,
                                           uint8_t        channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        WDT task.
 *
 * @param[in] task Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_wdt_subscribe_clear(nrf_wdt_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        WDT event.
 *
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
__STATIC_INLINE void nrf_wdt_publish_set(nrf_wdt_event_t event,
                                         uint8_t         channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        WDT event.
 *
 * @param[in] event Event for which to clear the configuration.
 */
__STATIC_INLINE void nrf_wdt_publish_clear(nrf_wdt_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for returning the address of a specific WDT task register.
 *
 * @param[in] task Task.
 */
__STATIC_INLINE uint32_t nrf_wdt_task_address_get(nrf_wdt_task_t task);

/**
 * @brief Function for returning the address of a specific WDT event register.
 *
 * @param[in] event Event.
 *
 * @return Address of requested event register
 */
__STATIC_INLINE uint32_t nrf_wdt_event_address_get(nrf_wdt_event_t event);

/**
 * @brief Function for retrieving the watchdog status.
 *
 * @retval true  The watchdog is started.
 * @retval false The watchdog is not started.
 */
__STATIC_INLINE bool nrf_wdt_started(void);

/**
 * @brief Function for retrieving the watchdog reload request status.
 *
 * @param[in] rr_register Reload request register to be checked.
 *
 * @retval true  Reload request is running.
 * @retval false No reload requests are running.
 */
__STATIC_INLINE bool nrf_wdt_request_status(nrf_wdt_rr_register_t rr_register);

/**
 * @brief Function for setting the watchdog reload value.
 *
 * @param[in] reload_value Watchdog counter initial value.
 */
__STATIC_INLINE void nrf_wdt_reload_value_set(uint32_t reload_value);

/**
 * @brief Function for retrieving the watchdog reload value.
 *
 * @return Reload value.
 */
__STATIC_INLINE uint32_t nrf_wdt_reload_value_get(void);

/**
 * @brief Function for enabling a specific reload request register.
 *
 * @param[in] rr_register Reload request register to be enabled.
 */
__STATIC_INLINE void nrf_wdt_reload_request_enable(nrf_wdt_rr_register_t rr_register);

/**
 * @brief Function for disabling a specific reload request register.
 *
 * @param[in] rr_register Reload request register to be disabled.
 */
__STATIC_INLINE void nrf_wdt_reload_request_disable(nrf_wdt_rr_register_t rr_register);

/**
 * @brief Function for retrieving the status of a specific reload request register.
 *
 * @param[in] rr_register Reload request register to be checked.
 *
 * @retval true  The reload request register is enabled.
 * @retval false The reload request register is not enabled.
 */
__STATIC_INLINE bool nrf_wdt_reload_request_is_enabled(nrf_wdt_rr_register_t rr_register);

/**
 * @brief Function for setting a specific reload request register.
 *
 * @param[in]  rr_register       Reload request register to set.
 */
__STATIC_INLINE void nrf_wdt_reload_request_set(nrf_wdt_rr_register_t rr_register);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_wdt_behaviour_set(nrf_wdt_behaviour_t behaviour)
{
    NRF_WDT->CONFIG = behaviour;
}

__STATIC_INLINE void nrf_wdt_task_trigger(nrf_wdt_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_WDT + (uint32_t)task)) = 0x01UL;
}

__STATIC_INLINE void nrf_wdt_event_clear(nrf_wdt_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_WDT + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_WDT + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_wdt_event_check(nrf_wdt_event_t event)
{
    return (bool)*((volatile uint32_t *)((uint8_t *)NRF_WDT + (uint32_t)event));
}

__STATIC_INLINE void nrf_wdt_int_enable(uint32_t int_mask)
{
    NRF_WDT->INTENSET = int_mask;
}

__STATIC_INLINE bool nrf_wdt_int_enable_check(uint32_t int_mask)
{
    return (bool)(NRF_WDT->INTENSET & int_mask);
}

__STATIC_INLINE void nrf_wdt_int_disable(uint32_t int_mask)
{
    NRF_WDT->INTENCLR = int_mask;
}

#if defined(DPPI_PRESENT)
__STATIC_INLINE void nrf_wdt_subscribe_set(nrf_wdt_task_t task,
                                           uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_WDT + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | WDT_SUBSCRIBE_START_EN_Msk);
}

__STATIC_INLINE void nrf_wdt_subscribe_clear(nrf_wdt_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_WDT + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_wdt_publish_set(nrf_wdt_event_t event,
                                         uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_WDT + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | WDT_PUBLISH_TIMEOUT_EN_Msk);
}

__STATIC_INLINE void nrf_wdt_publish_clear(nrf_wdt_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_WDT + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

__STATIC_INLINE uint32_t nrf_wdt_task_address_get(nrf_wdt_task_t task)
{
    return ((uint32_t)NRF_WDT + (uint32_t)task);
}

__STATIC_INLINE uint32_t nrf_wdt_event_address_get(nrf_wdt_event_t event)
{
    return ((uint32_t)NRF_WDT + (uint32_t)event);
}

__STATIC_INLINE bool nrf_wdt_started(void)
{
    return (bool)(NRF_WDT->RUNSTATUS);
}

__STATIC_INLINE bool nrf_wdt_request_status(nrf_wdt_rr_register_t rr_register)
{
    return (bool)(((NRF_WDT->REQSTATUS) >> rr_register) & 0x1UL);
}

__STATIC_INLINE void nrf_wdt_reload_value_set(uint32_t reload_value)
{
    NRF_WDT->CRV = reload_value;
}

__STATIC_INLINE uint32_t nrf_wdt_reload_value_get(void)
{
    return (uint32_t)NRF_WDT->CRV;
}

__STATIC_INLINE void nrf_wdt_reload_request_enable(nrf_wdt_rr_register_t rr_register)
{
    NRF_WDT->RREN |= 0x1UL << rr_register;
}

__STATIC_INLINE void nrf_wdt_reload_request_disable(nrf_wdt_rr_register_t rr_register)
{
    NRF_WDT->RREN &= ~(0x1UL << rr_register);
}

__STATIC_INLINE bool nrf_wdt_reload_request_is_enabled(nrf_wdt_rr_register_t rr_register)
{
    return (bool)(NRF_WDT->RREN & (0x1UL << rr_register));
}

__STATIC_INLINE void nrf_wdt_reload_request_set(nrf_wdt_rr_register_t rr_register)
{
    NRF_WDT->RR[rr_register] = NRF_WDT_RR_VALUE;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_WDT_H__
