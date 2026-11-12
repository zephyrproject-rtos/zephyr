/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>

#include "canopen_internal.h"

#define CANOPEN_OD_ERROR_FIELD  0x1003U
#define CANOPEN_OD_COB_ID_EMCY  0x1014U
#define CANOPEN_OD_INHIBIT_EMCY 0x1015U

#define EMCY_COB_BASE   0x080U
#define EMCY_INHIBIT_US 100U
#define EMCY_MSEF_LEN   5U

static bool emcy_allowed(const struct canopen_node *node)
{
	return ((node->nmt_state == CANOPEN_NMT_PRE_OPERATIONAL) ||
		(node->nmt_state == CANOPEN_NMT_OPERATIONAL)) &&
	       ((node->emcy.cob_id & CANOPEN_COB_ID_INVALID) == 0U);
}

static void emcy_queue(struct canopen_emcy *emcy, uint16_t code, uint8_t reg, const uint8_t *msef)
{
	uint8_t *data;

	if ((emcy->cob_id & CANOPEN_COB_ID_INVALID) != 0U) {
		return;
	}

	if (emcy->pending_count == CONFIG_CANOPEN_EMCY_QUEUE_SIZE) {
		emcy->pending_head = (emcy->pending_head + 1U) % CONFIG_CANOPEN_EMCY_QUEUE_SIZE;
		emcy->pending_count--;
	}

	data = emcy->pending[(emcy->pending_head + emcy->pending_count) %
			     CONFIG_CANOPEN_EMCY_QUEUE_SIZE];
	sys_put_le16(code, &data[0]);
	data[2] = reg;
	if (msef != NULL) {
		memcpy(&data[3], msef, EMCY_MSEF_LEN);
	} else {
		memset(&data[3], 0, EMCY_MSEF_LEN);
	}

	emcy->pending_count++;
}

static void emcy_history_add(struct canopen_emcy *emcy, uint16_t code, const uint8_t *msef)
{
	uint32_t info = (msef != NULL) ? sys_get_le16(msef) : 0U;

	memmove(&emcy->history[1], &emcy->history[0],
		(CONFIG_CANOPEN_EMCY_HISTORY_SIZE - 1U) * sizeof(emcy->history[0]));
	emcy->history[0] = (info << 16) | code;

	if (emcy->history_count < CONFIG_CANOPEN_EMCY_HISTORY_SIZE) {
		emcy->history_count++;
	}
}

static void emcy_update_register(struct canopen_node *node)
{
	const struct canopen_emcy *emcy = &node->emcy;
	uint8_t reg = (emcy->active_count != 0U) ? CANOPEN_ERR_REG_GENERIC : 0U;

	for (uint8_t i = 0U; i < emcy->active_count; i++) {
		reg |= emcy->active_reg[i];
	}

	node->error_register = reg;
}

void canopen_emcy_reset(struct canopen_node *node, bool app)
{
	struct canopen_emcy *emcy = &node->emcy;

	emcy->cob_id = EMCY_COB_BASE + node->cfg->node_id;
	emcy->inhibit_time = 0U;
	emcy->inhibit_end = sys_timepoint_calc(K_NO_WAIT);
	emcy->pending_count = 0U;
	emcy->history_count = 0U;

	if (app) {
		emcy->active_count = 0U;
	}

	emcy_update_register(node);
}

void canopen_emcy_signal(struct canopen_node *node, uint16_t code, uint8_t reg)
{
	emcy_history_add(&node->emcy, code, NULL);
	emcy_queue(&node->emcy, code, node->error_register | reg | CANOPEN_ERR_REG_GENERIC, NULL);
}

int canopen_emcy_set(struct canopen_node *node, uint16_t code, uint8_t reg, const uint8_t *msef)
{
	struct canopen_emcy *emcy = &node->emcy;

	for (uint8_t i = 0U; i < emcy->active_count; i++) {
		if (emcy->active[i] == code) {
			return -EALREADY;
		}
	}

	if (emcy->active_count == CONFIG_CANOPEN_EMCY_ACTIVE_MAX) {
		return -ENOSPC;
	}

	emcy->active[emcy->active_count] = code;
	emcy->active_reg[emcy->active_count] = reg;
	emcy->active_count++;
	emcy_update_register(node);

	emcy_history_add(emcy, code, msef);
	emcy_queue(emcy, code, node->error_register, msef);

	return 0;
}

int canopen_emcy_unset(struct canopen_node *node, uint16_t code)
{
	struct canopen_emcy *emcy = &node->emcy;

	for (uint8_t i = 0U; i < emcy->active_count; i++) {
		if (emcy->active[i] == code) {
			emcy->active_count--;
			emcy->active[i] = emcy->active[emcy->active_count];
			emcy->active_reg[i] = emcy->active_reg[emcy->active_count];
			emcy_update_register(node);
			emcy_queue(emcy, CANOPEN_EMCY_NO_ERROR, node->error_register, NULL);
			return 0;
		}
	}

	return -ENOENT;
}

bool canopen_emcy_active(struct canopen_node *node, uint16_t code)
{
	for (uint8_t i = 0U; i < node->emcy.active_count; i++) {
		if (node->emcy.active[i] == code) {
			return true;
		}
	}

	return false;
}

