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
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_assert.h>

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
};

static struct bap_broadcast_assistant_instance broadcast_assistants[CONFIG_BT_MAX_CONN];

static void bap_broadcast_source_started_cb(struct bt_bap_broadcast_source *source)
{
	ARRAY_FOR_EACH_PTR(broadcast_assistants, assistant) {
		bool receive_state_callback = true;

		if (assistant->conn == NULL) {
			continue;
		}

		assistant->recv_state.adv_sid = TEST_COMMON_ADV_SID;
		assistant->recv_state.addr.type = TEST_COMMON_ADV_TYPE;
		assistant->recv_state.broadcast_id = TEST_COMMON_BROADCAST_ID;

		/* If we have called the recv_state callback as part of add, mod or rem source
		 * operations, we should not call it here again
		 */
		for (uint8_t i = 0U; i < assistant->num_subgroups; i++) {
			if (assistant->recv_state.subgroups[i].bis_sync != 0) {
				receive_state_callback = false;
				break;
			}
			/* Set the BIS sync to any valid value */
			assistant->recv_state.subgroups[i].bis_sync = BT_ISO_BIS_INDEX_BIT(i);
		}

		if (receive_state_callback && broadcast_assistant_cb->recv_state != NULL) {
			broadcast_assistant_cb->recv_state(assistant->conn, 0,
							   &assistant->recv_state);
		}
	}
}

static void bap_broadcast_source_stopped_cb(struct bt_bap_broadcast_source *source, uint8_t reason)
{
	ARRAY_FOR_EACH_PTR(broadcast_assistants, assistant) {
		bool receive_state_callback = true;

		if (assistant->conn == NULL) {
			continue;
		}

		assistant->recv_state.adv_sid = TEST_COMMON_ADV_SID;
		assistant->recv_state.addr.type = TEST_COMMON_ADV_TYPE;
		assistant->recv_state.broadcast_id = TEST_COMMON_BROADCAST_ID;

		/* If we have called the recv_state callback as part of add, mod or rem source
		 * operations, we should not call it here again
		 */
		for (uint8_t i = 0U; i < assistant->num_subgroups; i++) {
			if (assistant->recv_state.subgroups[i].bis_sync == 0) {
				receive_state_callback = false;
				break;
			}
		}

		if (receive_state_callback && broadcast_assistant_cb->recv_state != NULL) {
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
	struct bap_broadcast_assistant_instance *assistant;

	__ASSERT(conn != NULL, "conn is NULL");

	assistant = &broadcast_assistants[bt_conn_index(conn)];

	return assistant;
}

int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	struct bap_broadcast_assistant_instance *assistant;

	/* Note that proper parameter checking is done in the caller */
	__ASSERT(conn != NULL, "conn is NULL");
	__ASSERT(param != NULL, "param is NULL");

	assistant = inst_by_conn(conn);
	__ASSERT(assistant != NULL, "assistant is NULL");

	assistant->recv_state.src_id = TEST_COMMON_SRC_ID;
	assistant->past_avail = false;
	assistant->recv_state.adv_sid = param->adv_sid;
	assistant->recv_state.broadcast_id = param->broadcast_id;
	assistant->pa_sync_state = param->pa_sync;
	assistant->num_subgroups = param->num_subgroups;
	assistant->recv_state.addr.type = TEST_COMMON_ADV_TYPE;
	for (size_t i = 0; i < param->num_subgroups; i++) {
		assistant->recv_state.subgroups[i].bis_sync = param->subgroups[i].bis_sync;
	}

	bt_addr_le_copy(&assistant->recv_state.addr, &param->addr);

	if (broadcast_assistant_cb != NULL) {
		if (broadcast_assistant_cb->add_src != NULL) {
			broadcast_assistant_cb->add_src(conn, 0);
		}
		if (broadcast_assistant_cb->recv_state != NULL) {
			broadcast_assistant_cb->recv_state(conn, 0, &assistant->recv_state);
		}
	}

	return 0;
}

int bt_bap_broadcast_assistant_mod_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_mod_src_param *param)
{
	struct bap_broadcast_assistant_instance *assistant;

	zassert_not_null(conn, "conn is NULL");
	zassert_not_null(param, "param is NULL");

	assistant = inst_by_conn(conn);
	zassert_not_null(assistant, "assistant is NULL");

	assistant->recv_state.src_id = param->src_id;
	assistant->pa_sync_state =
		param->pa_sync ? BT_BAP_PA_STATE_SYNCED : BT_BAP_PA_STATE_NOT_SYNCED;
	assistant->recv_state.adv_sid = TEST_COMMON_ADV_SID;
	assistant->recv_state.addr.type = TEST_COMMON_ADV_TYPE;
	assistant->recv_state.broadcast_id = TEST_COMMON_BROADCAST_ID;

	assistant->num_subgroups = param->num_subgroups;
	for (uint8_t i = 0U; i < param->num_subgroups; i++) {
		assistant->recv_state.subgroups[i].bis_sync = param->subgroups[i].bis_sync;
	}

	if (broadcast_assistant_cb->mod_src != NULL) {
		broadcast_assistant_cb->mod_src(conn, 0);
	}

	if (broadcast_assistant_cb->recv_state != NULL) {
		broadcast_assistant_cb->recv_state(conn, 0, &assistant->recv_state);
	}

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
	struct bap_broadcast_assistant_instance *assistant;

	zassert_not_null(conn, "conn is NULL");

	assistant = inst_by_conn(conn);
	zassert_not_null(assistant, "assistant is NULL");
	zassert_equal(src_id, assistant->recv_state.src_id, "Invalid src_id");
	zassert_equal(BT_BAP_PA_STATE_NOT_SYNCED, assistant->pa_sync_state, "Invalid sync state");
	for (uint8_t i = 0U; i < assistant->num_subgroups; i++) {
		zassert_equal(0U, assistant->recv_state.subgroups[i].bis_sync);
	}

	if (broadcast_assistant_cb->rem_src != NULL) {
		broadcast_assistant_cb->rem_src(conn, 0);
	}

	if (broadcast_assistant_cb->recv_state != NULL) {
		broadcast_assistant_cb->recv_state(conn, 0, NULL);
	}

	return 0;
}

int bt_bap_broadcast_assistant_discover(struct bt_conn *conn)
{
	struct bap_broadcast_assistant_instance *assistant;

	zassert_not_null(conn, "conn is NULL");

	assistant = inst_by_conn(conn);
	zassert_not_null(assistant, "assistant is NULL");
	assistant->conn = conn;

	return 0;
}
