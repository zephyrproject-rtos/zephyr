/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_lvd

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/misc/renesas_lvd/renesas_lvd.h>
#include <zephyr/logging/log.h>

#include <soc.h>

#include "r_lvd_rx_if.h"

LOG_MODULE_REGISTER(renesas_rx_lvd, CONFIG_SOC_LOG_LEVEL);

#define LVD0_NODE DT_NODELABEL(lvd0)
#define LVD1_NODE DT_NODELABEL(lvd1)
/*
 * The extern functions below are implemented in the r_lvd_rx_hw.c source file.
 * For more information, please refer to r_lvd_rx_hw.c in HAL Renesas
 */
extern void lvd_ch1_isr(void);
extern void lvd_ch2_isr(void);

struct rx_lvd_data {
	void (*callback)(void *args);
	renesas_lvd_callback_t user_callback;
	void *user_data;
};

struct rx_lvd_config {
	lvd_channel_t channel;
	lvd_config_t lvd_config;
	uint8_t vdet_target;
	uint8_t lvd_action;
	bool lvd_support_cmpa2;
};

static int renesas_rx_lvd_get_status(const struct device *dev, renesas_lvd_status_t *status)
{
	const struct rx_lvd_config *config = dev->config;
	lvd_status_position_t status_position;
	lvd_status_cross_t status_cross;
	lvd_err_t err;

	err = R_LVD_GetStatus(config->channel, &status_position, &status_cross);
	if (err != 0) {
		LOG_ERR("Failed to get LVD status");
		return -EINVAL;
	}

	status->position = (renesas_lvd_position_t)status_position;
	status->cross = (renesas_lvd_cross_t)status_cross;

	return 0;
}

static int renesas_rx_lvd_clear_status(const struct device *dev)
{
	const struct rx_lvd_config *config = dev->config;
	lvd_err_t err;

	err = R_LVD_ClearStatus(config->channel);
	if (err != 0) {
		LOG_ERR("Failed to clear LVD status");
		return -EINVAL;
	}

	return 0;
}

static int renesas_rx_lvd_callback_set(const struct device *dev, renesas_lvd_callback_t callback,
				       void *user_data)
{
	struct rx_lvd_data *data = dev->data;

	data->user_callback = callback;
	data->user_data = user_data;

	return 0;
}

static int renesas_rx_pin_set_cmpa2(const struct device *dev)
{
	const struct rx_lvd_config *config = dev->config;
	const struct pinctrl_dev_config *pcfg;

	if (config->channel == 0) {
		if (DT_NODE_HAS_PROP(LVD0_NODE, pinctrl_0)) {
			PINCTRL_DT_DEFINE(LVD0_NODE);
			pcfg = PINCTRL_DT_DEV_CONFIG_GET(LVD0_NODE);
		} else {
			LOG_ERR("No pinctrl-0 property found in the device tree");
			return -EINVAL;
		}
	} else {
		if (DT_NODE_HAS_PROP(LVD1_NODE, pinctrl_0)) {
			PINCTRL_DT_DEFINE(LVD1_NODE);
			pcfg = PINCTRL_DT_DEV_CONFIG_GET(LVD1_NODE);
		} else {
			LOG_ERR("No pinctrl_0 property found in the device tree");
			return -EINVAL;
		}
	}

	/* In the case of monitoring the CMPA2 pin, set the CMPA2 pin. */
	/* This only applicable to channel 1 with the LVDb driver */
	int ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

#define LVD_IRQ_CONNECT()                                                                          \
	do {                                                                                       \
		IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(LVD0_NODE), ( \
			IRQ_CONNECT(DT_IRQN(LVD0_NODE),               \
						DT_IRQ(LVD0_NODE, priority),      \
						lvd_ch1_isr,                 \
						DEVICE_DT_GET(LVD0_NODE),         \
						0);                               \
			irq_enable(DT_IRQN(LVD0_NODE));               \
		))                                 \
		IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(LVD1_NODE), ( \
			IRQ_CONNECT(DT_IRQN(LVD1_NODE),               \
						DT_IRQ(LVD1_NODE, priority),      \
						lvd_ch2_isr,                 \
						DEVICE_DT_GET(LVD1_NODE),         \
						0);                               \
			irq_enable(DT_IRQN(LVD1_NODE));               \
		))                                 \
	} while (0)

static int renesas_rx_lvd_init(const struct device *dev)
{
	lvd_err_t err;

	LVD_IRQ_CONNECT();

	const struct rx_lvd_config *config = dev->config;
	const struct rx_lvd_data *data = dev->data;

	/* In reset or no-action when LVD is detected, callback will not be triggered. */
	err = R_LVD_Open(config->channel, &config->lvd_config, data->callback);
	if (err != 0) {
		LOG_ERR("Failed to initialize LVD channel %d", config->channel);
		return -EIO;
	}

	/* Set the CMPA2 pin if the target is CMPA2 */
	/* NOTE: For the RX130 series, CMPA2 is only used on channel 2. */
	if ((config->lvd_support_cmpa2) && (config->vdet_target == 1)) {
		return renesas_rx_pin_set_cmpa2(dev);
	}

	return 0;
}

static DEVICE_API(renesas_lvd, renesas_rx_lvd_driver_api) = {
	.get_status = renesas_rx_lvd_get_status,
	.clear_status = renesas_rx_lvd_clear_status,
	.callback_set = renesas_rx_lvd_callback_set,
};

#define RENESAS_RX_LVD_INIT(index)                                                                 \
                                                                                                   \
	static const struct rx_lvd_config rx_lvd_config_##index = {                                \
		.channel = DT_INST_PROP(index, channel),                                           \
		.lvd_config =                                                                      \
			{                                                                          \
				.trigger = DT_INST_ENUM_IDX(index, lvd_trigger),                   \
			},                                                                         \
		.lvd_action = DT_INST_ENUM_IDX(index, lvd_action),                                 \
		.vdet_target = DT_INST_ENUM_IDX(index, vdet_target),                               \
		.lvd_support_cmpa2 = DT_INST_PROP(index, lvd_support_cmpa2),                       \
	};                                                                                         \
                                                                                                   \
	void rx_lvd_callback_##index(void *args)                                                   \
	{                                                                                          \
		ARG_UNUSED(args);                                                                  \
		const struct device *dev = DEVICE_DT_GET(DT_INST(index, renesas_rx_lvd));          \
		struct rx_lvd_data *data = dev->data;                                              \
                                                                                                   \
		/* Call the user's callback function*/                                             \
		if (data->user_callback) {                                                         \
			data->user_callback(dev, data->user_data);                                 \
		}                                                                                  \
	};                                                                                         \
                                                                                                   \
	static struct rx_lvd_data rx_lvd_data_##index = {                                          \
		.callback = rx_lvd_callback_##index,                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, renesas_rx_lvd_init, NULL, &rx_lvd_data_##index,              \
			      &rx_lvd_config_##index, PRE_KERNEL_1,                                \
			      CONFIG_RENESAS_LVD_INIT_PRIORITY, &renesas_rx_lvd_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RX_LVD_INIT)
