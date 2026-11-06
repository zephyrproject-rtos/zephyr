/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LR20XX_LR20XX_HAL_H_
#define ZEPHYR_DRIVERS_LORA_LR20XX_LR20XX_HAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

struct lr20xx_hal_data {
	struct gpio_callback irq_cb;
	void (*irq_callback)(const struct device *dev);
	const struct device *dev;
};

int lr20xx_hal_init(const struct device *dev);

int lr20xx_hal_reset(const struct device *dev);

int lr20xx_hal_wait_busy(const struct device *dev, uint32_t timeout_ms);

bool lr20xx_hal_is_busy(const struct device *dev);

int lr20xx_hal_write_cmd(const struct device *dev, uint16_t opcode,
			 const uint8_t *data, size_t len);

int lr20xx_hal_read_cmd(const struct device *dev, uint16_t opcode, uint8_t *data, size_t len);

int lr20xx_hal_read_status(const struct device *dev, uint8_t stats[2], uint32_t *irq_status);

int lr20xx_hal_write_fifo(const struct device *dev, const uint8_t *data, size_t len);

int lr20xx_hal_read_fifo(const struct device *dev, uint8_t *data, size_t len);

int lr20xx_hal_set_irq_callback(const struct device *dev,
				 void (*callback)(const struct device *dev));

void lr20xx_hal_irq_enable(const struct device *dev);

int lr20xx_hal_wakeup(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_LORA_LR20XX_LR20XX_HAL_H_ */
