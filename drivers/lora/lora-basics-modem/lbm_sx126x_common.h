/*
 * Copyright (c) 2025 Embeint Inc
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_SX126X_COMMON_H_
#define ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_SX126X_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "lbm_common.h"

#define SX126X_PA_OUTPUT_RFO_LP 0
#define SX126X_PA_OUTPUT_RFO_HP 1

enum sx126x_variant {
	VARIANT_SX1261,
	VARIANT_SX1262,
	VARIANT_STM32WL,
};

struct lbm_sx126x_config {
	struct lbm_lora_config_common lbm_common;
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	struct gpio_dt_spec ant_enable;
	struct gpio_dt_spec tx_enable;
	struct gpio_dt_spec rx_enable;
	int dio3_tcxo_startup_delay_ms;
	uint8_t dio3_tcxo_voltage;
	bool dio2_rf_switch;
	bool rx_boosted;
	bool regulator_ldo;
	enum sx126x_variant variant;
	/* STM32WL only, the part has two power amplifier outputs */
	uint8_t pa_output;
	int8_t rfo_lp_max_power;
	int8_t rfo_hp_max_power;
};

struct lbm_sx126x_data {
	struct lbm_lora_data_common lbm_common;
	const struct device *dev;
	struct gpio_callback dio1_callback;
	bool asleep;
};

/**
 * @brief Claim the pins the radio needs.
 *
 * Runs at device init, before anything drives the reset line. A part whose
 * radio is on-die has no pins to claim here.
 */
int lbm_sx126x_pins_init(const struct device *dev);

/**
 * @brief Bring the board glue up.
 *
 * Runs once the radio answers. Claims whatever the radio interrupt arrives on
 * and leaves it enabled.
 */
int lbm_sx126x_variant_init(const struct device *dev);

/** @brief Hold the radio in reset long enough for it to take, then release it. */
void lbm_sx126x_reset(const struct device *dev);

/** @brief Whether the radio is still working on the last command. */
bool lbm_sx126x_is_busy(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_SX126X_COMMON_H_ */
