/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_lvd

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "r_lvd_rx_if.h"

LOG_MODULE_REGISTER(renesas_rx_lvd, CONFIG_COMPARATOR_LOG_LEVEL);

#define LVD0_NODE           DT_NODELABEL(lvd0)
#define LVD1_NODE           DT_NODELABEL(lvd1)
#define LVD_RENESAS_RX_FLAG BIT(0)
/*
 * The extern functions below are implemented in the r_lvd_rx_hw.c source file.
 * For more information, please refer to r_lvd_rx_hw.c in HAL Renesas
 */
extern void lvd_ch1_isr(void);
extern void lvd_ch2_isr(void);
extern void lvd_start_lvd(lvd_channel_t ch, lvd_trigger_t trigger);
extern void lvd_stop_lvd(lvd_channel_t ch);
extern void lvd_start_int(lvd_channel_t ch, void (*p_callback)(void *));
extern void lvd_stop_int(lvd_channel_t ch);
extern void lvd_hw_enable_reset_int(lvd_channel_t ch, bool enable);
extern void lvd_hw_enable_reg_protect(bool enable);

struct lvd_renesas_rx_data {
	lvd_config_t lvd_config;
	void (*callback)(void *args);
	comparator_callback_t user_cb;
	void *user_cb_data;
	atomic_t flags;
};

struct lvd_renesas_rx_config {
	lvd_channel_t channel;
	uint8_t vdet_target;
	uint8_t lvd_action;
	bool lvd_support_cmpa;
};

static int lvd_renesas_rx_get_output(const struct device *dev)
{
	const struct lvd_renesas_rx_config *config = dev->config;
	lvd_status_position_t status_position;
	/* unused variable, just for API compatibility */
	lvd_status_cross_t unused_status_cross;
	lvd_err_t err;

	err = R_LVD_GetStatus(config->channel, &status_position, &unused_status_cross);
	if (err != 0) {
		LOG_ERR("Failed to get status");
		return -EIO;
	}

	switch (status_position) {
	case LVD_STATUS_POSITION_ABOVE:
		return 1;

	case LVD_STATUS_POSITION_BELOW:
		return 0;

	default:
		LOG_ERR("Invalid status, please check the configuration");
		return -EIO;
	}
}

static int lvd_renesas_rx_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	struct lvd_renesas_rx_data *data = dev->data;
	const struct lvd_renesas_rx_config *config = dev->config;

	lvd_hw_enable_reg_protect(false);
	lvd_stop_lvd(config->channel);
	lvd_stop_int(config->channel);

	switch (trigger) {
	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->lvd_config.trigger = LVD_TRIGGER_RISE;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->lvd_config.trigger = LVD_TRIGGER_FALL;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		data->lvd_config.trigger = LVD_TRIGGER_BOTH;
		break;

	case COMPARATOR_TRIGGER_NONE:
		LOG_ERR("Trigger NONE is not supported");
		return -ENOTSUP;

	default:
		LOG_ERR("Invalid trigger type.");
		return -EINVAL;
	}

	lvd_start_int(config->channel, data->callback);
	lvd_start_lvd(config->channel, data->lvd_config.trigger);
	lvd_hw_enable_reg_protect(true);

	return 0;
}

static int lvd_renesas_rx_set_trigger_callback(const struct device *dev,
					       comparator_callback_t callback, void *user_data)
{
	struct lvd_renesas_rx_data *data = dev->data;
	const struct lvd_renesas_rx_config *config = dev->config;

	if ((config->lvd_action == 0) || (config->lvd_action == 3)) {
		LOG_ERR("Callback function is not supported with the current action");
		return -ENOTSUP;
	}

	/* Disable interrupt */
	lvd_hw_enable_reset_int(config->channel, false);

	data->user_cb = callback;
	data->user_cb_data = user_data;

	/* Enable interrupt */
	lvd_hw_enable_reset_int(config->channel, true);
	return 0;
}

static int lvd_renesas_rx_trigger_is_pending(const struct device *dev)
{
	struct lvd_renesas_rx_data *data = dev->data;
	const struct lvd_renesas_rx_config *config = dev->config;

	if (data->flags & LVD_RENESAS_RX_FLAG) {
		atomic_and(&data->flags, ~LVD_RENESAS_RX_FLAG);
		R_LVD_ClearStatus(config->channel);
		return 1;
	}

	return 0;
}

