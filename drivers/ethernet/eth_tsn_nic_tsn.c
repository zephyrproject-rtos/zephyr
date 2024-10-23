/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/controller.h>

#include <zephyr/posix/posix_types.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/time.h>

#include "eth.h"
#include "eth_tsn_nic_priv.h"

#define LINK_1G 1000000000

#define TSN_ALWAYS_OPEN(from) (from - 1) /* For both timestamp and sysclock */

struct timestamps {
	net_time_t from;
	net_time_t to;
	net_time_t delay_from;
	net_time_t delay_to;
};

typedef uint64_t sysclock_t;

static inline net_time_t ext_time_to_net_time(struct net_ptp_extended_time ext)
{
	/* Unit of fract_nsecond is 2 * (-16) ns */
	return ext.second + (ext.fract_nsecond / (1 << 16));
}

static inline sysclock_t tsn_timestamp_to_sysclock(const struct device *dev, net_time_t timestamp)
{
	/* TODO: Implement PTP */
	net_time_t offset = 0;
	double ticks_scale = 8.0;

	timestamp -= TX_ADJUST_NS;

	return (sysclock_t)((timestamp - offset) / ticks_scale) - PHY_DELAY_CLOCKS;
}

void fill_default_metadata(const struct device *dev, net_time_t now, struct tx_metadata *metadata)
{
	metadata->fail_policy = TSN_FAIL_POLICY_DROP;

	metadata->from.tick = tsn_timestamp_to_sysclock(dev, now);
	metadata->from.priority = 0;
	metadata->to.tick = metadata->from.tick - 1;
	metadata->to.priority = 0;

	metadata->delay_from.tick = metadata->from.tick;
	metadata->delay_from.priority = 0;
	metadata->delay_to.tick = metadata->delay_from.tick - 1;
	metadata->delay_to.priority = 0;
}

/**
 * ============================================================================================
 * TSN Functions
 * ============================================================================================
 */

#if CONFIG_NET_TC_TX_COUNT > 0

static void bake_qos_config(struct tsn_config *config);
static net_time_t bytes_to_ns(uint64_t bytes);

static bool get_timestamps(struct timestamps *timestamps, const struct tsn_config *tsn_config,
			   net_time_t from, uint8_t vlan_prio, uint64_t bytes, bool consider_delay);
static void spend_qav_credit(struct tsn_config *tsn_config, net_time_t at, uint8_t vlan_prio,
			     uint64_t bytes);

/**
 * Exported functions
 */

void tsn_init_configs(const struct device *dev)
{
	struct eth_tsn_nic_data *data = dev->data;
	struct tsn_config *config = &data->tsn_config;

	memset(config, 0, sizeof(struct tsn_config));

	bake_qos_config(config);
}

int tsn_set_qbv(const struct device *dev, struct ethernet_qbv_param param)
{
	return 0;
}

int tsn_set_qav(const struct device *dev, struct ethernet_qav_param param)
{
	return 0;
}

int tsn_fill_metadata(const struct device *dev, net_time_t now, struct tx_buffer *tx_buf)
{
	struct eth_tsn_nic_data *data = dev->data;
	struct tsn_config *tsn_config = &data->tsn_config;
	struct tx_metadata *metadata = &tx_buf->metadata;
	struct timestamps timestamps;

	/* TODO: Handle PTP Packets */
	/* TODO: Track buffer */

	uint8_t vlan_prio = 0, queue_prio = 0; /* TODO: Handle vlan priority */
	bool consider_delay = false;
	net_time_t from, duration_ns;

	if (vlan_prio >= NET_TC_TX_COUNT) {
		/* Invalid priority */
		fill_default_metadata(dev, now, metadata);
		return 0;
	}

	from = now + H2C_LATENCY_NS;
	duration_ns = bytes_to_ns(metadata->frame_length);

	if (tsn_config->qbv.enabled == false && tsn_config->qav[vlan_prio].enabled == false) {
		timestamps.from = tsn_config->total_available_at;
		timestamps.to = timestamps.from + DEFAULT_TO_MARGIN;
		timestamps.delay_from = tsn_config->total_available_at;
		timestamps.delay_to = timestamps.delay_from + DEFAULT_TO_MARGIN;
		metadata->fail_policy = TSN_FAIL_POLICY_DROP;
	} else {
		if (tsn_config->qav[vlan_prio].enabled == true &&
		    tsn_config->qav[vlan_prio].available_at > from) {
			from = tsn_config->qav[vlan_prio].available_at;
		}
		if (consider_delay) {
			if (false /* buffer_tracker->pending_packets >= TSN_QUEUE_SIZE */) {
				return -EBUSY;
			}
		} else {
			if (false /* buffer_tracker->pending_packets >= BE_QUEUE_SIZE */) {
				return -EBUSY;
			}
			from = MAX(from, tsn_config->total_available_at);
		}

		get_timestamps(&timestamps, tsn_config, from, vlan_prio, metadata->frame_length,
			       consider_delay);
		metadata->fail_policy =
			consider_delay ? TSN_FAIL_POLICY_RETRY : TSN_FAIL_POLICY_DROP;
	}

	metadata->from.tick = tsn_timestamp_to_sysclock(dev, timestamps.from);
	metadata->from.priority = queue_prio;
	if (timestamps.to == TSN_ALWAYS_OPEN(timestamps.from)) {
		metadata->to.tick = TSN_ALWAYS_OPEN(metadata->from.tick);
	} else {
		metadata->to.tick = tsn_timestamp_to_sysclock(dev, timestamps.to);
	}
	metadata->to.priority = queue_prio;
	metadata->delay_from.tick = tsn_timestamp_to_sysclock(dev, timestamps.delay_from);
	metadata->delay_from.priority = queue_prio;
	metadata->delay_to.tick = tsn_timestamp_to_sysclock(dev, timestamps.delay_to);
	metadata->delay_to.priority = queue_prio;

	/* TODO: Timestamp ID */

	spend_qav_credit(tsn_config, from, vlan_prio, metadata->frame_length);
	tsn_config->queue_available_at[queue_prio] += duration_ns;
	tsn_config->total_available_at += duration_ns;

	return 0;
}

