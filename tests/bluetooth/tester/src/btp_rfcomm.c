/* btp_rfcomm.c - Bluetooth RFCOMM Tester */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME btp_rfcomm
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define MAX_RFCOMM_CHANNELS 10
#define MAX_RFCOMM_SERVERS 5

struct rfcomm_channel {
	struct bt_rfcomm_dlc dlc;
	struct bt_conn *conn;
	bt_addr_t dst;
	uint8_t channel;
	bool in_use;
};

static struct rfcomm_channel channels[MAX_RFCOMM_CHANNELS];
static struct bt_rfcomm_server servers[MAX_RFCOMM_SERVERS];
static uint8_t server_count;

NET_BUF_POOL_FIXED_DEFINE(rfcomm_pdu_pool, MAX_RFCOMM_CHANNELS,
			  BT_RFCOMM_BUF_SIZE(CONFIG_BT_RFCOMM_L2CAP_MTU), 8, NULL);

static struct rfcomm_channel *find_channel(const bt_addr_t *addr, uint8_t channel)
{
	for (int i = 0; i < MAX_RFCOMM_CHANNELS; i++) {
		if (channels[i].in_use == false) {
			continue;
		}

		if (channels[i].channel != channel) {
			continue;
		}

		if (channels[i].conn == NULL) {
			continue;
		}

		if (bt_addr_cmp(&channels[i].dst, addr) == 0) {
			return &channels[i];
		}
	}

	return NULL;
}

static struct rfcomm_channel *alloc_channel(const bt_addr_t *dst)
{
	for (int i = 0; i < MAX_RFCOMM_CHANNELS; i++) {
		if (channels[i].in_use == false) {
			channels[i].in_use = true;
			bt_addr_copy(&channels[i].dst, dst);
			return &channels[i];
		}
	}

	return NULL;
}

static void free_channel(struct rfcomm_channel *chan)
{
	if (chan->conn != NULL) {
		bt_conn_unref(chan->conn);
		chan->conn = NULL;
	}
	chan->in_use = false;
	chan->channel = 0;
	bt_addr_copy(&chan->dst, &bt_addr_none);
}

static void rfcomm_connected(struct bt_rfcomm_dlc *dlc)
{
	struct rfcomm_channel *chan = CONTAINER_OF(dlc, struct rfcomm_channel, dlc);
	struct btp_rfcomm_connected_ev ev;

	LOG_DBG("DLC connected");

	if (chan->conn == NULL) {
		LOG_ERR("No connection");
		return;
	}

	bt_addr_copy(&ev.address.a, &chan->dst);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.channel = chan->channel;
	ev.mtu = sys_cpu_to_le16(dlc->mtu);

	tester_event(BTP_SERVICE_ID_RFCOMM, BTP_RFCOMM_EV_CONNECTED, &ev, sizeof(ev));
}

static void rfcomm_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct rfcomm_channel *chan = CONTAINER_OF(dlc, struct rfcomm_channel, dlc);
	struct btp_rfcomm_disconnected_ev ev;

	LOG_DBG("DLC disconnected");

	if (chan->conn != NULL) {
		bt_addr_copy(&ev.address.a, &chan->dst);
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		ev.channel = chan->channel;

		tester_event(BTP_SERVICE_ID_RFCOMM, BTP_RFCOMM_EV_DISCONNECTED, &ev, sizeof(ev));
	}

	free_channel(chan);
}

static void rfcomm_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct rfcomm_channel *chan = CONTAINER_OF(dlc, struct rfcomm_channel, dlc);
	struct btp_rfcomm_data_received_ev *ev;
	uint8_t *ev_data;
	size_t ev_len;

	LOG_DBG("DLC received data");

	if (chan->conn == NULL) {
		LOG_ERR("No connection");
		return;
	}

	ev_len = sizeof(*ev) + buf->len;

	tester_rsp_buffer_lock();
	tester_rsp_buffer_allocate(ev_len, &ev_data);

	if (ev_data == NULL) {
		LOG_ERR("Failed to allocate event buffer");
		tester_rsp_buffer_unlock();
		return;
	}

	ev = (struct btp_rfcomm_data_received_ev *)ev_data;
	bt_addr_copy(&ev->address.a, &chan->dst);
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	ev->channel = chan->channel;
	ev->data_length = sys_cpu_to_le16(buf->len);
	memcpy(ev->data, buf->data, buf->len);

	tester_event(BTP_SERVICE_ID_RFCOMM, BTP_RFCOMM_EV_DATA_RECEIVED, ev, ev_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();
}

