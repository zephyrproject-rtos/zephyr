/* btp_ccp.c - Bluetooth CCP Tester */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "btp/btp.h"
#include "zephyr/sys/byteorder.h"
#include <stdint.h>

#include <../../subsys/bluetooth/audio/tbs_internal.h>

#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME bttester_ccp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

struct btp_ccp_chrc_handles_ev tbs_handles;
struct bt_tbs_instance *tbs_inst;
static uint8_t call_index;
static uint8_t inst_ccid;
static bool send_ev;

static uint8_t ccp_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_ccp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_CCP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static void tbs_client_discovered_ev(int err, uint8_t tbs_count, bool gtbs_found)
{
	struct btp_ccp_discovered_ev ev;

	ev.status = sys_cpu_to_le32(err);
	ev.tbs_count = tbs_count;
	ev.gtbs_found = gtbs_found;

	tester_event(BTP_SERVICE_ID_CCP, BTP_CCP_EV_DISCOVERED, &ev, sizeof(ev));
}

static void tbs_chrc_handles_ev(struct btp_ccp_chrc_handles_ev *tbs_handles)
{
	struct btp_ccp_chrc_handles_ev ev;

	ev.provider_name = sys_cpu_to_le16(tbs_handles->provider_name);
	ev.bearer_uci = sys_cpu_to_le16(tbs_handles->bearer_uci);
	ev.bearer_technology = sys_cpu_to_le16(tbs_handles->bearer_technology);
	ev.uri_list = sys_cpu_to_le16(tbs_handles->uri_list);
	ev.signal_strength = sys_cpu_to_le16(tbs_handles->signal_strength);
	ev.signal_interval = sys_cpu_to_le16(tbs_handles->signal_interval);
	ev.current_calls = sys_cpu_to_le16(tbs_handles->current_calls);
	ev.ccid = sys_cpu_to_le16(tbs_handles->ccid);
	ev.status_flags = sys_cpu_to_le16(tbs_handles->status_flags);
	ev.bearer_uri = sys_cpu_to_le16(tbs_handles->bearer_uri);
	ev.call_state = sys_cpu_to_le16(tbs_handles->call_state);
	ev.control_point = sys_cpu_to_le16(tbs_handles->control_point);
	ev.optional_opcodes = sys_cpu_to_le16(tbs_handles->optional_opcodes);
	ev.termination_reason = sys_cpu_to_le16(tbs_handles->termination_reason);
	ev.incoming_call = sys_cpu_to_le16(tbs_handles->incoming_call);
	ev.friendly_name = sys_cpu_to_le16(tbs_handles->friendly_name);

	tester_event(BTP_SERVICE_ID_CCP, BTP_CCP_EV_CHRC_HANDLES, &ev, sizeof(ev));
}

static void tbs_client_chrc_val_ev(struct bt_conn *conn, uint8_t status, uint8_t inst_index,
				   uint32_t value)
{
	struct btp_ccp_chrc_val_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.inst_index = inst_index;
	ev.value = value;

	tester_event(BTP_SERVICE_ID_CCP, BTP_CCP_EV_CHRC_VAL, &ev, sizeof(ev));
}

