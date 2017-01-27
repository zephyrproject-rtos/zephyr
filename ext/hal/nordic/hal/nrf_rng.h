/* Copyright (c) 2014-2017 Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 * @brief RNG HAL API.
 */

#ifndef NRF_RNG_H__
#define NRF_RNG_H__
/**
 * @defgroup nrf_rng_hal RNG HAL
 * @{
 * @ingroup nrf_rng
 * @brief Hardware access layer for managing the random number generator (RNG).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "nrf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_RNG_TASK_SET    (1UL)
#define NRF_RNG_EVENT_CLEAR (0UL)
/**
 * @enum nrf_rng_task_t
 * @brief RNG tasks.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_RNG_TASK_START = offsetof(NRF_RNG_Type, TASKS_START), /**< Start the random number generator. */
    NRF_RNG_TASK_STOP  = offsetof(NRF_RNG_Type, TASKS_STOP)   /**< Stop the random number generator. */
} nrf_rng_task_t;                                             /*lint -restore */

/**
 * @enum nrf_rng_event_t
 * @brief RNG events.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_RNG_EVENT_VALRDY = offsetof(NRF_RNG_Type, EVENTS_VALRDY) /**< New random number generated event. */
} nrf_rng_event_t;                                               /*lint -restore */

/**
 * @enum nrf_rng_int_mask_t
 * @brief RNG interrupts.
 */
typedef enum
{
    NRF_RNG_INT_VALRDY_MASK = RNG_INTENSET_VALRDY_Msk /**< Mask for enabling or disabling an interrupt on VALRDY event.  */
} nrf_rng_int_mask_t;

/**
 * @enum nrf_rng_short_mask_t
 * @brief Types of RNG shortcuts.
 */
typedef enum
{
    NRF_RNG_SHORT_VALRDY_STOP_MASK = RNG_SHORTS_VALRDY_STOP_Msk /**<  Mask for setting shortcut between EVENT_VALRDY and TASK_STOP. */
} nrf_rng_short_mask_t;

/**
 * @brief Function for enabling interrupts.
 *
 * @param[in]  rng_int_mask              Mask of interrupts.
 */
__STATIC_INLINE void nrf_rng_int_enable(uint32_t rng_int_mask)
{
    NRF_RNG->INTENSET = rng_int_mask;
}

/**
 * @brief Function for disabling interrupts.
 *
 * @param[in]  rng_int_mask              Mask of interrupts.
 */
__STATIC_INLINE void nrf_rng_int_disable(uint32_t rng_int_mask)
{
    NRF_RNG->INTENCLR = rng_int_mask;
}

/**
 * @brief Function for getting the state of a specific interrupt.
 *
 * @param[in]  rng_int_mask              Interrupt.
 *
 * @retval     true                   If the interrupt is not enabled.
 * @retval     false                  If the interrupt is enabled.
 */
__STATIC_INLINE bool nrf_rng_int_get(nrf_rng_int_mask_t rng_int_mask)
{
    return (bool)(NRF_RNG->INTENCLR & rng_int_mask);
}

/**
 * @brief Function for getting the address of a specific task.
 *
 * This function can be used by the PPI module.
 *
 * @param[in]  rng_task              Task.
 */
__STATIC_INLINE uint32_t * nrf_rng_task_address_get(nrf_rng_task_t rng_task)
{
    return (uint32_t *)((uint8_t *)NRF_RNG + rng_task);
}

/**
 * @brief Function for setting a specific task.
 *
 * @param[in]  rng_task              Task.
 */
__STATIC_INLINE void nrf_rng_task_trigger(nrf_rng_task_t rng_task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_RNG + rng_task)) = NRF_RNG_TASK_SET;
}

/**
 * @brief Function for getting address of a specific event.
 *
 * This function can be used by the PPI module.
 *
 * @param[in]  rng_event              Event.
 */
__STATIC_INLINE uint32_t * nrf_rng_event_address_get(nrf_rng_event_t rng_event)
{
    return (uint32_t *)((uint8_t *)NRF_RNG + rng_event);
}

/**
 * @brief Function for clearing a specific event.
 *
 * @param[in]  rng_event              Event.
 */
__STATIC_INLINE void nrf_rng_event_clear(nrf_rng_event_t rng_event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_RNG + rng_event)) = NRF_RNG_EVENT_CLEAR;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_RNG + rng_event));
    (void)dummy;
#endif
}

/**
 * @brief Function for getting the state of a specific event.
 *
 * @param[in]  rng_event              Event.
 *
 * @retval     true               If the event is not set.
 * @retval     false              If the event is set.
 */
__STATIC_INLINE bool nrf_rng_event_get(nrf_rng_event_t rng_event)
{
    return (bool) * ((volatile uint32_t *)((uint8_t *)NRF_RNG + rng_event));
}

/**
 * @brief Function for setting shortcuts.
 *
 * @param[in]  rng_short_mask              Mask of shortcuts.
 *
 */
__STATIC_INLINE void nrf_rng_shorts_enable(uint32_t rng_short_mask)
{
     NRF_RNG->SHORTS |= rng_short_mask;
}

/**
 * @brief Function for clearing shortcuts.
 *
 * @param[in]  rng_short_mask              Mask of shortcuts.
 *
 */
__STATIC_INLINE void nrf_rng_shorts_disable(uint32_t rng_short_mask)
{
     NRF_RNG->SHORTS &= ~rng_short_mask;
}

/**
 * @brief Function for getting the previously generated random value.
 *
 * @return     Previously generated random value.
 */
__STATIC_INLINE uint8_t nrf_rng_random_value_get(void)
{
    return (uint8_t)(NRF_RNG->VALUE & RNG_VALUE_VALUE_Msk);
}

/**
 * @brief Function for enabling digital error correction.
 */
__STATIC_INLINE void nrf_rng_error_correction_enable(void)
{
    NRF_RNG->CONFIG |= RNG_CONFIG_DERCEN_Msk;
}

/**
 * @brief Function for disabling digital error correction.
 */
__STATIC_INLINE void nrf_rng_error_correction_disable(void)
{
    NRF_RNG->CONFIG &= ~RNG_CONFIG_DERCEN_Msk;
}
/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_RNG_H__ */
