/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/atomic.h>
#include <kernel.h>
#include <nrfx_egu.h>
#include <drivers/interrupt_controller/intc_nrf_egu.h>

struct egu_channel_data {
	bool taken;
	egu_channel_cb_t callback;
	void *ctx;
};

struct egu_data {
	const struct device *dev;
	struct egu_channel_data *channels;
	struct k_spinlock lock;
};

struct egu_config {
	nrfx_egu_t egu;
	uint32_t ch_num;
};

static struct egu_channel_data *get_channel_data(const struct device *dev, int channel)
{
	struct egu_data *data = dev->data;
	const struct egu_config *config = dev->config;

	__ASSERT(config->ch_num > channel && channel >= 0, "EGU channel not within valid range");

	return &data->channels[channel];
}

static int channel_callback_set(const struct device *dev,
				int channel,
				egu_channel_cb_t cb,
				void *ctx)
{
	struct egu_channel_data *channel_data = get_channel_data(dev, channel);
	const struct egu_config *config = dev->config;

	if (channel_data->callback != NULL) {
		return -EALREADY;
	}

	channel_data->callback = cb;
	channel_data->ctx = ctx;
	nrfx_egu_int_enable(&config->egu, 1U << channel);

	return 0;
}

static int channel_callback_clear(const struct device *dev, int channel)
{
	struct egu_channel_data *channel_data = get_channel_data(dev, channel);
	const struct egu_config *config = dev->config;

	nrfx_egu_int_disable(&config->egu, 1U << channel);
	channel_data->callback = NULL;
	channel_data->ctx = NULL;

	return 0;
}

static int channel_task_trigger(const struct device *dev, int channel)
{
	const struct egu_config *config = dev->config;

	nrfx_egu_trigger(&config->egu, channel);

	return 0;
}

static int channel_alloc(const struct device *dev)
{
	struct egu_data *data = dev->data;
	struct egu_channel_data *channels = data->channels;
	const struct egu_config *config = dev->config;
	k_spinlock_key_t key;
	int result = -ENODEV;

	key = k_spin_lock(&data->lock);
	for (int i = 0; i < config->ch_num; i++) {
		if (!channels[i].taken) {
			channels[i].taken = true;
			result = i;
			break;
		}
	}
	k_spin_unlock(&data->lock, key);

	return result;
}

static int channel_free(const struct device *dev, int channel)
{
	struct egu_data *data = dev->data;
	struct egu_channel_data *channel_data = get_channel_data(dev, channel);
	k_spinlock_key_t key;

	egu_channel_callback_clear(dev, channel);

	key = k_spin_lock(&data->lock);
	channel_data->taken = false;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static void egu_event_handler(uint8_t event_idx, void *context)
{
	struct egu_data *data = context;
	const struct device *dev = data->dev;
	struct egu_channel_data *channel_data = get_channel_data(dev, event_idx);

	if (channel_data->callback) {
		channel_data->callback(dev, event_idx, channel_data->ctx);
	}
}

static struct egu_driver_api nrfx_egu_driver_api = {
	.channel_callback_set = channel_callback_set,
	.channel_callback_clear = channel_callback_clear,
	.channel_task_trigger = channel_task_trigger,
	.channel_alloc = channel_alloc,
	.channel_free = channel_free,
};

static int init_egu(const struct device *dev)
{
	struct egu_data *data = dev->data;
	const struct egu_config *config = dev->config;
	nrfx_err_t err;

	data->dev = dev;
	err = nrfx_egu_init(&config->egu, 0, egu_event_handler, data);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);

	return 0;
}

#define EGU(idx) DT_NODELABEL(egu##idx)

#define INTC_NRF_EGU_DEVICE(idx)                                                                \
	static struct egu_channel_data egu_##idx##_ch_data[EGU##idx##_CH_NUM];                  \
	static struct egu_data egu_##idx##_data = {                                             \
		.channels = egu_##idx##_ch_data,                                                \
	};                                                                                      \
	static struct egu_config egu_##idx##z_config = {                                        \
		.egu = NRFX_EGU_INSTANCE(idx),                                                  \
		.ch_num = EGU##idx##_CH_NUM,                                                    \
	};                                                                                      \
	static int egu_##idx##_init(const struct device *dev)                                   \
	{                                                                                       \
		IRQ_CONNECT(DT_IRQN(EGU(idx)), DT_IRQ(EGU(idx), priority),                      \
			    nrfx_isr, nrfx_egu_##idx##_irq_handler, 0);                         \
		return init_egu(dev);                                                           \
	}                                                                                       \
	DEVICE_DT_DEFINE(EGU(idx),                                                              \
			 egu_##idx##_init,                                                      \
			 NULL,                                                                  \
			 &egu_##idx##_data,                                                     \
			 &egu_##idx##z_config,                                                  \
			 PRE_KERNEL_1,                                                          \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                    \
			 &nrfx_egu_driver_api);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(egu0), okay)
INTC_NRF_EGU_DEVICE(0)
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(egu1), okay)
INTC_NRF_EGU_DEVICE(1)
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(egu2), okay)
INTC_NRF_EGU_DEVICE(2)
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(egu3), okay)
INTC_NRF_EGU_DEVICE(3)
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(egu4), okay)
INTC_NRF_EGU_DEVICE(4)
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(egu5), okay)
INTC_NRF_EGU_DEVICE(5)
#endif
