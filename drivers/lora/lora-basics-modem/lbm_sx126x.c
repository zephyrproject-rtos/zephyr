/*
 * Copyright (c) 2025 Embeint Inc
 * Copyright (c) 2026 Jakub Rzeszutko <jakub.rzeszutko@verkada.com>
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "lbm_sx126x_core.h"

LOG_MODULE_DECLARE(lbm_driver, CONFIG_LORA_LOG_LEVEL);

/* Amplifier settings from DS.SX1261-2.W.APP Rev 2.2, 13.1.14. */
#define SX1261_TX_PWR_MAX 15
#define SX1261_TX_PWR_MIN -17
#define SX1262_TX_PWR_MAX 22
#define SX1262_TX_PWR_MIN -9

enum lbm_sx126x_variant {
	VARIANT_SX1261,
	VARIANT_SX1262,
};

struct lbm_sx126x_config {
	struct lbm_sx126x_core_config common;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec busy;
	enum lbm_sx126x_variant variant;
};

static int lbm_sx126x_pins_init(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	int ret;

	ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("Could not configure reset pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->busy, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Could not configure busy pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->common.lbm_common.dio1, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Could not configure DIO1 pin: %d", ret);
		return ret;
	}

	return 0;
}

static void lbm_sx126x_reset(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	gpio_pin_set_dt(&config->reset, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set_dt(&config->reset, 0);
	k_sleep(K_MSEC(10));
}

static bool lbm_sx126x_is_busy(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	return gpio_pin_get_dt(&config->busy);
}

static void lbm_sx126x_dio1_irq_enable(const struct device *dev, bool enable)
{
	const struct lbm_sx126x_config *config = dev->config;

	(void)gpio_pin_interrupt_configure_dt(&config->common.lbm_common.dio1,
					      enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
}

static void lbm_sx126x_dio1_callback(const struct device *dev, struct gpio_callback *cb,
				     uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct lbm_sx126x_core_data *data =
		CONTAINER_OF(cb, struct lbm_sx126x_core_data, dio1_callback);

	LOG_DBG("");
	/* Submit work to process the interrupt immediately */
	k_work_schedule(&data->lbm_common.op_done_work, K_NO_WAIT);
}

static int lbm_sx126x_irq_init(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;
	struct lbm_sx126x_core_data *data = dev->data;
	int ret;

	gpio_init_callback(&data->dio1_callback, lbm_sx126x_dio1_callback,
			   BIT(config->common.lbm_common.dio1.pin));
	ret = gpio_add_callback(config->common.lbm_common.dio1.port, &data->dio1_callback);
	if (ret < 0) {
		LOG_ERR("Could not set GPIO callback for DIO1 interrupt.");
		return ret;
	}

	lbm_sx126x_dio1_irq_enable(dev, true);

	return 0;
}

static int lbm_sx126x_dio1_callback_add(const struct device *dev, struct gpio_callback *callback,
					gpio_callback_handler_t handler)
{
	const struct lbm_sx126x_config *config = dev->config;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (callback == NULL || handler == NULL) {
		return -EINVAL;
	}

	gpio_init_callback(callback, handler, BIT(config->common.lbm_common.dio1.pin));

	ret = gpio_add_callback(config->common.lbm_common.dio1.port, callback);
	if (ret < 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}

	lbm_sx126x_dio1_irq_enable(dev, true);

	LOG_DBG("Added user GPIO callback");
	return 0;
}

static int lbm_sx126x_dio1_callback_remove(const struct device *dev, struct gpio_callback *callback)
{
	const struct lbm_sx126x_config *config = dev->config;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (callback == NULL) {
		return -EINVAL;
	}

	ret = gpio_remove_callback(config->common.lbm_common.dio1.port, callback);
	if (ret < 0) {
		LOG_ERR("Failed to remove GPIO callback: %d", ret);
		return ret;
	}

	LOG_DBG("Removed user GPIO callback");
	return 0;
}

static void lbm_sx126x_get_pa_cfg(const struct device *dev, int16_t power,
				  ral_sx126x_bsp_tx_cfg_output_params_t *output_params)
{
	const struct lbm_sx126x_config *config = dev->config;

	if (config->variant == VARIANT_SX1261) {
		power = CLAMP(power, SX1261_TX_PWR_MIN, SX1261_TX_PWR_MAX);
		output_params->pa_cfg.device_sel = 0x01;
		output_params->chip_output_pwr_in_dbm_configured = power;
		output_params->chip_output_pwr_in_dbm_expected = power;
		if (power == 15) {
			output_params->chip_output_pwr_in_dbm_configured = 14;
			output_params->pa_cfg.pa_duty_cycle = 0x06;
		} else {
			output_params->pa_cfg.pa_duty_cycle = 0x04;
		}
	} else {
		power = CLAMP(power, SX1262_TX_PWR_MIN, SX1262_TX_PWR_MAX);
		output_params->pa_cfg.device_sel = 0x00;
		output_params->pa_cfg.hp_max = 0x07;
		output_params->pa_cfg.pa_duty_cycle = 0x04;
		output_params->chip_output_pwr_in_dbm_configured = power;
		output_params->chip_output_pwr_in_dbm_expected = power;
	}
}

static const struct lbm_sx126x_core_ops lbm_sx126x_ops = {
	.pins_init = lbm_sx126x_pins_init,
	.irq_init = lbm_sx126x_irq_init,
	.reset = lbm_sx126x_reset,
	.is_busy = lbm_sx126x_is_busy,
	.get_pa_cfg = lbm_sx126x_get_pa_cfg,
	.dio1_irq_enable = lbm_sx126x_dio1_irq_enable,
	.dio1_callback_add = lbm_sx126x_dio1_callback_add,
	.dio1_callback_remove = lbm_sx126x_dio1_callback_remove,
};

#define SX126X_DEFINE(node_id, sx_variant)                                                         \
	static const struct lbm_sx126x_config config_##node_id = {                                 \
		.common = {LBM_SX126X_CONFIG_COMMON(node_id, &lbm_sx126x_ops),                     \
			   .lbm_common.dio1 = GPIO_DT_SPEC_GET(node_id, dio1_gpios)},              \
		.reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),                                   \
		.busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),                                     \
		.variant = sx_variant,                                                             \
	};                                                                                         \
	static struct lbm_sx126x_core_data data_##node_id;                                         \
	DEVICE_DT_DEFINE(node_id, lbm_sx126x_core_init, NULL, &data_##node_id, &config_##node_id,  \
			 POST_KERNEL, CONFIG_LORA_INIT_PRIORITY, &lbm_lora_api)

#define SX1261_DEFINE(node_id) SX126X_DEFINE(node_id, VARIANT_SX1261)
#define SX1262_DEFINE(node_id) SX126X_DEFINE(node_id, VARIANT_SX1262)

DT_FOREACH_STATUS_OKAY(semtech_sx1261, SX1261_DEFINE);
DT_FOREACH_STATUS_OKAY(semtech_sx1262, SX1262_DEFINE);
DT_FOREACH_STATUS_OKAY(semtech_sx1268, SX1262_DEFINE);
DT_FOREACH_STATUS_OKAY(semtech_llcc68, SX1262_DEFINE);
