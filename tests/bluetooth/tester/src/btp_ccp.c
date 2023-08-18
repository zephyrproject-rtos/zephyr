/* btp_ccp.c - Bluetooth CCP Tester */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "btp/btp.h"
#include "zephyr/sys/byteorder.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_ccp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static uint8_t ccp_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_ccp_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_CCP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_CCP_DISCOVER_TBS);
	tester_set_bit(rp->data, BTP_CCP_ACCEPT_CALL);
	tester_set_bit(rp->data, BTP_CCP_TERMINATE_CALL);
	tester_set_bit(rp->data, BTP_CCP_ORIGINATE_CALL);
	tester_set_bit(rp->data, BTP_CCP_READ_CALL_STATE);

	*rsp_len = sizeof(*rp) + 1;

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

/**
 * @brief Callback function for ccp_discover.
 *
 * @param conn          The connection that was used to discover CCP for a
 *                      device.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param tbs_count     Number of TBS instances on peer device.
 * @param gtbs_found    Whether or not the server has a Generic TBS instance.
 */
static void tbs_client_discover_cb(struct bt_conn *conn,
				   int err,
				   uint8_t tbs_count,
				   bool gtbs_found)
{
	LOG_DBG("Discovered TBS - err (%u) GTBS (%u)", err, gtbs_found);

	tbs_client_discovered_ev(err, tbs_count, gtbs_found);
}

/**
 * @brief Callback function for the CCP call control functions.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index. For #bt_tbs_client_originate_call this will
 *                      always be 0, and does not reflect the actual call index.
 */
static void tbs_client_originate_call_cb(struct bt_conn *conn,
					 int err,
					 uint8_t inst_index,
					 uint8_t call_index)
{
	LOG_DBG("Originate call - err (%u) Call Index (%u)", err, call_index);
}

/**
 * @brief Callback function for the CCP call control functions.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index. For #bt_tbs_client_originate_call this will
 *                      always be 0, and does not reflect the actual call index.
 */
static void tbs_client_terminate_call_cb(struct bt_conn *conn,
					 int err,
					 uint8_t inst_index,
					 uint8_t call_index)
{
	LOG_DBG("Terminate call - err (%u) Call Index (%u)", err, call_index);
}

/**
 * @brief Callback function for the CCP call control functions.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index. For #bt_tbs_client_originate_call this will
 *                      always be 0, and does not reflect the actual call index.
 */
static void tbs_client_accept_call_cb(struct bt_conn *conn,
				      int err,
				      uint8_t inst_index,
				      uint8_t call_index)
{
	LOG_DBG("Accept call - err (%u) Call Index (%u)", err, call_index);
}

/**
 * @brief Callback function for the CCP call control functions.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index. For #bt_tbs_client_originate_call this will
 *                      always be 0, and does not reflect the actual call index.
 */
static void tbs_client_retrieve_call_cb(struct bt_conn *conn,
					int err,
					uint8_t inst_index,
					uint8_t call_index)
{
	LOG_DBG("Retrieve call - err (%u) Call Index (%u)", err, call_index);
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

/**
 * @brief Callback function for ccp_read_call_state.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_count    Number of call states read.
 * @param call_states   Array of call states. The array is not kept by
 *                      the client, so must be copied to be saved.
 */
static void tbs_client_call_states_cb(struct bt_conn *conn,
				      int err,
				      uint8_t inst_index,
				      uint8_t call_count,
				      const bt_tbs_client_call_state_t *call_states)
{
	LOG_DBG("Call states - err (%u) Call Count (%u)", err, call_count);

	tbs_client_call_states_ev(err, inst_index, call_count, call_states);
}

/**
 * @brief Callback function for ccp_read_termination_reason.
 *
 * @param conn          The connection used in the function.
 * @param err           Error value. BT_TBS_CLIENT_RESULT_CODE_*,
 *                      GATT error or errno value.
 * @param inst_index    The index of the TBS instance that was updated.
 * @param call_index    The call index.
 * @param reason        The termination reason.
 */
static void tbs_client_termination_reason_cb(struct bt_conn *conn,
					     int err,
					     uint8_t inst_index,
					     uint8_t call_index,
					     uint8_t reason)
{
	LOG_DBG("Termination reason - err (%u) Call Index (%u) Reason (%u)",
		err, call_index, reason);
}

static const struct bt_tbs_client_cb tbs_client_callbacks = {
	.discover = tbs_client_discover_cb,
	.originate_call = tbs_client_originate_call_cb,
	.terminate_call = tbs_client_terminate_call_cb,
	.accept_call = tbs_client_accept_call_cb,
	.retrieve_call = tbs_client_retrieve_call_cb,
	.call_state = tbs_client_call_states_cb,
	.termination_reason = tbs_client_termination_reason_cb
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
	}
};

uint8_t tester_init_ccp(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_CCP, ccp_handlers,
					 ARRAY_SIZE(ccp_handlers));

	bt_tbs_client_register_cb(&tbs_client_callbacks);

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ccp(void)
{
	return BTP_STATUS_SUCCESS;
}
