/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_HAL_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_HAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

struct sx128x_hal_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	struct gpio_dt_spec irq;
	uint8_t irq_dio;
	struct gpio_dt_spec tx_enable;
	struct gpio_dt_spec rx_enable;
	int8_t tx_power_max_dbm;
	bool tx_power_max_valid;
	bool regulator_ldo;
};

struct sx128x_hal_data {
	struct gpio_callback irq_cb;
	void (*irq_callback)(const struct device *dev);
	const struct device *dev;
};

int sx128x_hal_init(const struct device *dev);

int sx128x_hal_reset(const struct device *dev);

int sx128x_hal_wait_busy(const struct device *dev, uint32_t timeout_ms);

int sx128x_hal_write_cmd(const struct device *dev, uint8_t opcode, const uint8_t *data, size_t len);

int sx128x_hal_read_cmd(const struct device *dev, uint8_t opcode, uint8_t *data, size_t len);

int sx128x_hal_write_regs(const struct device *dev, uint16_t address, const uint8_t *data,
			  size_t len);

int sx128x_hal_read_regs(const struct device *dev, uint16_t address, uint8_t *data, size_t len);

int sx128x_hal_write_buffer(const struct device *dev, uint8_t offset, const uint8_t *data,
			    size_t len);

int sx128x_hal_read_buffer(const struct device *dev, uint8_t offset, uint8_t *data, size_t len);

int sx128x_hal_set_irq_callback(const struct device *dev,
				void (*callback)(const struct device *dev));

void sx128x_hal_set_rf_switch(const struct device *dev, bool enable, bool tx);

int sx128x_hal_wakeup(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_HAL_H_ */
