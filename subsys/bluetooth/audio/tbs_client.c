/*  Bluetooth TBS - Telephone Bearer Service - Client
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/tbs.h>

#include "tbs_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_tbs_client, CONFIG_BT_TBS_CLIENT_LOG_LEVEL);
/* TODO TBS client attempts to subscribe to all characteristics at once if the MTU is large enough.
 * This requires a significant amount of buffers, and should be optimized.
 */

/* Calculate the requiered buffers for TBS Client discovery */
#define TBS_CLIENT_BUF_COUNT                                                                       \
	(1 /* Discover buffer */ + 1 /* terminate reason */ +                                      \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) +                                   \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) +                                      \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) +                                 \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) +                              \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_INCOMING_URI) +                                           \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) +                                           \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES) +                                          \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) +                                     \
	 IS_ENABLED(CONFIG_BT_TBS_CLIENT_INCOMING_CALL))

BUILD_ASSERT(CONFIG_BT_L2CAP_TX_BUF_COUNT >= TBS_CLIENT_BUF_COUNT, "Too few L2CAP buffers");

#include "common/bt_str.h"

struct bt_tbs_server_inst {
#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	struct bt_tbs_instance tbs_insts[CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES];
	uint8_t inst_cnt;
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	struct bt_tbs_instance gtbs_inst;
#endif /* IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) */
	struct bt_gatt_discover_params discover_params;
	struct bt_tbs_instance *current_inst;
};

static const struct bt_tbs_client_cb *tbs_client_cbs;

static struct bt_tbs_server_inst srv_insts[CONFIG_BT_MAX_CONN];

static void discover_next_instance(struct bt_conn *conn);

typedef  bool (*tbs_instance_find_func_t)(struct bt_tbs_instance *inst, void *user_data);

static struct bt_tbs_instance *tbs_instance_find(struct bt_tbs_server_inst *server,
						 tbs_instance_find_func_t func, void *user_data)
{
#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
		if (func(&server->gtbs_inst, user_data)) {
			return &server->gtbs_inst;
		}
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */
#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	for (size_t i = 0; i < server->inst_cnt; i++) {
		if (func(&server->tbs_insts[i], user_data)) {
			return &server->tbs_insts[i];
		}
	}
#endif /* CONFIG_BT_TBS_CLIENT_TBS */

	return NULL;
}

static struct bt_tbs_instance *tbs_inst_by_index(struct bt_conn *conn, uint8_t index)
{
	struct bt_tbs_server_inst *server;

	__ASSERT(conn, "NULL conn");

	server = &srv_insts[bt_conn_index(conn)];

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	if (index == BT_TBS_GTBS_INDEX) {
		return &server->gtbs_inst;
	}
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */
#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	if (index < server->inst_cnt) {
		return &server->tbs_insts[index];
	}
#endif /* CONFIG_BT_TBS_CLIENT_TBS */

	return NULL;
}

