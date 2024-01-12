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

/** @brief Bitmask that defines DPPI channels that are reserved for use outside of the nrfx library. */
#define NRFX_DPPI_CHANNELS_USED   (NRFX_PPI_CHANNELS_USED_BY_BT_CTLR |    \
				   NRFX_PPI_CHANNELS_USED_BY_802154_DRV | \
				   NRFX_PPI_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside of the nrfx library. */
#define NRFX_DPPI_GROUPS_USED     (NRFX_PPI_GROUPS_USED_BY_BT_CTLR |    \
				   NRFX_PPI_GROUPS_USED_BY_802154_DRV | \
				   NRFX_PPI_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines PPI channels that are reserved for use outside of the nrfx library. */
#define NRFX_PPI_CHANNELS_USED    (NRFX_PPI_CHANNELS_USED_BY_BT_CTLR |    \
				   NRFX_PPI_CHANNELS_USED_BY_802154_DRV | \
				   NRFX_PPI_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines PPI groups that are reserved for use outside of the nrfx library. */
#define NRFX_PPI_GROUPS_USED      (NRFX_PPI_GROUPS_USED_BY_BT_CTLR |    \
				   NRFX_PPI_GROUPS_USED_BY_802154_DRV | \
				   NRFX_PPI_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines GPIOTE channels that are reserved for use outside of the nrfx library. */
#define NRFX_GPIOTE_CHANNELS_USED NRFX_GPIOTE_CHANNELS_USED_BY_BT_CTLR

#if defined(CONFIG_BT_CTLR)
/*
 * The enabled Bluetooth controller subsystem is responsible for providing
 * definitions of the BT_CTLR_USED_* symbols used below in a file named
 * bt_ctlr_used_resources.h and for adding its location to global include
 * paths so that the file can be included here for all Zephyr libraries that
 * are to be built.
 */
#include <bt_ctlr_used_resources.h>
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR    BT_CTLR_USED_PPI_CHANNELS
#define NRFX_PPI_GROUPS_USED_BY_BT_CTLR      BT_CTLR_USED_PPI_GROUPS
#define NRFX_GPIOTE_CHANNELS_USED_BY_BT_CTLR BT_CTLR_USED_GPIOTE_CHANNELS
#else
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR    0
#define NRFX_PPI_GROUPS_USED_BY_BT_CTLR      0
#define NRFX_GPIOTE_CHANNELS_USED_BY_BT_CTLR 0
#endif

#if defined(CONFIG_NRF_802154_RADIO_DRIVER)
#if defined(NRF52_SERIES)
#include <../src/nrf_802154_peripherals_nrf52.h>
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV   NRF_802154_PPI_CHANNELS_USED_MASK
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV     NRF_802154_PPI_GROUPS_USED_MASK
#elif defined(NRF53_SERIES)
#include <../src/nrf_802154_peripherals_nrf53.h>
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV   NRF_802154_DPPI_CHANNELS_USED_MASK
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV     NRF_802154_DPPI_GROUPS_USED_MASK
#elif defined(NRF54L_SERIES)
#include <../src/nrf_802154_peripherals_nrf54l.h>
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV   NRF_802154_DPPI_CHANNELS_USED_MASK
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV     NRF_802154_DPPI_GROUPS_USED_MASK
#else
#error Unsupported chip family
#endif
#else // CONFIG_NRF_802154_RADIO_DRIVER
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV   0
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV     0
#endif // CONFIG_NRF_802154_RADIO_DRIVER

#if defined(CONFIG_NRF_802154_RADIO_DRIVER) && !defined(CONFIG_NRF_802154_SL_OPENSOURCE)
#include <mpsl.h>
#define NRFX_PPI_CHANNELS_USED_BY_MPSL   MPSL_RESERVED_PPI_CHANNELS
#define NRFX_PPI_GROUPS_USED_BY_MPSL     0
#else
#define NRFX_PPI_CHANNELS_USED_BY_MPSL   0
#define NRFX_PPI_GROUPS_USED_BY_MPSL     0
#endif

#if defined(NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL)
BUILD_ASSERT(
	(NRFX_PPI_CHANNELS_USED_BY_802154_DRV & NRFX_PPI_CHANNELS_USED_BY_MPSL) == 0,
	"PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	"assigned to the MPSL.");

BUILD_ASSERT(
	(NRFX_PPI_GROUPS_USED_BY_802154_DRV & NRFX_PPI_GROUPS_USED_BY_MPSL) == 0,
	"PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	"assigned to the MPSL.");
#endif // NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL

/** @brief Bitmask that defines EGU instances that are reserved for use outside of the nrfx library. */
#define NRFX_EGUS_USED            0

/** @brief Bitmask that defines TIMER instances that are reserved for use outside of the nrfx library. */
#define NRFX_TIMERS_USED          0

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
