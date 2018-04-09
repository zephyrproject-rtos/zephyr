/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_glue nrfx_glue.h
 * @{
 * @ingroup nrfx
 *
 * @brief This file contains macros that should be implemented according to
 *        the needs of the host environment into which @em nrfx is integrated.
 */

//------------------------------------------------------------------------------

#include <assert.h>

/**
 * @brief Macro for placing a runtime assertion.
 *
 * @param expression  Expression to evaluate.
 */
#define NRFX_ASSERT(expression)  assert(expression)

/**
 * @brief Macro for placing a compile time assertion.
 *
 * @param expression  Expression to evaluate.
 */
#define NRFX_STATIC_ASSERT(expression)  static_assert(expression)

//------------------------------------------------------------------------------

#include <irq.h>

/**
 * @brief Macro for setting the priority of a specific IRQ.
 *
 * @param irq_number  IRQ number.
 * @param priority    Priority to set.
 */
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority)  // Intentionally empty.
                                                     // Priorities of IRQs are
                                                     // set through IRQ_CONNECT.

/**
 * @brief Macro for enabling a specific IRQ.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_ENABLE(irq_number)  irq_enable(irq_number)

/**
 * @brief Macro for checking if a specific IRQ is enabled.
 *
 * @param irq_number  IRQ number.
 *
 * @retval true  If the IRQ is enabled.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_ENABLED(irq_number)  irq_is_enabled(irq_number)

/**
 * @brief Macro for disabling a specific IRQ.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_DISABLE(irq_number)  irq_disable(irq_number)

/**
 * @brief Macro for setting a specific IRQ as pending.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_PENDING_SET(irq_number)  NVIC_SetPendingIRQ(irq_number)

/**
 * @brief Macro for clearing the pending status of a specific IRQ.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_PENDING_CLEAR(irq_number)  NVIC_ClearPendingIRQ(irq_number)

/**
 * @brief Macro for checking the pending status of a specific IRQ.
 *
 * @retval true  If the IRQ is pending.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_PENDING(irq_number)  (NVIC_GetPendingIRQ(irq_number) == 1)

/**
 * @brief Macro for entering into a critical section.
 */
#define NRFX_CRITICAL_SECTION_ENTER()  { unsigned int irq_lock_key = irq_lock();

/**
 * @brief Macro for exiting from a critical section.
 */
#define NRFX_CRITICAL_SECTION_EXIT()     irq_unlock(irq_lock_key); }

//------------------------------------------------------------------------------

#include <kernel.h>

/**
 * @brief Macro for delaying the code execution for at least the specified time.
 *
 * @param us_time Number of microseconds to wait.
 */
#define NRFX_DELAY_US(us_time)  k_busy_wait(us_time)

//------------------------------------------------------------------------------

/**
 * @brief When set to a non-zero value, this macro specifies that the
 *        @ref nrfx_error_codes and the @ref nrfx_err_t type itself are defined
 *        in a customized way and the default definitions from @c <nrfx_error.h>
 *        should not be used.
 */
#define NRFX_CUSTOM_ERROR_CODES 0

//------------------------------------------------------------------------------

/**
 * @brief Bitmask defining PPI channels reserved to be used outside of nrfx.
 */
#define NRFX_PPI_CHANNELS_USED  0

/**
 * @brief Bitmask defining PPI groups reserved to be used outside of nrfx.
 */
#define NRFX_PPI_GROUPS_USED    0

/**
 * @brief Bitmask defining SWI instances reserved to be used outside of nrfx.
 */
#define NRFX_SWI_USED           0

/**
 * @brief Bitmask defining TIMER instances reserved to be used outside of nrfx.
 */
#define NRFX_TIMERS_USED        0

//------------------------------------------------------------------------------

/**
 * @brief Macro for getting the interrupt number assigned to a specific
 *        peripheral.
 *
 * In Nordic SoCs the IRQ number assigned to a peripheral is equal to the ID
 * of this peripheral, and there is a direct relationship between this ID and
 * the peripheral base address, i.e. the address of a fixed block of 0x1000
 * bytes of address space assigned to this peripheral.
 * See the chapter "Peripheral interface" (sections "Peripheral ID" and
 * "Interrupts") in the product specification of a given SoC.
 *
 * @param[in] base_addr  Peripheral base address.
 *
 * @return Interrupt number associated with the specified peripheral.
 */
/* TODO - this macro should become a part of <nrfx_common.h> */
#define NRFX_IRQ_NUMBER_GET(base_addr)  (uint8_t)((uint32_t)(base_addr) >> 12)

/**
 * @brief Function helping to integrate nrfx IRQ handlers with IRQ_CONNECT.
 *
 * This function simply calls the nrfx IRQ handler supplied as the parameter.
 * It is intended to be used in the following way:
 * IRQ_CONNECT(IRQ_NUM, IRQ_PRI, nrfx_isr, nrfx_..._irq_handler, 0);
 *
 * @param[in] irq_handler  Pointer to the nrfx IRQ handler to be called.
 */
void nrfx_isr(void *irq_handler);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_GLUE_H__
