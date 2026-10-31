/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt route operations.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_IRQ_ROUTE_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_IRQ_ROUTE_H_

#include <stdbool.h>

#include <zephyr/device.h>

/**
 * @brief Operations for one level of an interrupt route.
 *
 * A route uses the controller's local line number. The caller resolves
 * the route from the device tree map before calling these operations.
 */
struct irq_route_ops {
	/** @brief Check whether the controller is ready. */
	bool (*is_ready)(const struct device *dev);
	/** @brief Enable a local line. */
	void (*enable)(const struct device *dev, unsigned int line);
	/** @brief Disable a local line. */
	void (*disable)(const struct device *dev, unsigned int line);
	/** @brief Check whether a local line is enabled. */
	int (*is_enabled)(const struct device *dev, unsigned int line);
	/** @brief Acknowledge a local line. */
	void (*eoi)(const struct device *dev, unsigned int line);
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_IRQ_ROUTE_H_ */