static void tbs_client_chrc_str_ev(struct bt_conn *conn, uint8_t status, uint8_t inst_index,
				   uint8_t data_len, const char *data)
{
	struct btp_ccp_chrc_str_ev *ev;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(sizeof(*ev) + data_len, (uint8_t **)&ev);
	bt_addr_le_copy(&ev->address, bt_conn_get_dst(conn));
	ev->status = status;
	ev->inst_index = inst_index;
	ev->data_len = data_len;
	memcpy(ev->data, data, data_len);

	tester_event(BTP_SERVICE_ID_CCP, BTP_CCP_EV_CHRC_STR, ev, sizeof(*ev) + data_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void tbs_client_cp_ev(struct bt_conn *conn, uint8_t status)
{
	struct btp_ccp_cp_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;

	tester_event(BTP_SERVICE_ID_CCP, BTP_CCP_EV_CP, &ev, sizeof(ev));
}

static void tbs_client_current_calls_ev(struct bt_conn *conn, uint8_t status)
{
	struct btp_ccp_current_calls_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;

	tester_event(BTP_SERVICE_ID_CCP, BTP_CCP_EV_CURRENT_CALLS, &ev, sizeof(ev));
}

static void tbs_client_discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count,
				   bool gtbs_found)
{
	if (err) {
		LOG_DBG("Discovery Failed (%d)", err);
		return;
	}

	LOG_DBG("Discovered TBS - err (%u) GTBS (%u)", err, gtbs_found);

	bt_tbs_client_read_ccid(conn, 0xFF);

	tbs_client_discovered_ev(err, tbs_count, gtbs_found);

	send_ev = true;
}

typedef struct bt_tbs_client_call_state bt_tbs_client_call_state_t;

#define CALL_STATES_EV_SIZE sizeof(struct btp_ccp_call_states_ev) + \
			    sizeof(bt_tbs_client_call_state_t) * \
			    CONFIG_BT_TBS_CLIENT_MAX_CALLS

static void tbs_client_call_states_ev(int err,
				      uint8_t inst_index,
				      uint8_t call_count,
				      const bt_tbs_client_call_state_t *call_states)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(CALL_STATES_EV_SIZE);
	struct btp_ccp_call_states_ev ev = {
		sys_cpu_to_le32(err), inst_index, call_count
	};

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_mem(buf, &ev, sizeof(ev));

	for (uint8_t n = 0; n < call_count; n++, call_states++) {
		net_buf_simple_add_mem(buf, call_states, sizeof(bt_tbs_client_call_state_t));
	}

	tester_event(BTP_SERVICE_ID_CCP, BTP_CCP_EV_CALL_STATES, buf->data, buf->len);
}

static void tbs_client_call_states_cb(struct bt_conn *conn,
				      int err,
				      uint8_t inst_index,
				      uint8_t call_count,
				      const bt_tbs_client_call_state_t *call_states)
{
	LOG_DBG("Call states - err (%u) Call Count (%u)", err, call_count);

	tbs_client_call_states_ev(err, inst_index, call_count, call_states);
}

static void tbs_client_termination_reason_cb(struct bt_conn *conn,
					     int err,
					     uint8_t inst_index,
					     uint8_t call_index,
					     uint8_t reason)
{
	LOG_DBG("Termination reason - err (%u) Call Index (%u) Reason (%u)",
		err, call_index, reason);
}

static void tbs_client_read_string_cb(struct bt_conn *conn, int err, uint8_t inst_index,
				      const char *value)
{
	LOG_DBG("TBS Client read string characteristic value cb");

	uint8_t data_len = strlen(value);

	tbs_client_chrc_str_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, inst_index,
			       data_len, value);
}

static void tbs_client_read_val_cb(struct bt_conn *conn, int err, uint8_t inst_index,
				   uint32_t value)
{
	LOG_DBG("TBS Client read characteristic value cb");

	tbs_client_chrc_val_ev(conn, err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS, inst_index,
			       value);

	if (send_ev == true) {
		inst_ccid = value;

		tbs_inst = bt_tbs_client_get_by_ccid(conn, inst_ccid);

		tbs_handles.provider_name = tbs_inst->name_sub_params.value_handle;
		tbs_handles.bearer_uci = tbs_inst->bearer_uci_handle;
		tbs_handles.bearer_technology = tbs_inst->technology_sub_params.value_handle;
		tbs_handles.uri_list = tbs_inst->uri_list_handle;
		tbs_handles.signal_strength = tbs_inst->signal_strength_sub_params.value_handle;
		tbs_handles.signal_interval = tbs_inst->signal_interval_handle;
		tbs_handles.current_calls = tbs_inst->current_calls_sub_params.value_handle;
		tbs_handles.ccid = tbs_inst->ccid_handle;
		tbs_handles.status_flags = tbs_inst->status_flags_sub_params.value_handle;
		tbs_handles.bearer_uri = tbs_inst->in_target_uri_sub_params.value_handle;
		tbs_handles.call_state = tbs_inst->call_state_sub_params.value_handle;
		tbs_handles.control_point = tbs_inst->call_cp_sub_params.value_handle;
		tbs_handles.optional_opcodes = tbs_inst->optional_opcodes_handle;
		tbs_handles.termination_reason = tbs_inst->termination_reason_handle;
		tbs_handles.incoming_call = tbs_inst->incoming_call_sub_params.value_handle;
		tbs_handles.friendly_name = tbs_inst->friendly_name_sub_params.value_handle;

		tbs_chrc_handles_ev(&tbs_handles);
		send_ev = false;
	}
}

