/*  Bluetooth TBS - Call Control Profile - Client
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include "ccp.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CCP)
#define LOG_MODULE_NAME bt_ccp
#include "common/log.h"

#define FIRST_HANDLE			0x0001
#define LAST_HANDLE			0xFFFF
#define MAX_UCI_SIZE			6 /* 5 chars + NULL terminator */
#define MAX_URI_SCHEME_LIST_SIZE	64
#define MAX_SUBSCRIPTION_COUNT		10
#define MAX_CONCURRENT_REQS		4
#if IS_ENABLED(CONFIG_BT_CCP_GTBS)
#define TBS_INSTANCE_MAX_CNT		(CONFIG_BT_CCP_MAX_TBS_INSTANCES + 1)
#else
#define TBS_INSTANCE_MAX_CNT		CONFIG_BT_CCP_MAX_TBS_INSTANCES
#endif /* IS_ENABLED(CONFIG_BT_CCP_GTBS) */
#define GTBS_INDEX			CONFIG_BT_CCP_MAX_TBS_INSTANCES

#include "tbs_internal.h"

struct ccp_read_params {
	struct bt_gatt_read_params params;
	/* TODO: Convert to net_buf_simple */
	size_t bytes_read;
	uint8_t buf[512];
};

struct tbs_instance_t {
	struct bt_ccp_call_state_t calls[CONFIG_BT_CCP_MAX_CALLS];

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t provider_name_handle;
	uint16_t bearer_uci_handle;
	uint16_t technology_handle;
	uint16_t uri_list_handle;
	uint16_t signal_strength_handle;
	uint16_t signal_interval_handle;
	uint16_t current_calls_handle;
	uint16_t ccid_handle;
	uint16_t status_flags_handle;
	uint16_t in_uri_handle;
	uint16_t call_state_handle;
	uint16_t call_cp_handle;
	uint16_t optional_opcodes_handle;
	uint16_t termination_reason_handle;
	uint16_t friendly_name_handle;
	uint16_t in_call_handle;

	bool busy;
	uint8_t subscribe_cnt;
	uint8_t index;
	bool gtbs;

	struct bt_gatt_subscribe_params name_sub_params;
	struct bt_gatt_discover_params name_sub_disc_params;
	struct bt_gatt_subscribe_params technology_sub_params;
	struct bt_gatt_discover_params technology_sub_disc_params;
	struct bt_gatt_subscribe_params signal_strength_sub_params;
	struct bt_gatt_discover_params signal_strength_sub_disc_params;
	struct bt_gatt_subscribe_params current_calls_sub_params;
	struct bt_gatt_discover_params current_calls_sub_disc_params;
	struct bt_gatt_subscribe_params in_target_uri_sub_params;
	struct bt_gatt_discover_params in_target_uri_sub_disc_params;
	struct bt_gatt_subscribe_params status_flags_sub_params;
	struct bt_gatt_discover_params status_sub_disc_params;
	struct bt_gatt_subscribe_params call_state_sub_params;
	struct bt_gatt_discover_params call_state_sub_disc_params;
	struct bt_gatt_subscribe_params call_cp_sub_params;
	struct bt_gatt_discover_params call_cp_sub_disc_params;
	struct bt_gatt_subscribe_params termination_sub_params;
	struct bt_gatt_discover_params termination_sub_disc_params;
	struct bt_gatt_subscribe_params incoming_call_sub_params;
	struct bt_gatt_discover_params incoming_call_sub_disc_params;
	struct bt_gatt_subscribe_params friendly_name_sub_params;
	struct bt_gatt_discover_params friendly_name_sub_disc_params;

	uint8_t write_buf[CONFIG_BT_L2CAP_TX_MTU];
};

struct tbs_server_inst {
	struct tbs_instance_t tbs_insts[TBS_INSTANCE_MAX_CNT];
	struct bt_gatt_discover_params discover_params;
	struct tbs_instance_t *current_inst;
	uint8_t inst_cnt;
	bool gtbs_found;
	bool subscribe_all;
};

static struct ccp_read_params read_params_arr[MAX_CONCURRENT_REQS];
static struct bt_ccp_cb_t *ccp_cbs;

static struct tbs_server_inst srv_inst;
static struct bt_uuid *tbs_uuid = BT_UUID_TBS;
static struct bt_uuid *gtbs_uuid = BT_UUID_GTBS;

static void discover_next_instance(struct bt_conn *conn, uint8_t index);

static bool valid_inst_index(uint8_t idx)
{
	if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && idx == BT_CCP_GTBS_INDEX) {
		return true;
	} else {
		return idx < srv_inst.inst_cnt;
	}
}

static struct tbs_instance_t *get_inst_by_index(uint8_t idx)
{
	if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && idx == BT_CCP_GTBS_INDEX) {
		return &srv_inst.tbs_insts[GTBS_INDEX];
	} else {
		return &srv_inst.tbs_insts[idx];
	}
}

static struct ccp_read_params *get_read_params(void)
{
	for (int i = 0; i < ARRAY_SIZE(read_params_arr); i++) {
		if (!read_params_arr[i].params.func) {
			read_params_arr[i].bytes_read = 0;
			return &read_params_arr[i];
		}
	}
	return NULL;
}

static struct ccp_read_params *lookup_read_params(
	struct bt_gatt_read_params *params)
{
	for (int i = 0; i < ARRAY_SIZE(read_params_arr); i++) {
		if (&read_params_arr[i].params == params) {
			return &read_params_arr[i];
		}
	}
	return NULL;
}

