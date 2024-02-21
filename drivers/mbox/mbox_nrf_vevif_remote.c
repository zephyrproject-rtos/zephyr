/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vevif_remote

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>

#include <haly/nrfy_vpr.h>

struct mbox_vevif_remote_conf {
	NRF_VPR_Type *vpr;
	uint32_t tasks_mask;
	uint8_t tasks;
};

static inline bool vevif_remote_is_task_valid(const struct device *dev, uint32_t id)
{
	const struct mbox_vevif_remote_conf *config = dev->config;

	return (id < config->tasks) && ((config->tasks_mask & BIT(id)) != 0U);
}

static int vevif_remote_send(const struct device *dev, uint32_t id, const struct mbox_msg *msg)
{
	const struct mbox_vevif_remote_conf *config = dev->config;

	if (!vevif_remote_is_task_valid(dev, id)) {
		return -EINVAL;
	}

	if (msg != NULL) {
		return -ENOTSUP;
	}

	nrfy_vpr_task_trigger(config->vpr, nrfy_vpr_trigger_task_get(id));

	return 0;
}

static int vevif_remote_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t vevif_remote_max_channels_get(const struct device *dev)
{
	const struct mbox_vevif_remote_conf *config = dev->config;

	return config->tasks;
}

static const struct mbox_driver_api vevif_remote_driver_api = {
	.send = vevif_remote_send,
	.mtu_get = vevif_remote_mtu_get,
	.max_channels_get = vevif_remote_max_channels_get,
};

#define VEVIF_REMOTE_DEFINE(inst)                                                                  \
	BUILD_ASSERT(DT_INST_PROP(inst, nordic_tasks) <= VPR_TASKS_TRIGGER_MaxCount,               \
		     "Number of tasks exceeds maximum");                                           \
                                                                                                   \
	static const struct mbox_vevif_remote_conf conf##inst = {                                  \
		.vpr = (NRF_VPR_Type *)DT_INST_REG_ADDR(inst),                                     \
		.tasks = DT_INST_PROP(inst, nordic_tasks),                                         \
		.tasks_mask = DT_INST_PROP(inst, nordic_tasks_mask),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &conf##inst, POST_KERNEL,                    \
			      CONFIG_MBOX_INIT_PRIORITY, &vevif_remote_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VEVIF_REMOTE_DEFINE)