static void tbs_client_current_calls_cb(struct bt_conn *conn, int err, uint8_t inst_index,
					uint8_t call_count, const struct bt_tbs_client_call *calls)
{
	LOG_DBG("");

	tbs_client_current_calls_ev(conn, err);
}

static void tbs_client_cp_cb(struct bt_conn *conn, int err, uint8_t inst_index, uint8_t call_index)
{
	LOG_DBG("");

	tbs_client_cp_ev(conn, err);
}

static struct bt_tbs_client_cb tbs_client_callbacks = {
	.discover = tbs_client_discover_cb,
	.originate_call = tbs_client_cp_cb,
	.terminate_call = tbs_client_cp_cb,
	.call_state = tbs_client_call_states_cb,
	.termination_reason = tbs_client_termination_reason_cb,
	.bearer_provider_name = tbs_client_read_string_cb,
	.bearer_uci = tbs_client_read_string_cb,
	.technology = tbs_client_read_val_cb,
	.uri_list = tbs_client_read_string_cb,
	.signal_strength = tbs_client_read_val_cb,
	.signal_interval = tbs_client_read_val_cb,
	.current_calls = tbs_client_current_calls_cb,
	.ccid = tbs_client_read_val_cb,
	.call_uri = tbs_client_read_string_cb,
	.status_flags = tbs_client_read_val_cb,
	.optional_opcodes = tbs_client_read_val_cb,
	.friendly_name = tbs_client_read_string_cb,
	.remote_uri = tbs_client_read_string_cb,
	.accept_call = tbs_client_cp_cb,
	.hold_call = tbs_client_cp_cb,
	.retrieve_call = tbs_client_cp_cb,
	.join_calls = tbs_client_cp_cb,
};