static int renesas_rx_pin_set_cmpa(const struct device *dev)
{
	const struct lvd_renesas_rx_config *config = dev->config;
	const struct pinctrl_dev_config *pcfg;
	int ret;

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

	/* In the case of monitoring the CMPA pin, set the CMPA pin. */
	ret = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static inline void lvd_irq_connect(void)
{
#if DT_NODE_HAS_STATUS_OKAY(LVD0_NODE)
	IRQ_CONNECT(DT_IRQN(LVD0_NODE), DT_IRQ(LVD0_NODE, priority), lvd_ch1_isr,
		    DEVICE_DT_GET(LVD0_NODE), 0);
	irq_enable(DT_IRQN(LVD0_NODE));
#endif
#if DT_NODE_HAS_STATUS_OKAY(LVD1_NODE)
	IRQ_CONNECT(DT_IRQN(LVD1_NODE), DT_IRQ(LVD1_NODE, priority), lvd_ch2_isr,
		    DEVICE_DT_GET(LVD1_NODE), 0);
	irq_enable(DT_IRQN(LVD1_NODE));
#endif
}

static int lvd_renesas_rx_init(const struct device *dev)
{
	lvd_err_t err;

	lvd_irq_connect();

	const struct lvd_renesas_rx_config *config = dev->config;
	const struct lvd_renesas_rx_data *data = dev->data;

	/* In reset or no-action when LVD is detected, callback will not be triggered. */
	err = R_LVD_Open(config->channel, &data->lvd_config, data->callback);
	if (err != 0) {
		LOG_ERR("Failed to initialize LVD channel %d", config->channel);
		return -EIO;
	}

	/* Set the CMPA pin if the target is CMPA */
	/* NOTE: For the RX130 series, CMPA is only used on channel 2. */
	if ((config->lvd_support_cmpa) && (config->vdet_target == 1)) {
		return renesas_rx_pin_set_cmpa(dev);
	}

	return 0;
}

static DEVICE_API(comparator, lvd_renesas_rx_api) = {
	.get_output = lvd_renesas_rx_get_output,
	.set_trigger = lvd_renesas_rx_set_trigger,
	.set_trigger_callback = lvd_renesas_rx_set_trigger_callback,
	.trigger_is_pending = lvd_renesas_rx_trigger_is_pending,
};

#define LVD_RENESAS_RX_INIT(index)                                                                 \
                                                                                                   \
	static const struct lvd_renesas_rx_config lvd_renesas_rx_config_##index = {                \
		.channel = DT_INST_PROP(index, channel),                                           \
		.lvd_action = DT_INST_ENUM_IDX(index, lvd_action),                                 \
		.vdet_target = DT_INST_ENUM_IDX(index, vdet_target),                               \
		.lvd_support_cmpa = DT_INST_PROP(index, lvd_support_cmpa),                         \
	};                                                                                         \
                                                                                                   \
	void rx_lvd_callback_##index(void *args)                                                   \
	{                                                                                          \
		ARG_UNUSED(args);                                                                  \
		const struct device *dev = DEVICE_DT_GET(DT_INST(index, renesas_rx_lvd));          \
		struct lvd_renesas_rx_data *data = dev->data;                                      \
		comparator_callback_t cb = data->user_cb;                                          \
                                                                                                   \
		/* Call the user's callback function*/                                             \
		if (cb) {                                                                          \
			cb(dev, data->user_cb_data);                                               \
			return;                                                                    \
		}                                                                                  \
		atomic_or(&data->flags, LVD_RENESAS_RX_FLAG);                                      \
	};                                                                                         \
                                                                                                   \
	static struct lvd_renesas_rx_data lvd_renesas_rx_data_##index = {                          \
		.lvd_config =                                                                      \
			{                                                                          \
				.trigger = DT_INST_ENUM_IDX(index, lvd_trigger),                   \
			},                                                                         \
		.callback = rx_lvd_callback_##index,                                               \
		.flags = 0,                                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, lvd_renesas_rx_init, NULL, &lvd_renesas_rx_data_##index,      \
			      &lvd_renesas_rx_config_##index, PRE_KERNEL_1,                        \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &lvd_renesas_rx_api);

DT_INST_FOREACH_STATUS_OKAY(LVD_RENESAS_RX_INIT)
