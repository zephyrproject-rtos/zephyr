/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_MCUX_LPI2C_COMMON_H_
#define ZEPHYR_DRIVERS_I2C_I2C_MCUX_LPI2C_COMMON_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>

/* The caller must serialize recovery with other controller operations. */
int mcux_lpi2c_recover_bus_common(const struct gpio_dt_spec *scl,
				const struct gpio_dt_spec *sda,
				const struct pinctrl_dev_config *pincfg, uint32_t bitrate);

#endif /* ZEPHYR_DRIVERS_I2C_I2C_MCUX_LPI2C_COMMON_H_ */
