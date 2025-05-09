/* btp_rfcomm.c - Bluetooth RFCOMM Tester */

/*
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_rfcomm
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define DATA_MTU 48
NET_BUF_POOL_FIXED_DEFINE(pool, 1, DATA_MTU, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void rfcomm_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	LOG_INF("Incoming data dlc %p len %u", dlci, buf->len);
}

static void rfcomm_connected(struct bt_rfcomm_dlc *dlci)
{
	LOG_INF("Dlc %p connected", dlci);
}

static void rfcomm_disconnected(struct bt_rfcomm_dlc *dlci)
{
	LOG_INF("Dlc %p disconnected", dlci);
}

static struct bt_rfcomm_dlc_ops rfcomm_ops = {
	.recv		= rfcomm_recv,
	.connected	= rfcomm_connected,
	.disconnected	= rfcomm_disconnected,
};

static struct bt_rfcomm_dlc rfcomm_dlc = {
	.ops = &rfcomm_ops,
	.mtu = 30,
};

static int rfcomm_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
	struct bt_rfcomm_dlc **dlc)
{
	if (rfcomm_dlc.session) {
		LOG_ERR("No channels available");
		return BTP_STATUS_FAILED;
	}
	*dlc = &rfcomm_dlc;

	return 0;
}

struct bt_rfcomm_server rfcomm_server = {
	.accept = &rfcomm_accept,
};

static uint8_t read_supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_rfcomm_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_RFCOMM, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t rfcomm_conn(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_connect_cmd *cp = cmd;
	struct btp_rfcomm_connect_rp *rp = rsp;
	uint8_t channel = cp->channel;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_rfcomm_dlc_connect(conn, &rfcomm_dlc, channel);
	if (err < 0) {
		LOG_ERR("Unable to connect to channel %d (err %u)",
			    channel, err);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

	rp->status = BTP_STATUS_SUCCESS;
	rp->dlc_state = rfcomm_dlc.state;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t rfcomm_disconnect(const void *cmd, uint16_t cmd_len,
	void *rsp, uint16_t *rsp_len)
{
	struct btp_rfcomm_disconnect_rp *rp = rsp;
	int err;

	if (!rfcomm_dlc.session) {
		LOG_ERR("No active RFCOMM session");
		return BTP_STATUS_FAILED;
	}

	err = bt_rfcomm_dlc_disconnect(&rfcomm_dlc);
	if (err < 0) {
		LOG_ERR("Failed to disconnect RFCOMM session (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = BTP_STATUS_SUCCESS;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}


static uint8_t rfcomm_register_server(const void *cmd, uint16_t cmd_len,
	void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_register_server_cmd *cp = cmd;
	struct btp_rfcomm_register_server_rp *rp = rsp;
	int err;

	rfcomm_server.channel = cp->channel;

	err = bt_rfcomm_server_register(&rfcomm_server);
	if (err < 0) {
		LOG_ERR("Unable to register RFCOMM server channel %d (err %d)",
		cp->channel, err);
		rfcomm_server.channel = 0;
		return BTP_STATUS_FAILED;
	}

	rp->status = BTP_STATUS_SUCCESS;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t rfcomm_send_data(const void *cmd, uint16_t cmd_len,
	void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_send_data_cmd *cp = cmd;
	struct btp_rfcomm_send_data_rp *rp = rsp;
	struct net_buf *buf;
	int err, len;

	if (!rfcomm_dlc.session) {
		LOG_ERR("No active RFCOMM session");
		return BTP_STATUS_FAILED;
	}

	/* Allocate buffer for data */
	buf = bt_rfcomm_create_pdu(&pool);
	len = MIN(rfcomm_dlc.mtu, net_buf_tailroom(buf) - 1);

	/* Copy data to buffer */
	net_buf_add_mem(buf, cp->data, len);

	/* Send data */
	err = bt_rfcomm_dlc_send(&rfcomm_dlc, buf);
	if (err < 0) {
		LOG_ERR("Failed to send data (err %d)", err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	rp->status = BTP_STATUS_SUCCESS;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler rfcomm_handlers[] = {
	{
		.opcode = BTP_RFCOMM_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = read_supported_commands
	},
	{
		.opcode = BTP_RFCOMM_CONNECT,
		.expect_len = sizeof(struct btp_rfcomm_connect_cmd),
		.func = rfcomm_conn
	},
	{
		.opcode = BTP_RFCOMM_REGISTER_SERVER,
		.expect_len = sizeof(struct btp_rfcomm_register_server_cmd),
		.func = rfcomm_register_server
	},
	{
		.opcode = BTP_RFCOMM_SEND_DATA,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = rfcomm_send_data
	},
	{
		.opcode = BTP_RFCOMM_DISCONNECT,
		.expect_len = 0,
		.func = rfcomm_disconnect
	},
};

uint8_t tester_init_rfcomm(void)
{

	tester_register_command_handlers(BTP_SERVICE_ID_RFCOMM, rfcomm_handlers,
		ARRAY_SIZE(rfcomm_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_rfcomm(void)
{
	return BTP_STATUS_SUCCESS;
}
