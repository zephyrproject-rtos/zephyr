/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/byteorder.h>

#include "canopen_internal.h"

#define TIME_COB_DEFAULT 0x100U
#define TIME_CONSUME     BIT(31)
#define TIME_PRODUCE     BIT(30)
#define TIME_MS_MASK     GENMASK(27, 0)
#define TIME_LEN         6U

static bool time_active(struct canopen_node *node)
{
	return (node->nmt_state == CANOPEN_NMT_PRE_OPERATIONAL) ||
	       (node->nmt_state == CANOPEN_NMT_OPERATIONAL);
}

void canopen_time_init(struct canopen_node *node)
{
	node->time.filter = -1;
}

void canopen_time_reset(struct canopen_node *node)
{
	node->time.cob_id = TIME_CONSUME | TIME_COB_DEFAULT;
}

void canopen_time_filter(struct canopen_node *node)
{
	uint32_t cob_id = node->time.cob_id;

	if (canopen_filter_set(node, &node->time.filter,
			       ((cob_id & TIME_CONSUME) != 0U) ? (cob_id & CAN_STD_ID_MASK)
							      : CANOPEN_COB_ID_INVALID) != 0) {
		node->time.cob_id &= ~TIME_CONSUME;
	}
}

uint32_t canopen_time_find(struct canopen_node *node, uint8_t sub, struct canopen_od_ref *ref)
{
	if (sub != 0U) {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	canopen_od_comm_ref(ref, CANOPEN_OD_TIME, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW,
			    &node->time.cob_id, &node->time.cob_id, sizeof(node->time.cob_id));

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_time_write(struct canopen_node *node, uint32_t value)
{
	uint32_t old = node->time.cob_id;
	uint32_t id = value & CAN_STD_ID_MASK;

	if (((value & (CANOPEN_COB_ID_FRAME | CANOPEN_COB_ID_RESERVED)) != 0U) ||
	    canopen_cob_id_restricted(id) ||
	    (((old & (TIME_CONSUME | TIME_PRODUCE)) != 0U) &&
	     (id != (old & CAN_STD_ID_MASK)))) {
		return CANOPEN_SDO_ABORT_INVALID_VALUE;
	}

	node->time.cob_id = value;
	canopen_time_filter(node);

	return CANOPEN_SDO_ABORT_NONE;
}

bool canopen_time_rx(struct canopen_node *node, const struct can_frame *frame)
{
	if (((node->time.cob_id & TIME_CONSUME) == 0U) ||
	    (frame->id != (node->time.cob_id & CAN_STD_ID_MASK))) {
		return false;
	}

	if ((frame->dlc == TIME_LEN) && time_active(node) && (node->cfg->time_cb != NULL)) {
		node->cfg->time_cb(node, sys_get_le32(frame->data) & TIME_MS_MASK,
				   sys_get_le16(&frame->data[4]), node->cfg->user_data);
	}

	return true;
}

int canopen_time_send(struct canopen_node *node, uint32_t ms, uint16_t days)
{
	struct can_frame frame = {
		.dlc = TIME_LEN,
	};
	int err = 0;

	if (ms > TIME_MS_MASK) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);

	if ((node->time.cob_id & TIME_PRODUCE) == 0U) {
		err = -EACCES;
	} else if (!time_active(node)) {
		err = -EPERM;
	} else {
		frame.id = node->time.cob_id & CAN_STD_ID_MASK;
		sys_put_le32(ms, frame.data);
		sys_put_le16(days, &frame.data[4]);
		if (canopen_send(node, &frame) != 0) {
			err = -EIO;
		}
	}

	k_mutex_unlock(&node->lock);

	return err;
}
