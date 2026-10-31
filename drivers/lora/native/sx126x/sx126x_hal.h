/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_SX126X_SX126X_HAL_H_
#define ZEPHYR_DRIVERS_LORA_SX126X_SX126X_HAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include "sx12xx_hal.h"

/* STM32WL PA output selection */
#define SX126X_PA_OUTPUT_RFO_LP 0
#define SX126X_PA_OUTPUT_RFO_HP 1

struct sx126x_hal_config {
	/* First: the common functions take it from dev->config */
	struct sx12xx_hal_config common;
#ifdef CONFIG_LORA_SX126X_NATIVE_STANDALONE
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	struct gpio_dt_spec dio1;
	bool is_sx1261;
#elif CONFIG_LORA_SX126X_NATIVE_STM32WL
	uint8_t pa_output;
	int8_t rfo_lp_max_power;
	int8_t rfo_hp_max_power;
#endif /* CONFIG_LORA_SX126X_NATIVE_STM32WL */
	struct gpio_dt_spec antenna_enable;
	uint16_t tcxo_startup_delay_ms;
	uint8_t dio3_tcxo_voltage;
	bool dio2_tx_enable;
	bool dio3_tcxo_enable;
	bool rx_boosted;
	bool regulator_ldo;
	bool force_ldro;
};

struct sx126x_hal_data {
	/* First: the common functions take it from dev->data */
	struct sx12xx_hal_data common;
	struct gpio_callback dio1_cb;
	void (*dio1_callback)(const struct device *dev);
	const struct device *dev;
};

int sx126x_hal_init(const struct device *dev);

int sx126x_hal_reset(const struct device *dev);

bool sx126x_hal_is_busy(const struct device *dev);

int sx126x_hal_set_dio1_callback(const struct device *dev,
				 void (*callback)(const struct device *dev));

void sx126x_hal_dio1_irq_enable(const struct device *dev);

int sx126x_hal_configure_tx_params(const struct device *dev, int8_t power,
				   uint32_t frequency, uint8_t ramp_time);

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_HAL_H_ */
