/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_HAL_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_HAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include "sx12xx_hal.h"

struct sx128x_hal_config {
	/* First: the common functions take it from dev->config */
	struct sx12xx_hal_config common;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	struct gpio_dt_spec irq;
	uint8_t irq_dio;
	int8_t tx_power_max_dbm;
	bool tx_power_max_valid;
	bool regulator_ldo;
};

struct sx128x_hal_data {
	/* First: the common functions take it from dev->data */
	struct sx12xx_hal_data common;
	struct gpio_callback irq_cb;
	void (*irq_callback)(const struct device *dev);
	const struct device *dev;
};

int sx128x_hal_init(const struct device *dev);

int sx128x_hal_reset(const struct device *dev);

bool sx128x_hal_is_busy(const struct device *dev);

int sx128x_hal_set_irq_callback(const struct device *dev,
				void (*callback)(const struct device *dev));

/**
 * @brief Whether the IRQ line is active now
 *
 * @return 1 if active, 0 if not or if no IRQ line is wired, negative errno
 */
int sx128x_hal_irq_active(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_HAL_H_ */
