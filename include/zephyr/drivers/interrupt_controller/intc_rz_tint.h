/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum intc_rz_tint_trigger {
	/** Interrupt triggered on falling edge */
	RZ_TINT_FAILING_EDGE,
	/** Interrupt triggered on rising edge */
	RZ_TINT_RISING_EDGE,
	/** Interrupt triggered on both edges */
	RZ_TINT_BOTH_EDGE,
	/** Interrupt triggered on low-level */
	RZ_TINT_LOW_LEVEL,
	/** Interrupt triggered on high-level */
	RZ_TINT_HIGH_LEVEL,
};

/** RZ GPIO interrupt (TINT) callback */
typedef void (*intc_rz_tint_callback_t)(void *arg);

/**
 * @brief Connect a TINT channel to a specific GPIO pins
 *
 * @param dev: pointer to interrupt controller instance
 * @param port: GPIO port
 * @param pin: GPIO pin
 * @return 0 on success, or negative value on error
 */
int intc_rz_tint_connect(const struct device *dev, uint8_t port, uint8_t pin);

/**
 * @brief Change trigger interrupt type
 *
 * @param dev: pointer to interrupt controller instance
 * @param trig: trigger type to be changed
 * @return 0 on success, or negative value on error
 */
int intc_rz_tint_set_type(const struct device *dev, enum intc_rz_tint_trigger trig);

/**
 * @brief Enable TINT interrupt.
 *
 * @param dev: pointer to interrupt controller instance
 * @return 0 on success, or negative value on error
 */
int intc_rz_tint_enable(const struct device *dev);

/**
 * @brief Disable TINT interrupt.
 *
 * @param dev: pointer to interrupt controller instance
 * @return 0 on success, or negative value on error
 */
int intc_rz_tint_disable(const struct device *dev);

/**
 * @brief Updates the user callback
 *
 * @param dev: pointer to interrupt controller instance
 * @param cb: callback to set
 * @param arg: user data passed to callback
 * @return 0 on success, or negative value on error
 */
int intc_rz_tint_set_callback(const struct device *dev, intc_rz_tint_callback_t cb, void *arg);