/**
 * Static functions
 */

static net_time_t bytes_to_ns(uint64_t bytes)
{
	uint64_t link_speed = LINK_1G;

	return MAX(bytes, (uint64_t)ETH_ZLEN) * 8 * NS_IN_1S / link_speed;
}

static void bake_qos_config(struct tsn_config *config)
{
	/**
	 * NOTE: This is the exact same copy of the one in the Linux driver
	 *       Review this when something unexpected happens
	 */
	int slot_id, vlan_prio;
	bool qav_disabled = true;
	struct qbv_baked_config *baked;

	if (config->qbv.enabled == false) {
		/* TODO: remove this when throughput issue without QoS gets resolved */
		for (vlan_prio = 0; vlan_prio < NET_TC_TX_COUNT; vlan_prio++) {
			if (config->qav[vlan_prio].enabled) {
				qav_disabled = false;
				break;
			}
		}

		if (qav_disabled) {
			config->qbv.enabled = true;
			config->qbv.start = 0;
			config->qbv.slot_count = 1;
			config->qbv.slots[0].duration_ns = NS_IN_1S;
			for (vlan_prio = 0; vlan_prio < NET_TC_TX_COUNT; vlan_prio++) {
				config->qbv.slots[0].opened_prios[vlan_prio] = true;
			}
		}
	}

	baked = &config->qbv_baked;
	memset(baked, 0, sizeof(struct qbv_baked_config));

	baked->cycle_ns = 0;

	for (vlan_prio = 0; vlan_prio < NET_TC_TX_COUNT; vlan_prio += 1) {
		baked->prios[vlan_prio].slot_count = 1;
		baked->prios[vlan_prio].slots[0].opened =
			config->qbv.slots[0].opened_prios[vlan_prio];
	}

	for (slot_id = 0; slot_id < config->qbv.slot_count; slot_id += 1) {
		uint64_t slot_duration = config->qbv.slots[slot_id].duration_ns;

		baked->cycle_ns += slot_duration;
		for (vlan_prio = 0; vlan_prio < NET_TC_TX_COUNT; vlan_prio += 1) {
			struct qbv_baked_prio *prio = &baked->prios[vlan_prio];

			if (prio->slots[prio->slot_count - 1].opened ==
			    config->qbv.slots[slot_id].opened_prios[vlan_prio]) {
				prio->slots[prio->slot_count - 1].duration_ns += slot_duration;
			} else {
				prio->slots[prio->slot_count].opened =
					config->qbv.slots[slot_id].opened_prios[vlan_prio];
				prio->slots[prio->slot_count].duration_ns = slot_duration;
				prio->slot_count += 1;
			}
		}
	}

	for (vlan_prio = 0; vlan_prio < NET_TC_TX_COUNT; vlan_prio += 1) {
		struct qbv_baked_prio *prio = &baked->prios[vlan_prio];

		if (prio->slot_count % 2 == 1) {
			prio->slots[prio->slot_count].opened =
				!prio->slots[prio->slot_count - 1].opened;
			prio->slots[prio->slot_count].duration_ns = 0;
			prio->slot_count += 1;
		}
	}
}

static bool get_timestamps(struct timestamps *timestamps, const struct tsn_config *tsn_config,
			   net_time_t from, uint8_t vlan_prio, uint64_t bytes, bool consider_delay)
{
	/* TODO: Implement Qbv */
	/* FIXME: These are for suppressing compile warnings */
	timestamps->from = 0;
	timestamps->to = 0;
	timestamps->delay_from = 0;
	timestamps->delay_to = 0;
	return false;
}

static void spend_qav_credit(struct tsn_config *tsn_config, net_time_t at, uint8_t vlan_prio,
			     uint64_t bytes)
{
	/* TODO: Implement Qav */
}

#endif /* CONFIG_NET_TC_TX_COUNT > 0 */
