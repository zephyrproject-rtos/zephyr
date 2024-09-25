/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vevif_task_rx

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>

#include <hal/nrf_vpr.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vevif.h>

#if defined(CONFIG_SOC_NRF54L15_ENGA_CPUFLPR)
#define TASKS_IDX_MIN 11U
#define TASKS_IDX_MAX 17U
#else
#define TASKS_IDX_MIN NRF_VPR_TASKS_TRIGGER_MIN
#define TASKS_IDX_MAX NRF_VPR_TASKS_TRIGGER_MAX
#endif

#define VEVIF_TASKS_NUM  DT_INST_PROP(0, nordic_tasks)
#define VEVIF_TASKS_MASK DT_INST_PROP(0, nordic_tasks_mask)

BUILD_ASSERT(VEVIF_TASKS_NUM <= VPR_TASKS_TRIGGER_MaxCount, "Number of tasks exceeds maximum");
BUILD_ASSERT(VEVIF_TASKS_NUM == DT_NUM_IRQS(DT_DRV_INST(0)), "# IRQs != # tasks");

/* callbacks */
struct mbox_vevif_task_rx_cbs {
	mbox_callback_t cb[TASKS_IDX_MAX - TASKS_IDX_MIN + 1U];
	void *user_data[TASKS_IDX_MAX - TASKS_IDX_MIN + 1U];
	uint32_t enabled_mask;
};

static struct mbox_vevif_task_rx_cbs cbs;

/* IRQ list */
#define VEVIF_IRQN(idx, _) DT_INST_IRQ_BY_IDX(0, idx, irq)

static const uint8_t vevif_irqs[VEVIF_TASKS_NUM] = {
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(0)), VEVIF_IRQN, (,))
};

static void vevif_task_rx_isr(const void *parameter)
{
	uint8_t channel = *(uint8_t *)parameter;
	uint8_t idx = channel - TASKS_IDX_MIN;

	nrf_vpr_csr_vevif_tasks_clear(BIT(channel));

	if (cbs.cb[idx] != NULL) {
		cbs.cb[idx](DEVICE_DT_INST_GET(0), channel, cbs.user_data[idx], NULL);
	}
}

static inline bool vevif_task_rx_is_task_valid(uint32_t id)
{
	return ((id <= TASKS_IDX_MAX) && ((VEVIF_TASKS_MASK & BIT(id)) != 0U));
}

static uint32_t vevif_task_rx_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return VEVIF_TASKS_NUM;
}

static int vevif_task_rx_register_callback(const struct device *dev, uint32_t id,
					   mbox_callback_t cb, void *user_data)
{
	ARG_UNUSED(dev);
	uint8_t idx = id - TASKS_IDX_MIN;

	if (!vevif_task_rx_is_task_valid(id)) {
		return -EINVAL;
	}

	cbs.cb[idx] = cb;
	cbs.user_data[idx] = user_data;

	return 0;
}

static int vevif_task_rx_set_enabled(const struct device *dev, uint32_t id, bool enable)
{
	ARG_UNUSED(dev);
	uint8_t idx = id - TASKS_IDX_MIN;

	if (!vevif_task_rx_is_task_valid(id)) {
		return -EINVAL;
	}

	if (enable) {
		if ((cbs.enabled_mask & BIT(id)) != 0U) {
			return -EALREADY;
		}

		cbs.enabled_mask |= BIT(id);
		irq_enable(vevif_irqs[idx]);
	} else {
		if ((cbs.enabled_mask & BIT(id)) == 0U) {
			return -EALREADY;
		}

		cbs.enabled_mask &= ~BIT(id);
		irq_disable(vevif_irqs[idx]);
	}

	return 0;
}

static const struct mbox_driver_api vevif_task_rx_driver_api = {
	.max_channels_get = vevif_task_rx_max_channels_get,
	.register_callback = vevif_task_rx_register_callback,
	.set_enabled = vevif_task_rx_set_enabled,
};

#define VEVIF_IRQ_CONNECT(idx, _)                                                                  \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, idx, irq), DT_INST_IRQ_BY_IDX(0, idx, priority),         \
		    vevif_task_rx_isr, &vevif_irqs[idx], 0)

static int vevif_task_rx_init(const struct device *dev)
{
	nrf_vpr_csr_vevif_tasks_clear(NRF_VPR_TASK_TRIGGER_ALL_MASK);

	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(0)), VEVIF_IRQ_CONNECT, (;));

	return 0;
}

DEVICE_DT_INST_DEFINE(0, vevif_task_rx_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_MBOX_INIT_PRIORITY, &vevif_task_rx_driver_api);
