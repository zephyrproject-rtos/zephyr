/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include "canopen_internal.h"

#define HB_NODE_ID(entry) ((uint8_t)((entry) >> 16))
#define HB_TIME_MS(entry) ((uint16_t)(entry))
#define HB_RESERVED       GENMASK(31, 24)
#define HB_STATE_MASK     0x7FU
#define HB_NODE_ID_MAX    127U

enum hb_status {
	HB_UNUSED,
	HB_WAITING,
	HB_ACTIVE,
	HB_TIMEOUT,
};

static const uint8_t hb_highest = CONFIG_CANOPEN_HB_CONSUMER_COUNT;

static bool hb_entry_active(uint32_t entry)
{
	uint8_t id = HB_NODE_ID(entry);

	return (id != 0U) && (id <= HB_NODE_ID_MAX) && (HB_TIME_MS(entry) != 0U);
}

static bool hb_duplicate(const uint32_t *entries, size_t count, size_t skip, uint32_t entry)
{
	for (size_t i = 0U; i < count; i++) {
		if ((i != skip) && hb_entry_active(entries[i]) &&
		    (HB_NODE_ID(entries[i]) == HB_NODE_ID(entry))) {
			return true;
		}
	}

	return false;
}

static void hb_configure(struct canopen_node *node, struct canopen_hb_consumer *hb)
{
	bool active = hb_entry_active(hb->entry);

	hb->status = active ? HB_WAITING : HB_UNUSED;
	hb->deadline = sys_timepoint_calc(K_FOREVER);

	if (canopen_filter_set(node, &hb->filter,
			       active ? (CANOPEN_COB_NMT_ERR + HB_NODE_ID(hb->entry))
				      : CANOPEN_COB_ID_INVALID) != 0) {
		hb->status = HB_UNUSED;
	}
}

bool canopen_hb_timed_out(struct canopen_node *node)
{
	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		if (node->hb[i].status == HB_TIMEOUT) {
			return true;
		}
	}

	return false;
}

static void hb_timeout_update(struct canopen_node *node)
{
	if (!canopen_hb_timed_out(node) &&
	    !(IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING) && canopen_guard_timed_out(node))) {
		(void)canopen_emcy_unset(node, CANOPEN_EMCY_HEARTBEAT);
	}
}

static void hb_event(struct canopen_node *node, const struct canopen_hb_consumer *hb,
		     enum canopen_hb_event event)
{
	if (node->cfg->hb_cb != NULL) {
		node->cfg->hb_cb(node, HB_NODE_ID(hb->entry), event,
				 (enum canopen_nmt_state)hb->state, node->cfg->user_data);
	}
}

int canopen_hb_init(struct canopen_node *node)
{
	const struct canopen_node_config *cfg = node->cfg;

	if (cfg->hb_consumer_count > CONFIG_CANOPEN_HB_CONSUMER_COUNT) {
		return -EINVAL;
	}

	for (uint8_t i = 0U; i < cfg->hb_consumer_count; i++) {
		if (((cfg->hb_consumer[i] & HB_RESERVED) != 0U) ||
		    (hb_entry_active(cfg->hb_consumer[i]) &&
		     hb_duplicate(cfg->hb_consumer, cfg->hb_consumer_count, i,
				  cfg->hb_consumer[i]))) {
			return -EINVAL;
		}
	}

	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		node->hb[i].filter = -1;
	}

	return 0;
}

void canopen_hb_reset(struct canopen_node *node)
{
	const struct canopen_node_config *cfg = node->cfg;

	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		node->hb[i].entry = (i < cfg->hb_consumer_count) ? cfg->hb_consumer[i] : 0U;
		node->hb[i].status = HB_UNUSED;
		node->hb[i].deadline = sys_timepoint_calc(K_FOREVER);
	}
}

void canopen_hb_filters(struct canopen_node *node)
{
	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		hb_configure(node, &node->hb[i]);
	}

	hb_timeout_update(node);
}