static void rfcomm_sent(struct bt_rfcomm_dlc *dlc, int err)
{
	struct rfcomm_channel *chan = CONTAINER_OF(dlc, struct rfcomm_channel, dlc);
	struct btp_rfcomm_data_sent_ev ev;

	LOG_DBG("DLC sent callback (err %d)", err);

	if (chan->conn == NULL) {
		LOG_ERR("No connection");
		return;
	}

	bt_addr_copy(&ev.address.a, &chan->dst);
	ev.address.type = BTP_BR_ADDRESS_TYPE;
	ev.channel = chan->channel;
	ev.err = err;

	tester_event(BTP_SERVICE_ID_RFCOMM, BTP_RFCOMM_EV_DATA_SENT, &ev, sizeof(ev));
}

static struct bt_rfcomm_dlc_ops rfcomm_ops = {
	.connected = rfcomm_connected,
	.disconnected = rfcomm_disconnected,
	.recv = rfcomm_recv,
	.sent = rfcomm_sent,
};

static int server_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	struct rfcomm_channel *chan;
	struct bt_conn_info info;
	int err;

	LOG_DBG("Server accept");

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		LOG_ERR("Failed to get conn info (err %d)", err);
		return -ENOTCONN;
	}

	chan = alloc_channel(info.br.dst);
	if (chan == NULL) {
		LOG_ERR("No free channels");
		return -ENOMEM;
	}

	chan->dlc.ops = &rfcomm_ops;
	chan->dlc.mtu = CONFIG_BT_RFCOMM_L2CAP_MTU;
	chan->conn = bt_conn_ref(conn);
	chan->channel = server->channel;

	*dlc = &chan->dlc;

	return 0;
}