static bool free_call_spot(struct tbs_instance_t *inst)
{
	for (int i = 0; i < CONFIG_BT_CCP_MAX_CALLS; i++) {
		if (inst->calls[i].index == BT_TBS_FREE_CALL_INDEX) {
			return true;
		}
	}
	return false;
}

static struct tbs_instance_t *lookup_instance_by_handle(uint16_t handle)
{
	for (int i = 0; i < ARRAY_SIZE(srv_inst.tbs_insts); i++) {
		if (srv_inst.tbs_insts[i].start_handle <= handle &&
		    srv_inst.tbs_insts[i].end_handle >= handle) {
			return &srv_inst.tbs_insts[i];
		}
	}
	BT_DBG("Could not find instance with handle 0x%04x", handle);
	return NULL;
}

static void call_cp_callback_handler(struct bt_conn *conn, int err,
				     uint8_t index, uint8_t opcode,
				     uint8_t call_index)
{
	bt_ccp_cp_cb_t cp_cb = NULL;

	BT_DBG("Status: %s for the %s opcode for call 0x%02x",
	       tbs_status_str(err), tbs_opcode_str(opcode), call_index);

	if (!ccp_cbs) {
		return;
	}

	switch (opcode) {
	case BT_TBS_CALL_OPCODE_ACCEPT:
		cp_cb = ccp_cbs->accept_call;
		break;
	case BT_TBS_CALL_OPCODE_TERMINATE:
		cp_cb = ccp_cbs->terminate_call;
		break;
	case BT_TBS_CALL_OPCODE_HOLD:
		cp_cb = ccp_cbs->hold_call;
		break;
	case BT_TBS_CALL_OPCODE_RETRIEVE:
		cp_cb = ccp_cbs->retrieve_call;
		break;
	case BT_TBS_CALL_OPCODE_ORIGINATE:
		cp_cb = ccp_cbs->originate_call;
		break;
	case BT_TBS_CALL_OPCODE_JOIN:
		cp_cb = ccp_cbs->join_calls;
		break;
	default:
		break;
	}

	if (cp_cb) {
		cp_cb(conn, err, index, call_index);
	}
}

static const char *parse_string_value(const void *data, uint16_t length,
				      uint16_t max_len)
{
	static char string_val[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];
	size_t len = MIN(length, max_len);

	if (len) {
		memcpy(string_val, data, len);
	}

	string_val[len] = '\0';

	return string_val;
}

static void provider_name_notify_handler(struct bt_conn *conn,
					 struct tbs_instance_t *tbs_inst,
					 const void *data, uint16_t length)
{
	const char *name = parse_string_value(
		data, length, CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);

	BT_DBG("%s", log_strdup(name));

	if (ccp_cbs && ccp_cbs->bearer_provider_name) {
		ccp_cbs->bearer_provider_name(conn, 0, tbs_inst->index, name);
	}
}

static void technology_notify_handler(struct bt_conn *conn,
				      struct tbs_instance_t *tbs_inst,
				      const void *data, uint16_t length)
{
	uint8_t technology;

	BT_DBG("");

	if (length == sizeof(technology)) {
		memcpy(&technology, data, length);
		BT_DBG("%s (0x%02x)",
		       tbs_technology_str(technology), technology);

		if (ccp_cbs && ccp_cbs->technology) {
			ccp_cbs->technology(conn, 0, tbs_inst->index,
					    technology);
		}
	}
}

static void signal_strength_notify_handler(struct bt_conn *conn,
					   struct tbs_instance_t *tbs_inst,
					   const void *data, uint16_t length)
{
	uint8_t signal_strength;

	BT_DBG("");

	if (length == sizeof(signal_strength)) {
		memcpy(&signal_strength, data, length);
		BT_DBG("0x%02x", signal_strength);

		if (ccp_cbs && ccp_cbs->signal_strength) {
			ccp_cbs->signal_strength(conn, 0, tbs_inst->index,
						 signal_strength);
		}
	}
}

static void current_calls_notify_handler(struct bt_conn *conn,
					 struct tbs_instance_t *tbs_inst,
					 const void *data, uint16_t length)
{
	struct bt_ccp_call_t calls[CONFIG_BT_CCP_MAX_CALLS];
	uint16_t bytes_parsed = 0;
	uint8_t call_index = 0;

	BT_DBG("");

	while (bytes_parsed < length && call_index < CONFIG_BT_CCP_MAX_CALLS) {
		uint8_t parsed_len;
		struct bt_ccp_call_t *call;
		uint8_t item_len;
		uint8_t uri_len;

		call = &calls[call_index];
		item_len = ((uint8_t *)data)[bytes_parsed++];

		if ((length - bytes_parsed) < item_len) {
			BT_DBG("Invalid item length, likely the value did not "
			       "fit in a notification");
			return;
		}

		parsed_len = MIN(item_len, sizeof(struct bt_ccp_call_t));

		memcpy(call, (uint8_t *)data + bytes_parsed, parsed_len);

		uri_len = parsed_len - sizeof(call->index) -
				sizeof(call->state) - sizeof(call->flags);

		call->remote_uri[uri_len] = '\0';

		BT_DBG("Call 0x%02x is in the %s state with flags 0x%02X and "
		       "URI %s",
		       call->index, tbs_state_str(call->state),
		       call->flags, log_strdup(call->remote_uri));
		bytes_parsed += item_len;
		call_index++;
	}

	if (ccp_cbs && ccp_cbs->current_calls) {
		ccp_cbs->current_calls(conn, 0, tbs_inst->index, call_index,
				       calls);
	}
}

