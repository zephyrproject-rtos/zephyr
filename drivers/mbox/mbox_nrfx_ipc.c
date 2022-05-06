/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>
#include <nrfx_ipc.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_nrfx_ipc);

#define DT_DRV_COMPAT nordic_mbox_nrf_ipc

struct mbox_nrf_data {
	mbox_callback_t cb[IPC_CONF_NUM];
	void *user_data[IPC_CONF_NUM];
	const struct device *dev;
	uint32_t enabled_mask;
};

static struct mbox_nrf_data nrfx_mbox_data;

static struct mbox_nrf_conf {
	uint32_t rx_mask;
	uint32_t tx_mask;
} nrfx_mbox_conf = {
	.rx_mask = DT_INST_PROP(0, rx_mask),
	.tx_mask = DT_INST_PROP(0, tx_mask),
};

static inline bool is_rx_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct mbox_nrf_conf *conf = dev->config;

	return ((ch < IPC_CONF_NUM) && (conf->rx_mask & BIT(ch)));
}

static inline bool is_tx_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct mbox_nrf_conf *conf = dev->config;

	return ((ch < IPC_CONF_NUM) && (conf->tx_mask & BIT(ch)));
}

static void mbox_dispatcher(uint32_t event_mask, void *p_context)
{
	struct mbox_nrf_data *data = (struct mbox_nrf_data *) p_context;
	const struct device *dev = data->dev;

	while (event_mask) {
		uint32_t channel = __CLZ(__RBIT(event_mask));

		if (!is_rx_channel_valid(dev, channel)) {
			LOG_WRN("RX event on illegal channel");
		}

		if (!(data->enabled_mask & BIT(channel))) {
			LOG_WRN("RX event on disabled channel");
		}

		event_mask &= ~BIT(channel);

		if (data->cb[channel] != NULL) {
			data->cb[channel](dev, channel, data->user_data[channel], NULL);
		}
	}
}

static int mbox_nrf_send(const struct device *dev, uint32_t channel,
			 const struct mbox_msg *msg)
{
	if (msg) {
		LOG_WRN("Sending data not supported");
	}

	if (!is_tx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	nrfx_ipc_signal(channel);

	return 0;
}

static int mbox_nrf_register_callback(const struct device *dev, uint32_t channel,
				      mbox_callback_t cb, void *user_data)
{
	struct mbox_nrf_data *data = dev->data;

	if (channel >= IPC_CONF_NUM) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int mbox_nrf_mtu_get(const struct device *dev)
{
	/* We only support signalling */
	return 0;
}

static uint32_t mbox_nrf_max_channels_get(const struct device *dev)
{
	return IPC_CONF_NUM;
}

static int mbox_nrf_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	struct mbox_nrf_data *data = dev->data;

	if (!is_rx_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	if ((enable == 0 && (!(data->enabled_mask & BIT(channel)))) ||
	    (enable != 0 &&   (data->enabled_mask & BIT(channel)))) {
		return -EALREADY;
	}

	if (enable && (data->cb[channel] == NULL)) {
		LOG_WRN("Enabling channel without a registered callback\n");
	}

	if (enable && data->enabled_mask == 0) {
		irq_enable(DT_INST_IRQN(0));
	}

	if (enable) {
		data->enabled_mask |= BIT(channel);
		compiler_barrier();
		nrfx_ipc_receive_event_enable(channel);
	} else {
		nrfx_ipc_receive_event_disable(channel);
		compiler_barrier();
		data->enabled_mask &= ~BIT(channel);
	}

	if (data->enabled_mask == 0) {
		irq_disable(DT_INST_IRQN(0));
	}

	return 0;
}

static void enable_dt_channels(const struct device *dev)
{
	const struct mbox_nrf_conf *conf = dev->config;
	nrfx_ipc_config_t ch_config = { 0 };

	if (conf->tx_mask >= BIT(IPC_CONF_NUM)) {
		LOG_WRN("tx_mask too big (or IPC_CONF_NUM too small)");
	}

	if (conf->rx_mask >= BIT(IPC_CONF_NUM)) {
		LOG_WRN("rx_mask too big (or IPC_CONF_NUM too small)");
	}

	/* Enable the interrupts on .set_enabled() only */
	ch_config.receive_events_enabled = 0;

	for (size_t ch = 0; ch < IPC_CONF_NUM; ch++) {
		if (conf->tx_mask & BIT(ch)) {
			ch_config.send_task_config[ch] = BIT(ch);
		}

		if (conf->rx_mask & BIT(ch)) {
			ch_config.receive_event_config[ch] = BIT(ch);
		}
	}

	nrfx_ipc_config_load(&ch_config);
}

static int mbox_nrf_init(const struct device *dev)
{
	struct mbox_nrf_data *data = dev->data;

	data->dev = dev;

	nrfx_ipc_init(0, mbox_dispatcher, (void *) data);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_ipc_irq_handler, 0);

	enable_dt_channels(dev);

	return 0;
}

static const struct mbox_driver_api mbox_nrf_driver_api = {
	.send = mbox_nrf_send,
	.register_callback = mbox_nrf_register_callback,
	.mtu_get = mbox_nrf_mtu_get,
	.max_channels_get = mbox_nrf_max_channels_get,
	.set_enabled = mbox_nrf_set_enabled,
};

DEVICE_DT_INST_DEFINE(0, mbox_nrf_init, NULL, &nrfx_mbox_data, &nrfx_mbox_conf,
		    POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,
		    &mbox_nrf_driver_api);