void canopen_emcy_can_state(struct canopen_node *node, enum can_state state)
{
	enum can_state last = node->can_state_last;

	if ((state == last) || (state == CAN_STATE_STOPPED)) {
		return;
	}

	node->can_state_last = state;

	if (state == CAN_STATE_ERROR_PASSIVE) {
		(void)canopen_emcy_set(node, CANOPEN_EMCY_CAN_PASSIVE,
				       CANOPEN_ERR_REG_COMMUNICATION, NULL);
	} else if (state != CAN_STATE_BUS_OFF) {
		(void)canopen_emcy_unset(node, CANOPEN_EMCY_CAN_PASSIVE);
	}

	if (state == CAN_STATE_BUS_OFF) {
		canopen_nmt_comm_error(node);
	} else if (last == CAN_STATE_BUS_OFF) {
		canopen_emcy_signal(node, CANOPEN_EMCY_BUS_OFF_RECOVERED,
				    CANOPEN_ERR_REG_COMMUNICATION);
	}
}

uint32_t canopen_emcy_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			   struct canopen_od_ref *ref)
{
	struct canopen_emcy *emcy = &node->emcy;

	if (index == CANOPEN_OD_ERROR_FIELD) {
		if (sub > CONFIG_CANOPEN_EMCY_HISTORY_SIZE) {
			return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
		}

		if (sub > emcy->history_count) {
			return CANOPEN_SDO_ABORT_NO_DATA;
		}

		if (sub == 0U) {
			canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8,
					    CANOPEN_OD_RW, &emcy->history_count,
					    &emcy->history_count, sizeof(emcy->history_count));
		} else {
			canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32,
					    CANOPEN_OD_RO, &emcy->history[sub - 1U], NULL,
					    sizeof(emcy->history[0]));
		}

		return CANOPEN_SDO_ABORT_NONE;
	}

	if (sub != 0U) {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	if (index == CANOPEN_OD_COB_ID_EMCY) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW,
				    &emcy->cob_id, &emcy->cob_id, sizeof(emcy->cob_id));
	} else {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW,
				    &emcy->inhibit_time, &emcy->inhibit_time,
				    sizeof(emcy->inhibit_time));
	}

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_emcy_write(struct canopen_node *node, uint16_t index, uint32_t value)
{
	struct canopen_emcy *emcy = &node->emcy;
	uint32_t id = value & CAN_STD_ID_MASK;

	switch (index) {
	case CANOPEN_OD_ERROR_FIELD:
		if (value != 0U) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}
		emcy->history_count = 0U;
		return CANOPEN_SDO_ABORT_NONE;
	case CANOPEN_OD_COB_ID_EMCY:
		if (((value & (CANOPEN_COB_ID_BIT30 | CANOPEN_COB_ID_FRAME |
			       CANOPEN_COB_ID_RESERVED)) != 0U) ||
		    (((value & CANOPEN_COB_ID_INVALID) == 0U) &&
		     ((((emcy->cob_id & CANOPEN_COB_ID_INVALID) == 0U) &&
		       (id != (emcy->cob_id & CAN_STD_ID_MASK))) ||
		      canopen_cob_id_restricted(id)))) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}
		emcy->cob_id = value;
		if ((value & CANOPEN_COB_ID_INVALID) != 0U) {
			emcy->pending_count = 0U;
		}
		return CANOPEN_SDO_ABORT_NONE;
	default:
		emcy->inhibit_time = (uint16_t)value;
		return CANOPEN_SDO_ABORT_NONE;
	}
}

static void emcy_tx_cb(const struct device *dev, int error, void *user_data)
{
	struct canopen_node *node = user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(error);

	atomic_clear_bit(&node->flags, CANOPEN_FLAG_EMCY_TX);
	k_poll_signal_raise(&node->signal, 0);
}

void canopen_emcy_process(struct canopen_node *node)
{
	struct canopen_emcy *emcy = &node->emcy;
	struct can_frame frame = {
		.id = emcy->cob_id & CAN_STD_ID_MASK,
		.dlc = CAN_MAX_DLC,
	};
	int err;

	if ((emcy->pending_count == 0U) || !emcy_allowed(node) ||
	    !sys_timepoint_expired(emcy->inhibit_end) ||
	    atomic_test_and_set_bit(&node->flags, CANOPEN_FLAG_EMCY_TX)) {
		return;
	}

	memcpy(frame.data, emcy->pending[emcy->pending_head], CAN_MAX_DLC);
	err = can_send(node->cfg->can, &frame, K_NO_WAIT, emcy_tx_cb, node);
	if (err != 0) {
		atomic_clear_bit(&node->flags, CANOPEN_FLAG_EMCY_TX);
		if (err == -EAGAIN) {
			emcy->inhibit_end = sys_timepoint_calc(K_MSEC(1));
			return;
		}
	}

	emcy->pending_head = (emcy->pending_head + 1U) % CONFIG_CANOPEN_EMCY_QUEUE_SIZE;
	emcy->pending_count--;
	emcy->inhibit_end = sys_timepoint_calc(K_USEC(emcy->inhibit_time * EMCY_INHIBIT_US));
}

k_timepoint_t canopen_emcy_deadline(struct canopen_node *node)
{
	if ((node->emcy.pending_count != 0U) && emcy_allowed(node) &&
	    !atomic_test_bit(&node->flags, CANOPEN_FLAG_EMCY_TX)) {
		return node->emcy.inhibit_end;
	}

	return sys_timepoint_calc(K_FOREVER);
}

int canopen_emcy_report(struct canopen_node *node, uint16_t code, uint8_t reg,
			const uint8_t *msef)
{
	int err;

	if (code == CANOPEN_EMCY_NO_ERROR) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	err = canopen_emcy_set(node, code, reg, msef);
	k_mutex_unlock(&node->lock);

	k_poll_signal_raise(&node->signal, 0);

	return err;
}

int canopen_emcy_clear(struct canopen_node *node, uint16_t code)
{
	int err;

	k_mutex_lock(&node->lock, K_FOREVER);
	err = canopen_emcy_unset(node, code);
	k_mutex_unlock(&node->lock);

	k_poll_signal_raise(&node->signal, 0);

	return err;
}
