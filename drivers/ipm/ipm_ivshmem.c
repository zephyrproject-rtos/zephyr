/*
 * Copyright (c) 2023 Linaro.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT linaro_ivshmem_ipm

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipm_ivshmem, CONFIG_IPM_LOG_LEVEL);

K_THREAD_STACK_DEFINE(ivshmem_ev_loop_stack, CONFIG_IPM_IVSHMEM_EVENT_LOOP_STACK_SIZE);
static struct k_thread ivshmem_ev_loop_thread;

struct ivshmem_ipm_data {
	ipm_callback_t cb;
	void *user_data;
};

struct ivshmem_ipm_config {
	const struct device *ivshmem_dev;
};

static void ivshmem_ipm_event_loop_thread(void *arg, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	unsigned int poll_signaled;
	int ivshmem_vector_rx;
	struct k_poll_signal sig;
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &sig),
	};

	const struct device *dev = (const struct device *)arg;
	struct ivshmem_ipm_data *dev_data = (struct ivshmem_ipm_data *)dev->data;
	struct ivshmem_ipm_config *dev_cfg = (struct ivshmem_ipm_config *)dev->config;

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
			dev_data->cb(dev, dev_data->user_data, 0, NULL);
		}
	}
}

static int ivshmem_ipm_send(const struct device *dev, int wait, uint32_t id,
			 const void *data, int size)
{
	ARG_UNUSED(wait);
	ARG_UNUSED(data);
	ARG_UNUSED(size);

	struct ivshmem_ipm_config *dev_cfg = (struct ivshmem_ipm_config *)dev->config;

	LOG_DBG("sending notification to the peer id 0x%x\n", id);
	return ivshmem_int_peer(dev_cfg->ivshmem_dev, id, 0);
}

static void ivshmem_ipm_register_callback(const struct device *dev,
					ipm_callback_t cb,
					void *user_data)
{
	struct ivshmem_ipm_data *dev_data = (struct ivshmem_ipm_data *)dev->data;

	dev_data->cb = cb;
	dev_data->user_data = user_data;
}

static int ivshmem_ipm_set_enabled(const struct device *dev, int enable)
{
	/* some subsystems needs this minimal function just return success here*/
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);

	return 0;
}

static int ivshmem_ipm_init(const struct device *dev)
{
	k_thread_create(&ivshmem_ev_loop_thread,
		ivshmem_ev_loop_stack,
		CONFIG_IPM_IVSHMEM_EVENT_LOOP_STACK_SIZE,
		ivshmem_ipm_event_loop_thread,
		(void *)dev, NULL, NULL,
		CONFIG_IPM_IVSHMEM_EVENT_LOOP_PRIO,
		0, K_NO_WAIT);

	return 0;
}

static DEVICE_API(ipm, ivshmem_ipm_driver_api) = {
	.send = ivshmem_ipm_send,
	.register_callback = ivshmem_ipm_register_callback,
	.set_enabled = ivshmem_ipm_set_enabled
};

#define IPM_IVSHMEM_INIT(inst)								    \
	static const struct ivshmem_ipm_config ivshmem_ipm_cfg_##inst = {	\
		.ivshmem_dev =	\
			DEVICE_DT_GET(DT_INST_PHANDLE(inst, ivshmem))\
	};	\
	static struct ivshmem_ipm_data ivshmem_ipm_data_##inst = {	\
		.cb = NULL,	\
		.user_data = NULL,	\
	};	\
	DEVICE_DT_INST_DEFINE(inst,	\
				ivshmem_ipm_init,	\
				NULL,	\
				&ivshmem_ipm_data_##inst, &ivshmem_ipm_cfg_##inst,  \
				POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,      \
				&ivshmem_ipm_driver_api);                           \

DT_INST_FOREACH_STATUS_OKAY(IPM_IVSHMEM_INIT);
