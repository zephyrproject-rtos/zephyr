/*  Bluetooth TBS - Telephone Bearer Service - Client
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/types.h>
#include <sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/tbs.h>

#include "tbs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_TBS_CLIENT)
#define LOG_MODULE_NAME bt_tbs_client
#include "common/log.h"

#define MAX_URI_SCHEME_LIST_SIZE   64
#if IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS)
#define BT_TBS_INSTANCE_MAX_CNT    (CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES + 1)
#else
#define BT_TBS_INSTANCE_MAX_CNT    CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES
#endif /* IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) */
#define GTBS_INDEX                 CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES

struct bt_tbs_server_inst {
	struct bt_tbs_instance tbs_insts[BT_TBS_INSTANCE_MAX_CNT];
	struct bt_gatt_discover_params discover_params;
	struct bt_tbs_instance *current_inst;
	uint8_t inst_cnt;
	bool gtbs_found;
	bool subscribe_all;
};

static const struct bt_tbs_client_cb *tbs_client_cbs;

static struct bt_tbs_server_inst srv_insts[CONFIG_BT_MAX_CONN];
static const struct bt_uuid *tbs_uuid = BT_UUID_TBS;
static const struct bt_uuid *gtbs_uuid = BT_UUID_GTBS;

static void discover_next_instance(struct bt_conn *conn, uint8_t index);

static bool valid_inst_index(struct bt_conn *conn, uint8_t idx)
{
	uint8_t conn_index;

	__ASSERT(conn, "NULL conn");

	conn_index = bt_conn_index(conn);

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && idx == BT_TBS_GTBS_INDEX) {
		return true;
	} else {
		return idx < srv_insts[conn_index].inst_cnt;
	}
}

static struct bt_tbs_instance *get_inst_by_index(struct bt_conn *conn,
						 uint8_t idx)
{
	uint8_t conn_index;

	__ASSERT(conn, "NULL conn");

	conn_index = bt_conn_index(conn);

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && idx == BT_TBS_GTBS_INDEX) {
		return &srv_insts[conn_index].tbs_insts[GTBS_INDEX];
	} else {
		return &srv_insts[conn_index].tbs_insts[idx];
	}
}

#if defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL)
static bool free_call_spot(struct bt_tbs_instance *inst)
{
	for (int i = 0; i < CONFIG_BT_TBS_CLIENT_MAX_CALLS; i++) {
		if (inst->calls[i].index == BT_TBS_FREE_CALL_INDEX) {
			return true;
		}
	}

	return false;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL) */

static struct bt_tbs_instance *lookup_inst_by_handle(struct bt_conn *conn,
							 uint16_t handle)
{
	uint8_t conn_index;
	struct bt_tbs_server_inst *srv_inst;

	__ASSERT(conn, "NULL conn");

	conn_index = bt_conn_index(conn);
	srv_inst = &srv_insts[conn_index];

	for (size_t i = 0; i < ARRAY_SIZE(srv_inst->tbs_insts); i++) {
		if (srv_inst->tbs_insts[i].start_handle <= handle &&
		    srv_inst->tbs_insts[i].end_handle >= handle) {
			return &srv_inst->tbs_insts[i];
		}
	}
	BT_DBG("Could not find instance with handle 0x%04x", handle);

	return NULL;
}

static uint8_t net_buf_pull_call_state(struct net_buf_simple *buf,
				       struct bt_tbs_client_call_state *call_state)
{
	if (buf->len < sizeof(*call_state)) {
		BT_DBG("Invalid buffer length %u", buf->len);
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	call_state->index = net_buf_simple_pull_u8(buf);
	call_state->state = net_buf_simple_pull_u8(buf);
	call_state->flags = net_buf_simple_pull_u8(buf);

	return 0;
}

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
static uint8_t net_buf_pull_call(struct net_buf_simple *buf,
				 struct bt_tbs_client_call *call)
{
	const size_t min_item_len = sizeof(call->call_info) + BT_TBS_MIN_URI_LEN;
	uint8_t item_len;
	uint8_t uri_len;
	uint8_t err;
	uint8_t *uri;

	__ASSERT(buf, "NULL buf");
	__ASSERT(call, "NULL call");

