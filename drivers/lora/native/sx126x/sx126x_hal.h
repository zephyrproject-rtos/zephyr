/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_SX126X_SX126X_HAL_H_
#define ZEPHYR_DRIVERS_LORA_SX126X_SX126X_HAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

/* STM32WL PA output selection */
#define SX126X_PA_OUTPUT_RFO_LP 0
#define SX126X_PA_OUTPUT_RFO_HP 1

struct sx126x_hal_config {
	struct spi_dt_spec spi;
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
	struct gpio_dt_spec tx_enable;
	struct gpio_dt_spec rx_enable;
	uint16_t tcxo_startup_delay_ms;
	uint8_t dio3_tcxo_voltage;
	bool dio2_tx_enable;
	bool dio3_tcxo_enable;
	bool rx_boosted;
	bool regulator_ldo;
	bool force_ldro;
};

struct sx126x_hal_data {
	struct gpio_callback dio1_cb;
	void (*dio1_callback)(const struct device *dev);
	const struct device *dev;
};

int sx126x_hal_init(const struct device *dev);

int sx126x_hal_reset(const struct device *dev);

int sx126x_hal_wait_busy(const struct device *dev, uint32_t timeout_ms);

bool sx126x_hal_is_busy(const struct device *dev);

int sx126x_hal_write_cmd(const struct device *dev, uint8_t opcode,
			 const uint8_t *data, size_t len);

int sx126x_hal_read_cmd(const struct device *dev, uint8_t opcode,
			uint8_t *data, size_t len);

int sx126x_hal_write_regs(const struct device *dev, uint16_t address,
			  const uint8_t *data, size_t len);

int sx126x_hal_read_regs(const struct device *dev, uint16_t address,
			 uint8_t *data, size_t len);

int sx126x_hal_write_buffer(const struct device *dev, uint8_t offset,
			    const uint8_t *data, size_t len);

int sx126x_hal_read_buffer(const struct device *dev, uint8_t offset,
			   uint8_t *data, size_t len);

int sx126x_hal_set_dio1_callback(const struct device *dev,
				 void (*callback)(const struct device *dev));

void sx126x_hal_dio1_irq_enable(const struct device *dev);

void sx126x_hal_set_antenna_enable(const struct device *dev, bool enable);

void sx126x_hal_set_rf_switch(const struct device *dev, bool tx);

int sx126x_hal_configure_gpio(const struct gpio_dt_spec *gpio,
			      gpio_flags_t flags, const char *name);

int sx126x_hal_configure_tx_params(const struct device *dev, int8_t power,
				   uint32_t frequency, uint8_t ramp_time);

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_HAL_H_ */