static void status_flags_notify_handler(struct bt_conn *conn,
					struct tbs_instance_t *tbs_inst,
					const void *data, uint16_t length)
{
	uint16_t status_flags;

	BT_DBG("");

	if (length == sizeof(status_flags)) {
		memcpy(&status_flags, data, length);
		BT_DBG("0x%04x", status_flags);
		if (ccp_cbs && ccp_cbs->status_flags) {
			ccp_cbs->status_flags(conn, 0, tbs_inst->index,
						    status_flags);
		}
	}
}

static void incoming_uri_notify_handler(struct bt_conn *conn,
					      struct tbs_instance_t *tbs_inst,
					      const void *data, uint16_t length)
{
	const char *uri = parse_string_value(data, length,
					     CONFIG_BT_TBS_MAX_URI_LENGTH);

	BT_DBG("%s", log_strdup(uri));

	if (ccp_cbs && ccp_cbs->call_uri) {
		ccp_cbs->call_uri(conn, 0, tbs_inst->index, uri);
	}
}

static void call_state_notify_handler(struct bt_conn *conn,
				      struct tbs_instance_t *tbs_inst,
				      const void *data, uint16_t length)
{
	struct bt_ccp_call_state_t *call_state;
	uint16_t bytes_parsed = 0;
	uint8_t call_cnt = 0;

	BT_DBG("");

	if (length > sizeof(tbs_inst->calls)) {
		BT_WARN("Could not store all call states");
		length = sizeof(tbs_inst->calls);
	}

	if ((length % sizeof(*call_state)) != 0) {
		BT_DBG("Invalid length %u", length);
		return;
	}


	memset(tbs_inst->calls, 0, sizeof(tbs_inst->calls));
	while (bytes_parsed < length && call_cnt < CONFIG_BT_CCP_MAX_CALLS) {
		call_state = &tbs_inst->calls[call_cnt++];
		memcpy(call_state, (uint8_t *)data + bytes_parsed,
		       sizeof(*call_state));
		bytes_parsed += sizeof(*call_state);
		BT_DBG("Call 0x%02x is in the %s state with flags 0x%02X",
		       call_state->index, tbs_state_str(call_state->state),
		       call_state->flags);
	}

	if (ccp_cbs && ccp_cbs->call_state) {
		ccp_cbs->call_state(conn, 0, tbs_inst->index, call_cnt,
				    tbs_inst->calls);
	}
}

static void call_cp_notify_handler(struct bt_conn *conn,
				  struct tbs_instance_t *tbs_inst,
				  const void *data, uint16_t length)
{
	struct bt_tbs_call_cp_not_t *ind_val;

	BT_DBG("");

	if (length == sizeof(*ind_val)) {
		ind_val = (struct bt_tbs_call_cp_not_t *)data;
		BT_DBG("Status: %s for the %s opcode for call 0x%02X",
		       tbs_status_str(ind_val->status),
		       tbs_opcode_str(ind_val->opcode),
		       ind_val->call_index);

		call_cp_callback_handler(conn, ind_val->status, tbs_inst->index,
					 ind_val->opcode, ind_val->call_index);
	}
}

static void termination_reason_notify_handler(struct bt_conn *conn,
					      struct tbs_instance_t *tbs_inst,
					      const void *data, uint16_t length)
{
	struct bt_tbs_terminate_reason_t reason;

	BT_DBG("");

	if (length == sizeof(reason)) {
		memcpy(&reason, data, length);
		BT_DBG("ID 0x%02X, reason %s",
		       reason.call_index,
		       tbs_term_reason_str(reason.reason));

		if (ccp_cbs && ccp_cbs->termination_reason) {
			ccp_cbs->termination_reason(conn, 0, tbs_inst->index,
						    reason.call_index,
						    reason.reason);
		}
	}
}

static void in_call_notify_handler(struct bt_conn *conn,
				   struct tbs_instance_t *tbs_inst,
				   const void *data, uint16_t length)
{
	const char *uri = parse_string_value(data, length,
					     CONFIG_BT_TBS_MAX_URI_LENGTH);

	BT_DBG("%s", log_strdup(uri));

	if (ccp_cbs && ccp_cbs->remote_uri) {
		ccp_cbs->remote_uri(conn, 0, tbs_inst->index, uri);
	}
}

static void friendly_name_notify_handler(struct bt_conn *conn,
					    struct tbs_instance_t *tbs_inst,
					    const void *data, uint16_t length)
{
	const char *name = parse_string_value(data, length,
					      CONFIG_BT_TBS_MAX_URI_LENGTH);

	BT_DBG("%s", log_strdup(name));

	if (ccp_cbs && ccp_cbs->friendly_name) {
		ccp_cbs->friendly_name(conn, 0, tbs_inst->index, name);
	}
}

