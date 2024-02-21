/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vevif_local

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>

#include <hal/nrf_vpr.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vevif.h>

#define VEVIF_TASKS_NUM  DT_INST_PROP(0, nordic_tasks)
#define VEVIF_TASKS_MASK DT_INST_PROP(0, nordic_tasks_mask)

BUILD_ASSERT(VEVIF_TASKS_NUM <= VPR_TASKS_TRIGGER_MaxCount, "Number of tasks exceeds maximum");
BUILD_ASSERT(VEVIF_TASKS_NUM == DT_NUM_IRQS(DT_DRV_INST(0)), "# IRQs != # tasks");

/* callbacks */
struct mbox_vevif_local_cbs {
	mbox_callback_t cb[VEVIF_TASKS_NUM];
	void *user_data[VEVIF_TASKS_NUM];
	uint32_t enabled_mask;
};

static struct mbox_vevif_local_cbs cbs;

/* IRQ list */
#define VEVIF_IRQN(idx, _) DT_INST_IRQ_BY_IDX(0, idx, irq)

static const uint8_t vevif_irqs[VEVIF_TASKS_NUM] = {
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(0)), VEVIF_IRQN, (,))
};

static void vevif_local_isr(const void *parameter)
{
	uint8_t id = *(uint8_t *)parameter;

	nrf_vpr_csr_vevif_tasks_clear(BIT(id));

	if (cbs.cb[id] != NULL) {
		cbs.cb[id](DEVICE_DT_INST_GET(0), id, cbs.user_data[id], NULL);
	}
}

static inline bool vevif_local_is_task_valid(uint32_t id)
{
	return (id < VEVIF_TASKS_NUM) && ((VEVIF_TASKS_MASK & BIT(id)) != 0U);
}

static uint32_t vevif_local_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return VEVIF_TASKS_NUM;
}

static int vevif_local_register_callback(const struct device *dev, uint32_t id, mbox_callback_t cb,
					 void *user_data)
{
	ARG_UNUSED(dev);

	if (!vevif_local_is_task_valid(id)) {
		return -EINVAL;
	}

	cbs.cb[id] = cb;
	cbs.user_data[id] = user_data;

	return 0;
}

static int vevif_local_set_enabled(const struct device *dev, uint32_t id, bool enable)
{
	ARG_UNUSED(dev);

	if (!vevif_local_is_task_valid(id)) {
		return -EINVAL;
	}

	if (enable) {
		if ((cbs.enabled_mask & BIT(id)) != 0U) {
			return -EALREADY;
		}

		cbs.enabled_mask |= BIT(id);
		irq_enable(vevif_irqs[id]);
	} else {
		if ((cbs.enabled_mask & BIT(id)) == 0U) {
			return -EALREADY;
		}

		cbs.enabled_mask &= ~BIT(id);
		irq_disable(vevif_irqs[id]);
	}

	return 0;
}

static const struct mbox_driver_api vevif_local_driver_api = {
	.max_channels_get = vevif_local_max_channels_get,
	.register_callback = vevif_local_register_callback,
	.set_enabled = vevif_local_set_enabled,
};

#define VEVIF_IRQ_CONNECT(idx, _)                                                                  \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, idx, irq), DT_INST_IRQ_BY_IDX(0, idx, priority),         \
		    vevif_local_isr, &vevif_irqs[idx], 0)

static int vevif_local_init(const struct device *dev)
{
	nrf_vpr_csr_rtperiph_enable_set(true);
	nrf_vpr_csr_vevif_tasks_clear(NRF_VPR_TASK_TRIGGER_ALL_MASK);

	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(0)), VEVIF_IRQ_CONNECT, (;));

	return 0;
}

DEVICE_DT_INST_DEFINE(0, vevif_local_init, NULL, NULL, NULL, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,
		      &vevif_local_driver_api);
