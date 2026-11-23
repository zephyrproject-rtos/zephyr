/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP Grouped GPIO Input Interrupt (GINT) driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_GINT_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_GINT_H_

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GINT trigger types
 */
typedef enum {
	/** Edge triggered */
	NXP_GINT_TRIG_EDGE = 0,
	/** Level triggered */
	NXP_GINT_TRIG_LEVEL = 1,
} nxp_gint_trigger_type;

/**
 * @brief GINT combination logic
 */
typedef enum {
	/** OR logic - any enabled pin triggers interrupt */
	NXP_GINT_COMB_OR = 0,
	/** AND logic - all enabled pins must trigger interrupt */
	NXP_GINT_COMB_AND = 1,
} nxp_gint_combination_type;

/**
 * @brief GINT pin polarity
 */
typedef enum {
	/** Active LOW */
	NXP_GINT_POL_LOW = 0,
	/** Active HIGH */
	NXP_GINT_POL_HIGH = 1,
} nxp_gint_polarity_type;

/**
 * @brief GINT group configuration
 */
struct nxp_gint_group_config {
	/** Trigger type (edge or level) */
	nxp_gint_trigger_type trigger;
	/** Combination logic (OR or AND) */
	nxp_gint_combination_type combination;
};

/**
 * @brief GINT callback function type
 *
 * @param dev GINT device
 * @param user_data User defined data
 */
typedef void (*nxp_gint_callback_t)(const struct device *dev, void *user_data);

/**
 * @brief Configure GINT settings
 *
 * @param dev GINT device
 * @param config Group configuration
 * @return 0 on success, negative errno on failure
 */
int nxp_gint_configure_group(const struct device *dev,
			     const struct nxp_gint_group_config *config);

/**
 * @brief Enable a pin for GINT
 *
 * @param dev GINT device
 * @param port Port number
 * @param pin Pin number (0-31)
 * @param polarity Pin polarity
 * @return 0 on success, negative errno on failure
 */
int nxp_gint_enable_pin(const struct device *dev, uint8_t port, uint8_t pin,
			nxp_gint_polarity_type polarity);

/**
 * @brief Disable a pin for GINT
 *
 * @param dev GINT device
 * @param port Port number
 * @param pin Pin number (0-31)
 * @return 0 on success, negative errno on failure
 */
int nxp_gint_disable_pin(const struct device *dev, uint8_t port, uint8_t pin);

/**
 * @brief Check if GINT interrupt is pending
 *
 * @param dev GINT device
 * @return true if interrupt is pending, false otherwise
 */
bool nxp_gint_is_pending(const struct device *dev);

/**
 * @brief Clear GINT interrupt pending status
 *
 * @param dev GINT device
 * @return 0 on success, negative errno on failure
 */
int nxp_gint_clear_pending(const struct device *dev);

/**
 * @brief Register GINT callback
 *
 * @param dev GINT device
 * @param callback Callback function to register
 * @param user_data User defined data to pass to callback
 * @return 0 on success, negative errno on failure
 */
int nxp_gint_register_callback(const struct device *dev,
			       nxp_gint_callback_t callback,
			       void *user_data);

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_GINT_H_ */
