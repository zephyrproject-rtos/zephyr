/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

#include <sys/__assert.h>
#include <sys/atomic.h>
#include <irq.h>

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
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority)  // Intentionally empty.
                                                     // Priorities of IRQs are
                                                     // set through IRQ_CONNECT.

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
#define NRFX_IRQ_PENDING_SET(irq_number)  NVIC_SetPendingIRQ(irq_number)

/**
 * @brief Macro for clearing the pending status of a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_PENDING_CLEAR(irq_number)  NVIC_ClearPendingIRQ(irq_number)

/**
 * @brief Macro for checking the pending status of a specific IRQ.
 *
 * @retval true  If the IRQ is pending.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_PENDING(irq_number)  (NVIC_GetPendingIRQ(irq_number) == 1)

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
				   NRFX_PPI_CHANNELS_USED_BY_MPSL |       \
				   NRFX_PPI_CHANNELS_USED_BY_PWM_SW)

/** @brief Bitmask that defines PPI groups that are reserved for use outside of the nrfx library. */
#define NRFX_PPI_GROUPS_USED      (NRFX_PPI_GROUPS_USED_BY_BT_CTLR |    \
				   NRFX_PPI_GROUPS_USED_BY_802154_DRV | \
				   NRFX_PPI_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines GPIOTE channels that are reserved for use outside of the nrfx library. */
#define NRFX_GPIOTE_CHANNELS_USED NRFX_GPIOTE_CHANNELS_USED_BY_PWM_SW

#if defined(CONFIG_BT_CTLR)
extern const uint32_t z_bt_ctlr_used_nrf_ppi_channels;
extern const uint32_t z_bt_ctlr_used_nrf_ppi_groups;
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR   z_bt_ctlr_used_nrf_ppi_channels
#define NRFX_PPI_GROUPS_USED_BY_BT_CTLR     z_bt_ctlr_used_nrf_ppi_groups
#else
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR   0
#define NRFX_PPI_GROUPS_USED_BY_BT_CTLR     0
#endif

#if defined(CONFIG_NRF_802154_RADIO_DRIVER)
extern const uint32_t g_nrf_802154_used_nrf_ppi_channels;
extern const uint32_t g_nrf_802154_used_nrf_ppi_groups;
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV   g_nrf_802154_used_nrf_ppi_channels
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV     g_nrf_802154_used_nrf_ppi_groups
#else
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV   0
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV     0
#endif

#if defined(CONFIG_NRF_802154_RADIO_DRIVER) && \
	!defined(CONFIG_NRF_802154_SL_OPENSOURCE)
extern const uint32_t z_mpsl_used_nrf_ppi_channels;
extern const uint32_t z_mpsl_used_nrf_ppi_groups;
#define NRFX_PPI_CHANNELS_USED_BY_MPSL   z_mpsl_used_nrf_ppi_channels
#define NRFX_PPI_GROUPS_USED_BY_MPSL     z_mpsl_used_nrf_ppi_groups
#else
#define NRFX_PPI_CHANNELS_USED_BY_MPSL   0
#define NRFX_PPI_GROUPS_USED_BY_MPSL     0
#endif

#if defined(CONFIG_PWM_NRF5_SW)
#define PWM_NRF5_SW_NODE DT_INST(0, nordic_nrf_sw_pwm)
#define PWM_NRF5_SW_GENERATOR_NODE DT_PHANDLE(PWM_NRF5_SW_NODE, generator)
#if DT_NODE_HAS_COMPAT(PWM_NRF5_SW_GENERATOR_NODE, nordic_nrf_rtc)
#define PWM_NRF5_SW_PPI_CHANNELS_PER_PIN	3
#else
#define PWM_NRF5_SW_PPI_CHANNELS_PER_PIN	2
#endif /* DT_NODE_HAS_COMPAT(PWM_NRF5_SW_GENERATOR_NODE, nordic_nrf_rtc) */
#define NRFX_PPI_CHANNELS_USED_BY_PWM_SW \
    (BIT_MASK(DT_PROP(PWM_NRF5_SW_NODE, channel_count) *	\
	      PWM_NRF5_SW_PPI_CHANNELS_PER_PIN)			\
         << DT_PROP(PWM_NRF5_SW_NODE, ppi_base))
#define NRFX_GPIOTE_CHANNELS_USED_BY_PWM_SW \
    DT_PROP(PWM_NRF5_SW_NODE, channel_count)
#else
#define NRFX_PPI_CHANNELS_USED_BY_PWM_SW    0
#define NRFX_GPIOTE_CHANNELS_USED_BY_PWM_SW 0
#endif

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
