/*
 * Copyright (c) 2025 Dmitrii Sharshakov <d3dx12.xx@gmail.com>
 *
 * Based on mbox_nrfx_ipc.c which is:
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#define DT_DRV_COMPAT zephyr_ipm_mbox

#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(mbox_ipm, CONFIG_MBOX_LOG_LEVEL);

#define MBOX_IPM_CHANNELS DT_INST_PROP(0, channels)

struct mbox_ipm_data {
	mbox_callback_t cb[MBOX_IPM_CHANNELS];
	void *user_data[MBOX_IPM_CHANNELS];
	const struct device *dev;
	uint32_t enabled_mask;
};

static struct mbox_ipm_data ipm_mbox_data;

static struct mbox_ipm_conf {
	const struct device *ipm_dev;
} ipm_mbox_conf = {
	.ipm_dev = DEVICE_DT_GET(DT_PARENT(DT_INST(0, zephyr_ipm_mbox))),
};

static inline bool is_rx_channel_valid(const struct device *dev, uint32_t ch)
{
	return ch < MBOX_IPM_CHANNELS;
}

static inline bool is_tx_channel_valid(const struct device *dev, uint32_t ch)
{
	return ch < MBOX_IPM_CHANNELS;
}

static void mbox_dispatcher(const struct device *ipmdev, void *user_data, uint32_t id,
			    volatile void *mbox_data)
{
	struct mbox_ipm_data *data = (struct mbox_ipm_data *)user_data;
	const struct device *dev = data->dev;

	if (!is_rx_channel_valid(dev, id)) {
		LOG_WRN("RX event on illegal channel %u", id);
	}

	if (!(data->enabled_mask & BIT(id))) {
		LOG_WRN("RX event on disabled channel %u", id);
	}

	if (data->cb[id] != NULL) {
		data->cb[id](dev, id, data->user_data[id], NULL);
	}
}

static int mbox_ipm_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	if (msg) {
		LOG_WRN("Sending data not supported");
	}

	if (!is_tx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	const struct mbox_ipm_conf *conf = dev->config;

	ipm_send(conf->ipm_dev, true, channel, NULL, 0);

	return 0;
}

static int mbox_ipm_register_callback(const struct device *dev, uint32_t channel,
				      mbox_callback_t cb, void *user_data)
{
	struct mbox_ipm_data *data = dev->data;

	if (channel >= MBOX_IPM_CHANNELS) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int mbox_ipm_mtu_get(const struct device *dev)
{
	/* We only support signalling */
	return 0;
}

static uint32_t mbox_ipm_max_channels_get(const struct device *dev)
{
	return MBOX_IPM_CHANNELS;
}

static int mbox_ipm_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	struct mbox_ipm_data *data = dev->data;
	const struct mbox_ipm_conf *conf = dev->config;

	if (!is_rx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	if ((enable == 0 && (!(data->enabled_mask & BIT(channel)))) ||
	    (enable != 0 && (data->enabled_mask & BIT(channel)))) {
		return -EALREADY;
	}

	if (enable && (data->cb[channel] == NULL)) {
		LOG_WRN("Enabling channel %d without a registered callback\n", channel);
	}

	if (enable) {
		data->enabled_mask |= BIT(channel);
	} else {
		data->enabled_mask &= ~BIT(channel);
	}

	ipm_set_enabled(conf->ipm_dev, !!data->enabled_mask);

	return 0;
}

static int mbox_ipm_init(const struct device *dev)
{
	struct mbox_ipm_data *data = dev->data;
	const struct mbox_ipm_conf *conf = dev->config;

	data->dev = dev;

	ipm_register_callback(conf->ipm_dev, mbox_dispatcher, (void *)data);

	return 0;
}

static DEVICE_API(mbox, mbox_ipm_driver_api) = {
	.send = mbox_ipm_send,
	.register_callback = mbox_ipm_register_callback,
	.mtu_get = mbox_ipm_mtu_get,
	.max_channels_get = mbox_ipm_max_channels_get,
	.set_enabled = mbox_ipm_set_enabled,
};

DEVICE_DT_INST_DEFINE(0, mbox_ipm_init, NULL, &ipm_mbox_data, &ipm_mbox_conf, POST_KERNEL,
		      CONFIG_MBOX_INIT_PRIORITY, &mbox_ipm_driver_api);