static uint8_t ccp_discover_tbs(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_discover_tbs_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	err = (conn) ? bt_tbs_client_discover(conn) : -ENOTCONN;
	if (conn) {
		bt_conn_unref(conn);
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t ccp_accept_call(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_accept_call_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	err = (conn) ? bt_tbs_client_accept_call(conn, cp->inst_index, cp->call_id) : -ENOTCONN;
	if (conn) {
		bt_conn_unref(conn);
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t ccp_terminate_call(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_terminate_call_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	err = (conn) ? bt_tbs_client_terminate_call(conn, cp->inst_index, cp->call_id) :
		-ENOTCONN;
	if (conn) {
		bt_conn_unref(conn);
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t ccp_originate_call(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_originate_call_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	err = (conn) ? bt_tbs_client_originate_call(conn, cp->inst_index, cp->uri) : -ENOTCONN;
	if (conn) {
		bt_conn_unref(conn);
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t ccp_read_call_state(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_read_call_state_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	err = (conn) ? bt_tbs_client_read_call_state(conn, cp->inst_index) : -ENOTCONN;
	if (conn) {
		bt_conn_unref(conn);
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t ccp_read_bearer_name(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_ccp_read_bearer_name_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_bearer_provider_name(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_bearer_uci(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_ccp_read_bearer_uci_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_bearer_uci(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_bearer_tech(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_ccp_read_bearer_technology_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_technology(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_uri_list(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_read_uri_list_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_uri_list(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_signal_strength(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_ccp_read_signal_strength_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_signal_strength(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_signal_interval(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_ccp_read_signal_interval_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_signal_interval(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_current_calls(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	const struct btp_ccp_read_current_calls_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_current_calls(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_ccid(const void *cmd, uint16_t cmd_len, void *rsp,
			     uint16_t *rsp_len)
{
	const struct btp_ccp_read_ccid_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_ccid(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_call_uri(const void *cmd, uint16_t cmd_len, void *rsp,
				 uint16_t *rsp_len)
{
	const struct btp_ccp_read_call_uri_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_call_uri(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_status_flags(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	const struct btp_ccp_read_status_flags_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_status_flags(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_optional_opcodes(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_ccp_read_optional_opcodes_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_optional_opcodes(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_friendly_name(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_ccp_read_friendly_name_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_friendly_name(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_read_remote_uri(const void *cmd, uint16_t cmd_len, void *rsp,
				   uint16_t *rsp_len)
{
	const struct btp_ccp_read_remote_uri_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_read_remote_uri(conn, cp->inst_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_set_signal_interval(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_ccp_set_signal_interval_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_set_signal_strength_interval(conn, cp->inst_index, cp->interval);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_hold_call(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_hold_call_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_hold_call(conn, cp->inst_index, cp->call_id);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_retrieve_call(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_retrieve_call_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_client_retrieve_call(conn, cp->inst_index, cp->call_id);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ccp_join_calls(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_ccp_join_calls_cmd *cp = cmd;
	const uint8_t *call_index;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	call_index = cp->call_index;

	err = bt_tbs_client_join_calls(conn, cp->inst_index, call_index, cp->count);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler ccp_handlers[] = {
	{
		.opcode = BTP_CCP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = ccp_supported_commands
	},
	{
		.opcode = BTP_CCP_DISCOVER_TBS,
		.expect_len = sizeof(struct btp_ccp_discover_tbs_cmd),
		.func = ccp_discover_tbs
	},
	{
		.opcode = BTP_CCP_ACCEPT_CALL,
		.expect_len = sizeof(struct btp_ccp_accept_call_cmd),
		.func = ccp_accept_call
	},
	{
		.opcode = BTP_CCP_TERMINATE_CALL,
		.expect_len = sizeof(struct btp_ccp_terminate_call_cmd),
		.func = ccp_terminate_call
	},
	{
		.opcode = BTP_CCP_ORIGINATE_CALL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = ccp_originate_call
	},
	{
		.opcode = BTP_CCP_READ_CALL_STATE,
		.expect_len = sizeof(struct btp_ccp_read_call_state_cmd),
		.func = ccp_read_call_state
	},
	{
		.opcode = BTP_CCP_READ_BEARER_NAME,
		.expect_len = sizeof(struct btp_ccp_read_bearer_name_cmd),
		.func = ccp_read_bearer_name
	},
	{
		.opcode = BTP_CCP_READ_BEARER_UCI,
		.expect_len = sizeof(struct btp_ccp_read_bearer_uci_cmd),
		.func = ccp_read_bearer_uci
	},
	{
		.opcode = BTP_CCP_READ_BEARER_TECH,
		.expect_len = sizeof(struct btp_ccp_read_bearer_technology_cmd),
		.func = ccp_read_bearer_tech
	},
	{
		.opcode = BTP_CCP_READ_URI_LIST,
		.expect_len = sizeof(struct btp_ccp_read_uri_list_cmd),
		.func = ccp_read_uri_list
	},
	{
		.opcode = BTP_CCP_READ_SIGNAL_STRENGTH,
		.expect_len = sizeof(struct btp_ccp_read_signal_strength_cmd),
		.func = ccp_read_signal_strength
	},
	{
		.opcode = BTP_CCP_READ_SIGNAL_INTERVAL,
		.expect_len = sizeof(struct btp_ccp_read_signal_interval_cmd),
		.func = ccp_read_signal_interval
	},
	{
		.opcode = BTP_CCP_READ_CURRENT_CALLS,
		.expect_len = sizeof(struct btp_ccp_read_current_calls_cmd),
		.func = ccp_read_current_calls
	},
	{
		.opcode = BTP_CCP_READ_CCID,
		.expect_len = sizeof(struct btp_ccp_read_ccid_cmd),
		.func = ccp_read_ccid
	},
	{
		.opcode = BTP_CCP_READ_CALL_URI,
		.expect_len = sizeof(struct btp_ccp_read_call_uri_cmd),
		.func = ccp_read_call_uri
	},
	{
		.opcode = BTP_CCP_READ_STATUS_FLAGS,
		.expect_len = sizeof(struct btp_ccp_read_status_flags_cmd),
		.func = ccp_read_status_flags
	},
	{
		.opcode = BTP_CCP_READ_OPTIONAL_OPCODES,
		.expect_len = sizeof(struct btp_ccp_read_optional_opcodes_cmd),
		.func = ccp_read_optional_opcodes
	},
	{
		.opcode = BTP_CCP_READ_FRIENDLY_NAME,
		.expect_len = sizeof(struct btp_ccp_read_friendly_name_cmd),
		.func = ccp_read_friendly_name
	},
	{
		.opcode = BTP_CCP_READ_REMOTE_URI,
		.expect_len = sizeof(struct btp_ccp_read_remote_uri_cmd),
		.func = ccp_read_remote_uri
	},
	{
		.opcode = BTP_CCP_SET_SIGNAL_INTERVAL,
		.expect_len = sizeof(struct btp_ccp_set_signal_interval_cmd),
		.func = ccp_set_signal_interval
	},
	{
		.opcode = BTP_CCP_HOLD_CALL,
		.expect_len = sizeof(struct btp_ccp_hold_call_cmd),
		.func = ccp_hold_call
	},
	{
		.opcode = BTP_CCP_RETRIEVE_CALL,
		.expect_len = sizeof(struct btp_ccp_retrieve_call_cmd),
		.func = ccp_retrieve_call
	},
	{
		.opcode = BTP_CCP_JOIN_CALLS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = ccp_join_calls
	},
};

uint8_t tester_init_ccp(void)
{
	int err;

	tester_register_command_handlers(BTP_SERVICE_ID_CCP, ccp_handlers,
					 ARRAY_SIZE(ccp_handlers));

	err = bt_tbs_client_register_cb(&tbs_client_callbacks);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ccp(void)
{
	return BTP_STATUS_SUCCESS;
}

/* Telephone Bearer Service */
static uint8_t tbs_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	struct btp_tbs_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_TBS, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_remote_incoming(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_tbs_remote_incoming_cmd *cp = cmd;
	char friendly_name[CONFIG_BT_TBS_MAX_URI_LENGTH];
	char caller_uri[CONFIG_BT_TBS_MAX_URI_LENGTH];
	char recv_uri[CONFIG_BT_TBS_MAX_URI_LENGTH];
	int err;

	LOG_DBG("");

	if ((cp->recv_len >= sizeof(recv_uri) || cp->caller_len >= sizeof(caller_uri)) ||
	     cp->fn_len >= sizeof(friendly_name)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(recv_uri, cp->data, cp->recv_len);
	memcpy(caller_uri, cp->data + cp->recv_len, cp->caller_len);
	memcpy(friendly_name, cp->data + cp->recv_len + cp->caller_len, cp->fn_len);

	recv_uri[cp->recv_len] = '\0';
	caller_uri[cp->caller_len] = '\0';
	friendly_name[cp->fn_len] = '\0';

	err = bt_tbs_remote_incoming(cp->index, recv_uri, caller_uri, friendly_name);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_originate(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_tbs_originate_cmd *cp = cmd;
	char uri[CONFIG_BT_TBS_MAX_URI_LENGTH];
	int err;

	LOG_DBG("TBS Originate Call");

	if (cp->uri_len >= sizeof(uri)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(uri, cp->uri, cp->uri_len);
	uri[cp->uri_len] = '\0';

	err = bt_tbs_originate(cp->index, uri, &call_index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_hold(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_tbs_hold_cmd *cp = cmd;
	int err;

	LOG_DBG("TBS Hold Call");

	err = bt_tbs_hold(cp->index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_remote_hold(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_tbs_remote_hold_cmd *cp = cmd;
	int err;

	LOG_DBG("TBS Remote Hold Call");

	err = bt_tbs_remote_hold(cp->index);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_set_bearer_name(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_tbs_set_bearer_name_cmd *cp = cmd;
	char bearer_name[CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH];
	int err;

	LOG_DBG("TBS Set Bearer Provider Name");

	if (cp->name_len >= sizeof(bearer_name)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(bearer_name, cp->name, cp->name_len);
	bearer_name[cp->name_len] = '\0';

	err = bt_tbs_set_bearer_provider_name(cp->index, bearer_name);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_set_bearer_technology(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_tbs_set_technology_cmd *cp = cmd;
	int err;

	LOG_DBG("TBS Set bearer technology");

	err = bt_tbs_set_bearer_technology(cp->index, cp->tech);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_set_uri_scheme_list(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_tbs_set_uri_schemes_list_cmd *cp = cmd;
	char uri_list[CONFIG_BT_TBS_MAX_SCHEME_LIST_LENGTH];
	char *uri_ptr = (char *)&uri_list;
	int err;

	LOG_DBG("TBS Set Uri Scheme list");

	if (cp->uri_len >= sizeof(uri_list)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(uri_list, cp->uri_list, cp->uri_len);
	uri_list[cp->uri_len] = '\0';

	if (cp->uri_count > 1) {
		/* TODO: currently supporting only one uri*/
		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_set_uri_scheme_list(cp->index, (const char **)&uri_ptr, cp->uri_count);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_set_status_flags(const void *cmd, uint16_t cmd_len, void *rsp,
				    uint16_t *rsp_len)
{
	const struct btp_tbs_set_status_flags_cmd *cp = cmd;
	uint16_t flags = sys_le16_to_cpu(cp->flags);
	int err;

	LOG_DBG("TBS Set Status Flags");

	err = bt_tbs_set_status_flags(cp->index, flags);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t tbs_set_signal_strength(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	const struct btp_tbs_set_signal_strength_cmd *cp = cmd;
	int err;

	LOG_DBG("TBS Set Signal Strength");

	err = bt_tbs_set_signal_strength(cp->index, cp->strength);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static bool btp_tbs_originate_call_cb(struct bt_conn *conn, uint8_t call_index, const char *uri)
{
	LOG_DBG("TBS Originate Call cb");

	return true;
}

static void btp_tbs_call_change_cb(struct bt_conn *conn, uint8_t call_index)
{
	LOG_DBG("TBS Call Status Changed cb");
}

static struct bt_tbs_cb tbs_cbs = {
	.originate_call = btp_tbs_originate_call_cb,
	.hold_call = btp_tbs_call_change_cb,
};

static const struct btp_handler tbs_handlers[] = {
	{
		.opcode = BTP_TBS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = tbs_supported_commands,
	},
	{
		.opcode = BTP_TBS_REMOTE_INCOMING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = tbs_remote_incoming,
	},
	{
		.opcode = BTP_TBS_HOLD,
		.expect_len = sizeof(struct btp_tbs_hold_cmd),
		.func = tbs_hold,
	},
	{
		.opcode = BTP_TBS_SET_BEARER_NAME,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = tbs_set_bearer_name,
	},
	{
		.opcode = BTP_TBS_SET_TECHNOLOGY,
		.expect_len = sizeof(struct btp_tbs_set_technology_cmd),
		.func = tbs_set_bearer_technology,
	},
	{
		.opcode = BTP_TBS_SET_URI_SCHEME,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = tbs_set_uri_scheme_list,
	},
	{
		.opcode = BTP_TBS_SET_STATUS_FLAGS,
		.expect_len = sizeof(struct btp_tbs_set_status_flags_cmd),
		.func = tbs_set_status_flags,
	},
	{
		.opcode = BTP_TBS_REMOTE_HOLD,
		.expect_len = sizeof(struct btp_tbs_remote_hold_cmd),
		.func = tbs_remote_hold,
	},
	{
		.opcode = BTP_TBS_ORIGINATE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = tbs_originate,
	},
	{
		.opcode = BTP_TBS_SET_SIGNAL_STRENGTH,
		.expect_len = sizeof(struct btp_tbs_set_signal_strength_cmd),
		.func = tbs_set_signal_strength,
	},
};

uint8_t tester_init_tbs(void)
{
	const struct bt_tbs_register_param gtbs_param = {
		.provider_name = "Generic TBS",
		.uci = "un000",
		.uri_schemes_supported = "tel,skype",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = CONFIG_BT_TBS_SUPPORTED_FEATURES,
	};
	const struct bt_tbs_register_param tbs_param = {
		.provider_name = "TBS",
		.uci = "un000",
		.uri_schemes_supported = "tel,skype",
		.gtbs = false,
		.authorization_required = false,
		/* Set different technologies per bearer */
		.technology = BT_TBS_TECHNOLOGY_4G,
		.supported_features = CONFIG_BT_TBS_SUPPORTED_FEATURES,
	};
	int err;

	bt_tbs_register_cb(&tbs_cbs);

	tester_register_command_handlers(BTP_SERVICE_ID_TBS, tbs_handlers,
					 ARRAY_SIZE(tbs_handlers));

	err = bt_tbs_register_bearer(&gtbs_param);
	if (err < 0) {
		LOG_DBG("Failed to register GTBS: %d", err);

		return BTP_STATUS_FAILED;
	}

	err = bt_tbs_register_bearer(&tbs_param);
	if (err < 0) {
		LOG_DBG("Failed to register TBS: %d", err);

		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_tbs(void)
{
	return BTP_STATUS_SUCCESS;
}
