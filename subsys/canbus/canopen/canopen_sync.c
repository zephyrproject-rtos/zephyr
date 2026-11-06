/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "canopen_internal.h"

#define CANOPEN_OD_COB_ID_SYNC   0x1005U
#define CANOPEN_OD_SYNC_PERIOD   0x1006U
#define CANOPEN_OD_SYNC_WINDOW   0x1007U
#define CANOPEN_OD_SYNC_OVERFLOW 0x1019U

#define SYNC_GENERATE     CANOPEN_COB_ID_BIT30
#define SYNC_OVERFLOW_MIN 2U
#define SYNC_OVERFLOW_MAX 240U

static bool sync_producer(struct canopen_node *node)
{
	return (node->sync.cob_id & SYNC_GENERATE) != 0U;
}

static void sync_expiry(struct k_timer *timer)
{
	struct canopen_node *node = CONTAINER_OF(timer, struct canopen_node, sync.timer);

	atomic_set_bit(&node->flags, CANOPEN_FLAG_SYNC_TICK);
	k_poll_signal_raise(&node->signal, 0);
}

static void sync_monitor(struct canopen_node *node)
{
	bool active = IS_ENABLED(CONFIG_CANOPEN_LEDS) && !sync_producer(node) &&
		      (node->sync.period != 0U) &&
		      ((node->nmt_state == CANOPEN_NMT_PRE_OPERATIONAL) ||
		       (node->nmt_state == CANOPEN_NMT_OPERATIONAL));

	node->sync.timed_out = false;
	node->sync.timeout = sys_timepoint_calc(
		active ? K_USEC(((uint64_t)node->sync.period * 3U) / 2U) : K_FOREVER);
}

static void sync_event(struct canopen_node *node, uint8_t counter)
{
	node->sync.last = k_uptime_ticks();
	canopen_pdo_sync(node, counter);

	if (node->cfg->sync_cb != NULL) {
		node->cfg->sync_cb(node, counter, node->cfg->user_data);
	}
}

void canopen_sync_init(struct canopen_node *node)
{
	node->sync.filter = -1;
	k_timer_init(&node->sync.timer, sync_expiry, NULL);
}

void canopen_sync_reset(struct canopen_node *node)
{
	node->sync.cob_id = CANOPEN_COB_SYNC;
	node->sync.period = 0U;
	node->sync.window = 0U;
	node->sync.overflow = 0U;
	node->sync.counter = 1U;
	node->sync.length_error = false;
	node->sync.timed_out = false;
	node->sync.timeout = sys_timepoint_calc(K_FOREVER);
	(void)canopen_emcy_unset(node, CANOPEN_EMCY_SYNC_LENGTH);
}

void canopen_sync_update(struct canopen_node *node)
{
	bool active = sync_producer(node) && (node->sync.period != 0U) &&
		      (node->nmt_state != CANOPEN_NMT_STOPPED) &&
		      (node->nmt_state != CANOPEN_NMT_INITIALISING);

	(void)canopen_filter_set(node, &node->sync.filter,
				 sync_producer(node) ? CANOPEN_COB_ID_INVALID : node->sync.cob_id);
	sync_monitor(node);

	if (active) {
		k_timer_start(&node->sync.timer, K_USEC(node->sync.period),
			      K_USEC(node->sync.period));
	} else {
		k_timer_stop(&node->sync.timer);
		atomic_clear_bit(&node->flags, CANOPEN_FLAG_SYNC_TICK);
	}
}

