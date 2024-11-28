/*
 * Copyright (c) 2024 Felipe Neves.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT linaro_ivshmem_mbox

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_ivshmem, CONFIG_MBOX_LOG_LEVEL);

K_THREAD_STACK_DEFINE(ivshmem_ev_loop_stack, CONFIG_MBOX_IVSHMEM_EVENT_LOOP_STACK_SIZE);
static struct k_thread ivshmem_ev_loop_thread;

struct ivshmem_mbox_data {
	mbox_callback_t cb;
	void *user_data;
};

struct ivshmem_mbox_config {
	const struct device *ivshmem_dev;
	int peer_id;
};

static void ivshmem_mbox_event_loop_thread(void *arg, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	unsigned int poll_signaled;
	int ivshmem_vector_rx;
	struct k_poll_signal sig;
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig),
	};

	const struct device *dev = (const struct device *)arg;
	struct ivshmem_mbox_data *dev_data = (struct ivshmem_mbox_data *)dev->data;
	struct ivshmem_mbox_config *dev_cfg = (struct ivshmem_mbox_config *)dev->config;

	k_poll_signal_init(&sig);
	int ret = ivshmem_register_handler(dev_cfg->ivshmem_dev, &sig, 0);

	if (ret < 0) {
		LOG_ERR("registering handlers must be supported: %d\n", ret);
		k_panic();
	}

	while (1) {
		LOG_DBG("%s: waiting interrupt from client...\n", __func__);
		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);

		k_poll_signal_check(&sig, &poll_signaled, &ivshmem_vector_rx);
		/* get ready for next signal */
		k_poll_signal_reset(&sig);

		if (dev_data->cb) {
			dev_data->cb(dev, 0, dev_data->user_data, NULL);
		}
	}
}

static int ivshmem_mbox_send(const struct device *dev, mbox_channel_id_t channel,
			     const struct mbox_msg *msg)
{
	ARG_UNUSED(msg);
	ARG_UNUSED(channel);

	struct ivshmem_mbox_config *dev_cfg = (struct ivshmem_mbox_config *)dev->config;

	LOG_DBG("sending notification to the peer id 0x%x\n", (int)channel);
	return ivshmem_int_peer(dev_cfg->ivshmem_dev, (int)channel, 0);
}

static int ivshmem_mbox_register_callback(const struct device *dev, mbox_channel_id_t channel,
					  mbox_callback_t cb, void *user_data)
{
	ARG_UNUSED(channel);

	struct ivshmem_mbox_data *dev_data = (struct ivshmem_mbox_data *)dev->data;

	if (!cb) {
		LOG_ERR("Must provide a callback");
		return -EINVAL;
	}

	dev_data->cb = cb;
	dev_data->user_data = user_data;

	return 0;
}

/* some subsystems needs those functions to be at least implemented,
 * returning some valid values instead of errors, just provide them.
 */

static int ivshmem_mbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t ivshmem_mbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return UINT16_MAX;
}

static int ivshmem_mbox_set_enabled(const struct device *dev, mbox_channel_id_t channel,
				    bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(enable);

	return 0;
}

static int ivshmem_mbox_init(const struct device *dev)
{
	k_thread_create(&ivshmem_ev_loop_thread, ivshmem_ev_loop_stack,
			CONFIG_MBOX_IVSHMEM_EVENT_LOOP_STACK_SIZE, ivshmem_mbox_event_loop_thread,
			(void *)dev, NULL, NULL, CONFIG_MBOX_IVSHMEM_EVENT_LOOP_PRIO, 0, K_NO_WAIT);

	return 0;
}

static DEVICE_API(mbox, ivshmem_mbox_driver_api) = {
	.send = ivshmem_mbox_send,
	.register_callback = ivshmem_mbox_register_callback,
	.mtu_get = ivshmem_mbox_mtu_get,
	.max_channels_get = ivshmem_mbox_max_channels_get,
	.set_enabled = ivshmem_mbox_set_enabled,
};

#define MBOX_IVSHMEM_INIT(inst)                                                                    \
	static const struct ivshmem_mbox_config ivshmem_mbox_cfg_##inst = {                        \
		.ivshmem_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, ivshmem)),                      \
	};                                                                                         \
	static struct ivshmem_mbox_data ivshmem_mbox_data_##inst = {                               \
		.cb = NULL,                                                                        \
		.user_data = NULL,                                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ivshmem_mbox_init, NULL, &ivshmem_mbox_data_##inst,            \
			      &ivshmem_mbox_cfg_##inst, POST_KERNEL,                               \
			      CONFIG_APPLICATION_INIT_PRIORITY, &ivshmem_mbox_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MBOX_IVSHMEM_INIT);
