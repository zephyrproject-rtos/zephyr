/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "canopen_internal.h"

#define GUARD_TOGGLE BIT(7)

enum guard_status {
	GUARD_IDLE,
	GUARD_ACTIVE,
	GUARD_TIMEOUT,
};

static uint32_t guard_lifetime(const struct canopen_guard *guard)
{
	return (uint32_t)guard->guard_time * guard->life_factor;
}

static void guard_restart(struct canopen_guard *guard)
{
	uint32_t lifetime = guard_lifetime(guard);

	guard->deadline = sys_timepoint_calc((lifetime == 0U) ? K_FOREVER : K_MSEC(lifetime));
}

static void guard_resolve(struct canopen_node *node)
{
	if (!canopen_hb_timed_out(node)) {
		(void)canopen_emcy_unset(node, CANOPEN_EMCY_HEARTBEAT);
	}
}

void canopen_guard_init(struct canopen_node *node)
{
	node->guard.filter = -1;
}

void canopen_guard_reset(struct canopen_node *node)
{
	struct canopen_guard *guard = &node->guard;

	guard->guard_time = 0U;
	guard->life_factor = 0U;
	guard->toggle = 0U;
	guard->status = GUARD_IDLE;
	guard->deadline = sys_timepoint_calc(K_FOREVER);
}

void canopen_guard_filter(struct canopen_node *node)
{
	(void)canopen_filter_set(node, &node->guard.filter,
				 CANOPEN_COB_NMT_ERR + node->cfg->node_id);
}

bool canopen_guard_timed_out(struct canopen_node *node)
{
	return node->guard.status == GUARD_TIMEOUT;
}

uint32_t canopen_guard_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			    struct canopen_od_ref *ref)
{
	struct canopen_guard *guard = &node->guard;

	if (sub != 0U) {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	if (index == CANOPEN_OD_GUARD_TIME) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW,
				    &guard->guard_time, &guard->guard_time,
				    sizeof(guard->guard_time));
	} else {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW,
				    &guard->life_factor, &guard->life_factor,
				    sizeof(guard->life_factor));
	}

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_guard_write(struct canopen_node *node, uint16_t index, uint32_t value)
{
	struct canopen_guard *guard = &node->guard;

	if (index == CANOPEN_OD_GUARD_TIME) {
		guard->guard_time = (uint16_t)value;
	} else {
		guard->life_factor = (uint8_t)value;
	}

	if (guard->status == GUARD_IDLE) {
		return CANOPEN_SDO_ABORT_NONE;
	}

	if (guard_lifetime(guard) == 0U) {
		bool timed_out = guard->status == GUARD_TIMEOUT;

		guard->status = GUARD_IDLE;
		if (timed_out) {
			guard_resolve(node);
		}
	}

	guard_restart(guard);

	return CANOPEN_SDO_ABORT_NONE;
}

void canopen_guard_rx(struct canopen_node *node, const struct can_frame *frame)
{
	struct canopen_guard *guard = &node->guard;
	struct can_frame reply = {
		.id = CANOPEN_COB_NMT_ERR + node->cfg->node_id,
		.dlc = 1U,
	};
	bool timed_out;

	if ((frame->id != reply.id) || (node->hb_producer_ms != 0U) ||
	    (node->nmt_state == CANOPEN_NMT_INITIALISING)) {
		return;
	}

	reply.data[0] = guard->toggle | node->nmt_state;
	guard->toggle ^= GUARD_TOGGLE;
	(void)canopen_send(node, &reply);

	if (guard_lifetime(guard) == 0U) {
		return;
	}

	timed_out = guard->status == GUARD_TIMEOUT;
	guard->status = GUARD_ACTIVE;
	guard_restart(guard);
	if (timed_out) {
		guard_resolve(node);
	}
}

void canopen_guard_process(struct canopen_node *node)
{
	struct canopen_guard *guard = &node->guard;

	if ((guard->status != GUARD_ACTIVE) || !sys_timepoint_expired(guard->deadline)) {
		return;
	}

	guard->status = GUARD_TIMEOUT;
	guard->deadline = sys_timepoint_calc(K_FOREVER);
	(void)canopen_emcy_set(node, CANOPEN_EMCY_HEARTBEAT, CANOPEN_ERR_REG_COMMUNICATION, NULL);
	canopen_nmt_comm_error(node);
}

k_timepoint_t canopen_guard_deadline(struct canopen_node *node)
{
	return (node->guard.status == GUARD_ACTIVE) ? node->guard.deadline
						    : sys_timepoint_calc(K_FOREVER);
}
