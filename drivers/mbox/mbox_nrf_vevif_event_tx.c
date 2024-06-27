/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vevif_event_tx

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>

#include <hal/nrf_vpr.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vevif.h>

#if defined(CONFIG_SOC_NRF54L15_ENGA_CPUFLPR)
#define EVENTS_IDX_MAX 17U
#else
#define EVENTS_IDX_MAX NRF_VPR_EVENTS_TRIGGERED_MAX
#endif

#define VEVIF_EVENTS_NUM  DT_INST_PROP(0, nordic_events)
#define VEVIF_EVENTS_MASK DT_INST_PROP(0, nordic_events_mask)

BUILD_ASSERT(DT_INST_PROP(0, nordic_events) <= NRF_VPR_EVENTS_TRIGGERED_COUNT,
	     "Number of events exceeds maximum");

static inline bool vevif_event_tx_is_valid(uint32_t id)
{
	return (id < EVENTS_IDX_MAX) && ((VEVIF_EVENTS_MASK & BIT(id)) != 0U);
}

static int vevif_event_tx_send(const struct device *dev, uint32_t id, const struct mbox_msg *msg)
{
	ARG_UNUSED(dev);

	if (!vevif_event_tx_is_valid(id)) {
		return -EINVAL;
	}

	if (msg != NULL) {
		return -EMSGSIZE;
	}

	nrf_vpr_csr_vevif_events_trigger(BIT(id));

	return 0;
}

static int vevif_event_tx_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t vevif_event_tx_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return VEVIF_EVENTS_NUM;
}

static const struct mbox_driver_api vevif_event_tx_driver_api = {
	.send = vevif_event_tx_send,
	.mtu_get = vevif_event_tx_mtu_get,
	.max_channels_get = vevif_event_tx_max_channels_get,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,
		      &vevif_event_tx_driver_api);
