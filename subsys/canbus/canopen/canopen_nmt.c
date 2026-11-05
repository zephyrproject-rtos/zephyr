/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include "canopen_internal.h"

static void nmt_send_state(struct canopen_node *node, uint8_t state)
{
	struct can_frame frame = {
		.id = CANOPEN_COB_NMT_ERR + node->cfg->node_id,
		.dlc = 1U,
		.data = {state},
	};

	(void)canopen_send(node, &frame);
}

static void nmt_set_state(struct canopen_node *node, enum canopen_nmt_state state)
{
	enum canopen_nmt_state old = node->nmt_state;

	if (old == state) {
		return;
	}

	node->nmt_state = state;

	if (state == CANOPEN_NMT_STOPPED) {
		canopen_sdo_server_reset(node);
	}

	canopen_sync_state(node, old);
	canopen_pdo_state(node);

	if (node->cfg->nmt_state_cb != NULL) {
		node->cfg->nmt_state_cb(node, state, node->cfg->user_data);
	}
}

void canopen_nmt_hb_restart(struct canopen_node *node)
{
	k_timeout_t period = (node->hb_producer_ms == 0U) ? K_FOREVER
							   : K_MSEC(node->hb_producer_ms);

	node->hb_next = sys_timepoint_calc(period);
}

void canopen_nmt_boot(struct canopen_node *node, bool reset_app)
{
	if (reset_app) {
		if (node->cfg->reset_app_cb != NULL) {
			node->cfg->reset_app_cb(node, node->cfg->user_data);
		}
		if (IS_ENABLED(CONFIG_CANOPEN_STORAGE)) {
			canopen_storage_load(node, true);
		}
		canopen_emcy_reset(node, true);
	}

	node->nmt_state = CANOPEN_NMT_INITIALISING;
	canopen_od_comm_reset(node);
	canopen_sdo_server_reset(node);
	canopen_sync_update(node);
	canopen_pdo_filters(node);
	canopen_hb_filters(node);
	if (IS_ENABLED(CONFIG_CANOPEN_TIME)) {
		canopen_time_filter(node);
	}
	if (IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING)) {
		canopen_guard_filter(node);
	}
	k_msgq_purge(&node->txq);

	nmt_send_state(node, CANOPEN_NMT_INITIALISING);
	canopen_nmt_hb_restart(node);
	nmt_set_state(node, CANOPEN_NMT_PRE_OPERATIONAL);
}

int canopen_nmt_apply(struct canopen_node *node, enum canopen_nmt_cmd cmd)
{
	switch (cmd) {
	case CANOPEN_NMT_CMD_START:
		nmt_set_state(node, CANOPEN_NMT_OPERATIONAL);
		break;
	case CANOPEN_NMT_CMD_STOP:
		nmt_set_state(node, CANOPEN_NMT_STOPPED);
		break;
	case CANOPEN_NMT_CMD_ENTER_PRE_OPERATIONAL:
		nmt_set_state(node, CANOPEN_NMT_PRE_OPERATIONAL);
		break;
	case CANOPEN_NMT_CMD_RESET_NODE:
		canopen_nmt_boot(node, true);
		break;
	case CANOPEN_NMT_CMD_RESET_COMMUNICATION:
		canopen_nmt_boot(node, false);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

void canopen_nmt_rx(struct canopen_node *node, const struct can_frame *frame)
{
	if ((frame->dlc != 2U) ||
	    ((frame->data[1] != 0U) && (frame->data[1] != node->cfg->node_id))) {
		return;
	}

	(void)canopen_nmt_apply(node, (enum canopen_nmt_cmd)frame->data[0]);
}

void canopen_nmt_comm_error(struct canopen_node *node)
{
	if (node->error_behavior == CANOPEN_ERROR_BEHAVIOR_STOPPED) {
		nmt_set_state(node, CANOPEN_NMT_STOPPED);
	} else if ((node->error_behavior == CANOPEN_ERROR_BEHAVIOR_PRE_OPERATIONAL) &&
		   (node->nmt_state == CANOPEN_NMT_OPERATIONAL)) {
		nmt_set_state(node, CANOPEN_NMT_PRE_OPERATIONAL);
	}
}

void canopen_nmt_process(struct canopen_node *node)
{
	if (sys_timepoint_expired(node->hb_next)) {
		nmt_send_state(node, node->nmt_state);
		canopen_nmt_hb_restart(node);
	}
}

k_timepoint_t canopen_nmt_deadline(struct canopen_node *node)
{
	return node->hb_next;
}

enum canopen_nmt_state canopen_nmt_get_state(struct canopen_node *node)
{
	return (enum canopen_nmt_state)node->nmt_state;
}

int canopen_nmt_local_cmd(struct canopen_node *node, enum canopen_nmt_cmd cmd)
{
	int err;

	k_mutex_lock(&node->lock, K_FOREVER);
	err = canopen_nmt_apply(node, cmd);
	k_mutex_unlock(&node->lock);

	k_poll_signal_raise(&node->signal, 0);

	return err;
}