uint32_t canopen_sync_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			   struct canopen_od_ref *ref)
{
	void *data;
	uint8_t type = CANOPEN_OD_TYPE_UNSIGNED32;
	uint16_t size = sizeof(uint32_t);

	if (sub != 0U) {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	switch (index) {
	case CANOPEN_OD_COB_ID_SYNC:
		data = &node->sync.cob_id;
		break;
	case CANOPEN_OD_SYNC_PERIOD:
		data = &node->sync.period;
		break;
	case CANOPEN_OD_SYNC_WINDOW:
		data = &node->sync.window;
		break;
	default:
		data = &node->sync.overflow;
		type = CANOPEN_OD_TYPE_UNSIGNED8;
		size = sizeof(uint8_t);
		break;
	}

	canopen_od_comm_ref(ref, index, sub, type, CANOPEN_OD_RW, data, data, size);

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t sync_cob_id_write(struct canopen_node *node, uint32_t value)
{
	uint32_t id = value & CAN_STD_ID_MASK;

	if (((value & (CANOPEN_COB_ID_FRAME | CANOPEN_COB_ID_RESERVED)) != 0U) ||
	    canopen_cob_id_restricted(id)) {
		return CANOPEN_SDO_ABORT_INVALID_VALUE;
	}

	if (sync_producer(node) && ((value & SYNC_GENERATE) != 0U) &&
	    (id != (node->sync.cob_id & CAN_STD_ID_MASK))) {
		return CANOPEN_SDO_ABORT_INVALID_VALUE;
	}

	if (!sync_producer(node) && ((value & SYNC_GENERATE) != 0U)) {
		node->sync.counter = 1U;
	}

	node->sync.cob_id = value & (SYNC_GENERATE | CAN_STD_ID_MASK);

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_sync_write(struct canopen_node *node, uint16_t index, uint32_t value)
{
	uint32_t abort;

	switch (index) {
	case CANOPEN_OD_COB_ID_SYNC:
		abort = sync_cob_id_write(node, value);
		if (abort == CANOPEN_SDO_ABORT_NONE) {
			canopen_sync_update(node);
		}
		return abort;
	case CANOPEN_OD_SYNC_PERIOD:
		if ((node->sync.period == 0U) && (value != 0U)) {
			node->sync.counter = 1U;
		}
		node->sync.period = value;
		canopen_sync_update(node);
		return CANOPEN_SDO_ABORT_NONE;
	case CANOPEN_OD_SYNC_WINDOW:
		node->sync.window = value;
		return CANOPEN_SDO_ABORT_NONE;
	default:
		if (node->sync.period != 0U) {
			return CANOPEN_SDO_ABORT_DEVICE_STATE;
		}
		if ((value == 1U) || (value > SYNC_OVERFLOW_MAX)) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}
		node->sync.overflow = (uint8_t)value;
		return CANOPEN_SDO_ABORT_NONE;
	}
}

bool canopen_sync_rx(struct canopen_node *node, const struct can_frame *frame)
{
	if (sync_producer(node) || (frame->id != (node->sync.cob_id & CAN_STD_ID_MASK))) {
		return false;
	}

	if (node->nmt_state == CANOPEN_NMT_STOPPED) {
		return true;
	}

	if (frame->dlc != ((node->sync.overflow >= SYNC_OVERFLOW_MIN) ? 1U : 0U)) {
		if (!node->sync.length_error) {
			node->sync.length_error = true;
			(void)canopen_emcy_set(node, CANOPEN_EMCY_SYNC_LENGTH,
					       CANOPEN_ERR_REG_COMMUNICATION, NULL);
		}
	} else if (node->sync.length_error) {
		node->sync.length_error = false;
		(void)canopen_emcy_unset(node, CANOPEN_EMCY_SYNC_LENGTH);
	}

	sync_monitor(node);
	sync_event(node, (frame->dlc == 1U) ? frame->data[0] : 0U);

	return true;
}

void canopen_sync_state(struct canopen_node *node, enum canopen_nmt_state old)
{
	if (old == CANOPEN_NMT_STOPPED) {
		node->sync.counter = 1U;
	}

	canopen_sync_update(node);
}

void canopen_sync_process(struct canopen_node *node)
{
	struct can_frame frame = {
		.id = node->sync.cob_id & CAN_STD_ID_MASK,
	};
	uint8_t counter = 0U;

	if (!node->sync.timed_out && sys_timepoint_expired(node->sync.timeout)) {
		node->sync.timed_out = true;
	}

	if (!atomic_test_and_clear_bit(&node->flags, CANOPEN_FLAG_SYNC_TICK)) {
		return;
	}

	if (node->sync.overflow >= SYNC_OVERFLOW_MIN) {
		counter = node->sync.counter;
		frame.dlc = 1U;
		frame.data[0] = counter;
		node->sync.counter = (counter >= node->sync.overflow) ? 1U : (counter + 1U);
	}

	(void)canopen_send(node, &frame);
	sync_event(node, counter);
}

k_timepoint_t canopen_sync_deadline(struct canopen_node *node)
{
	return node->sync.timed_out ? sys_timepoint_calc(K_FOREVER) : node->sync.timeout;
}

bool canopen_sync_window_open(struct canopen_node *node)
{
	return (node->sync.window == 0U) ||
	       (k_ticks_to_us_floor64(k_uptime_ticks() - node->sync.last) <= node->sync.window);
}
