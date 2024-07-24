/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/bluetooth/audio/bap.h"

static struct bt_bap_broadcast_assistant_cb *broadcast_assistant_cbs;

int bt_bap_broadcast_assistant_register_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	broadcast_assistant_cbs = cb;

	return 0;
}

int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	/* Note that proper parameter checking is done in the caller */
	zassert_not_null(conn, "conn is NULL");
	zassert_not_null(param, "param is NULL");

	if (broadcast_assistant_cbs->add_src != NULL) {
		broadcast_assistant_cbs->add_src(conn, 0);
	}

	return 0;
}

int bt_bap_broadcast_assistant_mod_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_mod_src_param *param)
{
	struct bt_bap_scan_delegator_recv_state state;

	zassert_not_null(conn, "conn is NULL");
	zassert_not_null(param, "param is NULL");

	state.pa_sync_state = param->pa_sync ? BT_BAP_PA_STATE_SYNCED : BT_BAP_PA_STATE_NOT_SYNCED;
	state.src_id = param->src_id;
	state.num_subgroups = param->num_subgroups;
	for (size_t i = 0; i < param->num_subgroups; i++) {
		state.subgroups[i].bis_sync = param->subgroups[i].bis_sync;
	}

	if (broadcast_assistant_cbs->mod_src != NULL) {
		broadcast_assistant_cbs->mod_src(conn, 0);
	}
	if (broadcast_assistant_cbs->recv_state != NULL) {
		broadcast_assistant_cbs->recv_state(conn, 0, &state);
	}

	return 0;
}

int bt_bap_broadcast_assistant_rem_src(struct bt_conn *conn, uint8_t src_id)
{
	zassert_not_null(conn, "conn is NULL");
	zassert_not_equal(src_id, 0, "src_id is 0");

	if (broadcast_assistant_cbs->rem_src != NULL) {
		broadcast_assistant_cbs->rem_src(conn, 0);
	}

	return 0;
}
