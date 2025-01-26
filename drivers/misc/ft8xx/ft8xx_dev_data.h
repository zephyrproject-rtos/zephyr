/*
 * Copyright (c) 2024 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX device driver data structure
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_DEV_DATA_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_DEV_DATA_H_

#include <stdint.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ft8xx_data {
	const struct device *ft8xx_dev; /* Required for GPIO IRQ handling */
	ft8xx_int_callback irq_callback;
	void *irq_callback_ud;

	const struct spi_dt_spec spi;
	const struct gpio_dt_spec irq_gpio;
	struct gpio_callback irq_cb_data;

	uint16_t reg_cmd_read;
	uint16_t reg_cmd_write;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_DEV_DATA_H_ */