	if (buf->len < sizeof(item_len) + min_item_len) {
		BT_DBG("Invalid buffer length %u", buf->len);
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	item_len = net_buf_simple_pull_u8(buf);
	uri_len = item_len - sizeof(call->call_info);

	if (item_len > buf->len || item_len < min_item_len) {
		BT_DBG("Invalid current call item length %u", item_len);
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	err = net_buf_pull_call_state(buf, &call->call_info);
	if (err != 0) {
		return err;
	}

	uri = net_buf_simple_pull_mem(buf, uri_len);
	if (uri_len > sizeof(call->remote_uri) - 1) {
		BT_WARN("Current call (index %u) uri length larger than supported %u/%zu",
			call->call_info.index, uri_len,
			sizeof(call->remote_uri) - 1);
		return BT_ATT_ERR_INSUFFICIENT_RESOURCES;
	}

	(void)memcpy(call->remote_uri, uri, uri_len);
	call->remote_uri[uri_len] = '\0';

	return 0;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
static void call_cp_callback_handler(struct bt_conn *conn, int err,
				     uint8_t index, uint8_t opcode,
				     uint8_t call_index)
{
	bt_tbs_client_cp_cb cp_cb = NULL;

	BT_DBG("Status: %s for the %s opcode for call 0x%02x",
	       bt_tbs_status_str(err), bt_tbs_opcode_str(opcode), call_index);

	if (tbs_client_cbs == NULL) {
		return;
	}

	switch (opcode) {
#if defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL)
	case BT_TBS_CALL_OPCODE_ACCEPT:
		cp_cb = tbs_client_cbs->accept_call;
		break;
#endif /* defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL)
	case BT_TBS_CALL_OPCODE_TERMINATE:
		cp_cb = tbs_client_cbs->terminate_call;
		break;
#endif /* defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL)
	case BT_TBS_CALL_OPCODE_HOLD:
		cp_cb = tbs_client_cbs->hold_call;
		break;
#endif /* defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL)
	case BT_TBS_CALL_OPCODE_RETRIEVE:
		cp_cb = tbs_client_cbs->retrieve_call;
		break;
#endif /* defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL)
	case BT_TBS_CALL_OPCODE_ORIGINATE:
		cp_cb = tbs_client_cbs->originate_call;
		break;
#endif /* defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS)
	case BT_TBS_CALL_OPCODE_JOIN:
		cp_cb = tbs_client_cbs->join_calls;
		break;
#endif /* defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS) */
	default:
		break;
	}

	if (cp_cb != 0) {
		cp_cb(conn, err, index, call_index);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */

const char *parse_string_value(const void *data, uint16_t length,
				      uint16_t max_len)
{
	static char string_val[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
	const size_t len = MIN(length, max_len);

	if (len != 0) {
		(void)memcpy(string_val, data, len);
	}

	string_val[len] = '\0';

	return string_val;
}

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
static void provider_name_notify_handler(struct bt_conn *conn,
					 const struct bt_tbs_instance *tbs_inst,
					 const void *data, uint16_t length)
{
	const char *name = parse_string_value(data, length,
					      CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);

	BT_DBG("%s", name);

	if (tbs_client_cbs != NULL && tbs_client_cbs->bearer_provider_name != NULL) {
		tbs_client_cbs->bearer_provider_name(conn, 0, tbs_inst->index, name);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
static void technology_notify_handler(struct bt_conn *conn,
				      const struct bt_tbs_instance *tbs_inst,
				      const void *data, uint16_t length)
{
	uint8_t technology;

	BT_DBG("");

	if (length == sizeof(technology)) {
		(void)memcpy(&technology, data, length);
		BT_DBG("%s (0x%02x)",
		       bt_tbs_technology_str(technology), technology);

		if (tbs_client_cbs != NULL && tbs_client_cbs->technology != NULL) {
			tbs_client_cbs->technology(conn, 0, tbs_inst->index,
					    technology);
		}
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
static void signal_strength_notify_handler(struct bt_conn *conn,
					   const struct bt_tbs_instance *tbs_inst,
					   const void *data, uint16_t length)
{
	uint8_t signal_strength;

	BT_DBG("");

	if (length == sizeof(signal_strength)) {
		(void)memcpy(&signal_strength, data, length);
		BT_DBG("0x%02x", signal_strength);

		if (tbs_client_cbs != NULL && tbs_client_cbs->signal_strength != NULL) {
			tbs_client_cbs->signal_strength(conn, 0, tbs_inst->index,
						 signal_strength);
		}
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
static void current_calls_notify_handler(struct bt_conn *conn,
					 const struct bt_tbs_instance *tbs_inst,
					 const void *data, uint16_t length)
{
	struct bt_tbs_client_call calls[CONFIG_BT_TBS_CLIENT_MAX_CALLS];
	uint8_t cnt = 0;
	struct net_buf_simple buf;

	BT_DBG("");

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	/* TODO: If length == MTU, do long read for all calls */

	while (buf.len) {
		struct bt_tbs_client_call *call = &calls[cnt];
		int err;

		err = net_buf_pull_call(&buf, call);
		if (err == BT_ATT_ERR_INSUFFICIENT_RESOURCES) {
			BT_WARN("Call with skipped due to too long URI");
			continue;
		} else if (err != 0) {
			BT_DBG("Invalid current call notification: %d", err);
			return;
		}

		cnt++;
		if (cnt == CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
			BT_WARN("Could not parse all calls due to memory restrictions");
			break;
		}
	}

	if (tbs_client_cbs != NULL && tbs_client_cbs->current_calls != NULL) {
		tbs_client_cbs->current_calls(conn, 0, tbs_inst->index, cnt, calls);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
static void status_flags_notify_handler(struct bt_conn *conn,
					const struct bt_tbs_instance *tbs_inst,
					const void *data, uint16_t length)
{
	uint16_t status_flags;

	BT_DBG("");

	if (length == sizeof(status_flags)) {
		(void)memcpy(&status_flags, data, length);
		BT_DBG("0x%04x", status_flags);
		if (tbs_client_cbs != NULL && tbs_client_cbs->status_flags != NULL) {
			tbs_client_cbs->status_flags(conn, 0, tbs_inst->index,
					      status_flags);
		}
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
static void incoming_uri_notify_handler(struct bt_conn *conn,
					const struct bt_tbs_instance *tbs_inst,
					const void *data, uint16_t length)
{
	const char *uri = parse_string_value(data, length,
					     CONFIG_BT_TBS_MAX_URI_LENGTH);

	BT_DBG("%s", uri);

	if (tbs_client_cbs != NULL && tbs_client_cbs->call_uri != NULL) {
		tbs_client_cbs->call_uri(conn, 0, tbs_inst->index, uri);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */

static void call_state_notify_handler(struct bt_conn *conn,
				      const struct bt_tbs_instance *tbs_inst,
				      const void *data, uint16_t length)
{
	struct bt_tbs_client_call_state call_states[CONFIG_BT_TBS_CLIENT_MAX_CALLS];
	uint8_t cnt = 0;
	struct net_buf_simple buf;

	BT_DBG("");

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	/* TODO: If length == MTU, do long read for all call states */

	while (buf.len) {
		struct bt_tbs_client_call_state *call_state = &call_states[cnt];
		int err;

		err = net_buf_pull_call_state(&buf, call_state);
		if (err != 0) {
			BT_DBG("Invalid current call notification: %d", err);
			return;
		}

		cnt++;
		if (cnt == CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
			BT_WARN("Could not parse all calls due to memory restrictions");
			break;
		}
	}

	if (tbs_client_cbs != NULL && tbs_client_cbs->call_state != NULL) {
		tbs_client_cbs->call_state(conn, 0, tbs_inst->index, cnt, call_states);
	}
}

#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
static void call_cp_notify_handler(struct bt_conn *conn,
				  const struct bt_tbs_instance *tbs_inst,
				  const void *data, uint16_t length)
{
	struct bt_tbs_call_cp_notify *ind_val;

	BT_DBG("");

	if (length == sizeof(*ind_val)) {
		ind_val = (struct bt_tbs_call_cp_notify *)data;
		BT_DBG("Status: %s for the %s opcode for call 0x%02X",
		       bt_tbs_status_str(ind_val->status),
		       bt_tbs_opcode_str(ind_val->opcode),
		       ind_val->call_index);

		call_cp_callback_handler(conn, ind_val->status, tbs_inst->index,
					 ind_val->opcode, ind_val->call_index);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES) */

static void termination_reason_notify_handler(struct bt_conn *conn,
					      const struct bt_tbs_instance *tbs_inst,
					      const void *data, uint16_t length)
{
	struct bt_tbs_terminate_reason reason;

	BT_DBG("");

	if (length == sizeof(reason)) {
		(void)memcpy(&reason, data, length);
		BT_DBG("ID 0x%02X, reason %s",
		       reason.call_index,
		       bt_tbs_term_reason_str(reason.reason));

		if (tbs_client_cbs != NULL && tbs_client_cbs->termination_reason != NULL) {
			tbs_client_cbs->termination_reason(conn, 0, tbs_inst->index,
						    reason.call_index,
						    reason.reason);
		}
	}
}

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
static void in_call_notify_handler(struct bt_conn *conn,
				   const struct bt_tbs_instance *tbs_inst,
				   const void *data, uint16_t length)
{
	const char *uri = parse_string_value(data, length,
					     CONFIG_BT_TBS_MAX_URI_LENGTH);

	BT_DBG("%s", uri);

	if (tbs_client_cbs != NULL && tbs_client_cbs->remote_uri != NULL) {
		tbs_client_cbs->remote_uri(conn, 0, tbs_inst->index, uri);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
static void friendly_name_notify_handler(struct bt_conn *conn,
					 const struct bt_tbs_instance *tbs_inst,
					 const void *data, uint16_t length)
{
	const char *name = parse_string_value(data, length,
					      CONFIG_BT_TBS_MAX_URI_LENGTH);

	BT_DBG("%s", name);

	if (tbs_client_cbs != NULL && tbs_client_cbs->friendly_name != NULL) {
		tbs_client_cbs->friendly_name(conn, 0, tbs_inst->index, name);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */

/** @brief Handles notifications and indications from the server */
static uint8_t notify_handler(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_tbs_instance *tbs_inst = lookup_inst_by_handle(conn, handle);

	if (data == NULL) {
		BT_DBG("[UNSUBSCRIBED] 0x%04X", params->value_handle);
		params->value_handle = 0U;
		if (tbs_inst != NULL) {
			tbs_inst->subscribe_cnt--;
		}

		return BT_GATT_ITER_STOP;
	}

	if (tbs_inst != NULL) {
		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && tbs_inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", tbs_inst->index);
		}

		BT_HEXDUMP_DBG(data, length, "notify handler value");

		if (handle == tbs_inst->call_state_sub_params.value_handle) {
			call_state_notify_handler(conn, tbs_inst, data, length);
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
		} else if (handle == tbs_inst->name_sub_params.value_handle) {
			provider_name_notify_handler(conn, tbs_inst, data,
						     length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
		} else if (handle == tbs_inst->technology_sub_params.value_handle) {
			technology_notify_handler(conn, tbs_inst, data, length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
		} else if (handle == tbs_inst->signal_strength_sub_params.value_handle) {
			signal_strength_notify_handler(conn, tbs_inst, data,
						       length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */
#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
		} else if (handle == tbs_inst->status_flags_sub_params.value_handle) {
			status_flags_notify_handler(conn, tbs_inst, data,
						    length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
		} else if (handle == tbs_inst->current_calls_sub_params.value_handle) {
			current_calls_notify_handler(conn, tbs_inst, data,
						     length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
		} else if (handle == tbs_inst->in_target_uri_sub_params.value_handle) {
			incoming_uri_notify_handler(conn, tbs_inst, data,
						    length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */
#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
		} else if (handle == tbs_inst->call_cp_sub_params.value_handle) {
			call_cp_notify_handler(conn, tbs_inst, data, length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES) */
		} else if (handle == tbs_inst->termination_reason_handle) {
			termination_reason_notify_handler(conn, tbs_inst, data,
							  length);
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
		} else if (handle == tbs_inst->incoming_call_sub_params.value_handle) {
			in_call_notify_handler(conn, tbs_inst, data, length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
		} else if (handle == tbs_inst->friendly_name_sub_params.value_handle) {
			friendly_name_notify_handler(conn, tbs_inst, data,
						     length);
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */
		}
	} else {
		BT_DBG("Notification/Indication on unknown TBS inst");
	}

	return BT_GATT_ITER_CONTINUE;
}

#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
static int tbs_client_common_call_control(struct bt_conn *conn,
					  uint8_t inst_index,
					  uint8_t call_index,
					  uint8_t opcode)
{
	const struct bt_tbs_instance *inst = get_inst_by_index(conn, inst_index);
	struct bt_tbs_call_cp_acc common;

	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->call_cp_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	common.opcode = opcode;
	common.call_index = call_index;

	return bt_gatt_write_without_response(conn, inst->call_cp_sub_params.value_handle,
					      &common, sizeof(common), false);
}
#endif /* CONFIG_BT_TBS_CLIENT_CP_PROCEDURES */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
static uint8_t read_bearer_provider_name_cb(struct bt_conn *conn, uint8_t err,
					    struct bt_gatt_read_params *params,
					    const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		const char *provider_name = NULL;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			provider_name =
				parse_string_value(data, length,
					CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);
			BT_DBG("%s", provider_name);
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL && tbs_client_cbs->bearer_provider_name != NULL) {
			tbs_client_cbs->bearer_provider_name(conn, err, inst->index,
						      provider_name);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
static uint8_t read_bearer_uci_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		const char *bearer_uci = NULL;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			bearer_uci = parse_string_value(data, length,
							BT_TBS_MAX_UCI_SIZE);
			BT_DBG("%s", bearer_uci);
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL && tbs_client_cbs->bearer_uci != NULL) {
			tbs_client_cbs->bearer_uci(conn, err, inst->index, bearer_uci);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
static uint8_t read_technology_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		uint8_t cb_err = err;
		uint8_t technology = 0;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(technology)) {
				(void)memcpy(&technology, data, length);
				BT_DBG("%s (0x%02x)",
				       bt_tbs_technology_str(technology),
				       technology);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL && tbs_client_cbs->technology != NULL) {
			tbs_client_cbs->technology(conn, cb_err, inst->index,
					    technology);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
static uint8_t read_uri_list_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		const char *uri_scheme_list = NULL;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			uri_scheme_list = parse_string_value(data, length,
						MAX_URI_SCHEME_LIST_SIZE);
			BT_DBG("%s", uri_scheme_list);
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL && tbs_client_cbs->uri_list != NULL) {
			tbs_client_cbs->uri_list(conn, err, inst->index,
					  uri_scheme_list);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
static uint8_t read_signal_strength_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		uint8_t cb_err = err;
		uint8_t signal_strength = 0;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(signal_strength)) {
				(void)memcpy(&signal_strength, data, length);
				BT_DBG("0x%02x", signal_strength);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL && tbs_client_cbs->signal_strength != NULL) {
			tbs_client_cbs->signal_strength(conn, cb_err, inst->index,
						 signal_strength);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */

#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL)
static uint8_t read_signal_interval_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		uint8_t cb_err = err;
		uint8_t signal_interval = 0;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(signal_interval)) {
				(void)memcpy(&signal_interval, data, length);
				BT_DBG("0x%02x", signal_interval);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}

		inst->busy = false;

		if (tbs_client_cbs && tbs_client_cbs->signal_interval) {
			tbs_client_cbs->signal_interval(conn, cb_err, inst->index,
						 signal_interval);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
static uint8_t read_current_calls_cb(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_read_params *params,
				      const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);
	int tbs_err = err;
	struct bt_tbs_client_call calls[CONFIG_BT_TBS_CLIENT_MAX_CALLS];
	uint8_t cnt = 0;

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
		BT_DBG("GTBS");
	} else {
		BT_DBG("Index %u", inst->index);
	}

	if (tbs_err != 0) {
		BT_DBG("err: %d", tbs_err);
		(void)memset(params, 0, sizeof(*params));
		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->current_calls != NULL) {
			tbs_client_cbs->current_calls(conn, tbs_err,
						      inst->index, 0, NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		BT_DBG("Current calls read (offset %u): %s",
		       params->single.offset, bt_hex(data, length));

		if (inst->net_buf.size < inst->net_buf.len + length) {
			BT_DBG("Could not read all data, aborting");
			(void)memset(params, 0, sizeof(*params));

			if (tbs_client_cbs != NULL &&
			    tbs_client_cbs->current_calls != NULL) {
				tbs_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
				tbs_client_cbs->current_calls(conn, err,
							      inst->index,
							      0, NULL);
			}

			return BT_GATT_ITER_STOP;
		}

		net_buf_simple_add_mem(&inst->net_buf, data, length);

		/* Returning continue will try to read more using read long procedure */
		return BT_GATT_ITER_CONTINUE;
	}

	if (inst->net_buf.len == 0) {
		(void)memset(params, 0, sizeof(*params));
		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->current_calls != NULL) {
			tbs_client_cbs->current_calls(conn, 0, inst->index, 0,
						      NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	/* Finished reading, start parsing */
	while (inst->net_buf.len != 0) {
		struct bt_tbs_client_call *call = &calls[cnt];

		tbs_err = net_buf_pull_call(&inst->net_buf, call);
		if (tbs_err == BT_ATT_ERR_INSUFFICIENT_RESOURCES) {
			BT_WARN("Call skipped due to too long URI");
			continue;
		} else if (tbs_err != 0) {
			BT_DBG("Invalid current call read: %d", err);
			break;
		}

		cnt++;
		if (cnt == CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
			BT_WARN("Could not parse all calls due to memory restrictions");
			break;
		}
	}

	(void)memset(params, 0, sizeof(*params));

	if (tbs_client_cbs != NULL && tbs_client_cbs->current_calls != NULL) {
		tbs_client_cbs->current_calls(conn, tbs_err, inst->index, cnt,
					      calls);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
static uint8_t read_ccid_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_read_params *params,
			    const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		uint8_t cb_err = err;
		uint8_t ccid = 0;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(ccid)) {
				(void)memcpy(&ccid, data, length);
				BT_DBG("0x%02x", ccid);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL && tbs_client_cbs->ccid != NULL) {
			tbs_client_cbs->ccid(conn, cb_err, inst->index, ccid);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
static uint8_t read_status_flags_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		uint8_t cb_err = err;
		uint16_t status_flags = 0;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(status_flags)) {
				(void)memcpy(&status_flags, data, length);
				BT_DBG("0x%04x", status_flags);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->status_flags != NULL) {
			tbs_client_cbs->status_flags(conn, cb_err, inst->index,
						     status_flags);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
static uint8_t read_call_uri_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_read_params *params,
				const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		const char *in_target_uri = NULL;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			in_target_uri = parse_string_value(
						data, length,
						CONFIG_BT_TBS_MAX_URI_LENGTH);
			BT_DBG("%s", in_target_uri);
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL && tbs_client_cbs->call_uri != NULL) {
			tbs_client_cbs->call_uri(conn, err, inst->index,
						 in_target_uri);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */

static uint8_t read_call_state_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);
	uint8_t cnt = 0;
	struct bt_tbs_client_call_state call_states[CONFIG_BT_TBS_CLIENT_MAX_CALLS];
	int tbs_err = err;

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
		BT_DBG("GTBS");
	} else {
		BT_DBG("Index %u", inst->index);
	}

	if (tbs_err != 0) {
		BT_DBG("err: %d", tbs_err);
		(void)memset(params, 0, sizeof(*params));
		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->call_state != NULL) {
			tbs_client_cbs->call_state(conn, tbs_err, inst->index,
						   0, NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		BT_DBG("Call states read (offset %u): %s",
		       params->single.offset, bt_hex(data, length));

		if (inst->net_buf.size < inst->net_buf.len + length) {
			BT_DBG("Could not read all data, aborting");
			(void)memset(params, 0, sizeof(*params));

			if (tbs_client_cbs != NULL &&
			    tbs_client_cbs->call_state != NULL) {
				tbs_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
				tbs_client_cbs->call_state(conn, err,
							   inst->index, 0,
							   NULL);
			}

			return BT_GATT_ITER_STOP;
		}

		net_buf_simple_add_mem(&inst->net_buf, data, length);

		/* Returning continue will try to read more using read long procedure */
		return BT_GATT_ITER_CONTINUE;
	}

	if (inst->net_buf.len == 0) {
		(void)memset(params, 0, sizeof(*params));
		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->call_state != NULL) {
			tbs_client_cbs->call_state(conn, 0, inst->index, 0,
						   NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	/* Finished reading, start parsing */
	while (inst->net_buf.len != 0) {
		struct bt_tbs_client_call_state *call_state = &call_states[cnt];

		tbs_err = net_buf_pull_call_state(&inst->net_buf, call_state);
		if (tbs_err != 0) {
			BT_DBG("Invalid current call notification: %d", err);
			break;
		}

		cnt++;
		if (cnt == CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
			BT_WARN("Could not parse all calls due to memory restrictions");
			break;
		}
	}

	(void)memset(params, 0, sizeof(*params));

	if (tbs_client_cbs != NULL && tbs_client_cbs->call_state != NULL) {
		tbs_client_cbs->call_state(conn, tbs_err, inst->index, cnt,
					   call_states);
	}

	return BT_GATT_ITER_STOP;
}

#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
static uint8_t read_optional_opcodes_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		uint8_t cb_err = err;
		uint16_t optional_opcodes = 0;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(optional_opcodes)) {
				(void)memcpy(&optional_opcodes, data, length);
				BT_DBG("0x%04x", optional_opcodes);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->optional_opcodes != NULL) {
			tbs_client_cbs->optional_opcodes(conn, cb_err,
							 inst->index,
							 optional_opcodes);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
static uint8_t read_remote_uri_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		const char *remote_uri = NULL;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			remote_uri = parse_string_value(data, length,
							CONFIG_BT_TBS_MAX_URI_LENGTH);
			BT_DBG("%s", remote_uri);
		}

		inst->busy = false;

		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->remote_uri != NULL) {
			tbs_client_cbs->remote_uri(conn, err, inst->index,
						   remote_uri);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
static uint8_t read_friendly_name_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);

	(void)memset(params, 0, sizeof(*params));

	if (inst != NULL) {
		const char *friendly_name = NULL;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err != 0) {
			BT_DBG("err: 0x%02X", err);
		} else if (data != NULL) {
			friendly_name = parse_string_value(data, length,
							   CONFIG_BT_TBS_MAX_URI_LENGTH);
			BT_DBG("%s", friendly_name);
		}
		inst->busy = false;

		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->friendly_name != NULL) {
			tbs_client_cbs->friendly_name(conn, err, inst->index,
						      friendly_name);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
static uint8_t disc_read_ccid_cb(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_read_params *params,
				 const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[bt_conn_index(conn)];
	int cb_err = err;

	(void)memset(params, 0, sizeof(*params));

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) && inst->gtbs) {
		BT_DBG("GTBS");
	} else {
		BT_DBG("Index %u", inst->index);
	}

	if (cb_err != 0) {
		BT_DBG("err: 0x%02X", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(inst->ccid)) {
			inst->ccid = ((uint8_t *)data)[0];
			BT_DBG("0x%02x", inst->ccid);
		} else {
			BT_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	inst->busy = false;

	if (cb_err != 0) {
		tbs_client_cbs->discover(conn, cb_err, 0U, false);
	} else {
		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) &&
		    inst->index == GTBS_INDEX) {
			BT_DBG("Setup complete GTBS");
		} else {
			BT_DBG("Setup complete for %u / %u TBS",
			       inst->index + 1U, srv_inst->inst_cnt);
		}

		(void)memset(params, 0, sizeof(*params));

		if (BT_TBS_INSTANCE_MAX_CNT > 1U &&
		    (((inst->index + 1U) < srv_inst->inst_cnt) ||
			(IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) &&
			 srv_inst->gtbs_found &&
			 inst->index + 1U == GTBS_INDEX))) {
			discover_next_instance(conn, inst->index + 1U);
		} else {
			srv_inst->current_inst = NULL;
			if (tbs_client_cbs != NULL &&
			    tbs_client_cbs->discover != NULL) {
				tbs_client_cbs->discover(conn, 0,
							 srv_inst->inst_cnt,
							 srv_inst->gtbs_found);
			}
		}

	}

	return BT_GATT_ITER_STOP;
}

static void tbs_client_disc_read_ccid(struct bt_conn *conn)
{
	const uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];
	struct bt_tbs_instance *inst = srv_inst->current_inst;
	int err;

	inst->read_params.func = disc_read_ccid_cb;
	inst->read_params.handle_count = 1U;
	inst->read_params.single.handle = inst->ccid_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
		srv_inst->current_inst = NULL;
		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->discover != NULL) {
			tbs_client_cbs->discover(conn, err, 0U, false);
		}
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	const uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];
	struct bt_tbs_instance *current_inst = srv_inst->current_inst;

	if (attr == NULL) {
#if defined(CONFIG_BT_TBS_CLIENT_CCID)
		/* Read the CCID as the last part of discovering a TBS instance */
		tbs_client_disc_read_ccid(conn);
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		const struct bt_gatt_chrc *chrc;
		struct bt_gatt_subscribe_params *sub_params = NULL;

		chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_CALL_STATE) == 0) {
			BT_DBG("Call state");
			sub_params = &current_inst->call_state_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->call_state_sub_disc_params;
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_PROVIDER_NAME) == 0) {
			BT_DBG("Provider name");
			sub_params = &current_inst->name_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->name_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_UCI) == 0) {
			BT_DBG("Bearer UCI");
			current_inst->bearer_uci_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_TECHNOLOGY) == 0) {
			BT_DBG("Technology");
			sub_params = &current_inst->technology_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->technology_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_URI_LIST) == 0) {
			BT_DBG("URI Scheme List");
			current_inst->uri_list_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_SIGNAL_STRENGTH) == 0) {
			BT_DBG("Signal strength");
			sub_params = &current_inst->signal_strength_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->signal_strength_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */
#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) \
|| defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_SIGNAL_INTERVAL) == 0) {
			BT_DBG("Signal strength reporting interval");
			current_inst->signal_interval_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */
/* || defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_LIST_CURRENT_CALLS) == 0) {
			BT_DBG("Current calls");
			sub_params = &current_inst->current_calls_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->current_calls_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_CCID)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CCID) == 0) {
			BT_DBG("CCID");
			current_inst->ccid_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_INCOMING_URI) == 0) {
			BT_DBG("Incoming target URI");
			sub_params = &current_inst->in_target_uri_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->in_target_uri_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */
#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_STATUS_FLAGS) == 0) {
			BT_DBG("Status flags");
			sub_params = &current_inst->status_flags_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->status_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */
#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_CALL_CONTROL_POINT) == 0) {
			BT_DBG("Call control point");
			sub_params = &current_inst->call_cp_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->call_cp_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES) */
#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_OPTIONAL_OPCODES) == 0) {
			BT_DBG("Supported opcodes");
			current_inst->optional_opcodes_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_TERMINATE_REASON) == 0) {
			BT_DBG("Termination reason");
			current_inst->termination_reason_handle = chrc->value_handle;
			sub_params = &current_inst->termination_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->termination_sub_disc_params;
#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_FRIENDLY_NAME) == 0) {
			BT_DBG("Incoming friendly name");
			sub_params = &current_inst->friendly_name_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->friendly_name_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_INCOMING_CALL) == 0) {
			BT_DBG("Incoming call");
			sub_params = &current_inst->incoming_call_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->incoming_call_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */
		}

		if (srv_insts[conn_index].subscribe_all && sub_params != NULL) {
			sub_params->value = 0;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				sub_params->value = BT_GATT_CCC_NOTIFY;
			} else if (chrc->properties & BT_GATT_CHRC_INDICATE) {
				sub_params->value = BT_GATT_CCC_INDICATE;
			}

			if (sub_params->value != 0) {
				int err;

				/* Setting ccc_handle = will use auto discovery feature */
				sub_params->ccc_handle = 0;
				sub_params->end_handle = current_inst->end_handle;
				sub_params->notify = notify_handler;
				err = bt_gatt_subscribe(conn, sub_params);
				if (err != 0) {
					BT_DBG("Could not subscribe to "
					       "characterstic at handle 0x%04X"
					       "(%d)",
					       sub_params->value_handle, err);
				} else {
					BT_DBG("Subscribed to characterstic at "
					       "handle 0x%04X",
					       sub_params->value_handle);
				}
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void discover_next_instance(struct bt_conn *conn, uint8_t index)
{
	int err;
	uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];

	srv_inst->current_inst = &srv_inst->tbs_insts[index];
	(void)memset(&srv_inst->discover_params, 0, sizeof(srv_inst->discover_params));
	srv_inst->discover_params.uuid = NULL;
	srv_inst->discover_params.start_handle = srv_inst->current_inst->start_handle;
	srv_inst->discover_params.end_handle = srv_inst->current_inst->end_handle;
	srv_inst->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	srv_inst->discover_params.func = discover_func;

	err = bt_gatt_discover(conn, &srv_inst->discover_params);
	if (err != 0) {
		BT_DBG("Discover failed (err %d)", err);
		srv_inst->current_inst = NULL;
		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->discover != NULL) {
			tbs_client_cbs->discover(conn, err, srv_inst->inst_cnt,
						 srv_inst->gtbs_found);
		}
	}
}

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t primary_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	const uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];

	/*
	 * TODO: Since we know the ranges of each instance, we could do
	 * discover of more than just prim_service->start and
	 * prim_service->end_handle, so that we can detect multiple instances
	 * faster
	 */

	if (attr == NULL || srv_inst->inst_cnt == BT_TBS_INSTANCE_MAX_CNT) {
		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) &&
		    bt_uuid_cmp(params->uuid, BT_UUID_GTBS) == 0) {
			int err;
			/* Didn't find GTBS; look for TBS */
			params->uuid = tbs_uuid;
			params->start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;

			err = bt_gatt_discover(conn, params);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				srv_inst->current_inst = NULL;
				if (tbs_client_cbs != NULL &&
				    tbs_client_cbs->discover != NULL) {
					tbs_client_cbs->discover(conn, err, 0,
								 false);
				}
			}

			return BT_GATT_ITER_STOP;
		}

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS)) {
			srv_inst->gtbs_found = srv_inst->tbs_insts[GTBS_INDEX].gtbs;
			BT_DBG("Discover complete, found %u instances "
			       "(GTBS%s found)",
				srv_inst->inst_cnt,
				srv_inst->gtbs_found ? "" : " not");
		} else {
			BT_DBG("Discover complete, found %u instances",
			       srv_inst->inst_cnt);
		}

		if (srv_inst->inst_cnt != 0) {
			discover_next_instance(conn, 0);
		} else if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) &&
			   srv_inst->gtbs_found) {
			discover_next_instance(conn, GTBS_INDEX);
		} else {
			srv_inst->current_inst = NULL;
			if (tbs_client_cbs != NULL &&
			    tbs_client_cbs->discover != NULL) {
				tbs_client_cbs->discover(conn, 0,
							 srv_inst->inst_cnt,
							 srv_inst->gtbs_found);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		const struct bt_gatt_service_val *prim_service;

		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		params->start_handle = attr->handle + 1;

		srv_inst->current_inst = &srv_inst->tbs_insts[srv_inst->inst_cnt];
		srv_inst->current_inst->index = srv_inst->inst_cnt;

		if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) &&
		    bt_uuid_cmp(params->uuid, BT_UUID_GTBS) == 0) {
			int err;

			/* GTBS is placed as the "last" instance */
			srv_inst->current_inst = &srv_inst->tbs_insts[GTBS_INDEX];
			srv_inst->current_inst->index = GTBS_INDEX;
			srv_inst->current_inst->gtbs = true;
			srv_inst->current_inst->start_handle = attr->handle + 1;
			srv_inst->current_inst->end_handle = prim_service->end_handle;

			params->uuid = tbs_uuid;
			params->start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;

			err = bt_gatt_discover(conn, params);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				srv_inst->current_inst = NULL;
				if (tbs_client_cbs != NULL &&
				    tbs_client_cbs->discover != NULL) {
					tbs_client_cbs->discover(conn, err, 0,
								 false);
				}
			}

			return BT_GATT_ITER_STOP;
		}

		srv_inst->current_inst->start_handle = attr->handle + 1;
		srv_inst->current_inst->end_handle = prim_service->end_handle;
		srv_inst->inst_cnt++;
	}

	return BT_GATT_ITER_CONTINUE;
}

/****************************** PUBLIC API ******************************/

#if defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL)
int bt_tbs_client_hold_call(struct bt_conn *conn, uint8_t inst_index,
			    uint8_t call_index)
{
	return tbs_client_common_call_control(conn, inst_index, call_index,
					      BT_TBS_CALL_OPCODE_HOLD);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL)
int bt_tbs_client_accept_call(struct bt_conn *conn, uint8_t inst_index,
			      uint8_t call_index)
{
	return tbs_client_common_call_control(conn, inst_index, call_index,
					      BT_TBS_CALL_OPCODE_ACCEPT);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL)
int bt_tbs_client_retrieve_call(struct bt_conn *conn, uint8_t inst_index,
				uint8_t call_index)
{
	return tbs_client_common_call_control(conn, inst_index, call_index,
					      BT_TBS_CALL_OPCODE_RETRIEVE);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL)
int bt_tbs_client_terminate_call(struct bt_conn *conn, uint8_t inst_index,
				 uint8_t call_index)
{
	return tbs_client_common_call_control(conn, inst_index, call_index,
					      BT_TBS_CALL_OPCODE_TERMINATE);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL)
int bt_tbs_client_originate_call(struct bt_conn *conn, uint8_t inst_index,
				 const char *uri)
{
	struct bt_tbs_instance *inst;
	uint8_t write_buf[CONFIG_BT_L2CAP_TX_MTU];
	struct bt_tbs_call_cp_originate *originate;
	size_t uri_len;
	const size_t max_uri_len = sizeof(write_buf) - sizeof(*originate);

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	} else if (!bt_tbs_valid_uri(uri)) {
		BT_DBG("Invalid URI: %s", uri);
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	/* Check if there are free spots */
	if (!free_call_spot(inst)) {
		BT_DBG("Cannot originate more calls");
		return -ENOMEM;
	}

	uri_len = strlen(uri);

	if (uri_len > max_uri_len) {
		BT_DBG("URI len (%zu) longer than maximum writable %zu",
		       uri_len, max_uri_len);
		return -ENOMEM;
	}

	originate = (struct bt_tbs_call_cp_originate *)write_buf;
	originate->opcode = BT_TBS_CALL_OPCODE_ORIGINATE;
	(void)memcpy(originate->uri, uri, uri_len);

	return bt_gatt_write_without_response(conn, inst->call_cp_sub_params.value_handle,
					      originate,
					      sizeof(*originate) + uri_len,
					      false);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS)
int bt_tbs_client_join_calls(struct bt_conn *conn, uint8_t inst_index,
			     const uint8_t *call_indexes, uint8_t count)
{
	if (conn == NULL) {
		return -ENOTCONN;
	}

	/* Write to call control point */
	if (call_indexes && count > 1 &&
	    count <= CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
		struct bt_tbs_instance *inst;
		struct bt_tbs_call_cp_join *join;
		uint8_t write_buf[CONFIG_BT_L2CAP_TX_MTU];
		const size_t max_call_cnt = sizeof(write_buf) - sizeof(join->opcode);

		inst = get_inst_by_index(conn, inst_index);
		if (inst == NULL) {
			return -EINVAL;
		} else if (inst->call_cp_sub_params.value_handle == 0) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		if (count > max_call_cnt) {
			BT_DBG("Call count (%u) larger than maximum writable %zu",
			       count, max_call_cnt);
			return -ENOMEM;
		}

		join = (struct bt_tbs_call_cp_join *)write_buf;

		join->opcode = BT_TBS_CALL_OPCODE_JOIN;
		(void)memcpy(join->call_indexes, call_indexes, count);

		return bt_gatt_write_without_response(conn,
						      inst->call_cp_sub_params.value_handle,
						      join,
						      sizeof(*join) + count,
						      false);
	}

	return -EINVAL;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL)
int bt_tbs_client_set_signal_strength_interval(struct bt_conn *conn,
					       uint8_t inst_index,
					       uint8_t interval)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);
	/* Populate Outgoing Remote URI */
	if (inst->signal_interval_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	return bt_gatt_write_without_response(conn,
					      inst->signal_interval_handle,
					      &interval, sizeof(interval),
					      false);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
int bt_tbs_client_read_bearer_provider_name(struct bt_conn *conn,
					    uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->name_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_bearer_provider_name_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->name_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
int bt_tbs_client_read_bearer_uci(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->bearer_uci_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_bearer_uci_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->bearer_uci_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
int bt_tbs_client_read_technology(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->technology_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_technology_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->technology_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
int bt_tbs_client_read_uri_list(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->uri_list_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_uri_list_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->uri_list_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
int bt_tbs_client_read_signal_strength(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->signal_strength_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_signal_strength_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->signal_strength_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */

#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL)
int bt_tbs_client_read_signal_interval(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->signal_interval_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_signal_interval_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->signal_interval_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
int bt_tbs_client_read_current_calls(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->current_calls_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_current_calls_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->current_calls_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
int bt_tbs_client_read_ccid(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->ccid_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_ccid_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->ccid_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
int bt_tbs_client_read_call_uri(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->in_target_uri_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_call_uri_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->in_target_uri_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */

#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
int bt_tbs_client_read_status_flags(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->status_flags_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_status_flags_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->status_flags_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */

int bt_tbs_client_read_call_state(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->call_state_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_call_state_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->call_state_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}

#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
int bt_tbs_client_read_optional_opcodes(struct bt_conn *conn,
					uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->optional_opcodes_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_optional_opcodes_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->optional_opcodes_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
int bt_tbs_client_read_remote_uri(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->incoming_call_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_remote_uri_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->incoming_call_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
int bt_tbs_client_read_friendly_name(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	} else if (!valid_inst_index(conn, inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(conn, inst_index);

	if (inst->friendly_name_sub_params.value_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	inst->read_params.func = read_friendly_name_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->friendly_name_sub_params.value_handle;
	inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */

int bt_tbs_client_discover(struct bt_conn *conn, bool subscribe)
{
	uint8_t conn_index;
	struct bt_tbs_server_inst *srv_inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	conn_index = bt_conn_index(conn);
	srv_inst = &srv_insts[conn_index];

	if (srv_inst->current_inst) {
		return -EBUSY;
	}

	(void)memset(srv_inst->tbs_insts, 0, sizeof(srv_inst->tbs_insts)); /* reset data */
	srv_inst->inst_cnt = 0;
	srv_inst->gtbs_found = false;
	/* Discover TBS on peer, setup handles and notify/indicate */
	srv_inst->subscribe_all = subscribe;
	(void)memset(&srv_inst->discover_params, 0, sizeof(srv_inst->discover_params));
	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS)) {
		BT_DBG("Discovering GTBS");
		srv_inst->discover_params.uuid = gtbs_uuid;
	} else {
		srv_inst->discover_params.uuid = tbs_uuid;
	}
	srv_inst->discover_params.func = primary_discover_func;
	srv_inst->discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	srv_inst->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	srv_inst->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_discover(conn, &srv_inst->discover_params);
}

void bt_tbs_client_register_cb(const struct bt_tbs_client_cb *cbs)
{
	tbs_client_cbs = cbs;
}

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
struct bt_tbs_instance *bt_tbs_client_get_by_ccid(const struct bt_conn *conn,
						  uint8_t ccid)
{
	struct bt_tbs_server_inst *server;

	CHECKIF(conn == NULL) {
		BT_DBG("conn was NULL");
		return NULL;
	}

	server = &srv_insts[bt_conn_index(conn)];

	for (size_t i = 0; i < ARRAY_SIZE(server->tbs_insts); i++) {
		if (server->tbs_insts[i].ccid == ccid) {
			return &server->tbs_insts[i];
		}
	}

	return NULL;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */
