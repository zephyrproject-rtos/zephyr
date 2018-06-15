/*
 * Copyright (c) 2012 - 2018, Nordic Semiconductor ASA
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

#ifndef NRF_ECB_H__
#define NRF_ECB_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_ecb_drv AES ECB encryption driver
 * @{
 * @ingroup nrf_ecb
 * @brief   Driver for the AES Electronic Code Book (ECB) peripheral.
 *
 * To encrypt data, the peripheral must first be powered on
 * using @ref nrf_ecb_init. Next, the key must be set using @ref nrf_ecb_set_key.
 */

/**
 * @brief Function for initializing and powering on the ECB peripheral.
 *
 * This function allocates memory for the ECBDATAPTR.
 * @retval true If initialization was successful.
 * @retval false If powering on failed.
 */
bool nrf_ecb_init(void);

/**
 * @brief Function for encrypting 16-byte data using current key.
 *
 * This function avoids unnecessary copying of data if the parameters point to the
 * correct locations in the ECB data structure.
 *
 * @param dst Result of encryption, 16 bytes will be written.
 * @param src Source with 16-byte data to be encrypted.
 *
 * @retval true  If the encryption operation completed.
 * @retval false If the encryption operation did not complete.
 */
bool nrf_ecb_crypt(uint8_t * dst, const uint8_t * src);

/**
 * @brief Function for setting the key to be used for encryption.
 *
 * @param key Pointer to the key. 16 bytes will be read.
 */
void nrf_ecb_set_key(const uint8_t * key);

/** @} */

/**
 * @defgroup nrf_ecb_hal AES ECB encryption HAL
 * @{
 * @ingroup nrf_ecb
 * @brief   Hardware access layer for managing the AES Electronic Codebook (ECB) peripheral.
 */

/**
 * @brief ECB tasks.
 */
typedef enum
{
    /*lint -save -e30 -esym(628,__INTADDR__)*/
    NRF_ECB_TASK_STARTECB = offsetof(NRF_ECB_Type, TASKS_STARTECB), /**< Task for starting ECB block encryption. */
    NRF_ECB_TASK_STOPECB  = offsetof(NRF_ECB_Type, TASKS_STOPECB),  /**< Task for stopping ECB block encryption. */
    /*lint -restore*/
} nrf_ecb_task_t;

/**
 * @brief ECB events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_ECB_EVENT_ENDECB   = offsetof(NRF_ECB_Type, EVENTS_ENDECB),   /**< ECB block encrypt complete. */
    NRF_ECB_EVENT_ERRORECB = offsetof(NRF_ECB_Type, EVENTS_ERRORECB), /**< ECB block encrypt aborted because of a STOPECB task or due to an error. */
    /*lint -restore*/
} nrf_ecb_event_t;

/**
 * @brief ECB interrupts.
 */
typedef enum
{
    NRF_ECB_INT_ENDECB_MASK   = ECB_INTENSET_ENDECB_Msk,   ///< Interrupt on ENDECB event.
    NRF_ECB_INT_ERRORECB_MASK = ECB_INTENSET_ERRORECB_Msk, ///< Interrupt on ERRORECB event.
} nrf_ecb_int_mask_t;


/**
 * @brief Function for activating a specific ECB task.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] task  Task to activate.
 */
__STATIC_INLINE void nrf_ecb_task_trigger(NRF_ECB_Type * p_reg, nrf_ecb_task_t task);

/**
 * @brief Function for getting the address of a specific ECB task register.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] task  Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_ecb_task_address_get(NRF_ECB_Type const * p_reg,
                                                  nrf_ecb_task_t       task);

/**
 * @brief Function for clearing a specific ECB event.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_ecb_event_clear(NRF_ECB_Type * p_reg, nrf_ecb_event_t event);

/**
 * @brief Function for checking the state of a specific ECB event.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_ecb_event_check(NRF_ECB_Type const * p_reg, nrf_ecb_event_t event);

/**
 * @brief Function for getting the address of a specific ECB event register.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_ecb_event_address_get(NRF_ECB_Type const * p_reg,
                                                   nrf_ecb_event_t      event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_ecb_int_enable(NRF_ECB_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_ecb_int_disable(NRF_ECB_Type * p_reg, uint32_t mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg   Pointer to the peripheral register structure.
 * @param[in] ecb_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_ecb_int_enable_check(NRF_ECB_Type const * p_reg,
                                              nrf_ecb_int_mask_t   ecb_int);

/**
 * @brief Function for setting the pointer to the ECB data buffer.
 *
 * @note The buffer has to be placed in the Data RAM region.
 *       For description of the data structure in this buffer, see the Product Specification.
 *
 * @param[in] p_reg    Pointer to the peripheral register structure.
 * @param[in] p_buffer Pointer to the ECB data buffer.
 */
__STATIC_INLINE void nrf_ecb_data_pointer_set(NRF_ECB_Type * p_reg, void const * p_buffer);

/**
 * @brief Function for getting the pointer to the ECB data buffer.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 *
 * @return Pointer to the ECB data buffer.
 */
__STATIC_INLINE void * nrf_ecb_data_pointer_get(NRF_ECB_Type const * p_reg);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_ecb_task_trigger(NRF_ECB_Type * p_reg, nrf_ecb_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_ecb_task_address_get(NRF_ECB_Type const * p_reg,
                                                  nrf_ecb_task_t       task)
{
    return ((uint32_t)p_reg + (uint32_t)task);
}

__STATIC_INLINE void nrf_ecb_event_clear(NRF_ECB_Type * p_reg, nrf_ecb_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_ecb_event_check(NRF_ECB_Type const * p_reg, nrf_ecb_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE uint32_t nrf_ecb_event_address_get(NRF_ECB_Type const * p_reg,
                                                   nrf_ecb_event_t      event)
{
    return ((uint32_t)p_reg + (uint32_t)event);
}

__STATIC_INLINE void nrf_ecb_int_enable(NRF_ECB_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

__STATIC_INLINE void nrf_ecb_int_disable(NRF_ECB_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

__STATIC_INLINE bool nrf_ecb_int_enable_check(NRF_ECB_Type const * p_reg,
                                              nrf_ecb_int_mask_t   ecb_int)
{
    return (bool)(p_reg->INTENSET & ecb_int);
}

__STATIC_INLINE void nrf_ecb_data_pointer_set(NRF_ECB_Type * p_reg, void const * p_buffer)
{
    p_reg->ECBDATAPTR = (uint32_t)p_buffer;
}

__STATIC_INLINE void * nrf_ecb_data_pointer_get(NRF_ECB_Type const * p_reg)
{
    return (void *)(p_reg->ECBDATAPTR);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif  // NRF_ECB_H__

