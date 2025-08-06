/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "test_common.h"

static struct bt_bap_broadcast_assistant_cb *broadcast_assistant_cb;

struct bap_broadcast_assistant_instance {
	struct bt_conn *conn;
	struct bt_bap_scan_delegator_recv_state recv_state;
	/*
	 * the following are not part of the broadcast_assistant instance, but adding them allow us
	 * to easily check pa_sync and bis_sync states
	 */
	enum bt_bap_pa_state pa_sync_state;
	bool past_avail;
	uint8_t num_subgroups;
	struct bt_bap_bass_subgroup subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];
};

static struct bap_broadcast_assistant_instance broadcast_assistants[CONFIG_BT_MAX_CONN];

static void bap_broadcast_source_started_cb(struct bt_bap_broadcast_source *source)
{
	ARRAY_FOR_EACH_PTR(broadcast_assistants, assistant) {
		/* TODO: We need to set appropriate values for sid, addr_type and broadcast ID */
		assistant->recv_state.adv_sid = TEST_COMMON_ADV_SID;
		assistant->recv_state.addr.type = TEST_COMMON_ADV_TYPE;
		assistant->recv_state.broadcast_id = TEST_COMMON_BROADCAST_ID;

		if (broadcast_assistant_cb->recv_state != NULL) {
			broadcast_assistant_cb->recv_state(assistant->conn, 0,
							   &assistant->recv_state);
		}
	}
}

static void bap_broadcast_source_stopped_cb(struct bt_bap_broadcast_source *source, uint8_t reason)
{
	ARRAY_FOR_EACH_PTR(broadcast_assistants, assistant) {
		assistant->recv_state.adv_sid = TEST_COMMON_ADV_SID;
		assistant->recv_state.addr.type = TEST_COMMON_ADV_TYPE;
		assistant->recv_state.broadcast_id = TEST_COMMON_BROADCAST_ID;

		if (broadcast_assistant_cb->recv_state != NULL) {
			broadcast_assistant_cb->recv_state(assistant->conn, 0,
							   &assistant->recv_state);
		}
	}
}

int bt_bap_broadcast_assistant_register_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	static bool broadcast_source_cbs_registered;

	if (!broadcast_source_cbs_registered) {
		static struct bt_bap_broadcast_source_cb bap_broadcast_source_cb = {
			.started = bap_broadcast_source_started_cb,
			.stopped = bap_broadcast_source_stopped_cb,
		};
		const int err = bt_bap_broadcast_source_register_cb(&bap_broadcast_source_cb);

		if (err != 0) {
			return err;
		}

		broadcast_source_cbs_registered = true;
	}

	broadcast_assistant_cb = cb;

	return 0;
}

static struct bap_broadcast_assistant_instance *inst_by_conn(struct bt_conn *conn)
{
	struct bap_broadcast_assistant_instance *inst;

	__ASSERT(conn != NULL, "conn is NULL");

	inst = &broadcast_assistants[bt_conn_index(conn)];

	return inst;
}

int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	struct bap_broadcast_assistant_instance *inst;
	struct bt_bap_scan_delegator_recv_state state;

	/* Note that proper parameter checking is done in the caller */
	__ASSERT(conn != NULL, "conn is NULL");
	__ASSERT(param != NULL, "param is NULL");

	inst = inst_by_conn(conn);
	__ASSERT(inst != NULL, "inst is NULL");

	inst->recv_state.src_id = 1U;
	inst->past_avail = false;
	inst->recv_state.adv_sid = param->adv_sid;
	inst->recv_state.broadcast_id = param->broadcast_id;
	inst->pa_sync_state = param->pa_sync;
	inst->num_subgroups = param->num_subgroups;
	state.pa_sync_state = param->pa_sync;
	state.src_id = inst->recv_state.src_id;
	state.num_subgroups = param->num_subgroups;
	for (size_t i = 0; i < param->num_subgroups; i++) {
		state.subgroups[i].bis_sync = param->subgroups[i].bis_sync;
		inst->subgroups[i].bis_sync = param->subgroups[i].bis_sync;
	}

	bt_addr_le_copy(&inst->recv_state.addr, &param->addr);

	if (broadcast_assistant_cb != NULL) {
		if (broadcast_assistant_cb->add_src != NULL) {
			broadcast_assistant_cb->add_src(conn, 0);
		}
		if (broadcast_assistant_cb->recv_state != NULL) {
			broadcast_assistant_cb->recv_state(conn, 0, &state);
		}
	}

	return 0;
}

int bt_bap_broadcast_assistant_mod_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_mod_src_param *param)
{
	return 0;
}

int bt_bap_broadcast_assistant_set_broadcast_code(
	struct bt_conn *conn, uint8_t src_id,
	const uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	return 0;
}

int bt_bap_broadcast_assistant_rem_src(struct bt_conn *conn, uint8_t src_id)
{
	return 0;
}