/** @brief Handles notifications and indications from the server */
static uint8_t notify_handler(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct tbs_instance_t *tbs_inst = NULL;

	for (int i = 0; i < ARRAY_SIZE(srv_inst.tbs_insts); i++) {
		if (handle <= srv_inst.tbs_insts[i].end_handle &&
		    handle >= srv_inst.tbs_insts[i].start_handle) {
			tbs_inst = &srv_inst.tbs_insts[i];
			break;
		}
	}

	if (!data) {
		BT_DBG("[UNSUBSCRIBED] 0x%04X", params->value_handle);
		params->value_handle = 0U;
		if (tbs_inst) {
			tbs_inst->subscribe_cnt--;
		}

		return BT_GATT_ITER_STOP;
	}

	if (tbs_inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && tbs_inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", tbs_inst->index);
		}
		BT_HEXDUMP_DBG(data, length, "notify handler value");

		if (handle == tbs_inst->provider_name_handle) {
			provider_name_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->technology_handle) {
			technology_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->signal_strength_handle) {
			signal_strength_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->status_flags_handle) {
			status_flags_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->current_calls_handle) {
			current_calls_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->in_uri_handle) {
			incoming_uri_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->call_state_handle) {
			call_state_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->call_cp_handle) {
			call_cp_notify_handler(conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->termination_reason_handle) {
			termination_reason_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->in_call_handle) {
			in_call_notify_handler(
				conn, tbs_inst, data, length);
		} else if (handle == tbs_inst->friendly_name_handle) {
			friendly_name_notify_handler(
				conn, tbs_inst, data, length);
		}
	} else {
		BT_DBG("Notification/Indication on unknown TBS inst");
	}

	return BT_GATT_ITER_CONTINUE;
}

