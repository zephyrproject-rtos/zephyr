/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vevif_task_tx

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>

#include <haly/nrfy_vpr.h>

#define TASKS_IDX_MAX NRF_VPR_TASKS_TRIGGER_MAX
#define VEVIF_RETRIGGER_DELAY_USEC 12

#define VPR_READY_CHECK_NEEDED DT_ANY_INST_HAS_PROP_STATUS_OKAY(nordic_vpr_ready_event)

struct mbox_vevif_task_tx_conf {
	NRF_VPR_Type *vpr;
	uint32_t tasks_mask;
	uint8_t tasks;
	uint8_t ready_event;
};

#if VPR_READY_CHECK_NEEDED
struct mbox_vevif_task_tx_data {
	bool ready;
};

static bool vpr_ready_check(const struct device *dev)
{
	const struct mbox_vevif_task_tx_conf *config = dev->config;
	struct mbox_vevif_task_tx_data *data = dev->data;

	if (data->ready) {
		return true;
	}

	if (nrfy_vpr_event_check(config->vpr, nrf_vpr_triggered_event_get(config->ready_event))) {
		data->ready = true;
		return true;
	}

	return false;
}
#endif

static inline bool vevif_task_tx_is_valid(const struct device *dev, uint32_t id)
{
	const struct mbox_vevif_task_tx_conf *config = dev->config;

	return ((id <= TASKS_IDX_MAX) && ((config->tasks_mask & BIT(id)) != 0U));
}

static int vevif_task_tx_send(const struct device *dev, uint32_t id, const struct mbox_msg *msg)
{
	const struct mbox_vevif_task_tx_conf *config = dev->config;

	if (!vevif_task_tx_is_valid(dev, id)) {
		return -EINVAL;
	}

	if (msg != NULL) {
		return -EMSGSIZE;
	}

#if VPR_READY_CHECK_NEEDED
	/* Trigger task in the remote core only if core is ready to accept it as otherwise the task
	 * will be lost. PPR core starts with unknown delay and it signals when it is ready.
	 */
	while (!vpr_ready_check(dev)) {
	}
#endif
	nrfy_vpr_task_trigger(config->vpr, nrfy_vpr_trigger_task_get(id));

#ifdef CONFIG_SOC_NRF54H20
	k_busy_wait(VEVIF_RETRIGGER_DELAY_USEC);

	nrfy_vpr_task_trigger(config->vpr, nrfy_vpr_trigger_task_get(id));
#endif /* CONFIG_SOC_NRF54H20 */

	return 0;
}

static int vevif_task_tx_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t vevif_task_tx_max_channels_get(const struct device *dev)
{
	const struct mbox_vevif_task_tx_conf *config = dev->config;

	return config->tasks;
}

static DEVICE_API(mbox, vevif_task_tx_driver_api) = {
	.send = vevif_task_tx_send,
	.mtu_get = vevif_task_tx_mtu_get,
	.max_channels_get = vevif_task_tx_max_channels_get,
};

#define VEVIF_TASK_TX_DEFINE(inst)                                                                 \
	BUILD_ASSERT(DT_INST_PROP(inst, nordic_tasks) <= VPR_TASKS_TRIGGER_MaxCount,               \
		     "Number of tasks exceeds maximum");                                           \
                                                                                                   \
	COND_CODE_0(VPR_READY_CHECK_NEEDED, (),                                                    \
		(static struct mbox_vevif_task_tx_data data##inst = {                              \
			.ready = !DT_INST_NODE_HAS_PROP(inst, nordic_vpr_ready_event),             \
		};))                                                                               \
	static const struct mbox_vevif_task_tx_conf conf##inst = {                                 \
		.vpr = (NRF_VPR_Type *)DT_INST_REG_ADDR(inst),                                     \
		.tasks = DT_INST_PROP(inst, nordic_tasks),                                         \
		.tasks_mask = DT_INST_PROP(inst, nordic_tasks_mask),                               \
		.ready_event = DT_INST_PROP_OR(inst, nordic_vpr_ready_event, 0),                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL,                                                    \
			      COND_CODE_0(VPR_READY_CHECK_NEEDED, (NULL), (&data##inst)),          \
			      &conf##inst,                                                         \
			      POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,                              \
			      &vevif_task_tx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VEVIF_TASK_TX_DEFINE)
