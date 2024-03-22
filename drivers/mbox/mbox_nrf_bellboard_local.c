/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_bellboard_local

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/util.h>

#include <hal/nrf_bellboard.h>

#define BELLBOARD_NUM_IRQS 4U

BUILD_ASSERT(DT_NUM_IRQS(DT_DRV_INST(0)) <= BELLBOARD_NUM_IRQS, "# interrupt exceeds maximum");

BUILD_ASSERT((DT_INST_PROP_LEN(0, nordic_interrupt_mapping) % 2) == 0,
	     "# interrupt mappings not specified in pairs");

/* BELLBOARD event mappings */
#define EVT_MAPPING_ITEM(idx) DT_INST_PROP_BY_IDX(0, nordic_interrupt_mapping, idx)
#define BELLBOARD_GET_EVT_MAPPING(idx, _)                                                          \
	COND_CODE_1(                                                                               \
		DT_INST_PROP_HAS_IDX(0, nordic_interrupt_mapping, UTIL_INC(UTIL_X2(idx))),         \
		([EVT_MAPPING_ITEM(UTIL_INC(UTIL_X2(idx)))] = EVT_MAPPING_ITEM(UTIL_X2(idx)),),    \
		())

static const uint32_t evt_mappings[BELLBOARD_NUM_IRQS] = {
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(0)), BELLBOARD_GET_EVT_MAPPING, ())};

/* BELLBOARD instance */
static NRF_BELLBOARD_Type *bellboard = (NRF_BELLBOARD_Type *)DT_INST_REG_ADDR(0);

/* BELLBOARD runtime resources */
static mbox_callback_t cbs[NRF_BELLBOARD_EVENTS_TRIGGERED_COUNT];
static void *cbs_ctx[NRF_BELLBOARD_EVENTS_TRIGGERED_COUNT];
static uint32_t evt_enabled_masks[BELLBOARD_NUM_IRQS];

static void bellboard_local_isr(const void *parameter)
{
	uint8_t irq_idx = (uint8_t)(uintptr_t)parameter;
	uint32_t int_pend;

	int_pend = nrf_bellboard_int_pending_get(bellboard, irq_idx);

	for (uint8_t i = 0U; i < NRF_BELLBOARD_EVENTS_TRIGGERED_COUNT; i++) {
		nrf_bellboard_event_t event = nrf_bellboard_triggered_event_get(i);

		if (nrf_bellboard_event_check(bellboard, event)) {
			nrf_bellboard_event_clear(bellboard, event);
		}

		if ((int_pend & BIT(i)) != 0U) {
			if (cbs[i] != NULL) {
				cbs[i](DEVICE_DT_INST_GET(0), i, cbs_ctx[i], NULL);
			}
		}
	}
}

static uint32_t bellboard_local_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NRF_BELLBOARD_EVENTS_TRIGGERED_COUNT;
}

static int bellboard_local_register_callback(const struct device *dev, uint32_t id,
					     mbox_callback_t cb, void *user_data)
{
	ARG_UNUSED(dev);

	if (id >= NRF_BELLBOARD_EVENTS_TRIGGERED_COUNT) {
		return -EINVAL;
	}

	cbs[id] = cb;
	cbs_ctx[id] = user_data;

	return 0;
}

static int bellboard_local_set_enabled(const struct device *dev, uint32_t id, bool enable)
{
	bool valid_found = false;

	ARG_UNUSED(dev);

	if (id >= NRF_BELLBOARD_EVENTS_TRIGGERED_COUNT) {
		return -EINVAL;
	}

	for (uint8_t i = 0U; i < BELLBOARD_NUM_IRQS; i++) {
		uint32_t *evt_enabled_mask;

		if ((evt_mappings[i] == 0U) || ((evt_mappings[i] & BIT(id)) == 0U)) {
			continue;
		}

		valid_found = true;
		evt_enabled_mask = &evt_enabled_masks[i];

		if (enable) {
			if ((*evt_enabled_mask & BIT(id)) != 0U) {
				return -EALREADY;
			}

			*evt_enabled_mask |= BIT(id);
			nrf_bellboard_int_enable(bellboard, i, BIT(id));
		} else {
			if ((*evt_enabled_mask & BIT(id)) == 0U) {
				return -EALREADY;
			}

			*evt_enabled_mask &= ~BIT(id);
			nrf_bellboard_int_disable(bellboard, i, BIT(id));
		}
	}

	if (!valid_found) {
		return -EINVAL;
	}

	return 0;
}

static const struct mbox_driver_api bellboard_local_driver_api = {
	.max_channels_get = bellboard_local_max_channels_get,
	.register_callback = bellboard_local_register_callback,
	.set_enabled = bellboard_local_set_enabled,
};

#define BELLBOARD_IRQ_CONFIGURE(name, idx)                                                         \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(0, name),                                                 \
		    (IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, name, irq),                                \
				 DT_INST_IRQ_BY_NAME(0, name, priority), bellboard_local_isr,      \
				 (const void *)idx, 0);                                            \
		     irq_enable(DT_INST_IRQ_BY_NAME(0, name, irq));),                              \
		    ())

static int bellboard_local_init(const struct device *dev)
{
	uint32_t evt_all_mappings =
		evt_mappings[0] | evt_mappings[1] | evt_mappings[2] | evt_mappings[3];

	ARG_UNUSED(dev);

	nrf_bellboard_int_disable(bellboard, 0, evt_mappings[0]);
	nrf_bellboard_int_disable(bellboard, 1, evt_mappings[1]);
	nrf_bellboard_int_disable(bellboard, 2, evt_mappings[2]);
	nrf_bellboard_int_disable(bellboard, 3, evt_mappings[3]);

	for (uint8_t i = 0U; i < NRF_BELLBOARD_EVENTS_TRIGGERED_COUNT; i++) {
		if ((evt_all_mappings & BIT(i)) != 0U) {
			nrf_bellboard_event_clear(bellboard, nrf_bellboard_triggered_event_get(i));
		}
	}

	BELLBOARD_IRQ_CONFIGURE(irq0, 0);
	BELLBOARD_IRQ_CONFIGURE(irq1, 1);
	BELLBOARD_IRQ_CONFIGURE(irq2, 2);
	BELLBOARD_IRQ_CONFIGURE(irq3, 3);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, bellboard_local_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_MBOX_INIT_PRIORITY, &bellboard_local_driver_api);