static int ccp_common_call_control(struct bt_conn *conn, uint8_t inst_index,
				   uint8_t call_index, uint8_t opcode)
{	struct tbs_instance_t *inst = get_inst_by_index(inst_index);
	struct bt_tbs_call_cp_acc_t *common;

	if (!inst) {
		return -EINVAL;
	}

	if (!inst->call_cp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	common = (struct bt_tbs_call_cp_acc_t *)inst->write_buf;
	common->opcode = opcode;
	common->call_index = call_index;

	return bt_gatt_write_without_response(conn, inst->call_cp_handle,
					      common, sizeof(*common), false);
}

static uint8_t ccp_read_bearer_provider_name_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);
	const char *provider_name = NULL;

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			provider_name =
				parse_string_value(data, length,
					CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);
			BT_DBG("%s", log_strdup(provider_name));
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->bearer_provider_name) {
			ccp_cbs->bearer_provider_name(conn, err, inst->index,
						      provider_name);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_bearer_uci_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);
	const char *bearer_uci = NULL;

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			bearer_uci = parse_string_value(data, length,
							BT_TBS_MAX_UCI_SIZE);
			BT_DBG("%s", log_strdup(bearer_uci));
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->bearer_uci) {
			ccp_cbs->bearer_uci(conn, err, inst->index, bearer_uci);
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_technology_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t technology = 0;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(technology)) {
				memcpy(&technology, data, length);
				BT_DBG("%s (0x%02x)",
				       tbs_technology_str(technology),
				       technology);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->technology) {
			ccp_cbs->technology(conn, cb_err, inst->index,
					    technology);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_uri_list_cb(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_read_params *params,
				 const void *data, uint16_t length)
{
	const char *uri_scheme_list = NULL;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			uri_scheme_list = parse_string_value(data, length,
						MAX_URI_SCHEME_LIST_SIZE);
			BT_DBG("%s", log_strdup(uri_scheme_list));
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->uri_list) {
			ccp_cbs->uri_list(conn, err, inst->index,
					  uri_scheme_list);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_signal_strength_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t signal_strength = 0;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(signal_strength)) {
				memcpy(&signal_strength, data, length);
				BT_DBG("0x%02x", signal_strength);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->signal_strength) {
			ccp_cbs->signal_strength(conn, cb_err, inst->index,
						 signal_strength);
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_signal_interval_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t signal_interval = 0;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(signal_interval)) {
				memcpy(&signal_interval, data, length);
				BT_DBG("0x%02x", signal_interval);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->signal_interval) {
			ccp_cbs->signal_interval(conn, cb_err, inst->index,
						 signal_interval);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_current_calls_cb(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_read_params *params,
				      const void *data, uint16_t length)
{
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	if (inst) {
		struct bt_ccp_call_t *cb_val = NULL;
		uint8_t cnt = 0;
		struct ccp_read_params *read_params;

		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		read_params = lookup_read_params(params);
		if (err) {
			BT_DBG("err: 0x%02X", err);
			memset(params, 0, sizeof(*params));
			if (ccp_cbs && ccp_cbs->current_calls) {
				ccp_cbs->current_calls(conn, err, inst->index,
						       cnt, cb_val);
			}
			return BT_GATT_ITER_STOP;
		}

		if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");

			if (read_params->bytes_read + length <=
				sizeof(read_params->buf)) {
				memcpy(read_params->buf +
						read_params->bytes_read,
				       data, length);
				read_params->bytes_read += length;
				return BT_GATT_ITER_CONTINUE;
			}

			BT_DBG("Could not read all data, aborting");
			read_params->bytes_read = 0;
			memset(params, 0, sizeof(*params));

			if (ccp_cbs && ccp_cbs->current_calls) {
				err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
				ccp_cbs->current_calls(conn, err, inst->index,
						       cnt, cb_val);
			}
			return BT_GATT_ITER_STOP;
		}

		if (read_params->bytes_read) {
			uint16_t bytes_processed = 0;
			struct bt_ccp_call_t calls[CONFIG_BT_CCP_MAX_CALLS];

			while (bytes_processed < read_params->bytes_read &&
			       cnt < CONFIG_BT_CCP_MAX_CALLS) {
				uint8_t item_len;
				uint8_t uri_len;
				struct bt_ccp_call_t *call;

				call = &calls[cnt];
				memcpy(&item_len,
				       read_params->buf + bytes_processed,
				       sizeof(item_len));
				bytes_processed += sizeof(item_len);
				memcpy(call,
				       read_params->buf + bytes_processed,
				       item_len);
				bytes_processed += item_len;
				uri_len = item_len -
						sizeof(call->index) -
						sizeof(call->state) -
						sizeof(call->flags);

				call->remote_uri[uri_len] = '\0';

				BT_DBG("Call 0x%02x is in the %s state with "
				       "flags 0x%02X and URI %s",
				       call->index, tbs_state_str(call->state),
				       call->flags,
				       log_strdup(call->remote_uri));
				cnt++;
			}

			if (bytes_processed < read_params->bytes_read) {
				BT_DBG("Could not store all calls "
				       "(maximum reached)");
			}
			read_params->bytes_read = 0;

			if (cnt) {
				cb_val = calls;
			}
		}

		memset(params, 0, sizeof(*params));
		if (ccp_cbs && ccp_cbs->current_calls) {
			ccp_cbs->current_calls(conn, 0, inst->index, cnt,
					       cb_val);
		}
	} else {
		memset(params, 0, sizeof(*params));
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_ccid_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_read_params *params,
			     const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t ccid = 0;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(ccid)) {
				memcpy(&ccid, data, length);
				BT_DBG("0x%02x", ccid);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->ccid) {
			ccp_cbs->ccid(conn, cb_err, inst->index, ccid);
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_status_flags_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint16_t status_flags = 0;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(status_flags)) {
				memcpy(&status_flags, data, length);
				BT_DBG("0x%04x", status_flags);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}

		inst->busy = false;

		if (ccp_cbs && ccp_cbs->status_flags) {
			ccp_cbs->status_flags(conn, cb_err, inst->index,
						    status_flags);
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_call_uri_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	const char *in_target_uri = NULL;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			in_target_uri = parse_string_value(
						data, length,
						CONFIG_BT_TBS_MAX_URI_LENGTH);
			BT_DBG("%s", log_strdup(in_target_uri));
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->call_uri) {
			ccp_cbs->call_uri(conn, err, inst->index,
						in_target_uri);
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_call_state_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	BT_DBG("");

	if (inst) {
		struct ccp_read_params *read_params;
		struct bt_ccp_call_state_t *cb_val = NULL;
		uint8_t cnt = 0;

		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		read_params = lookup_read_params(params);
		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");

			if (read_params->bytes_read + length <=
				sizeof(read_params->buf)) {
				memcpy(read_params->buf +
						read_params->bytes_read,
				       data, length);
				read_params->bytes_read += length;
				return BT_GATT_ITER_CONTINUE;
			}

			BT_DBG("Could not read all data, aborting");
			err = BT_ATT_ERR_INSUFFICIENT_RESOURCES;
		} else if ((read_params->bytes_read %
				sizeof(struct bt_ccp_call_state_t)) == 1) {
			BT_DBG("Invalid data length: %zu",
			       read_params->bytes_read);
			err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		} else if (read_params->bytes_read) {
			uint16_t bytes_parsed = 0;

			if (sizeof(inst->calls) < read_params->bytes_read) {
				BT_WARN("Could not store all call states");
			}

			while (bytes_parsed < read_params->bytes_read &&
			       cnt < CONFIG_BT_CCP_MAX_CALLS) {
				struct bt_ccp_call_state_t *call_state;

				call_state = &inst->calls[cnt++];
				memcpy(call_state,
				       read_params->buf + bytes_parsed,
				       sizeof(*call_state));
				bytes_parsed += sizeof(*call_state);
				BT_DBG("Call 0x%02x is in the %s state with "
				       "flags 0x%02X",
				       call_state->index,
				       tbs_state_str(call_state->state),
				       call_state->flags);
			}
			read_params->bytes_read = 0;
		}
		memset(params, 0, sizeof(*params));

		if (ccp_cbs && ccp_cbs->call_state) {

			if (cnt) {
				cb_val = inst->calls;
			}
			ccp_cbs->call_state(conn, err, inst->index, cnt,
					    cb_val);
		}
	} else {
		memset(params, 0, sizeof(*params));
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_optional_opcodes_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint16_t optional_opcodes = 0;
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			BT_HEXDUMP_DBG(data, length, "Data read");
			if (length == sizeof(optional_opcodes)) {
				memcpy(&optional_opcodes, data, length);
				BT_DBG("0x%04x", optional_opcodes);
			} else {
				BT_DBG("Invalid length");
				cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
			}
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->optional_opcodes) {
			ccp_cbs->optional_opcodes(conn, cb_err, inst->index,
						   optional_opcodes);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_remote_uri_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);
	const char *remote_uri = NULL;

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			remote_uri = parse_string_value(
					data, length,
					CONFIG_BT_TBS_MAX_URI_LENGTH);
			BT_DBG("%s", log_strdup(remote_uri));
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->remote_uri) {
			ccp_cbs->remote_uri(conn, err, inst->index, remote_uri);
		}
	}
	return BT_GATT_ITER_STOP;
}

