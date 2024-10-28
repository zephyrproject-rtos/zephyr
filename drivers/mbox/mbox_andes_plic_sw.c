/*
 * Copyright (c) 2022 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
LOG_MODULE_REGISTER(mbox_andes_plic_sw);

#define DT_DRV_COMPAT andestech_mbox_plic_sw

struct mbox_plic_data {
	mbox_callback_t *cb;
	void **user_data;
	struct k_spinlock lock;
};

struct mbox_plic_conf {
	uint32_t channel_max;
	const uint32_t *irq_sources;
};

static inline bool is_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct mbox_plic_conf *conf = dev->config;

	return (ch <= conf->channel_max) && conf->irq_sources[ch];
}

static int mbox_plic_send(const struct device *dev, uint32_t ch, const struct mbox_msg *msg)
{
	const struct mbox_plic_conf *conf = dev->config;

	if (msg) {
		LOG_WRN("Transfer mode is not supported");
	}

	if (!is_channel_valid(dev, ch)) {
		return -EINVAL;
	}

	/* Send the MBOX signal by setting the Pending bit register in the PLIC. */
	riscv_plic_irq_set_pending(conf->irq_sources[ch]);

	return 0;
}

static int mbox_plic_register_callback(const struct device *dev, uint32_t ch, mbox_callback_t cb,
				       void *user_data)
{
	struct mbox_plic_data *data = dev->data;

	if (!is_channel_valid(dev, ch)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->cb[ch] = cb;
	data->user_data[ch] = user_data;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mbox_plic_mtu_get(const struct device *dev)
{
	/* MBOX PLIC only support signalling mode */
	return -ENOTSUP;
}

static uint32_t mbox_plic_max_channels_get(const struct device *dev)
{
	const struct mbox_plic_conf *conf = dev->config;

	return conf->channel_max;
}

static int mbox_plic_set_enabled(const struct device *dev, uint32_t ch, bool enable)
{
	struct mbox_plic_data *data = dev->data;
	const struct mbox_plic_conf *conf = dev->config;

	if (!is_channel_valid(dev, ch)) {
		return -EINVAL;
	}

	if (enable && !(data->cb[ch])) {
		LOG_WRN("Enabling channel without a registered callback\n");
	}

	if (enable) {
		riscv_plic_irq_enable(conf->irq_sources[ch]);
	} else {
		riscv_plic_irq_disable(conf->irq_sources[ch]);
	}

	return 0;
}

static const struct mbox_driver_api mbox_plic_driver_api = {
	.send = mbox_plic_send,
	.register_callback = mbox_plic_register_callback,
	.mtu_get = mbox_plic_mtu_get,
	.max_channels_get = mbox_plic_max_channels_get,
	.set_enabled = mbox_plic_set_enabled,
};

#define MBOX_PLIC_ISR_FUNCTION_IDX(node, prop, idx, n)                                             \
	static void mbox_plic_irq_handler##n##_##idx(const struct device *dev)                     \
	{                                                                                          \
		struct mbox_plic_data *data = dev->data;                                           \
		const uint32_t irq = DT_IRQ_BY_IDX(node, idx, irq);                                \
		if (data->cb[irq]) {                                                               \
			data->cb[irq](dev, irq, data->user_data[irq], NULL);                       \
		}                                                                                  \
	}
#define MBOX_PLIC_ISR_FUNCTION(n)                                                                  \
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, interrupt_names, MBOX_PLIC_ISR_FUNCTION_IDX, (), n)
#define MBOX_PLIC_IRQ_CONNECT_IDX(node, prop, idx, n)                                              \
	IRQ_CONNECT(DT_IRQN_BY_IDX(node, idx), 1, mbox_plic_irq_handler##n##_##idx,                \
		    DEVICE_DT_INST_GET(n), 0)
#define MBOX_PLIC_IRQ_CONNECT(n)                                                                   \
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, interrupt_names, MBOX_PLIC_IRQ_CONNECT_IDX, (;), n)
#define MBOX_PLIC_INIT_FUNCTION(n)                                                                 \
	static int mbox_plic_init##n(const struct device *dev)                                     \
	{                                                                                          \
		MBOX_PLIC_IRQ_CONNECT(n);                                                          \
		return 0;                                                                          \
	}
#define MBOX_PLIC_IRQ_SOURCE_IDX(node, prop, idx)                                                  \
	[DT_IRQ_BY_IDX(node, idx, irq)] = DT_IRQN_BY_IDX(node, idx)
#define MBOX_PLIC_IRQ_SOURCE(n)                                                                    \
	static const unsigned int irq_sources##n[] = {DT_INST_FOREACH_PROP_ELEM_SEP(               \
		n, interrupt_names, MBOX_PLIC_IRQ_SOURCE_IDX, (,))};
#define MBOX_PLIC_DEVICE_INIT(n)                                                                   \
	MBOX_PLIC_ISR_FUNCTION(n)                                                                  \
	MBOX_PLIC_INIT_FUNCTION(n)                                                                 \
	MBOX_PLIC_IRQ_SOURCE(n)                                                                    \
	static mbox_callback_t mbox_callback##n[ARRAY_SIZE(irq_sources##n)];                       \
	static void *user_data##n[ARRAY_SIZE(irq_sources##n)];                                     \
	static struct mbox_plic_data mbox_plic_data##n = {                                         \
		.cb = mbox_callback##n,                                                            \
		.user_data = user_data##n,                                                         \
	};                                                                                         \
	static const struct mbox_plic_conf mbox_plic_conf##n = {                                   \
		.channel_max = ARRAY_SIZE(irq_sources##n),                                         \
		.irq_sources = irq_sources##n,                                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &mbox_plic_init##n, NULL, &mbox_plic_data##n, &mbox_plic_conf##n, \
			      PRE_KERNEL_2, CONFIG_MBOX_INIT_PRIORITY, &mbox_plic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MBOX_PLIC_DEVICE_INIT)
