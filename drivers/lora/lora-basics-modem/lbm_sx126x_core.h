/*
 * Copyright (c) 2025 Embeint Inc
 * Copyright (c) 2026 Jakub Rzeszutko <jakub.rzeszutko@verkada.com>
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
 * @brief Operations a driver provides for the SX126x core it owns
 *
 * The core issues the SX126x command set over SPI. Reset, busy, the radio
 * interrupt and the power amplifier reach the silicon differently on a
 * discrete part and on an STM32WL, so the driver answers for those.
 */
struct lbm_sx126x_core_ops {
	/**
	 * @driver_ops_optional Configure the pins the radio is reached over,
	 * before anything drives reset. Absent when the radio is on-die.
	 *
	 * @param dev Modem device
	 * @retval 0 On success
	 * @retval -errno Negative errno code on failure
	 */
	int (*pins_init)(const struct device *dev);
	/**
	 * @driver_ops_mandatory Claim the radio interrupt and unmask it. Runs
	 * once the radio answers.
	 *
	 * @param dev Modem device
	 * @retval 0 On success
	 * @retval -errno Negative errno code on failure
	 */
	int (*irq_init)(const struct device *dev);
	/**
	 * @driver_ops_mandatory Assert reset for at least 100 us, then release
	 * it (DS.SX1261-2.W.APP Rev 2.2, 8.1).
	 *
	 * @param dev Modem device
	 */
	void (*reset)(const struct device *dev);
	/**
	 * @driver_ops_mandatory Read the busy signal.
	 *
	 * @param dev Modem device
	 * @return true while the radio is still executing the last command
	 */
	bool (*is_busy)(const struct device *dev);
	/**
	 * @driver_ops_mandatory Fill in the amplifier configuration for a
	 * requested output power.
	 *
	 * @param dev Modem device
	 * @param power Requested output power in dBm
	 * @param output_params Amplifier settings to fill in
	 */
	void (*get_pa_cfg)(const struct device *dev, int16_t power,
			   ral_sx126x_bsp_tx_cfg_output_params_t *output_params);
	/**
	 * @driver_ops_optional Overcurrent limit for the amplifier in use.
	 * Absent leaves the radio at its reset value.
	 *
	 * @param dev Modem device
	 * @param ocp_in_step_of_2_5_ma Limit to apply, in steps of 2.5 mA
	 */
	void (*get_ocp)(const struct device *dev, uint8_t *ocp_in_step_of_2_5_ma);
	/**
	 * @driver_ops_mandatory Unmask or mask the radio interrupt. It reaches
	 * the core over a pin on a discrete part and over the NVIC on an
	 * STM32WL.
	 *
	 * @param dev Modem device
	 * @param enable true to let the interrupt through
	 */
	void (*dio1_irq_enable)(const struct device *dev, bool enable);
	/**
	 * @driver_ops_optional Register a caller's handler for the radio
	 * interrupt. Absent when the interrupt is not a GPIO.
	 *
	 * @param dev Modem device
	 * @param callback GPIO callback structure, initialized by the driver
	 * @param handler Handler to invoke
	 * @retval 0 On success
	 * @retval -errno Negative errno code on failure
	 */
	int (*dio1_callback_add)(const struct device *dev, struct gpio_callback *callback,
				 gpio_callback_handler_t handler);
	/**
	 * @driver_ops_optional Remove a handler added by @ref
	 * lbm_sx126x_core_ops.dio1_callback_add.
	 *
	 * @param dev Modem device
	 * @param callback GPIO callback structure to remove
	 * @retval 0 On success
	 * @retval -errno Negative errno code on failure
	 */
	int (*dio1_callback_remove)(const struct device *dev, struct gpio_callback *callback);
};

struct lbm_sx126x_core_config {
	struct lbm_lora_config_common lbm_common;
	const struct lbm_sx126x_core_ops *ops;
	struct spi_dt_spec spi;
	struct gpio_dt_spec ant_enable;
	struct gpio_dt_spec tx_enable;
	struct gpio_dt_spec rx_enable;
	int dio3_tcxo_startup_delay_ms;
	uint8_t dio3_tcxo_voltage;
	bool dio2_rf_switch;
	bool rx_boosted;
	bool regulator_ldo;
};

struct lbm_sx126x_core_data {
	struct lbm_lora_data_common lbm_common;
	const struct device *dev;
	struct gpio_callback dio1_callback;
	bool asleep;
};

#define LBM_SX126X_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB)

/**
 * @brief Devicetree properties every SX126x carries
 *
 * A driver adds the properties only its own radio has.
 *
 * @param node_id Devicetree node identifier of the radio
 * @param _ops Operations the driver answers with
 */
#define LBM_SX126X_CONFIG_COMMON(node_id, _ops)                                                    \
	.ops = (_ops), .lbm_common.ralf = RALF_SX126X_INSTANTIATE(DEVICE_DT_GET(node_id)),         \
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

/**
 * @brief Bring the radio up
 *
 * Passed to DEVICE_DT_DEFINE() by the driver.
 *
 * @param dev Modem device
 * @retval 0 On success
 * @retval -ENODEV If the SPI bus is not ready
 * @retval -errno Negative errno code on failure
 */
int lbm_sx126x_core_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_LORA_LORA_BASICS_MODEM_LBM_SX126X_CORE_H_ */