static uint8_t ccp_read_friendly_name_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	struct tbs_instance_t *inst =
		lookup_instance_by_handle(params->single.handle);
	const char *friendly_name = NULL;

	memset(params, 0, sizeof(*params));

	if (inst) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) && inst->gtbs) {
			BT_DBG("GTBS");
		} else {
			BT_DBG("Index %u", inst->index);
		}

		if (err) {
			BT_DBG("err: 0x%02X", err);
		} else if (data) {
			friendly_name =
				parse_string_value(
					data, length,
					CONFIG_BT_TBS_MAX_URI_LENGTH);
			BT_DBG("%s", log_strdup(friendly_name));
		}
		inst->busy = false;

		if (ccp_cbs && ccp_cbs->friendly_name) {
			ccp_cbs->friendly_name(conn, err, inst->index,
					       friendly_name);
		}
	}
	return BT_GATT_ITER_STOP;
}

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t discover_func(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	struct bt_gatt_subscribe_params *sub_params = NULL;
	struct tbs_instance_t *current_inst = srv_inst.current_inst;

	if (!attr) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) &&
		    srv_inst.current_inst->index == GTBS_INDEX) {
			BT_DBG("Setup complete GTBS");
		} else {
			BT_DBG("Setup complete for %u / %u TBS",
			       srv_inst.current_inst->index + 1,
			       srv_inst.inst_cnt);
		}
		(void)memset(params, 0, sizeof(*params));

		if (TBS_INSTANCE_MAX_CNT > 1 &&
		    (((srv_inst.current_inst->index + 1) < srv_inst.inst_cnt) ||
			(IS_ENABLED(CONFIG_BT_CCP_GTBS) &&
			 srv_inst.gtbs_found &&
			 srv_inst.current_inst->index + 1 == GTBS_INDEX))) {
			discover_next_instance(conn, srv_inst.current_inst->index + 1);
		} else {
			srv_inst.current_inst = NULL;
			if (ccp_cbs && ccp_cbs->discover) {
				ccp_cbs->discover(conn, 0, srv_inst.inst_cnt,
						  srv_inst.gtbs_found);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_PROVIDER_NAME)) {
			BT_DBG("Provider name");
			current_inst->provider_name_handle = chrc->value_handle;
			sub_params = &current_inst->name_sub_params;
			sub_params->disc_params = &current_inst->name_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_UCI)) {
			BT_DBG("Bearer UCI");
			srv_inst.current_inst->bearer_uci_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_TECHNOLOGY)) {
			BT_DBG("Technology");
			current_inst->technology_handle = chrc->value_handle;
			sub_params = &current_inst->technology_sub_params;
			sub_params->disc_params = &current_inst->technology_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_TBS_URI_LIST)) {
			BT_DBG("URI Scheme List");
			srv_inst.current_inst->uri_list_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_SIGNAL_STRENGTH)) {
			BT_DBG("Signal strength");
			current_inst->signal_strength_handle =
				chrc->value_handle;
			sub_params = &current_inst->signal_strength_sub_params;
			sub_params->disc_params = &current_inst->signal_strength_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_SIGNAL_INTERVAL)) {
			BT_DBG("Signal strength reporting interval");
			srv_inst.current_inst->signal_interval_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_LIST_CURRENT_CALLS)) {
			BT_DBG("Current calls");
			current_inst->current_calls_handle = chrc->value_handle;
			sub_params = &current_inst->current_calls_sub_params;
			sub_params->disc_params = &current_inst->current_calls_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_CCID)) {
			BT_DBG("CCID");
			srv_inst.current_inst->ccid_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_STATUS_FLAGS)) {
			BT_DBG("Status flags");
			current_inst->status_flags_handle =
				chrc->value_handle;
			sub_params = &current_inst->status_flags_sub_params;
			sub_params->disc_params = &current_inst->status_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_INCOMING_URI)) {
			BT_DBG("Incoming target URI");
			current_inst->in_uri_handle = chrc->value_handle;
			sub_params = &current_inst->in_target_uri_sub_params;
			sub_params->disc_params = &current_inst->in_target_uri_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_CALL_STATE)) {
			BT_DBG("Call state");
			current_inst->call_state_handle = chrc->value_handle;
			sub_params = &current_inst->call_state_sub_params;
			sub_params->disc_params = &current_inst->call_state_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_CALL_CONTROL_POINT)) {
			BT_DBG("Call control point");
			current_inst->call_cp_handle = chrc->value_handle;
			sub_params = &current_inst->call_cp_sub_params;
			sub_params->disc_params = &current_inst->call_cp_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_OPTIONAL_OPCODES)) {
			BT_DBG("Supported opcodes");
			srv_inst.current_inst->optional_opcodes_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_TERMINATE_REASON)) {
			BT_DBG("Termination reason");
			current_inst->termination_reason_handle =
				chrc->value_handle;
			sub_params = &current_inst->termination_sub_params;
			sub_params->disc_params = &current_inst->termination_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_FRIENDLY_NAME)) {
			BT_DBG("Incoming friendly name");
			current_inst->friendly_name_handle =
				chrc->value_handle;
			sub_params = &current_inst->friendly_name_sub_params;
			sub_params->disc_params = &current_inst->friendly_name_sub_disc_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_TBS_INCOMING_CALL)) {
			BT_DBG("Incoming call");
			current_inst->in_call_handle =
				chrc->value_handle;
			sub_params = &current_inst->incoming_call_sub_params;
			sub_params->disc_params = &current_inst->incoming_call_sub_disc_params;
		}

		if (srv_inst.subscribe_all && sub_params) {
			sub_params->value = 0;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				sub_params->value = BT_GATT_CCC_NOTIFY;
			} else if (chrc->properties & BT_GATT_CHRC_INDICATE) {
				sub_params->value = BT_GATT_CCC_INDICATE;
			}

			if (sub_params->value) {
				int err;

				/* Setting ccc_handle = will use auto discovery feature */
				sub_params->ccc_handle = 0;
				sub_params->end_handle = current_inst->end_handle;
				sub_params->value_handle = chrc->value_handle;
				sub_params->notify = notify_handler;
				err = bt_gatt_subscribe(conn, sub_params);
				if (err) {
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

	srv_inst.current_inst = &srv_inst.tbs_insts[index];
	memset(&srv_inst.discover_params, 0, sizeof(srv_inst.discover_params));
	srv_inst.discover_params.uuid = NULL;
	srv_inst.discover_params.start_handle = srv_inst.current_inst->start_handle;
	srv_inst.discover_params.end_handle = srv_inst.current_inst->end_handle;
	srv_inst.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	srv_inst.discover_params.func = discover_func;

	err = bt_gatt_discover(conn, &srv_inst.discover_params);
	if (err) {
		BT_DBG("Discover failed (err %d)", err);
		srv_inst.current_inst = NULL;
		if (ccp_cbs && ccp_cbs->discover) {
			ccp_cbs->discover(conn, err, srv_inst.inst_cnt, srv_inst.gtbs_found);
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
	struct bt_gatt_service_val *prim_service;
	/*
	 * TODO: Since we know the ranges of each instance, we could do
	 * discover of more than just prim_service->start and
	 * prim_service->end_handle, so that we can detect multiple instances
	 * faster
	 */

	if (!attr || srv_inst.inst_cnt == TBS_INSTANCE_MAX_CNT) {
		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) &&
		    !bt_uuid_cmp(params->uuid, BT_UUID_GTBS)) {
			int err;
			/* Didn't find GTBS; look for TBS */
			params->uuid = tbs_uuid;
			params->start_handle = FIRST_HANDLE;

			err = bt_gatt_discover(conn, params);

			if (err) {
				BT_DBG("Discover failed (err %d)", err);
				srv_inst.current_inst = NULL;
				if (ccp_cbs && ccp_cbs->discover) {
					ccp_cbs->discover(conn, err, 0, false);
				}
			}
			return BT_GATT_ITER_STOP;
		}

		if (IS_ENABLED(CONFIG_BT_CCP_GTBS)) {
			srv_inst.gtbs_found = srv_inst.tbs_insts[GTBS_INDEX].gtbs;
			BT_DBG("Discover complete, found %u instances "
			       "(GTBS%s found)",
				srv_inst.inst_cnt,
				srv_inst.gtbs_found ? "" : " not");
		} else {
			BT_DBG("Discover complete, found %u instances",
			       srv_inst.inst_cnt);
		}

		if (srv_inst.inst_cnt) {
			discover_next_instance(conn, 0);
		} else if (IS_ENABLED(CONFIG_BT_CCP_GTBS) &&
			   srv_inst.gtbs_found) {
			discover_next_instance(conn, GTBS_INDEX);
		} else {
			srv_inst.current_inst = NULL;
			if (ccp_cbs && ccp_cbs->discover) {
				ccp_cbs->discover(conn, 0, srv_inst.inst_cnt,
						  srv_inst.gtbs_found);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		params->start_handle = attr->handle + 1;

		srv_inst.current_inst = &srv_inst.tbs_insts[srv_inst.inst_cnt];
		srv_inst.current_inst->index = srv_inst.inst_cnt;

		if (IS_ENABLED(CONFIG_BT_CCP_GTBS) &&
		    !bt_uuid_cmp(params->uuid, BT_UUID_GTBS)) {
			int err;

			/* GTBS is placed as the "last" instance */
			srv_inst.current_inst = &srv_inst.tbs_insts[GTBS_INDEX];
			srv_inst.current_inst->index = GTBS_INDEX;
			srv_inst.current_inst->gtbs = true;
			srv_inst.current_inst->start_handle = attr->handle + 1;
			srv_inst.current_inst->end_handle = prim_service->end_handle;

			params->uuid = tbs_uuid;
			params->start_handle = FIRST_HANDLE;

			err = bt_gatt_discover(conn, params);
			if (err) {
				BT_DBG("Discover failed (err %d)", err);
				srv_inst.current_inst = NULL;
				if (ccp_cbs && ccp_cbs->discover) {
					ccp_cbs->discover(conn, err, 0, false);
				}
			}
			return BT_GATT_ITER_STOP;
		}
		srv_inst.current_inst->start_handle = attr->handle + 1;
		srv_inst.current_inst->end_handle = prim_service->end_handle;
		srv_inst.inst_cnt++;
	}

	return BT_GATT_ITER_CONTINUE;
}

/****************************** PUBLIC API ******************************/

int bt_ccp_hold_call(struct bt_conn *conn, uint8_t inst_index,
		     uint8_t call_index)
{
	return ccp_common_call_control(conn, inst_index, call_index,
				       BT_TBS_CALL_OPCODE_HOLD);
}

int bt_ccp_accept_call(struct bt_conn *conn, uint8_t inst_index,
		       uint8_t call_index)
{
	return ccp_common_call_control(conn, inst_index, call_index,
				       BT_TBS_CALL_OPCODE_ACCEPT);
}

int bt_ccp_retrieve_call(struct bt_conn *conn, uint8_t inst_index,
			 uint8_t call_index)
{
	return ccp_common_call_control(conn, inst_index, call_index,
				       BT_TBS_CALL_OPCODE_RETRIEVE);
}

int bt_ccp_terminate_call(struct bt_conn *conn, uint8_t inst_index,
			  uint8_t call_index)
{
	return ccp_common_call_control(conn, inst_index, call_index,
				       BT_TBS_CALL_OPCODE_TERMINATE);
}

int bt_ccp_originate_call(struct bt_conn *conn, uint8_t inst_index,
			  const char *uri)
{
	struct tbs_instance_t *inst;
	struct bt_tbs_call_cp_originate_t *originate;
	size_t uri_len;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	} else if (!tbs_valid_uri(uri)) {
		BT_DBG("Invalid URI: %s", log_strdup(uri));
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	/* Check if there are free spots */
	if (!free_call_spot(inst)) {
		BT_DBG("Cannot originate more calls");
		return -ENOMEM;
	}

	uri_len = strlen(uri);
	originate = (struct bt_tbs_call_cp_originate_t *)inst->write_buf;
	originate->opcode = BT_TBS_CALL_OPCODE_ORIGINATE;
	memcpy(originate->uri, uri, uri_len);

	return bt_gatt_write_without_response(conn, inst->call_cp_handle,
					      originate,
					      sizeof(*originate) + uri_len,
					      false);
}

int bt_ccp_join_calls(struct bt_conn *conn, uint8_t inst_index,
		      const uint8_t *call_indexes, uint8_t count)
{
	if (!conn) {
		return -ENOTCONN;
	}

	/* Write to call control point */
	if (call_indexes && count > 1 && count <= CONFIG_BT_CCP_MAX_CALLS) {
		struct tbs_instance_t *inst;
		struct bt_tbs_call_cp_join_t *join;

		inst = get_inst_by_index(inst_index);
		if (!inst) {
			return -EINVAL;
		} else if (!inst->call_cp_handle) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		join = (struct bt_tbs_call_cp_join_t *)inst->write_buf;

		join->opcode = BT_TBS_CALL_OPCODE_JOIN;
		memcpy(join->call_indexes, call_indexes, count);

		return bt_gatt_write_without_response(
			conn, inst->call_cp_handle, join,
			sizeof(*join) + count, false);
	}
	return -EINVAL;
}

int bt_ccp_set_signal_strength_interval(struct bt_conn *conn,
					uint8_t inst_index, uint8_t interval)
{
	struct tbs_instance_t *inst;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);
	/* Populate Outgoing Remote URI */
	if (!inst->signal_interval_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	return bt_gatt_write_without_response(conn,
					      inst->signal_interval_handle,
					      &interval, sizeof(interval),
					      false);
}

int bt_ccp_read_bearer_provider_name(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->provider_name_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_bearer_provider_name_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->provider_name_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_bearer_uci(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->bearer_uci_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_bearer_uci_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->bearer_uci_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_technology(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->technology_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_technology_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->technology_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_uri_list(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->uri_list_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_uri_list_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->uri_list_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_signal_strength(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->signal_strength_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_signal_strength_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->signal_strength_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_signal_interval(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->signal_interval_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_signal_interval_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->signal_interval_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_current_calls(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->current_calls_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->bytes_read = 0;
	read_params->params.func = ccp_read_current_calls_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->current_calls_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_ccid(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->ccid_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_ccid_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->ccid_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_status_flags(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->status_flags_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_status_flags_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->status_flags_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_call_uri(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->in_uri_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_call_uri_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->in_uri_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_call_state(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->call_state_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_call_state_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->call_state_handle;
	read_params->params.single.offset = 0U;
	read_params->bytes_read = 0;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_optional_opcodes(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->optional_opcodes_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_optional_opcodes_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->optional_opcodes_handle;
	read_params->params.single.offset = 0U;
	read_params->bytes_read = 0;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}
int bt_ccp_read_remote_uri(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->in_call_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_remote_uri_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->in_call_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_read_friendly_name(struct bt_conn *conn, uint8_t inst_index)
{
	int err;
	struct tbs_instance_t *inst;
	struct ccp_read_params *read_params;

	if (!conn) {
		return -ENOTCONN;
	} else if (!valid_inst_index(inst_index)) {
		return -EINVAL;
	}

	inst = get_inst_by_index(inst_index);

	if (!inst->friendly_name_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	read_params = get_read_params();
	if (!read_params) {
		return -EBUSY;
	}

	read_params->params.func = ccp_read_friendly_name_cb;
	read_params->params.handle_count = 1;
	read_params->params.single.handle = inst->friendly_name_handle;
	read_params->params.single.offset = 0U;

	err = bt_gatt_read(conn, &read_params->params);
	if (err) {
		memset(read_params, 0, sizeof(*read_params));
	}
	return err;
}

int bt_ccp_discover(struct bt_conn *conn, bool subscribe)
{
	if (!conn) {
		return -ENOTCONN;
	} else if (srv_inst.current_inst) {
		return -EBUSY;
	}

	memset(srv_inst.tbs_insts, 0, sizeof(srv_inst.tbs_insts)); /* reset data */
	srv_inst.inst_cnt = 0;
	srv_inst.gtbs_found = false;
	/* Discover TBS on peer, setup handles and notify/indicate */
	srv_inst.subscribe_all = subscribe;
	(void)memset(&srv_inst.discover_params, 0, sizeof(srv_inst.discover_params));
	if (IS_ENABLED(CONFIG_BT_CCP_GTBS)) {
		BT_DBG("Discovering GTBS");
		srv_inst.discover_params.uuid = gtbs_uuid;
	} else {
		srv_inst.discover_params.uuid = tbs_uuid;
	}
	srv_inst.discover_params.func = primary_discover_func;
	srv_inst.discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	srv_inst.discover_params.start_handle = FIRST_HANDLE;
	srv_inst.discover_params.end_handle = LAST_HANDLE;
	return bt_gatt_discover(conn, &srv_inst.discover_params);
}

void bt_ccp_register_cb(struct bt_ccp_cb_t *cb)
{
	ccp_cbs = cb;
}
