/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

#if defined(CONFIG_CPU_CORTEX_M)
/* Workaround for missing __ICACHE_PRESENT and __DCACHE_PRESENT symbols in MDK
 * SoC definitions. To be removed when this is fixed.
 */
#include <cmsis_core_m_defaults.h>
#endif

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/irq.h>

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

/**
 * @brief Macro for placing a runtime assertion.
 *
 * @param expression Expression to be evaluated.
 */
#ifndef NRFX_ASSERT
#define NRFX_ASSERT(expression)  __ASSERT_NO_MSG(expression)
#endif

#if defined(CONFIG_RISCV)
/* included here due to dependency on NRFX_ASSERT definition */
#include <hal/nrf_vpr_clic.h>
#endif

/**
 * @brief Macro for placing a compile time assertion.
 *
 * @param expression Expression to be evaluated.
 */
#define NRFX_STATIC_ASSERT(expression) \
        BUILD_ASSERT(expression, "assertion failed")

//------------------------------------------------------------------------------

/**
 * @brief Macro for setting the priority of a specific IRQ.
 *
 * @param irq_number IRQ number.
 * @param priority   Priority to be set.
 */
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority)                                                \
	ARG_UNUSED(priority)                                                                       \
	/* Intentionally empty. Priorities of IRQs are set through IRQ_CONNECT. */

/**
 * @brief Macro for enabling a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_ENABLE(irq_number)  irq_enable(irq_number)

/**
 * @brief Macro for checking if a specific IRQ is enabled.
 *
 * @param irq_number IRQ number.
 *
 * @retval true  If the IRQ is enabled.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_ENABLED(irq_number)  irq_is_enabled(irq_number)

/**
 * @brief Macro for disabling a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_DISABLE(irq_number)  irq_disable(irq_number)

/**
 * @brief Macro for setting a specific IRQ as pending.
 *
 * @param irq_number IRQ number.
 */
#if defined(CONFIG_RISCV)
#define NRFX_IRQ_PENDING_SET(irq_number) nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, irq_number)
#else
#define NRFX_IRQ_PENDING_SET(irq_number) NVIC_SetPendingIRQ(irq_number)
#endif

/**
 * @brief Macro for clearing the pending status of a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#if defined(CONFIG_RISCV)
#define NRFX_IRQ_PENDING_CLEAR(irq_number) nrf_vpr_clic_int_pending_clear(NRF_VPRCLIC, irq_number)
#else
#define NRFX_IRQ_PENDING_CLEAR(irq_number) NVIC_ClearPendingIRQ(irq_number)
#endif

/**
 * @brief Macro for checking the pending status of a specific IRQ.
 *
 * @retval true  If the IRQ is pending.
 * @retval false Otherwise.
 */
#if defined(CONFIG_RISCV)
#define NRFX_IRQ_IS_PENDING(irq_number) nrf_vpr_clic_int_pending_check(NRF_VPRCLIC, irq_number)
#else
#define NRFX_IRQ_IS_PENDING(irq_number) (NVIC_GetPendingIRQ(irq_number) == 1)
#endif

/** @brief Macro for entering into a critical section. */
#define NRFX_CRITICAL_SECTION_ENTER()  { unsigned int irq_lock_key = irq_lock();

/** @brief Macro for exiting from a critical section. */
#define NRFX_CRITICAL_SECTION_EXIT()     irq_unlock(irq_lock_key); }

//------------------------------------------------------------------------------

/**
 * @brief When set to a non-zero value, this macro specifies that
 *        @ref nrfx_coredep_delay_us uses a precise DWT-based solution.
 *        A compilation error is generated if the DWT unit is not present
 *        in the SoC used.
 */
#define NRFX_DELAY_DWT_BASED    0

/**
 * @brief Macro for delaying the code execution for at least the specified time.
 *
 * @param us_time Number of microseconds to wait.
 */
#define NRFX_DELAY_US(us_time)  nrfx_busy_wait(us_time)
/* This is a k_busy_wait wrapper, added to avoid the inclusion of kernel.h */
void nrfx_busy_wait(uint32_t usec_to_wait);

//------------------------------------------------------------------------------

/** @brief Atomic 32-bit unsigned type. */
#define nrfx_atomic_t  atomic_t

