/*
 * Copyright (c) 2018 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_KMU_H__
#define NRF_KMU_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_kmu_hal KMU HAL
 * @{
 * @ingroup nrf_kmu
 * @brief   Hardware access layer for managing the Key Management Unit (KMU) peripheral.
 */

/** @brief KMU tasks. */
typedef enum
{
    NRF_KMU_TASK_PUSH_KEYSLOT = offsetof(NRF_KMU_Type, TASKS_PUSH_KEYSLOT), ///< Push a key slot over secure APB.
} nrf_kmu_task_t;

/** @brief KMU events. */
typedef enum
{
    NRF_KMU_EVENT_KEYSLOT_PUSHED  = offsetof(NRF_KMU_Type, EVENTS_KEYSLOT_PUSHED),  ///< Key successfully pushed over secure APB.
    NRF_KMU_EVENT_KEYSLOT_REVOKED = offsetof(NRF_KMU_Type, EVENTS_KEYSLOT_REVOKED), ///< Key has been revoked and cannot be tasked for selection.
    NRF_KMU_EVENT_KEYSLOT_ERROR   = offsetof(NRF_KMU_Type, EVENTS_KEYSLOT_ERROR)    ///< No key slot selected or no destination address defined or error during push mechanism.
} nrf_kmu_event_t;

/** @brief KMU interrupts. */
typedef enum
{
    NRF_KMU_INT_PUSHED_MASK  = KMU_INTEN_KEYSLOT_PUSHED_Msk,  ///< Interrupt on KEYSLOT_PUSHED event.
    NRF_KMU_INT_REVOKED_MASK = KMU_INTEN_KEYSLOT_REVOKED_Msk, ///< Interrupt on KEYSLOT_REVOKED event.
    NRF_KMU_INT_ERROR_MASK   = KMU_INTEN_KEYSLOT_ERROR_Msk    ///< Interrupt on KEYSLOT_ERROR event.
} nrf_kmu_int_mask_t;

/** @brief KMU operation status. */
typedef enum
{
    NRF_KMU_STATUS_BLOCKED_MASK  = KMU_STATUS_BLOCKED_Msk,  ///< Access violation detected and blocked.
    NRF_KMU_STATUS_SELECTED_MASK = KMU_STATUS_SELECTED_Msk, ///< Key slot ID successfully selected by KMU
} nrf_kmu_status_t;


/**
 * @brief Function for activating a specific KMU task.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] task  Task to activate.
 */
__STATIC_INLINE void nrf_kmu_task_trigger(NRF_KMU_Type * p_reg, nrf_kmu_task_t task);

/**
 * @brief Function for getting the address of a specific KMU task register.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] task  Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_kmu_task_address_get(NRF_KMU_Type const * p_reg, nrf_kmu_task_t task);

/**
 * @brief Function for clearing a specific KMU event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_kmu_event_clear(NRF_KMU_Type * p_reg, nrf_kmu_event_t event);

/**
 * @brief Function for checking the state of a specific KMU event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_kmu_event_check(NRF_KMU_Type const * p_reg, nrf_kmu_event_t event);

/**
 * @brief Function for getting the address of a specific KMU event register.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_kmu_event_address_get(NRF_KMU_Type const * p_reg,
                                                   nrf_kmu_event_t      event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_kmu_int_enable(NRF_KMU_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_kmu_int_disable(NRF_KMU_Type * p_reg, uint32_t mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] kmu_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_kmu_int_enable_check(NRF_KMU_Type const * p_reg,
                                              nrf_kmu_int_mask_t   kmu_int);

/**
 * @brief Function for retrieving the state of interrupts.
 *
 * Function returns bitmask. Please use @ref nrf_kmu_int_mask_t to check interrupts status.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return Bitmask with pending interrupts bits.
 */
__STATIC_INLINE uint32_t nrf_kmu_intpend_get(NRF_KMU_Type const * p_reg);

/**
 * @brief Function for getting status bits of the KMU operation.
 *
 * Function returns bitmask. Please use @ref nrf_kmu_status_t to check operations status.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return Bitmask with operation status bits.
 */
__STATIC_INLINE uint32_t nrf_kmu_status_get(NRF_KMU_Type const * p_reg);

/**
 * @brief Function for selecting the key slot ID.
 *
 * @param[in] p_reg      Pointer to the peripheral registers structure.
 * @param[in] keyslot_id Key slot ID to be read over AHB or pushed over
 *                       secure APB when TASKS_PUSH_KEYSLOT is started.
 */
__STATIC_INLINE void nrf_kmu_keyslot_set(NRF_KMU_Type * p_reg, uint8_t keyslot_id);

/**
 * @brief Function for getting the key slot ID.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @return Key slot ID.
 */
__STATIC_INLINE uint8_t nrf_kmu_keyslot_get(NRF_KMU_Type const * p_reg);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_kmu_task_trigger(NRF_KMU_Type * p_reg, nrf_kmu_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_kmu_task_address_get(NRF_KMU_Type const * p_reg, nrf_kmu_task_t task)
{
    return ((uint32_t)p_reg + (uint32_t)task);
}

__STATIC_INLINE void nrf_kmu_event_clear(NRF_KMU_Type * p_reg, nrf_kmu_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
}

__STATIC_INLINE bool nrf_kmu_event_check(NRF_KMU_Type const * p_reg, nrf_kmu_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE uint32_t nrf_kmu_event_address_get(NRF_KMU_Type const * p_reg,
                                                   nrf_kmu_event_t      event)
{
    return ((uint32_t)p_reg + (uint32_t)event);
}

__STATIC_INLINE void nrf_kmu_int_enable(NRF_KMU_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

__STATIC_INLINE void nrf_kmu_int_disable(NRF_KMU_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

__STATIC_INLINE bool nrf_kmu_int_enable_check(NRF_KMU_Type const * p_reg,
                                              nrf_kmu_int_mask_t   kmu_int)
{
    return (bool)(p_reg->INTENSET & kmu_int);
}

__STATIC_INLINE uint32_t nrf_kmu_intpend_get(NRF_KMU_Type const * p_reg)
{
    return p_reg->INTPEND;
}

__STATIC_INLINE uint32_t nrf_kmu_status_get(NRF_KMU_Type const * p_reg)
{
    return p_reg->STATUS;
}

__STATIC_INLINE void nrf_kmu_keyslot_set(NRF_KMU_Type * p_reg, uint8_t keyslot_id)
{
    p_reg->SELECTKEYSLOT = (uint32_t) keyslot_id;
}

__STATIC_INLINE uint8_t nrf_kmu_keyslot_get(NRF_KMU_Type const * p_reg)
{
    return (uint8_t) p_reg->SELECTKEYSLOT;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_KMU_H__
