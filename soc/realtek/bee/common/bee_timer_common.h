/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bee_timer_common.h
 * @brief Realtek Bee Series Common Timer Abstraction Layer.
 */

#ifndef ZEPHYR_DRIVERS_COMMON_BEE_TIMER_H_
#define ZEPHYR_DRIVERS_COMMON_BEE_TIMER_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_tim.h>
#include <rtl_enh_tim.h>
#include <rtl_rcc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_tim.h>
#include <rtl876x_enh_tim.h>
#include <rtl876x_rcc.h>
#include <rtl876x_nvic.h>
#include <vector_table.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

/**
 * @defgroup bee_timer_abstraction Timer Abstraction Interface
 * @brief Common structures and enums for timer operations.
 * @{
 */

/**
 * @brief Bee Timer Operation Modes.
 */
enum bee_timer_mode {
	/** Mode for Zephyr Counter driver (Timer counting down). */
	BEE_TIMER_MODE_COUNTER,
	/** Mode for Zephyr PWM driver (Pulse Width Modulation). */
	BEE_TIMER_MODE_PWM,
};

/**
 * @brief Unified Timer Operations Structure.
 *
 * This structure holds function pointers to the specific HAL implementations.
 * Upper-layer drivers use these pointers to control the hardware.
 */
struct bee_timer_ops {
	/**
	 * @brief Initialize the timer hardware.
	 * @param reg Base address of the timer register.
	 * @param prescaler_idx Prescaler value or index (SoC dependent).
	 * @param top_val The reload/max count value (Period).
	 * @param mode Operation mode (Counter or PWM).
	 */
	void (*init)(uint32_t reg, uint8_t prescaler_idx, uint32_t top_val,
		     enum bee_timer_mode mode);

	/**
	 * @brief Start the timer.
	 * @param reg Base address of the timer register.
	 */
	void (*start)(uint32_t reg);

	/**
	 * @brief Stop the timer.
	 * @param reg Base address of the timer register.
	 */
	void (*stop)(uint32_t reg);

	/**
	 * @brief Get the current counter value.
	 * @param reg Base address of the timer register.
	 * @return Current counter tick value.
	 */
	uint32_t (*get_count)(uint32_t reg);

	/**
	 * @brief Get the current top (load/max) value.
	 * @param reg Base address of the timer register.
	 * @return Current top value.
	 */
	uint32_t (*get_top)(uint32_t reg);

	/**
	 * @brief Set a new top (period) value.
	 * @param reg Base address of the timer register.
	 * @param top_val New top value to set.
	 */
	void (*set_top)(uint32_t reg, uint32_t top_val);

	/**
	 * @brief Configure PWM duty cycle.
	 * @param reg Base address of the timer register.
	 * @param period_cyc Total cycle count for the period.
	 * @param pulse_cyc Active pulse width in cycles.
	 * @param inverted If true, the polarity is inverted.
	 */
	void (*set_pwm_duty)(uint32_t reg, uint32_t period_cyc, uint32_t pulse_cyc, bool inverted);

	/**
	 * @brief Enable the timer interrupt.
	 * @param reg Base address of the timer register.
	 */
	void (*int_enable)(uint32_t reg);

	/**
	 * @brief Disable the timer interrupt.
	 * @param reg Base address of the timer register.
	 */
	void (*int_disable)(uint32_t reg);

	/**
	 * @brief Clear the timer interrupt pending status.
	 * @param reg Base address of the timer register.
	 */
	void (*int_clear)(uint32_t reg);

	/**
	 * @brief Check if the timer interrupt is pending.
	 * @param reg Base address of the timer register.
	 * @return true if interrupt is pending, false otherwise.
	 */
	bool (*int_status)(uint32_t reg);
};

/**
 * @brief Retrieve the Timer Operations structure.
 *
 * This is the factory function used by drivers to get the correct function pointers.
 *
 * @param enhanced Set to true if requesting Enhanced Timer ops, false for Basic Timer.
 * @return const struct bee_timer_ops* Pointer to the operations structure, or NULL if not
 * supported.
 */
const struct bee_timer_ops *bee_timer_get_ops(bool enhanced);

/** @} */

#endif /* ZEPHYR_DRIVERS_COMMON_BEE_TIMER_H_ */
