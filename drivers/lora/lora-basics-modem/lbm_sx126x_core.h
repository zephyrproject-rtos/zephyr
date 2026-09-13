/*
 * Copyright (c) 2025 Embeint Inc
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_SX126X_CORE_H_
#define ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_SX126X_CORE_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "ralf_sx126x.h"
#include "ral_sx126x_bsp.h"

#include "lbm_common.h"

/**
 * @brief What the core needs from the driver that owns the radio
 *
 * The core speaks the SX126x command set and nothing else. Everything that
 * depends on where the radio sits, and on which of its two amplifiers the
 * board wired up, is answered here by the driver.
 */
struct lbm_sx126x_ops {
	/** @driver_ops_optional Claim the pins the radio needs, before anything drives reset. */
	int (*pins_init)(const struct device *dev);
	/** @driver_ops_mandatory Claim the radio interrupt once the radio answers. */
	int (*variant_init)(const struct device *dev);
	/** @driver_ops_mandatory Hold the radio in reset long enough for it to take. */
	void (*reset)(const struct device *dev);
	/** @driver_ops_mandatory Whether the radio is still working on the last command. */
	bool (*is_busy)(const struct device *dev);
	/** @driver_ops_mandatory Amplifier settings for a requested output power. */
	void (*get_tx_cfg)(const struct device *dev, int16_t power,
			   ral_sx126x_bsp_tx_cfg_output_params_t *output_params);
	/** @driver_ops_optional Overcurrent limit, left alone when absent. */
	void (*get_ocp)(const struct device *dev, uint8_t *ocp_in_step_of_2_5_ma);
	/**
	 * @driver_ops_mandatory Let the radio interrupt through, or keep it
	 * out. It does not always arrive on a pin, so how it is unmasked is
	 * the driver's to say.
	 */
	void (*dio1_irq_set)(const struct device *dev, bool enable);
	/** @driver_ops_optional Add a caller's handler to the radio interrupt. */
	int (*dio1_callback_add)(const struct device *dev, struct gpio_callback *callback,
				 gpio_callback_handler_t handler);
	/** @driver_ops_optional Take one back off. */
	int (*dio1_callback_remove)(const struct device *dev, struct gpio_callback *callback);
};

struct lbm_sx126x_config {
	struct lbm_lora_config_common lbm_common;
	const struct lbm_sx126x_ops *ops;
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
};

struct lbm_sx126x_data {
	struct lbm_lora_data_common lbm_common;
	const struct device *dev;
	struct gpio_callback dio1_callback;
	bool asleep;
};

#define LBM_SX126X_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB)

/**
 * @brief The properties a radio has wherever its core sits
 *
 * What a driver adds to this is what only its own radio has: the pins a
 * discrete part is reached over, or the amplifier a built-in one carries.
 */
#define LBM_SX126X_CONFIG_COMMON(node_id)                                                          \
	.lbm_common.ralf = RALF_SX126X_INSTANTIATE(DEVICE_DT_GET(node_id)),                        \
	.lbm_common.force_ldro = DT_PROP(node_id, force_ldro),                                     \
	.spi = SPI_DT_SPEC_GET(node_id, LBM_SX126X_SPI_OPERATION),                                 \
	.ant_enable = GPIO_DT_SPEC_GET_OR(node_id, antenna_enable_gpios, {0}),                     \
	.tx_enable = GPIO_DT_SPEC_GET_OR(node_id, tx_enable_gpios, {0}),                           \
	.rx_enable = GPIO_DT_SPEC_GET_OR(node_id, rx_enable_gpios, {0}),                           \
	.dio3_tcxo_startup_delay_ms = DT_PROP_OR(node_id, tcxo_power_startup_delay_ms, 0),         \
	.dio3_tcxo_voltage = DT_PROP_OR(node_id, dio3_tcxo_voltage, UINT8_MAX),                    \
	.dio2_rf_switch = DT_PROP(node_id, dio2_tx_enable),                                        \
	.rx_boosted = DT_PROP(node_id, rx_boosted),                                                \
	.regulator_ldo = DT_PROP(node_id, regulator_ldo)

/** @brief Bring the radio up. Passed to DEVICE_DT_DEFINE() by the driver. */
int lbm_sx126x_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_SX126X_CORE_H_ */
