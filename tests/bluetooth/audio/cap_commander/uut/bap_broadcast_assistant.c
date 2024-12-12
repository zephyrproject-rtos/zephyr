/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/bluetooth/audio/bap.h"
#include "test_common.h"

static sys_slist_t broadcast_assistant_cbs = SYS_SLIST_STATIC_INIT(&broadcast_assistant_cbs);

struct bap_broadcast_assistant_recv_state_info {
	uint8_t src_id;
	/** Cached PAST available */
	bool past_avail;
	uint8_t adv_sid;
	uint32_t broadcast_id;
	bt_addr_le_t addr;
};

struct bap_broadcast_assistant_instance {
	struct bt_conn *conn;
	struct bap_broadcast_assistant_recv_state_info recv_states;
	/*
	 * the following are not part of the broadcast_assistant instance, but adding them allow us
	 * to easily check pa_sync and bis_sync states
	 */
	enum bt_bap_pa_state pa_sync_state;
	uint8_t num_subgroups;
	struct bt_bap_bass_subgroup subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];
};

static struct bap_broadcast_assistant_instance broadcast_assistants[CONFIG_BT_MAX_CONN];
static uint8_t max_src_id;

static struct bap_broadcast_assistant_instance *inst_by_conn(struct bt_conn *conn)
{
	struct bap_broadcast_assistant_instance *inst;

	zassert_not_null(conn, "conn is NULL");

	inst = &broadcast_assistants[bt_conn_index(conn)];

	return inst;
}

int bt_bap_broadcast_assistant_register_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	struct bt_bap_broadcast_assistant_cb *tmp;

	if (cb == NULL) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&broadcast_assistant_cbs, tmp, _node) {
		if (tmp == cb) {
			return -EALREADY;
		}
	}

	sys_slist_append(&broadcast_assistant_cbs, &cb->_node);

	return 0;
}

int bt_bap_broadcast_assistant_unregister_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&broadcast_assistant_cbs, &cb->_node)) {
		return -EALREADY;
	}

	return 0;
}

int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	struct bap_broadcast_assistant_instance *inst;
	struct bt_bap_scan_delegator_recv_state state;
	struct bt_bap_broadcast_assistant_cb *listener, *next;

	/* Note that proper parameter checking is done in the caller */
	zassert_not_null(conn, "conn is NULL");
	zassert_not_null(param, "param is NULL");

	inst = inst_by_conn(conn);
	zassert_not_null(inst, "inst is NULL");

	max_src_id++;
	inst->recv_states.src_id = max_src_id;
	inst->recv_states.past_avail = false;
	inst->recv_states.adv_sid = param->adv_sid;
	inst->recv_states.broadcast_id = param->broadcast_id;
	inst->pa_sync_state = param->pa_sync;
	inst->num_subgroups = param->num_subgroups;
	state.pa_sync_state = param->pa_sync;
	state.src_id = max_src_id;
	state.num_subgroups = param->num_subgroups;
	for (size_t i = 0; i < param->num_subgroups; i++) {
		state.subgroups[i].bis_sync = param->subgroups[i].bis_sync;
		inst->subgroups[i].bis_sync = param->subgroups[i].bis_sync;
	}

	bt_addr_le_copy(&inst->recv_states.addr, &param->addr);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs, listener, next, _node) {
		if (listener->add_src != NULL) {
			listener->add_src(conn, 0);
		}
		if (listener->recv_state != NULL) {
			listener->recv_state(conn, 0, &state);
		}
	}

	return 0;
}

int bt_bap_broadcast_assistant_mod_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_mod_src_param *param)
{
	struct bap_broadcast_assistant_instance *inst;
	struct bt_bap_scan_delegator_recv_state state;
	struct bt_bap_broadcast_assistant_cb *listener, *next;

	zassert_not_null(conn, "conn is NULL");
	zassert_not_null(param, "param is NULL");

	inst = inst_by_conn(conn);
	zassert_not_null(inst, "inst is NULL");

	state.pa_sync_state = param->pa_sync ? BT_BAP_PA_STATE_SYNCED : BT_BAP_PA_STATE_NOT_SYNCED;
	state.src_id = param->src_id;
	inst->recv_states.src_id = param->src_id;
	inst->pa_sync_state = param->pa_sync;

	state.num_subgroups = param->num_subgroups;
	inst->num_subgroups = param->num_subgroups;
	for (size_t i = 0; i < param->num_subgroups; i++) {
		state.subgroups[i].bis_sync = param->subgroups[i].bis_sync;
		inst->subgroups[i].bis_sync = param->subgroups[i].bis_sync;
	}
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs, listener, next, _node) {
			if (listener->mod_src != NULL) {
				listener->mod_src(conn, 0);
			}
			if (listener->recv_state != NULL) {
				listener->recv_state(conn, 0, &state);
			}
		}

	return 0;
}

int bt_bap_broadcast_assistant_rem_src(struct bt_conn *conn, uint8_t src_id)
{
	struct bap_broadcast_assistant_instance *inst;
	struct bt_bap_broadcast_assistant_cb *listener, *next;

	zassert_not_null(conn, "conn is NULL");

	inst = inst_by_conn(conn);
	zassert_equal(src_id, inst->recv_states.src_id, "Invalid src_id");
	zassert_equal(BT_BAP_PA_STATE_NOT_SYNCED, inst->pa_sync_state, "Invalid sync state");
	for (int i = 0; i < inst->num_subgroups; i++) {
		zassert_equal(0, inst->subgroups[i].bis_sync);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs, listener, next, _node) {
		if (listener->rem_src != NULL) {
			listener->rem_src(conn, 0);
		}
	}

	return 0;
}

int bt_bap_broadcast_assistant_set_broadcast_code(
	struct bt_conn *conn, uint8_t src_id,
	const uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	struct bt_bap_broadcast_assistant_cb *listener, *next;
	int err;

	zassert_not_null(conn, "conn is NULL");

	zassert_equal(src_id, RANDOM_SRC_ID, "Invalid src_id");

	err = strncmp((const char *)broadcast_code, BROADCAST_CODE, sizeof(BROADCAST_CODE));
	zassert_equal(0, err, "Unexpected broadcast code");

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs, listener, next, _node) {
		if (listener->broadcast_code != NULL) {
			listener->broadcast_code(conn, 0);
		}
	}

	return 0;
}
