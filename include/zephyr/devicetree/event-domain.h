/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_EVENT_DOMAIN_H_
#define ZEPHYR_INCLUDE_DEVICETREE_EVENT_DOMAIN_H_

#define DT_EVENT_DOMAIN_EVENT_LATENCIES_US(_node) \
	DT_PROP(_node, event_latencies_us)

#define DT_EVENT_DOMAIN_EVENT_LATENCIES_US_LEN(_node) \
	DT_PROP_LEN(_node, event_latencies_us)

#define DT_EVENT_DOMAIN_EVENT_LATENCY_US_BY_IDX(_node, _idx) \
	DT_PROP_BY_IDX(_node, event_latencies_us, _idx)

#define DT_EVENT_DOMAIN_EVENT_DEVICE_BY_IDX(_node, _idx) \
	DT_PROP_BY_IDX(_node, event_devices, _idx)

#define DT_EVENT_DOMAIN_EVENT_DEVICE(_node) \
	DT_PROP_BY_IDX(_node, event_devices, 0)

#define DT_EVENT_DOMAIN_EVENT_DEVICES(_node) \
	DT_PROP(_node, event_devices)

#define DT_EVENT_DOMAIN_EVENT_DEVICES_LEN(_node) \
	DT_PROP_LEN(_node, event_devices)

#define DT_EVENT_DOMAIN_EVENT_DEVICE_STATES(_node) \
	DT_PROP(_node, event_device_states)

#define DT_EVENT_DOMAIN_EVENT_DEVICE_STATE_BY_IDX(_node, _idx) \
	DT_PROP_BY_IDX(_node, event_device_states, _idx)

#define DT_EVENT_DOMAIN_EVENT_DEVICE_STATES_LEN(_node) \
	DT_PROP_LEN(_node, event_device_states)

#endif /* ZEPHYR_INCLUDE_DEVICETREE_EVENT_DOMAIN_H_ */
