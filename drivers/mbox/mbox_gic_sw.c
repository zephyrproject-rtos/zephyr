/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_mbox_gic_sw

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#define GIC_SPI_INTID_BASE 32U

struct mbox_gic_sw_channel {
	const struct device *dev;
	mbox_channel_id_t id;
};

struct mbox_gic_sw_config {
	const unsigned int *rx_irqs;
	const uint32_t *tx_spis;
	uint32_t num_channels;
	void (*irq_config)(void);
};

struct mbox_gic_sw_data {
	mbox_callback_t *cb;
	void **user_data;
	struct k_spinlock lock;
};

static int mbox_gic_sw_send(const struct device *dev, mbox_channel_id_t channel,
			    const struct mbox_msg *msg)
{
	const struct mbox_gic_sw_config *config = dev->config;

	if (channel >= config->num_channels) {
		return -EINVAL;
	}

	if ((msg != NULL) && (msg->size != 0U)) {
		return -EMSGSIZE;
	}

	arm_gic_irq_set_pending(config->tx_spis[channel] + GIC_SPI_INTID_BASE);

	return 0;
}

static int mbox_gic_sw_register_callback(const struct device *dev, mbox_channel_id_t channel,
					 mbox_callback_t cb, void *user_data)
{
	const struct mbox_gic_sw_config *config = dev->config;
	struct mbox_gic_sw_data *data = dev->data;
	k_spinlock_key_t key;

	if (channel >= config->num_channels) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	data->cb[channel] = cb;
	data->user_data[channel] = user_data;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mbox_gic_sw_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t mbox_gic_sw_max_channels_get(const struct device *dev)
{
	const struct mbox_gic_sw_config *config = dev->config;

	return config->num_channels;
}

static int mbox_gic_sw_set_enabled(const struct device *dev, mbox_channel_id_t channel,
				   bool enabled)
{
	const struct mbox_gic_sw_config *config = dev->config;

	if (channel >= config->num_channels) {
		return -EINVAL;
	}

	if (enabled) {
		irq_enable(config->rx_irqs[channel]);
	} else {
		irq_disable(config->rx_irqs[channel]);
	}

	return 0;
}

static void mbox_gic_sw_isr(const void *arg)
{
	const struct mbox_gic_sw_channel *channel = arg;
	struct mbox_gic_sw_data *data = channel->dev->data;

	if (data->cb[channel->id] != NULL) {
		data->cb[channel->id](channel->dev, channel->id, data->user_data[channel->id],
				      NULL);
	}
}

static int mbox_gic_sw_init(const struct device *dev)
{
	const struct mbox_gic_sw_config *config = dev->config;

	config->irq_config();

	return 0;
}

static DEVICE_API(mbox, mbox_gic_sw_api) = {
	.send = mbox_gic_sw_send,
	.register_callback = mbox_gic_sw_register_callback,
	.mtu_get = mbox_gic_sw_mtu_get,
	.max_channels_get = mbox_gic_sw_max_channels_get,
	.set_enabled = mbox_gic_sw_set_enabled,
};

#define MBOX_GIC_SW_NUM_CHANNELS(n) DT_NUM_IRQS(DT_DRV_INST(n))

#define MBOX_GIC_SW_CHANNEL(i, n) {.dev = DEVICE_DT_INST_GET(n), .id = i}

#define MBOX_GIC_SW_RX_IRQ(i, n) DT_INST_IRQN_BY_IDX(n, i)

#define MBOX_GIC_SW_IRQ_CONNECT(i, n)                                                              \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, i), DT_INST_IRQ_BY_IDX(n, i, priority),                 \
		    mbox_gic_sw_isr, &mbox_gic_sw_channels_##n[i], DT_INST_IRQ_BY_IDX(n, i, flags))

#define MBOX_GIC_SW_INIT(n)                                                                        \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, tx_spis) == MBOX_GIC_SW_NUM_CHANNELS(n),                  \
		     "tx-spis and interrupts must have one entry per channel");                    \
                                                                                                   \
	static const struct mbox_gic_sw_channel mbox_gic_sw_channels_##n[] = {                     \
		LISTIFY(MBOX_GIC_SW_NUM_CHANNELS(n), MBOX_GIC_SW_CHANNEL, (,), n)};                \
	static const unsigned int mbox_gic_sw_rx_irqs_##n[] = {                                    \
		LISTIFY(MBOX_GIC_SW_NUM_CHANNELS(n), MBOX_GIC_SW_RX_IRQ, (,), n)};                 \
	static const uint32_t mbox_gic_sw_tx_spis_##n[] = DT_INST_PROP(n, tx_spis);                \
                                                                                                   \
	static void mbox_gic_sw_irq_config_##n(void)                                               \
	{                                                                                          \
		LISTIFY(MBOX_GIC_SW_NUM_CHANNELS(n), MBOX_GIC_SW_IRQ_CONNECT, (;), n);             \
	}                                                                                          \
                                                                                                   \
	static mbox_callback_t mbox_gic_sw_cb_##n[MBOX_GIC_SW_NUM_CHANNELS(n)];                    \
	static void *mbox_gic_sw_user_data_##n[MBOX_GIC_SW_NUM_CHANNELS(n)];                       \
	static struct mbox_gic_sw_data mbox_gic_sw_data_##n = {                                    \
		.cb = mbox_gic_sw_cb_##n,                                                          \
		.user_data = mbox_gic_sw_user_data_##n,                                            \
	};                                                                                         \
                                                                                                   \
	static const struct mbox_gic_sw_config mbox_gic_sw_config_##n = {                          \
		.rx_irqs = mbox_gic_sw_rx_irqs_##n,                                                \
		.tx_spis = mbox_gic_sw_tx_spis_##n,                                                \
		.num_channels = MBOX_GIC_SW_NUM_CHANNELS(n),                                       \
		.irq_config = mbox_gic_sw_irq_config_##n,                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mbox_gic_sw_init, NULL, &mbox_gic_sw_data_##n,                    \
			      &mbox_gic_sw_config_##n, PRE_KERNEL_2, CONFIG_MBOX_INIT_PRIORITY,    \
			      &mbox_gic_sw_api);

DT_INST_FOREACH_STATUS_OKAY(MBOX_GIC_SW_INIT)