/**
 * @brief Macro for storing a value to an atomic object and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value to store.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_STORE(p_data, value)  atomic_set(p_data, value)

/**
 * @brief Macro for running a bitwise OR operation on an atomic object and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the OR operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_OR(p_data, value)  atomic_or(p_data, value)

/**
 * @brief Macro for running a bitwise AND operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the AND operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_AND(p_data, value)  atomic_and(p_data, value)

/**
 * @brief Macro for running a bitwise XOR operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the XOR operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_XOR(p_data, value)  atomic_xor(p_data, value)

/**
 * @brief Macro for running an addition operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the ADD operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_ADD(p_data, value)  atomic_add(p_data, value)

/**
 * @brief Macro for running a subtraction operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the SUB operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_SUB(p_data, value)  atomic_sub(p_data, value)

/**
 * @brief Macro for running compare and swap on an atomic object.
 *
 * Value is updated to the new value only if it previously equaled old value.
 *
 * @param[in,out] p_data      Atomic memory pointer.
 * @param[in]     old_value Expected old value.
 * @param[in]     new_value   New value.
 *
 * @retval true If value was updated.
 * @retval false If value was not updated because location was not equal to @p old_value.
 */
#define NRFX_ATOMIC_CAS(p_data, old_value, new_value) \
	atomic_cas(p_data, old_value, new_value)

/**
 * @brief Macro for counting leading zeros.
 *
 * @param[in] value A word value.
 *
 * @return Number of leading 0-bits in @p value, starting at the most significant bit position.
 *         If x is 0, the result is undefined.
 */
#define NRFX_CLZ(value) __builtin_clz(value)

/**
 * @brief Macro for counting trailing zeros.
 *
 * @param[in] value A word value.
 *
 * @return Number of trailing 0-bits in @p value, starting at the least significant bit position.
 *         If x is 0, the result is undefined.
 */
#define NRFX_CTZ(value) __builtin_ctz(value)

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
 * @brief When set to a non-zero value, this macro specifies that inside HALs
 *        the event registers are read back after clearing, on devices that
 *        otherwise could defer the actual register modification.
 */
#define NRFX_EVENT_READBACK_ENABLED 1

//------------------------------------------------------------------------------

/**
 * @brief Macro for writing back cache lines associated with the specified buffer.
 *
 * @param[in] p_buffer Pointer to the buffer.
 * @param[in] size     Size of the buffer.
 */
#define NRFY_CACHE_WB(p_buffer, size) \
	do {                          \
		(void)p_buffer;       \
		(void)size;           \
	} while (0)

/**
 * @brief Macro for invalidating cache lines associated with the specified buffer.
 *
 * @param[in] p_buffer Pointer to the buffer.
 * @param[in] size     Size of the buffer.
 */
#define NRFY_CACHE_INV(p_buffer, size) \
	do {                           \
		(void)p_buffer;        \
		(void)size;            \
	} while (0)

/**
 * @brief Macro for writing back and invalidating cache lines associated with
 *        the specified buffer.
 *
 * @param[in] p_buffer Pointer to the buffer.
 * @param[in] size     Size of the buffer.
 */
#define NRFY_CACHE_WBINV(p_buffer, size) \
	do {                             \
		(void)p_buffer;          \
		(void)size;              \
	} while (0)

/*------------------------------------------------------------------------------*/

#ifdef CONFIG_NRFX_RESERVED_RESOURCES_HEADER
#include CONFIG_NRFX_RESERVED_RESOURCES_HEADER
#endif

//------------------------------------------------------------------------------

/**
 * @brief Function helping to integrate nrfx IRQ handlers with IRQ_CONNECT.
 *
 * This function simply calls the nrfx IRQ handler supplied as the parameter.
 * It is intended to be used in the following way:
 * IRQ_CONNECT(IRQ_NUM, IRQ_PRI, nrfx_isr, nrfx_..._irq_handler, 0);
 *
 * @param[in] irq_handler  Pointer to the nrfx IRQ handler to be called.
 */
void nrfx_isr(const void *irq_handler);

#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
#include "nrfx_glue_bsim.h"
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_GLUE_H__