static uint8_t tbs_index(struct bt_conn *conn, const struct bt_tbs_instance *inst)
{
	struct bt_tbs_server_inst *server;
	ptrdiff_t index = 0;

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(inst);

	server = &srv_insts[bt_conn_index(conn)];

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	if (inst == &server->gtbs_inst) {
		return BT_TBS_GTBS_INDEX;
	}
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */
#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	index = inst - server->tbs_insts;
	__ASSERT(index >= 0 && index < ARRAY_SIZE(server->tbs_insts),
		 "Invalid bt_tbs_instance pointer");

#else
	__ASSERT_PRINT("Invalid bt_tbs_instance pointer");
#endif /* CONFIG_BT_TBS_CLIENT_TBS */

	return (uint8_t)index;
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

static bool is_instance_handle(struct bt_tbs_instance *inst, void *user_data)
{
	uint16_t handle = POINTER_TO_UINT(user_data);

	return inst->start_handle <= handle && inst->end_handle >= handle;
}

static struct bt_tbs_instance *lookup_inst_by_handle(struct bt_conn *conn,
							 uint16_t handle)
{
	uint8_t conn_index;
	struct bt_tbs_server_inst *srv_inst;
	struct bt_tbs_instance *inst;

	__ASSERT(conn, "NULL conn");

	conn_index = bt_conn_index(conn);
	srv_inst = &srv_insts[conn_index];

	inst = tbs_instance_find(srv_inst, is_instance_handle, UINT_TO_POINTER(handle));
	if (inst != NULL) {
		return inst;
	}

	LOG_DBG("Could not find instance with handle 0x%04x", handle);

	return NULL;
}

static uint8_t net_buf_pull_call_state(struct net_buf_simple *buf,
				       struct bt_tbs_client_call_state *call_state)
{
	if (buf->len < sizeof(*call_state)) {
		LOG_DBG("Invalid buffer length %u", buf->len);
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
		LOG_DBG("Invalid buffer length %u", buf->len);
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	item_len = net_buf_simple_pull_u8(buf);
	uri_len = item_len - sizeof(call->call_info);

	if (item_len > buf->len || item_len < min_item_len) {
		LOG_DBG("Invalid current call item length %u", item_len);
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	err = net_buf_pull_call_state(buf, &call->call_info);
	if (err != 0) {
		return err;
	}

	uri = net_buf_simple_pull_mem(buf, uri_len);
	if (uri_len > CONFIG_BT_TBS_MAX_URI_LENGTH) {
		LOG_WRN("Current call (index %u) uri length larger than supported %u/%zu",
			call->call_info.index, uri_len, CONFIG_BT_TBS_MAX_URI_LENGTH);
		return BT_ATT_ERR_INSUFFICIENT_RESOURCES;
	}

	(void)memcpy(call->remote_uri, uri, uri_len);
	call->remote_uri[uri_len] = '\0';

	return 0;
}

static void bearer_list_current_calls(struct bt_conn *conn, const struct bt_tbs_instance *inst,
				      struct net_buf_simple *buf)
{
	struct bt_tbs_client_call calls[CONFIG_BT_TBS_CLIENT_MAX_CALLS];
	char remote_uris[CONFIG_BT_TBS_CLIENT_MAX_CALLS][CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
	uint8_t cnt = 0;
	int err;

	while (buf->len) {
		struct bt_tbs_client_call *call;

		if (cnt == CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
			LOG_WRN("Could not parse all calls due to memory restrictions");
			break;
		}

		call = &calls[cnt];
		call->remote_uri = remote_uris[cnt];

		err = net_buf_pull_call(buf, call);
		if (err == BT_ATT_ERR_INSUFFICIENT_RESOURCES) {
			LOG_WRN("Call with skipped due to too long URI");
			continue;
		} else if (err != 0) {
			LOG_DBG("Invalid current call notification: %d", err);
			return;
		}

		cnt++;
	}

	if (tbs_client_cbs != NULL && tbs_client_cbs->current_calls != NULL) {
		tbs_client_cbs->current_calls(conn, 0, tbs_index(conn, inst), cnt, calls);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
static void call_cp_callback_handler(struct bt_conn *conn, int err,
				     uint8_t index, uint8_t opcode,
				     uint8_t call_index)
{
	bt_tbs_client_cp_cb cp_cb = NULL;

	LOG_DBG("Status: %s for the %s opcode for call 0x%02x", bt_tbs_status_str(err),
		bt_tbs_opcode_str(opcode), call_index);

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

	LOG_DBG("%s", name);

	if (tbs_client_cbs != NULL && tbs_client_cbs->bearer_provider_name != NULL) {
		tbs_client_cbs->bearer_provider_name(conn, 0, tbs_index(conn, tbs_inst), name);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
static void technology_notify_handler(struct bt_conn *conn,
				      const struct bt_tbs_instance *tbs_inst,
				      const void *data, uint16_t length)
{
	uint8_t technology;

	LOG_DBG("");

	if (length == sizeof(technology)) {
		(void)memcpy(&technology, data, length);
		LOG_DBG("%s (0x%02x)", bt_tbs_technology_str(technology), technology);

		if (tbs_client_cbs != NULL && tbs_client_cbs->technology != NULL) {
			tbs_client_cbs->technology(conn, 0, tbs_index(conn, tbs_inst), technology);
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

	LOG_DBG("");

	if (length == sizeof(signal_strength)) {
		(void)memcpy(&signal_strength, data, length);
		LOG_DBG("0x%02x", signal_strength);

		if (tbs_client_cbs != NULL && tbs_client_cbs->signal_strength != NULL) {
			tbs_client_cbs->signal_strength(conn, 0, tbs_index(conn, tbs_inst),
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
	struct net_buf_simple buf;

	LOG_DBG("");

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	/* TODO: If length == MTU, do long read for all calls */

	bearer_list_current_calls(conn, tbs_inst, &buf);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
static void status_flags_notify_handler(struct bt_conn *conn,
					const struct bt_tbs_instance *tbs_inst,
					const void *data, uint16_t length)
{
	uint16_t status_flags;

	LOG_DBG("");

	if (length == sizeof(status_flags)) {
		(void)memcpy(&status_flags, data, length);
		LOG_DBG("0x%04x", status_flags);
		if (tbs_client_cbs != NULL && tbs_client_cbs->status_flags != NULL) {
			tbs_client_cbs->status_flags(conn, 0, tbs_index(conn, tbs_inst),
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

	LOG_DBG("%s", uri);

	if (tbs_client_cbs != NULL && tbs_client_cbs->call_uri != NULL) {
		tbs_client_cbs->call_uri(conn, 0, tbs_index(conn, tbs_inst), uri);
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

	LOG_DBG("");

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	/* TODO: If length == MTU, do long read for all call states */

	while (buf.len) {
		struct bt_tbs_client_call_state *call_state;
		int err;

		if (cnt == CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
			LOG_WRN("Could not parse all calls due to memory restrictions");
			break;
		}

		call_state = &call_states[cnt];

		err = net_buf_pull_call_state(&buf, call_state);
		if (err != 0) {
			LOG_DBG("Invalid current call notification: %d", err);
			return;
		}

		cnt++;
	}

	if (tbs_client_cbs != NULL && tbs_client_cbs->call_state != NULL) {
		tbs_client_cbs->call_state(conn, 0, tbs_index(conn, tbs_inst), cnt, call_states);
	}
}

#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
static void call_cp_notify_handler(struct bt_conn *conn,
				  const struct bt_tbs_instance *tbs_inst,
				  const void *data, uint16_t length)
{
	struct bt_tbs_call_cp_notify *ind_val;

	LOG_DBG("");

	if (length == sizeof(*ind_val)) {
		ind_val = (struct bt_tbs_call_cp_notify *)data;
		LOG_DBG("Status: %s for the %s opcode for call 0x%02X",
			bt_tbs_status_str(ind_val->status), bt_tbs_opcode_str(ind_val->opcode),
			ind_val->call_index);

		call_cp_callback_handler(conn, ind_val->status, tbs_index(conn, tbs_inst),
					 ind_val->opcode, ind_val->call_index);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES) */

static void termination_reason_notify_handler(struct bt_conn *conn,
					      const struct bt_tbs_instance *tbs_inst,
					      const void *data, uint16_t length)
{
	struct bt_tbs_terminate_reason reason;

	LOG_DBG("");

	if (length == sizeof(reason)) {
		(void)memcpy(&reason, data, length);
		LOG_DBG("ID 0x%02X, reason %s", reason.call_index,
			bt_tbs_term_reason_str(reason.reason));

		if (tbs_client_cbs != NULL && tbs_client_cbs->termination_reason != NULL) {
			tbs_client_cbs->termination_reason(conn, 0, tbs_index(conn, tbs_inst),
							   reason.call_index, reason.reason);
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

	LOG_DBG("%s", uri);

	if (tbs_client_cbs != NULL && tbs_client_cbs->remote_uri != NULL) {
		tbs_client_cbs->remote_uri(conn, 0, tbs_index(conn, tbs_inst), uri);
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

	LOG_DBG("%s", name);

	if (tbs_client_cbs != NULL && tbs_client_cbs->friendly_name != NULL) {
		tbs_client_cbs->friendly_name(conn, 0, tbs_index(conn, tbs_inst), name);
	}
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */

/** @brief Handles notifications and indications from the server */
static uint8_t notify_handler(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_tbs_instance *tbs_inst;

	if (data == NULL || conn == NULL) {
		LOG_DBG("[UNSUBSCRIBED] 0x%04X", params->value_handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	tbs_inst = lookup_inst_by_handle(conn, handle);
	if (tbs_inst != NULL) {
		uint8_t inst_index = tbs_index(conn, tbs_inst);

		LOG_DBG("Index %u", inst_index);

		LOG_HEXDUMP_DBG(data, length, "notify handler value");

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
		LOG_DBG("Notification/Indication on unknown TBS inst");
	}

	return BT_GATT_ITER_CONTINUE;
}

static void initialize_net_buf_read_buffer(struct bt_tbs_instance *inst)
{
	net_buf_simple_init_with_data(&inst->net_buf, &inst->read_buf,
				      sizeof(inst->read_buf));
	net_buf_simple_reset(&inst->net_buf);
}

static void tbs_client_gatt_read_complete(struct bt_tbs_instance *inst)
{
	(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	inst->busy = false;
}

static int tbs_client_gatt_read(struct bt_conn *conn, struct bt_tbs_instance *inst, uint16_t handle,
				bt_gatt_read_func_t func)
{
	int err;

	if (inst->busy) {
		return -EBUSY;
	}

	/* Use read_buf; length may be larger than minimum BT_ATT_MTU */
	initialize_net_buf_read_buffer(inst);
	inst->read_params.func = func;
	inst->read_params.handle_count = 1U;
	inst->read_params.single.handle = handle;
	inst->read_params.single.offset = 0U;
	inst->busy = true;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		tbs_client_gatt_read_complete(inst);

		return err;
	}

	return 0;
}

static bool gtbs_found(struct bt_tbs_server_inst *srv_inst)
{
#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	return srv_inst->gtbs_inst.start_handle != 0;
#else
	return false;
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */
}

static uint8_t inst_cnt(struct bt_tbs_server_inst *srv_inst)
{
#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	return srv_inst->inst_cnt;
#else
	return 0;
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
}

static void tbs_client_discover_complete(struct bt_conn *conn, int err)
{
	struct bt_tbs_server_inst *srv_inst = &srv_insts[bt_conn_index(conn)];

	LOG_DBG("conn %p err %d", (void *)conn, err);

	/* Clear the current instance in discovery */
	srv_inst->current_inst = NULL;

	if (tbs_client_cbs != NULL && tbs_client_cbs->discover != NULL) {
		tbs_client_cbs->discover(conn, err, inst_cnt(srv_inst), gtbs_found(srv_inst));
	}
}

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) ||                                          \
	defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) ||                                                \
	defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) ||                         \
	defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) ||                                              \
	defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) ||                                             \
	defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
/* Common function to read tbs_client strings which may require long reads */
static uint8_t handle_string_long_read(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data,
				       uint16_t length,
				       bt_tbs_client_read_string_cb cb,
				       bool truncatable)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint16_t offset = params->single.offset;
	uint8_t inst_index = tbs_index(conn, inst);
	const char *received_string;
	int tbs_err = err;

	if ((tbs_err == 0) && (data != NULL) &&
	    (net_buf_simple_tailroom(&inst->net_buf) < length)) {
		LOG_DBG("Read length %u: String buffer full", length);
		if (truncatable) {
			/* Use the remaining buffer and continue reading */
			LOG_DBG("Truncating string");
			length = net_buf_simple_tailroom(&inst->net_buf);
		} else {
			tbs_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
		}
	}

	if (tbs_err != 0) {
		LOG_DBG("err: %d", tbs_err);

		tbs_client_gatt_read_complete(inst);

		if (cb != NULL) {
			cb(conn, tbs_err, inst_index, NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		/* Get data and try to read more using read long procedure */
		LOG_DBG("Read (offset %u): %s", offset, bt_hex(data, length));

		net_buf_simple_add_mem(&inst->net_buf, data, length);

		return BT_GATT_ITER_CONTINUE;
	}

	if (inst->net_buf.len == 0) {
		received_string = NULL;
	} else {
		uint16_t str_length = inst->net_buf.len;

		/* Ensure there is space for string termination */
		if (net_buf_simple_tailroom(&inst->net_buf) < 1) {
			LOG_DBG("Truncating string");
			if (truncatable) {
				/* Truncate */
				str_length--;
			} else {
				tbs_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
			}
		}

		if (tbs_err == 0) {
			char *str_data;

			/* Get a reference to the string buffer */
			str_data = net_buf_simple_pull_mem(&inst->net_buf,
							   inst->net_buf.len);

			/* All strings are UTF-8, truncate properly if needed */
			str_data[str_length] = '\0';
			received_string = utf8_trunc(str_data);

			/* The string might have been truncated */
			if (strlen(received_string) < str_length) {
				LOG_DBG("Truncating string");
				if (!truncatable) {
					tbs_err =
					      BT_ATT_ERR_INSUFFICIENT_RESOURCES;
				}
			}

			LOG_DBG("%s", received_string);
		}
	}

	if (tbs_err) {
		received_string = NULL;
	}

	tbs_client_gatt_read_complete(inst);

	if (cb != NULL) {
		cb(conn, tbs_err, inst_index, received_string);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME ||                                             \
	* CONFIG_BT_TBS_CLIENT_BEARER_UCI ||                                                       \
	* CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST ||                                \
	* CONFIG_BT_TBS_CLIENT_INCOMING_URI ||                                                     \
	* CONFIG_BT_TBS_CLIENT_INCOMING_CALL ||                                                    \
	* CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME                                                  \
	*/

#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
static int tbs_client_common_call_control(struct bt_conn *conn,
					  uint8_t inst_index,
					  uint8_t call_index,
					  uint8_t opcode)
{
	struct bt_tbs_instance *inst;
	struct bt_tbs_call_cp_acc common;

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->call_cp_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
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
	bt_tbs_client_read_string_cb cb = NULL;

	LOG_DBG("Read bearer provider name");

	if (tbs_client_cbs != NULL &&
	    tbs_client_cbs->bearer_provider_name != NULL) {
		cb = tbs_client_cbs->bearer_provider_name;
	}

	return handle_string_long_read(conn, err, params, data,
				       length, cb, true);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
static uint8_t read_bearer_uci_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	bt_tbs_client_read_string_cb cb = NULL;

	LOG_DBG("Read bearer UCI");

	if (tbs_client_cbs != NULL && tbs_client_cbs->bearer_uci != NULL) {
		cb = tbs_client_cbs->bearer_uci;
	}

	/* The specification does not indicate truncation as an option, so
	 * fail if insufficient buffer space.
	 */
	return handle_string_long_read(conn, err, params, data,
				       length, cb, false);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
static uint8_t read_technology_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	uint8_t cb_err = err;
	uint8_t technology = 0;

	LOG_DBG("Index %u", inst_index);

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "Data read");
		if (length == sizeof(technology)) {
			(void)memcpy(&technology, data, length);
			LOG_DBG("%s (0x%02x)", bt_tbs_technology_str(technology), technology);
		} else {
			LOG_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	tbs_client_gatt_read_complete(inst);

	if (tbs_client_cbs != NULL && tbs_client_cbs->technology != NULL) {
		tbs_client_cbs->technology(conn, cb_err, inst_index, technology);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
static uint8_t read_uri_list_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	bt_tbs_client_read_string_cb cb = NULL;

	LOG_DBG("Read bearer UCI");

	if (tbs_client_cbs != NULL && tbs_client_cbs->uri_list != NULL) {
		cb = tbs_client_cbs->uri_list;
	}

	return handle_string_long_read(conn, err, params, data,
				       length, cb, false);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
static uint8_t read_signal_strength_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	uint8_t cb_err = err;
	uint8_t signal_strength = 0;

	LOG_DBG("Index %u", inst_index);

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "Data read");
		if (length == sizeof(signal_strength)) {
			(void)memcpy(&signal_strength, data, length);
			LOG_DBG("0x%02x", signal_strength);
		} else {
			LOG_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	tbs_client_gatt_read_complete(inst);

	if (tbs_client_cbs != NULL && tbs_client_cbs->signal_strength != NULL) {
		tbs_client_cbs->signal_strength(conn, cb_err, inst_index,
						signal_strength);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */

#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL)
static uint8_t read_signal_interval_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	uint8_t cb_err = err;
	uint8_t signal_interval = 0;

	LOG_DBG("Index %u", inst_index);

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "Data read");
		if (length == sizeof(signal_interval)) {
			(void)memcpy(&signal_interval, data, length);
			LOG_DBG("0x%02x", signal_interval);
		} else {
			LOG_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	tbs_client_gatt_read_complete(inst);

	if (tbs_client_cbs && tbs_client_cbs->signal_interval) {
		tbs_client_cbs->signal_interval(conn, cb_err, inst_index,
						signal_interval);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
static uint8_t read_current_calls_cb(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_read_params *params,
				      const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	int tbs_err = err;

	LOG_DBG("Read bearer list current calls, index %u", inst_index);

	if ((tbs_err == 0) && (data != NULL) &&
	    (net_buf_simple_tailroom(&inst->net_buf) < length)) {
		tbs_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
	}

	if (tbs_err != 0) {
		LOG_DBG("err: %d", tbs_err);

		tbs_client_gatt_read_complete(inst);

		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->current_calls != NULL) {
			tbs_client_cbs->current_calls(conn, tbs_err,
						      inst_index, 0, NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		LOG_DBG("Current calls read (offset %u): %s",
			params->single.offset,
			bt_hex(data, length));

		net_buf_simple_add_mem(&inst->net_buf, data, length);

		/* Returning continue will try to read more using read
		 * long procedure
		 */
		return BT_GATT_ITER_CONTINUE;
	}

	tbs_client_gatt_read_complete(inst);

	if (inst->net_buf.len == 0) {
		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->current_calls != NULL) {
			tbs_client_cbs->current_calls(conn, 0,
						      inst_index, 0, NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	bearer_list_current_calls(conn, inst, &inst->net_buf);

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
static uint8_t read_ccid_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_read_params *params,
			    const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	uint8_t cb_err = err;
	uint8_t ccid = 0;

	LOG_DBG("Index %u", inst_index);

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "Data read");
		if (length == sizeof(ccid)) {
			(void)memcpy(&ccid, data, length);
			LOG_DBG("0x%02x", ccid);
		} else {
			LOG_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	tbs_client_gatt_read_complete(inst);

	if (tbs_client_cbs != NULL && tbs_client_cbs->ccid != NULL) {
		tbs_client_cbs->ccid(conn, cb_err, inst_index, ccid);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
static uint8_t read_status_flags_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	uint8_t cb_err = err;
	uint16_t status_flags = 0;

	LOG_DBG("Index %u", inst_index);

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "Data read");
		if (length == sizeof(status_flags)) {
			(void)memcpy(&status_flags, data, length);
			LOG_DBG("0x%04x", status_flags);
		} else {
			LOG_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	tbs_client_gatt_read_complete(inst);

	if (tbs_client_cbs != NULL &&
		tbs_client_cbs->status_flags != NULL) {
		tbs_client_cbs->status_flags(conn, cb_err, inst_index,
					     status_flags);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
static uint8_t read_call_uri_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_read_params *params,
				const void *data, uint16_t length)
{
	bt_tbs_client_read_string_cb cb = NULL;

	LOG_DBG("Read incoming call target bearer URI");

	if (tbs_client_cbs != NULL && tbs_client_cbs->call_uri != NULL) {
		cb = tbs_client_cbs->call_uri;
	}

	return handle_string_long_read(conn, err, params, data,
				       length, cb, false);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */

static uint8_t read_call_state_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params,
						    struct bt_tbs_instance,
						    read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	uint8_t cnt = 0;
	struct bt_tbs_client_call_state call_states[CONFIG_BT_TBS_CLIENT_MAX_CALLS];
	int tbs_err = err;

	LOG_DBG("Index %u", inst_index);

	if ((tbs_err == 0) && (data != NULL) &&
	    (net_buf_simple_tailroom(&inst->net_buf) < length)) {
		tbs_err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
	}

	if (tbs_err != 0) {
		LOG_DBG("err: %d", tbs_err);

		tbs_client_gatt_read_complete(inst);

		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->call_state != NULL) {
			tbs_client_cbs->call_state(conn, tbs_err,
						   inst_index, 0, NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		LOG_DBG("Call states read (offset %u): %s", params->single.offset,
			bt_hex(data, length));

		net_buf_simple_add_mem(&inst->net_buf, data, length);

		/* Returning continue will try to read more using read long procedure */
		return BT_GATT_ITER_CONTINUE;
	}

	if (inst->net_buf.len == 0) {
		tbs_client_gatt_read_complete(inst);

		if (tbs_client_cbs != NULL &&
		    tbs_client_cbs->call_state != NULL) {
			tbs_client_cbs->call_state(conn, 0, inst_index, 0, NULL);
		}

		return BT_GATT_ITER_STOP;
	}

	/* Finished reading, start parsing */
	while (inst->net_buf.len != 0) {
		struct bt_tbs_client_call_state *call_state;

		if (cnt == CONFIG_BT_TBS_CLIENT_MAX_CALLS) {
			LOG_WRN("Could not parse all calls due to memory restrictions");
			break;
		}

		call_state = &call_states[cnt];

		tbs_err = net_buf_pull_call_state(&inst->net_buf, call_state);
		if (tbs_err != 0) {
			LOG_DBG("Invalid current call notification: %d", err);
			break;
		}

		cnt++;
	}

	tbs_client_gatt_read_complete(inst);

	if (tbs_client_cbs != NULL && tbs_client_cbs->call_state != NULL) {
		tbs_client_cbs->call_state(conn, tbs_err, inst_index, cnt, call_states);
	}

	return BT_GATT_ITER_STOP;
}

#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
static uint8_t read_optional_opcodes_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	uint8_t cb_err = err;
	uint16_t optional_opcodes = 0;

	LOG_DBG("Index %u", inst_index);

	if (err != 0) {
		LOG_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		LOG_HEXDUMP_DBG(data, length, "Data read");
		if (length == sizeof(optional_opcodes)) {
			(void)memcpy(&optional_opcodes, data, length);
			LOG_DBG("0x%04x", optional_opcodes);
		} else {
			LOG_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	tbs_client_gatt_read_complete(inst);

	if (tbs_client_cbs != NULL &&
		tbs_client_cbs->optional_opcodes != NULL) {
		tbs_client_cbs->optional_opcodes(conn, cb_err, inst_index, optional_opcodes);
	}

	return BT_GATT_ITER_STOP;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
static uint8_t read_remote_uri_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	bt_tbs_client_read_string_cb cb = NULL;

	LOG_DBG("Read incoming call URI");

	if (tbs_client_cbs != NULL &&
		tbs_client_cbs->remote_uri != NULL) {
		cb = tbs_client_cbs->remote_uri;
	}

	return handle_string_long_read(conn, err, params, data,
				       length, cb, false);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
static uint8_t read_friendly_name_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	bt_tbs_client_read_string_cb cb = NULL;

	LOG_DBG("Read incoming call target bearer URI");

	if (tbs_client_cbs != NULL &&
		tbs_client_cbs->friendly_name != NULL) {
		cb = tbs_client_cbs->friendly_name;
	}

	return handle_string_long_read(conn, err, params, data,
				       length, cb, true);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
static uint8_t disc_read_ccid_cb(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_read_params *params,
				 const void *data, uint16_t length)
{
	struct bt_tbs_instance *inst = CONTAINER_OF(params, struct bt_tbs_instance, read_params);
	uint8_t inst_index = tbs_index(conn, inst);
	int cb_err = err;

	LOG_DBG("Index %u", inst_index);

	if (cb_err != 0) {
		LOG_DBG("err: 0x%02X", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(inst->ccid)) {
			inst->ccid = ((uint8_t *)data)[0];
			LOG_DBG("0x%02x", inst->ccid);
		} else {
			LOG_DBG("Invalid length");
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	tbs_client_gatt_read_complete(inst);

	if (cb_err != 0) {
		tbs_client_discover_complete(conn, cb_err);
	} else {
		discover_next_instance(conn);
	}

	return BT_GATT_ITER_STOP;
}

static void tbs_client_disc_read_ccid(struct bt_conn *conn)
{
	const uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];
	struct bt_tbs_instance *inst = srv_inst->current_inst;
	int err;

	err = tbs_client_gatt_read(conn, inst, inst->ccid_handle, disc_read_ccid_cb);
	if (err != 0) {
		tbs_client_discover_complete(conn, err);
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
#else
		discover_next_instance(conn);
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		const struct bt_gatt_chrc *chrc;
		struct bt_gatt_subscribe_params *sub_params = NULL;

		chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_CALL_STATE) == 0) {
			LOG_DBG("Call state");
			sub_params = &current_inst->call_state_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->call_state_sub_disc_params;
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_PROVIDER_NAME) == 0) {
			LOG_DBG("Provider name");
			sub_params = &current_inst->name_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->name_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_UCI) == 0) {
			LOG_DBG("Bearer UCI");
			current_inst->bearer_uci_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_TECHNOLOGY) == 0) {
			LOG_DBG("Technology");
			sub_params = &current_inst->technology_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->technology_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_URI_LIST) == 0) {
			LOG_DBG("URI Scheme List");
			current_inst->uri_list_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_SIGNAL_STRENGTH) == 0) {
			LOG_DBG("Signal strength");
			sub_params = &current_inst->signal_strength_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->signal_strength_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */
#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) \
|| defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_SIGNAL_INTERVAL) == 0) {
			LOG_DBG("Signal strength reporting interval");
			current_inst->signal_interval_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */
/* || defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_LIST_CURRENT_CALLS) == 0) {
			LOG_DBG("Current calls");
			sub_params = &current_inst->current_calls_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->current_calls_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_CCID)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_CCID) == 0) {
			LOG_DBG("CCID");
			current_inst->ccid_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_INCOMING_URI) == 0) {
			LOG_DBG("Incoming target URI");
			sub_params = &current_inst->in_target_uri_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->in_target_uri_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */
#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_STATUS_FLAGS) == 0) {
			LOG_DBG("Status flags");
			sub_params = &current_inst->status_flags_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->status_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */
#if defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_CALL_CONTROL_POINT) == 0) {
			LOG_DBG("Call control point");
			sub_params = &current_inst->call_cp_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->call_cp_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CP_PROCEDURES) */
#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_OPTIONAL_OPCODES) == 0) {
			LOG_DBG("Supported opcodes");
			current_inst->optional_opcodes_handle = chrc->value_handle;
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_TERMINATE_REASON) == 0) {
			LOG_DBG("Termination reason");
			current_inst->termination_reason_handle = chrc->value_handle;
			sub_params = &current_inst->termination_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->termination_sub_disc_params;
#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_FRIENDLY_NAME) == 0) {
			LOG_DBG("Incoming friendly name");
			sub_params = &current_inst->friendly_name_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->friendly_name_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_INCOMING_CALL) == 0) {
			LOG_DBG("Incoming call");
			sub_params = &current_inst->incoming_call_sub_params;
			sub_params->value_handle = chrc->value_handle;
			sub_params->disc_params = &current_inst->incoming_call_sub_disc_params;
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */
		}

		if (sub_params != NULL) {
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
				atomic_set_bit(sub_params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
				err = bt_gatt_subscribe(conn, sub_params);
				if (err != 0) {
					LOG_DBG("Could not subscribe to "
					       "characterstic at handle 0x%04X"
					       "(%d)",
					       sub_params->value_handle, err);
				} else {
					LOG_DBG("Subscribed to characterstic at "
					       "handle 0x%04X",
					       sub_params->value_handle);
				}
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_tbs_instance *get_next_instance(struct bt_conn *conn,
						 struct bt_tbs_server_inst *srv_inst)
{
	uint8_t inst_index;

	if (srv_inst->current_inst != NULL) {
		inst_index = tbs_index(conn, srv_inst->current_inst);
		if (inst_index == BT_TBS_GTBS_INDEX) {
			inst_index = 0;
		} else {
			inst_index++;
		}

		return tbs_inst_by_index(conn, inst_index);
	}

	inst_index = gtbs_found(srv_inst) ? BT_TBS_GTBS_INDEX : 0;

	return tbs_inst_by_index(conn, inst_index);
}

static void discover_next_instance(struct bt_conn *conn)
{
	int err;
	uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];

	srv_inst->current_inst = get_next_instance(conn, srv_inst);
	if (srv_inst->current_inst == NULL) {
		tbs_client_discover_complete(conn, 0);
		return;
	}

	LOG_DBG("inst_index %u", tbs_index(conn, srv_inst->current_inst));

	(void)memset(&srv_inst->discover_params, 0, sizeof(srv_inst->discover_params));
	srv_inst->discover_params.uuid = NULL;
	srv_inst->discover_params.start_handle = srv_inst->current_inst->start_handle;
	srv_inst->discover_params.end_handle = srv_inst->current_inst->end_handle;
	srv_inst->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	srv_inst->discover_params.func = discover_func;

	err = bt_gatt_discover(conn, &srv_inst->discover_params);
	if (err != 0) {
		tbs_client_discover_complete(conn, err);
	}
}

static void primary_discover_complete(struct bt_tbs_server_inst *server, struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS)) {
		LOG_DBG("Discover complete, found %u instances (GTBS%s found)",
			inst_cnt(server), gtbs_found(server) ? "" : " not");
	} else {
		LOG_DBG("Discover complete, found %u instances", inst_cnt(server));
	}

	server->current_inst = NULL;

	if (gtbs_found(server) || inst_cnt(server) > 0) {
		discover_next_instance(conn);
	} else {
		tbs_client_discover_complete(conn, 0);
	}
}

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
#if defined(CONFIG_BT_TBS_CLIENT_TBS)
static const struct bt_uuid *tbs_uuid = BT_UUID_TBS;

static uint8_t primary_discover_tbs_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       struct bt_gatt_discover_params *params)
{
	const uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];

	LOG_DBG("conn %p attr %p", (void *)conn, attr);

	if (attr != NULL) {
		const struct bt_gatt_service_val *prim_service;

		LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		prim_service = (struct bt_gatt_service_val *)attr->user_data;

		srv_inst->current_inst = &srv_inst->tbs_insts[srv_inst->inst_cnt++];
		srv_inst->current_inst->start_handle = attr->handle + 1;
		srv_inst->current_inst->end_handle = prim_service->end_handle;

		if (srv_inst->inst_cnt < ARRAY_SIZE(srv_inst->tbs_insts)) {
			return BT_GATT_ITER_CONTINUE;
		}
	}

	primary_discover_complete(srv_inst, conn);

	return BT_GATT_ITER_STOP;
}

static int primary_discover_tbs(struct bt_conn *conn)
{
	struct bt_tbs_server_inst *srv_inst = &srv_insts[bt_conn_index(conn)];
	struct bt_gatt_discover_params *params = &srv_inst->discover_params;

	LOG_DBG("conn %p", (void *)conn);

	(void)memset(params, 0, sizeof(*params));
	params->uuid = tbs_uuid;
	params->func = primary_discover_tbs_cb;
	params->type = BT_GATT_DISCOVER_PRIMARY;
	params->start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_discover(conn, params);
}
#endif /* CONFIG_BT_TBS_CLIENT_TBS */

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
static const struct bt_uuid *gtbs_uuid = BT_UUID_GTBS;

static uint8_t primary_discover_gtbs_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					struct bt_gatt_discover_params *params)
{
	const uint8_t conn_index = bt_conn_index(conn);
	struct bt_tbs_server_inst *srv_inst = &srv_insts[conn_index];

	LOG_DBG("conn %p attr %p", (void *)conn, attr);

	if (attr != NULL) {
		const struct bt_gatt_service_val *prim_service;

		LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

		prim_service = (struct bt_gatt_service_val *)attr->user_data;

		srv_inst->current_inst = &srv_inst->gtbs_inst;
		srv_inst->current_inst->start_handle = attr->handle + 1;
		srv_inst->current_inst->end_handle = prim_service->end_handle;
	}

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	int err;

	err = primary_discover_tbs(conn);
	if (err == 0) {
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("Discover failed (err %d)", err);
#endif /* CONFIG_BT_TBS_CLIENT_TBS */

	primary_discover_complete(srv_inst, conn);

	return BT_GATT_ITER_STOP;
}

static int primary_discover_gtbs(struct bt_conn *conn)
{
	struct bt_tbs_server_inst *srv_inst = &srv_insts[bt_conn_index(conn)];
	struct bt_gatt_discover_params *params = &srv_inst->discover_params;

	LOG_DBG("conn %p", (void *)conn);

	(void)memset(params, 0, sizeof(*params));
	params->uuid = gtbs_uuid;
	params->func = primary_discover_gtbs_cb;
	params->type = BT_GATT_DISCOVER_PRIMARY;
	params->start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params->end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_discover(conn, params);
}
#endif /* IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS) */

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
	} else if (!bt_tbs_valid_uri(uri)) {
		LOG_DBG("Invalid URI: %s", uri);
		return -EINVAL;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	/* Check if there are free spots */
	if (!free_call_spot(inst)) {
		LOG_DBG("Cannot originate more calls");
		return -ENOMEM;
	}

	uri_len = strlen(uri);

	if (uri_len > max_uri_len) {
		LOG_DBG("URI len (%zu) longer than maximum writable %zu", uri_len, max_uri_len);
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

		inst = tbs_inst_by_index(conn, inst_index);
		if (inst == NULL) {
			return -EINVAL;
		}

		if (inst->call_cp_sub_params.value_handle == 0) {
			LOG_DBG("Handle not set");
			return -EINVAL;
		}

		if (count > max_call_cnt) {
			LOG_DBG("Call count (%u) larger than maximum writable %zu", count,
				max_call_cnt);
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
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	/* Populate Outgoing Remote URI */
	if (inst->signal_interval_handle == 0) {
		LOG_DBG("Handle not set");
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
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->name_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->name_sub_params.value_handle,
				    read_bearer_provider_name_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
int bt_tbs_client_read_bearer_uci(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->bearer_uci_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->bearer_uci_handle, read_bearer_uci_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
int bt_tbs_client_read_technology(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->technology_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->technology_sub_params.value_handle,
				    read_technology_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
int bt_tbs_client_read_uri_list(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->uri_list_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->uri_list_handle, read_uri_list_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
int bt_tbs_client_read_signal_strength(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->signal_strength_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->signal_strength_sub_params.value_handle,
				    read_signal_strength_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */

#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL)
int bt_tbs_client_read_signal_interval(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->signal_interval_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->signal_interval_handle,
				    read_signal_interval_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
int bt_tbs_client_read_current_calls(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->current_calls_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->current_calls_sub_params.value_handle,
				    read_current_calls_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
int bt_tbs_client_read_ccid(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->ccid_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->ccid_handle, read_ccid_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
int bt_tbs_client_read_call_uri(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->in_target_uri_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->in_target_uri_sub_params.value_handle,
				    read_call_uri_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */

#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
int bt_tbs_client_read_status_flags(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->status_flags_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->status_flags_sub_params.value_handle,
				    read_status_flags_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */

int bt_tbs_client_read_call_state(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->call_state_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->call_state_sub_params.value_handle,
				    read_call_state_cb);
}

#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
int bt_tbs_client_read_optional_opcodes(struct bt_conn *conn,
					uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->optional_opcodes_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->optional_opcodes_handle,
				    read_optional_opcodes_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
int bt_tbs_client_read_remote_uri(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->incoming_call_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->incoming_call_sub_params.value_handle,
				    read_remote_uri_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
int bt_tbs_client_read_friendly_name(struct bt_conn *conn, uint8_t inst_index)
{
	struct bt_tbs_instance *inst;

	if (conn == NULL) {
		return -ENOTCONN;
	}

	inst = tbs_inst_by_index(conn, inst_index);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->friendly_name_sub_params.value_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	return tbs_client_gatt_read(conn, inst, inst->friendly_name_sub_params.value_handle,
				    read_friendly_name_cb);
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */

int bt_tbs_client_discover(struct bt_conn *conn)
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

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	(void)memset(srv_inst->tbs_insts, 0, sizeof(srv_inst->tbs_insts)); /* reset data */
	srv_inst->inst_cnt = 0;
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	(void)memset(&srv_inst->gtbs_inst, 0, sizeof(srv_inst->gtbs_inst)); /* reset data */
	return primary_discover_gtbs(conn);
#else
	return primary_discover_tbs(conn);
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */
}

void bt_tbs_client_register_cb(const struct bt_tbs_client_cb *cbs)
{
	tbs_client_cbs = cbs;
}

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
static bool tbs_instance_ccid_is_eq(struct bt_tbs_instance *inst, void *user_data)
{
	uint8_t ccid = POINTER_TO_UINT(user_data);

	return inst->ccid == ccid;
}

struct bt_tbs_instance *bt_tbs_client_get_by_ccid(const struct bt_conn *conn,
						  uint8_t ccid)
{
	struct bt_tbs_server_inst *server;

	CHECKIF(conn == NULL) {
		LOG_DBG("conn was NULL");
		return NULL;
	}

	server = &srv_insts[bt_conn_index(conn)];

	return tbs_instance_find(server, tbs_instance_ccid_is_eq, UINT_TO_POINTER(ccid));
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */
