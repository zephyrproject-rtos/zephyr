/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX serial driver API
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_FT8XX_FT8XX_DRV_H_
#define ZEPHYR_DRIVERS_DISPLAY_FT8XX_FT8XX_DRV_H_

#include <stdint.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

int ft8xx_drv_init(const struct device *dev);
int ft8xx_drv_read(const struct device *dev, uint32_t address, uint8_t *data, unsigned int length);
int ft8xx_drv_write(const struct device *dev, uint32_t address, const uint8_t *data,
		    unsigned int length);
int ft8xx_drv_command(const struct device *dev, uint8_t command);

extern void ft8xx_drv_irq_triggered(const struct device *dev,
		struct gpio_callback *cb, uint32_t pins);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_DISPLAY_FT8XX_FT8XX_DRV_H_ */