uint32_t canopen_hb_find(struct canopen_node *node, uint8_t sub, struct canopen_od_ref *ref)
{
	const uint16_t index = CANOPEN_OD_HB_CONSUMER;

	if (sub == 0U) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_CONST,
				    &hb_highest, NULL, sizeof(hb_highest));
	} else if (sub <= CONFIG_CANOPEN_HB_CONSUMER_COUNT) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW,
				    &node->hb[sub - 1U].entry, &node->hb[sub - 1U].entry,
				    sizeof(node->hb[0].entry));
	} else {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_hb_write(struct canopen_node *node, uint8_t sub, uint32_t value)
{
	struct canopen_hb_consumer *hb = &node->hb[sub - 1U];

	if ((value & HB_RESERVED) != 0U) {
		return CANOPEN_SDO_ABORT_INVALID_VALUE;
	}

	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		if ((i != (sub - 1U)) && hb_entry_active(value) &&
		    hb_entry_active(node->hb[i].entry) &&
		    (HB_NODE_ID(node->hb[i].entry) == HB_NODE_ID(value))) {
			return CANOPEN_SDO_ABORT_PARAM_INCOMPATIBLE;
		}
	}

	hb->entry = value;
	hb_configure(node, hb);
	hb_timeout_update(node);

	return CANOPEN_SDO_ABORT_NONE;
}

bool canopen_hb_rx(struct canopen_node *node, const struct can_frame *frame)
{
	if ((frame->id & ~(uint32_t)HB_NODE_ID_MAX) != CANOPEN_COB_NMT_ERR) {
		return false;
	}

	if (frame->dlc != 1U) {
		return true;
	}

	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		struct canopen_hb_consumer *hb = &node->hb[i];
		uint8_t state = frame->data[0] & HB_STATE_MASK;
		bool timed_out;
		bool report;

		if ((hb->status == HB_UNUSED) ||
		    ((CANOPEN_COB_NMT_ERR + HB_NODE_ID(hb->entry)) != frame->id)) {
			continue;
		}

		report = (hb->status != HB_ACTIVE) || (hb->state != state);
		timed_out = hb->status == HB_TIMEOUT;
		hb->status = HB_ACTIVE;
		if (timed_out) {
			hb_timeout_update(node);
		}

		hb->state = state;
		hb->deadline = sys_timepoint_calc(K_MSEC(HB_TIME_MS(hb->entry)));

		if (report) {
			hb_event(node, hb, CANOPEN_HB_EVENT_STATE);
		}

		break;
	}

	return true;
}

void canopen_hb_process(struct canopen_node *node)
{
	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		struct canopen_hb_consumer *hb = &node->hb[i];

		if ((hb->status != HB_ACTIVE) || !sys_timepoint_expired(hb->deadline)) {
			continue;
		}

		hb->status = HB_TIMEOUT;
		hb_event(node, hb, CANOPEN_HB_EVENT_TIMEOUT);
		(void)canopen_emcy_set(node, CANOPEN_EMCY_HEARTBEAT, CANOPEN_ERR_REG_COMMUNICATION,
				       NULL);
		canopen_nmt_comm_error(node);
	}
}

k_timepoint_t canopen_hb_deadline(struct canopen_node *node)
{
	k_timepoint_t next = sys_timepoint_calc(K_FOREVER);

	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		if ((node->hb[i].status == HB_ACTIVE) &&
		    (sys_timepoint_cmp(node->hb[i].deadline, next) < 0)) {
			next = node->hb[i].deadline;
		}
	}

	return next;
}

int canopen_hb_consumer_state(struct canopen_node *node, uint8_t remote_id,
			      enum canopen_nmt_state *state)
{
	int err = -ENOENT;

	k_mutex_lock(&node->lock, K_FOREVER);

	for (uint8_t i = 0U; i < CONFIG_CANOPEN_HB_CONSUMER_COUNT; i++) {
		const struct canopen_hb_consumer *hb = &node->hb[i];

		if ((hb->status == HB_UNUSED) || (HB_NODE_ID(hb->entry) != remote_id)) {
			continue;
		}

		*state = (enum canopen_nmt_state)hb->state;
		if (hb->status == HB_WAITING) {
			err = -EAGAIN;
		} else if (hb->status == HB_TIMEOUT) {
			err = -ETIMEDOUT;
		} else {
			err = 0;
		}
		break;
	}

	k_mutex_unlock(&node->lock);

	return err;
}
