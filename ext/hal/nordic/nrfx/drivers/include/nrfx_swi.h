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

#ifndef NRFX_SWI_H__
#define NRFX_SWI_H__

#include <nrfx.h>

#if NRFX_CHECK(NRFX_EGU_ENABLED)
#include <hal/nrf_egu.h>
#endif

#ifndef SWI_COUNT
#define SWI_COUNT EGU_COUNT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_swi SWI driver
 * @{
 * @ingroup  nrf_swi_egu
 *
 * @brief    Driver for managing software interrupts (SWI).
 */

/** @brief SWI instance. */
typedef uint8_t nrfx_swi_t;

/**
 * @brief SWI user flags.
 *
 * User flags are set during the SWI trigger and passed to the callback function as an argument.
 */
typedef uint16_t nrfx_swi_flags_t;

/** @brief Unallocated instance value. */
#define NRFX_SWI_UNALLOCATED  ((nrfx_swi_t)0xFFuL)

/** @brief Default SWI priority. */
#define NRFX_SWI_DEFAULT_PRIORITY  APP_IRQ_PRIORITY_LOWEST

/**
 * @brief SWI handler function.
 *
 * @param swi   SWI instance.
 * @param flags User flags.
 */
typedef void (*nrfx_swi_handler_t)(nrfx_swi_t swi, nrfx_swi_flags_t flags);

/**
 * @brief Function for allocating the first unused SWI instance and setting a handler.
 *
 * If provided handler is not NULL, an allocated SWI has its interrupt enabled by default.
 * The interrupt can be disabled by @ref nrfx_swi_int_disable.
 *
 * @param[out] p_swi         Points to a place where the allocated SWI instance
 *                           number is to be stored.
 * @param[in]  event_handler Event handler function.
 *                           If NULL, no interrupt will be enabled.
 *                           It can be NULL only if the EGU driver is enabled.
 *                           For classic SWI, it must be a valid handler pointer.
 * @param[in]  irq_priority  Interrupt priority.
 *
 * @retval NRFX_SUCCESS      The SWI was successfully allocated.
 * @retval NRFX_ERROR_NO_MEM There is no available SWI to be used.
 */
nrfx_err_t nrfx_swi_alloc(nrfx_swi_t *       p_swi,
                          nrfx_swi_handler_t event_handler,
                          uint32_t           irq_priority);

/**
 * @brief Function for disabling an allocated SWI interrupt.
 *
 * Use @ref nrfx_swi_int_enable to re-enable the interrupt.
 *
 * @param[in] swi SWI instance.
 */
void nrfx_swi_int_disable(nrfx_swi_t swi);

/**
 * @brief Function for enabling an allocated SWI interrupt.
 *
 * @param[in] swi SWI instance.
 */
void nrfx_swi_int_enable(nrfx_swi_t swi);

/**
 * @brief Function for freeing a previously allocated SWI.
 *
 * @param[in,out] p_swi SWI instance to free. The value is changed to
 *                      @ref NRFX_SWI_UNALLOCATED on success.
 */
void nrfx_swi_free(nrfx_swi_t * p_swi);

/** @brief Function for freeing all allocated SWIs. */
void nrfx_swi_all_free(void);

/**
 * @brief Function for triggering the SWI.
 *
 * @param[in] swi         SWI to trigger.
 * @param[in] flag_number Number of user flag to trigger.
 */
void nrfx_swi_trigger(nrfx_swi_t swi,
                      uint8_t    flag_number);

/**
 * @brief Function for checking if the specified SWI is currently allocated.
 *
 * @param[in] swi SWI instance.
 *
 * @retval true  The SWI instance is allocated.
 * @retval false The SWI instance is not allocated.
 */
bool nrfx_swi_is_allocated(nrfx_swi_t swi);

#if NRFX_CHECK(NRFX_EGU_ENABLED) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for returning the base address of the EGU peripheral
 *        associated with the specified SWI instance.
 *
 * @param[in] swi SWI instance.
 *
 * @return EGU base address or NULL if the specified SWI instance number
 *         is too high.
 */
__STATIC_INLINE NRF_EGU_Type * nrfx_swi_egu_instance_get(nrfx_swi_t swi)
{
#if (EGU_COUNT < SWI_COUNT)
    if (swi >= EGU_COUNT)
    {
        return NULL;
    }
#endif
    uint32_t offset = ((uint32_t)swi) * ((uint32_t)NRF_EGU1 - (uint32_t)NRF_EGU0);
    return (NRF_EGU_Type *)((uint32_t)NRF_EGU0 + offset);
}

/**
 * @brief Function for returning the EGU trigger task address.
 *
 * @param[in] swi     SWI instance.
 * @param[in] channel Number of the EGU channel.
 *
 * @return Address of the EGU trigger task.
 */
__STATIC_INLINE uint32_t nrfx_swi_task_trigger_address_get(nrfx_swi_t swi,
                                                           uint8_t    channel)
{
    NRFX_ASSERT(nrfx_swi_is_allocated(swi));

    NRF_EGU_Type * p_egu = nrfx_swi_egu_instance_get(swi);
#if (EGU_COUNT < SWI_COUNT)
    if (p_egu == NULL)
    {
        return 0;
    }
#endif

    return (uint32_t)nrf_egu_task_trigger_address_get(p_egu, channel);
}

/**
 * @brief Function for returning the EGU-triggered event address.
 *
 * @param[in] swi     SWI instance.
 * @param[in] channel Number of the EGU channel.
 *
 * @return Address of the EGU-triggered event.
 */
__STATIC_INLINE uint32_t nrfx_swi_event_triggered_address_get(nrfx_swi_t swi,
                                                              uint8_t    channel)
{
    NRFX_ASSERT(nrfx_swi_is_allocated(swi));

    NRF_EGU_Type * p_egu = nrfx_swi_egu_instance_get(swi);
#if (EGU_COUNT < SWI_COUNT)
    if (p_egu == NULL)
    {
        return 0;
    }
#endif

    return (uint32_t)nrf_egu_event_triggered_address_get(p_egu, channel);
}

#endif // NRFX_CHECK(NRFX_EGU_ENABLED) || defined(__NRFX_DOXYGEN__)

/** @} */


void nrfx_swi_0_irq_handler(void);
void nrfx_swi_1_irq_handler(void);
void nrfx_swi_2_irq_handler(void);
void nrfx_swi_3_irq_handler(void);
void nrfx_swi_4_irq_handler(void);
void nrfx_swi_5_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif // NRFX_SWI_H__
