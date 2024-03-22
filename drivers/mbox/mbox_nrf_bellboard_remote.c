/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_bellboard_remote

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>

#include <haly/nrfy_bellboard.h>

struct mbox_bellboard_remote_conf {
	NRF_BELLBOARD_Type *bellboard;
};

static int bellboard_remote_send(const struct device *dev, uint32_t id, const struct mbox_msg *msg)
{
	const struct mbox_bellboard_remote_conf *config = dev->config;

	if (id >= BELLBOARD_TASKS_TRIGGER_MaxCount) {
		return -EINVAL;
	}

	if (msg != NULL) {
		return -ENOTSUP;
	}

	nrfy_bellboard_task_trigger(config->bellboard, nrf_bellboard_trigger_task_get(id));

	return 0;
}

static int bellboard_remote_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t bellboard_remote_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return BELLBOARD_TASKS_TRIGGER_MaxCount;
}

static const struct mbox_driver_api bellboard_remote_driver_api = {
	.send = bellboard_remote_send,
	.mtu_get = bellboard_remote_mtu_get,
	.max_channels_get = bellboard_remote_max_channels_get,
};

#define BELLBOARD_REMOTE_DEFINE(inst)                                                              \
	static const struct mbox_bellboard_remote_conf conf##inst = {                              \
		.bellboard = (NRF_BELLBOARD_Type *)DT_INST_REG_ADDR(inst),                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &conf##inst, POST_KERNEL,                    \
			      CONFIG_MBOX_INIT_PRIORITY, &bellboard_remote_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BELLBOARD_REMOTE_DEFINE)
