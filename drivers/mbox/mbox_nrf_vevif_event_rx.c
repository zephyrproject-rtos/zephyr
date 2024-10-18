/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vevif_event_rx

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>

#include <haly/nrfy_vpr.h>

#define EVENTS_IDX_MIN NRF_VPR_EVENTS_TRIGGERED_MIN
#define EVENTS_IDX_MAX NRF_VPR_EVENTS_TRIGGERED_MAX

/* callbacks */
struct mbox_vevif_event_rx_cbs {
	mbox_callback_t cb[EVENTS_IDX_MAX - EVENTS_IDX_MIN + 1U];
	void *user_data[EVENTS_IDX_MAX - EVENTS_IDX_MIN + 1U];
	uint32_t enabled_mask;
};

struct mbox_vevif_event_rx_conf {
	NRF_VPR_Type *vpr;
	uint32_t events_mask;
	uint8_t events;
	void (*irq_connect)(void);
};

static void vevif_event_rx_isr(const void *device)
{
	const struct device *dev = (struct device *)device;
	const struct mbox_vevif_event_rx_conf *config = dev->config;
	struct mbox_vevif_event_rx_cbs *cbs = dev->data;

	for (uint8_t id = EVENTS_IDX_MIN; id < EVENTS_IDX_MAX + 1U; id++) {
		nrf_vpr_event_t event = nrfy_vpr_triggered_event_get(id);

		if (nrfy_vpr_event_check(config->vpr, event)) {
			nrfy_vpr_event_clear(config->vpr, event);
			uint8_t idx = id - EVENTS_IDX_MIN;

			if ((cbs->enabled_mask & BIT(id)) && (cbs->cb[idx] != NULL)) {
				cbs->cb[idx](dev, id, cbs->user_data[idx], NULL);
			}
		}
	}
}

static inline bool vevif_event_rx_event_is_valid(uint32_t events_mask, uint32_t id)
{
	return ((id <= EVENTS_IDX_MAX) && ((events_mask & BIT(id)) != 0U));
}

static uint32_t vevif_event_rx_max_channels_get(const struct device *dev)
{
	const struct mbox_vevif_event_rx_conf *config = dev->config;

	return config->events;
}

static int vevif_event_rx_register_callback(const struct device *dev, uint32_t id,
					    mbox_callback_t cb, void *user_data)
{
	const struct mbox_vevif_event_rx_conf *config = dev->config;
	struct mbox_vevif_event_rx_cbs *cbs = dev->data;
	uint8_t idx = id - EVENTS_IDX_MIN;

	if (!vevif_event_rx_event_is_valid(config->events_mask, id)) {
		return -EINVAL;
	}

	cbs->cb[idx] = cb;
	cbs->user_data[idx] = user_data;

	return 0;
}

static int vevif_event_rx_set_enabled(const struct device *dev, uint32_t id, bool enable)
{
	const struct mbox_vevif_event_rx_conf *config = dev->config;
	struct mbox_vevif_event_rx_cbs *cbs = dev->data;

	if (!vevif_event_rx_event_is_valid(config->events_mask, id)) {
		return -EINVAL;
	}

	if (enable) {
		if ((cbs->enabled_mask & BIT(id)) != 0U) {
			return -EALREADY;
		}

		cbs->enabled_mask |= BIT(id);
		nrfy_vpr_int_enable(config->vpr, BIT(id));
	} else {
		if ((cbs->enabled_mask & BIT(id)) == 0U) {
			return -EALREADY;
		}

		cbs->enabled_mask &= ~BIT(id);
		nrfy_vpr_int_disable(config->vpr, BIT(id));
	}

	return 0;
}

static const struct mbox_driver_api vevif_event_rx_driver_api = {
	.max_channels_get = vevif_event_rx_max_channels_get,
	.register_callback = vevif_event_rx_register_callback,
	.set_enabled = vevif_event_rx_set_enabled,
};

static int vevif_event_rx_init(const struct device *dev)
{
	const struct mbox_vevif_event_rx_conf *config = dev->config;

	config->irq_connect();

	return 0;
}

#define VEVIF_EVENT_RX_DEFINE(inst)                                                                \
	BUILD_ASSERT(DT_INST_PROP(inst, nordic_events) <= NRF_VPR_EVENTS_TRIGGERED_COUNT,          \
		     "Number of events exceeds maximum");                                          \
                                                                                                   \
	static void irq_connect##inst(void)                                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_DRV_INST(inst)), DT_IRQ(DT_DRV_INST(inst), priority),       \
			    vevif_event_rx_isr, (const void *)DEVICE_DT_GET(DT_DRV_INST(inst)),    \
			    0);                                                                    \
		irq_enable(DT_IRQN(DT_DRV_INST(inst)));                                            \
	};                                                                                         \
                                                                                                   \
	static struct mbox_vevif_event_rx_cbs data##inst = {                                       \
		.enabled_mask = 0,                                                                 \
	};                                                                                         \
	static const struct mbox_vevif_event_rx_conf conf##inst = {                                \
		.vpr = (NRF_VPR_Type *)DT_INST_REG_ADDR(inst),                                     \
		.events = DT_INST_PROP(inst, nordic_events),                                       \
		.events_mask = DT_INST_PROP(inst, nordic_events_mask),                             \
		.irq_connect = irq_connect##inst,                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, vevif_event_rx_init, NULL, &data##inst, &conf##inst,           \
			      POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY, &vevif_event_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VEVIF_EVENT_RX_DEFINE)