static uint8_t connect(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_connect_cmd *cp = cmd;
	struct bt_conn *conn;
	struct rfcomm_channel *chan;
	int err;

	LOG_DBG("RFCOMM Connect");
	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	chan = alloc_channel(&cp->address.a);
	if (chan == NULL) {
		LOG_ERR("No free channels");
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	chan->dlc.ops = &rfcomm_ops;
	chan->dlc.mtu = CONFIG_BT_RFCOMM_L2CAP_MTU;
	chan->conn = conn;
	chan->channel = cp->channel;

	err = bt_rfcomm_dlc_connect(conn, &chan->dlc, cp->channel);
	if (err != 0) {
		LOG_ERR("Failed to create DLC (err %d)", err);
		free_channel(chan);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t disconnect(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_disconnect_cmd *cp = cmd;
	struct rfcomm_channel *chan;
	int err;

	LOG_DBG("RFCOMM Disconnect");
	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	chan = find_channel(&cp->address.a, cp->channel);
	if (chan == NULL) {
		LOG_ERR("Channel not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_rfcomm_dlc_disconnect(&chan->dlc);
	if (err != 0) {
		LOG_ERR("Failed to destroy DLC (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t send_data(const void *cmd, uint16_t cmd_len,
			 void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_send_data_cmd *cp = cmd;
	struct rfcomm_channel *chan;
	struct net_buf *buf;
	uint16_t data_len;
	int err;

	LOG_DBG("RFCOMM Send Data");
	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	chan = find_channel(&cp->address.a, cp->channel);
	if (chan == NULL) {
		LOG_ERR("Channel not found");
		return BTP_STATUS_FAILED;
	}

	data_len = sys_le16_to_cpu(cp->data_len);

	buf = bt_rfcomm_create_pdu(&rfcomm_pdu_pool);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return BTP_STATUS_FAILED;
	}

	if (net_buf_tailroom(buf) < data_len) {
		LOG_ERR("Buffer too small for data %u < %u", net_buf_tailroom(buf), data_len);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	net_buf_add_mem(buf, cp->data, data_len);

	err = bt_rfcomm_dlc_send(&chan->dlc, buf);
	if (err < 0) {
		LOG_ERR("Failed to send (err %d)", err);
		net_buf_unref(buf);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t listen(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_listen_cmd *cp = cmd;
	struct bt_rfcomm_server *server;
	int err;

	LOG_DBG("RFCOMM Listen");

	if (server_count >= MAX_RFCOMM_SERVERS) {
		LOG_ERR("No free servers");
		return BTP_STATUS_FAILED;
	}

	server = &servers[server_count];
	server->channel = cp->channel;
	server->accept = server_accept;

	err = bt_rfcomm_server_register(server);
	if (err != 0) {
		LOG_ERR("Failed to register server (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	server_count++;

	return BTP_STATUS_SUCCESS;
}

static uint8_t send_rpn(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_send_rpn_cmd *cp = cmd;
	struct rfcomm_channel *chan;
	struct bt_rfcomm_rpn rpn;
	int err;

	LOG_DBG("RFCOMM Send RPN");
	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	chan = find_channel(&cp->address.a, cp->channel);
	if (chan == NULL) {
		LOG_ERR("Channel not found");
		return BTP_STATUS_FAILED;
	}

	rpn.dlci = chan->dlc.dlci;
	rpn.baud_rate = cp->baud_rate;
	rpn.line_settings = cp->line_settings;
	rpn.flow_control = cp->flow_control;
	rpn.xon_char = cp->xon_char;
	rpn.xoff_char = cp->xoff_char;
	rpn.param_mask = sys_le16_to_cpu(cp->param_mask);

	err = bt_rfcomm_send_rpn_cmd(&chan->dlc, &rpn);
	if (err != 0) {
		LOG_ERR("Failed to send RPN (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t get_dlc_info(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_rfcomm_get_dlc_info_cmd *cp = cmd;
	struct btp_rfcomm_get_dlc_info_rp *rp = rsp;
	struct rfcomm_channel *chan;

	LOG_DBG("RFCOMM Get DLC Info");
	if (cp->address.type != BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	chan = find_channel(&cp->address.a, cp->channel);
	if (chan == NULL) {
		LOG_ERR("Channel not found");
		return BTP_STATUS_FAILED;
	}

	rp->mtu = sys_cpu_to_le16(chan->dlc.mtu);
	rp->state = chan->dlc.state;

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_rfcomm_read_supported_commands_rp *rp = rsp;

	/* Octet 0 */
	tester_set_bit(rp->data, BTP_RFCOMM_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_RFCOMM_CONNECT);
	tester_set_bit(rp->data, BTP_RFCOMM_DISCONNECT);
	tester_set_bit(rp->data, BTP_RFCOMM_SEND_DATA);
	tester_set_bit(rp->data, BTP_RFCOMM_LISTEN);
	tester_set_bit(rp->data, BTP_RFCOMM_SEND_RPN);
	tester_set_bit(rp->data, BTP_RFCOMM_GET_DLC_INFO);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_RFCOMM_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_RFCOMM_CONNECT,
		.expect_len = sizeof(struct btp_rfcomm_connect_cmd),
		.func = connect,
	},
	{
		.opcode = BTP_RFCOMM_DISCONNECT,
		.expect_len = sizeof(struct btp_rfcomm_disconnect_cmd),
		.func = disconnect,
	},
	{
		.opcode = BTP_RFCOMM_SEND_DATA,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = send_data,
	},
	{
		.opcode = BTP_RFCOMM_LISTEN,
		.expect_len = sizeof(struct btp_rfcomm_listen_cmd),
		.func = listen,
	},
	{
		.opcode = BTP_RFCOMM_SEND_RPN,
		.expect_len = sizeof(struct btp_rfcomm_send_rpn_cmd),
		.func = send_rpn,
	},
	{
		.opcode = BTP_RFCOMM_GET_DLC_INFO,
		.expect_len = sizeof(struct btp_rfcomm_get_dlc_info_cmd),
		.func = get_dlc_info,
	},
};

uint8_t tester_init_rfcomm(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_RFCOMM, handlers,
					  ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_rfcomm(void)
{
	return BTP_STATUS_SUCCESS;
}
