/* l2cap.c - Bluetooth L2CAP Tester */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>

#include <errno.h>
#include <bluetooth/l2cap.h>
#include <sys/byteorder.h>

#include <logging/log.h>
#define LOG_MODULE_NAME bttester_l2cap
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define DATA_MTU 256
#define DATA_BUF_SIZE BT_L2CAP_SDU_BUF_SIZE(DATA_MTU)
#define CHANNELS 2
#define SERVERS 1

NET_BUF_POOL_FIXED_DEFINE(data_pool, CHANNELS, DATA_BUF_SIZE, NULL);

static bool authorize_flag;
static uint8_t req_keysize;

static struct channel {
	uint8_t chan_id; /* Internal number that identifies L2CAP channel. */
	struct bt_l2cap_le_chan le;
	bool in_use;
} channels[CHANNELS];

/* TODO Extend to support multiple servers */
static struct bt_l2cap_server servers[SERVERS];

static struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&data_pool, K_FOREVER);
}

static uint8_t recv_cb_buf[DATA_BUF_SIZE + sizeof(struct l2cap_data_received_ev)];

static int recv_cb(struct bt_l2cap_chan *l2cap_chan, struct net_buf *buf)
{
	struct l2cap_data_received_ev *ev = (void *) recv_cb_buf;
	struct channel *chan = CONTAINER_OF(l2cap_chan, struct channel, le);

	ev->chan_id = chan->chan_id;
	ev->data_length = sys_cpu_to_le16(buf->len);
	memcpy(ev->data, buf->data, buf->len);

	tester_send(BTP_SERVICE_ID_L2CAP, L2CAP_EV_DATA_RECEIVED,
		    CONTROLLER_INDEX, recv_cb_buf, sizeof(*ev) + buf->len);

	return 0;
}

static void connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct l2cap_connected_ev ev;
	struct channel *chan = CONTAINER_OF(l2cap_chan, struct channel, le);
	struct bt_conn_info info;

	ev.chan_id = chan->chan_id;
	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_LE:
			ev.mtu_remote = sys_cpu_to_le16(chan->le.tx.mtu);
			ev.mps_remote = sys_cpu_to_le16(chan->le.tx.mps);
			ev.mtu_local = sys_cpu_to_le16(chan->le.rx.mtu);
			ev.mps_local = sys_cpu_to_le16(chan->le.rx.mps);
			ev.address_type = info.le.dst->type;
			memcpy(ev.address, info.le.dst->a.val,
			       sizeof(ev.address));
			break;
		case BT_CONN_TYPE_BR:
			memcpy(ev.address, info.br.dst->val,
			       sizeof(ev.address));
			break;
		}
	}

	tester_send(BTP_SERVICE_ID_L2CAP, L2CAP_EV_CONNECTED, CONTROLLER_INDEX,
		    (uint8_t *) &ev, sizeof(ev));
}

static void disconnected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct l2cap_disconnected_ev ev;
	struct channel *chan = CONTAINER_OF(l2cap_chan, struct channel, le);
	struct bt_conn_info info;

	(void)memset(&ev, 0, sizeof(struct l2cap_disconnected_ev));

	/* TODO: ev.result */
	ev.chan_id = chan->chan_id;
	/* TODO: ev.psm */
	if (!bt_conn_get_info(l2cap_chan->conn, &info)) {
		switch (info.type) {
		case BT_CONN_TYPE_LE:
			ev.address_type = info.le.dst->type;
			memcpy(ev.address, info.le.dst->a.val,
			       sizeof(ev.address));
			break;
		case BT_CONN_TYPE_BR:
			memcpy(ev.address, info.br.dst->val,
			       sizeof(ev.address));
			break;
		}
	}

	tester_send(BTP_SERVICE_ID_L2CAP, L2CAP_EV_DISCONNECTED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf	= alloc_buf_cb,
	.recv		= recv_cb,
	.connected	= connected_cb,
	.disconnected	= disconnected_cb,
};

static struct channel *get_free_channel()
{
	uint8_t i;
	struct channel *chan;

	for (i = 0U; i < CHANNELS; i++) {
		if (channels[i].in_use) {
			continue;
		}

		chan = &channels[i];
		chan->chan_id = i;

		channels[i].in_use = true;

		return chan;
	}

	return NULL;
}


static void connect(uint8_t *data, uint16_t len)
{
	const struct l2cap_connect_cmd *cmd = (void *) data;
	struct l2cap_connect_rp *rp;
	struct bt_conn *conn;
	struct channel *chan = NULL;
	struct bt_l2cap_chan *allocated_channels[5] = {};
	uint16_t mtu = sys_le16_to_cpu(cmd->mtu);
	uint8_t buf[sizeof(*rp) + CHANNELS];
	uint8_t i = 0;
	int err;

	if (cmd->num > CHANNELS || mtu > DATA_MTU) {
		goto fail;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail;
	}

	rp = (void *)buf;

	for (i = 0U; i < cmd->num; i++) {
		chan = get_free_channel();
		if (!chan) {
			goto fail;
		}
		chan->le.chan.ops = &l2cap_ops;
		chan->le.rx.mtu = mtu;
		rp->chan_id[i] = chan->chan_id;
		allocated_channels[i] = &chan->le.chan;
	}

	if (cmd->num == 1) {
		err = bt_l2cap_chan_connect(conn, &chan->le.chan, cmd->psm);
		if (err < 0) {
			goto fail;
		}
	} else if (cmd->num > 1) {
#if defined(CONFIG_BT_L2CAP_ECRED)
		err = bt_l2cap_ecred_chan_connect(conn, allocated_channels,
							cmd->psm);
		if (err < 0) {
			goto fail;
		}
#endif
	} else {
		LOG_ERR("Invalid 'num' parameter value");
		goto fail;
	}

	rp->num = cmd->num;

	tester_send(BTP_SERVICE_ID_L2CAP, L2CAP_CONNECT, CONTROLLER_INDEX,
		    (uint8_t *)rp, sizeof(*rp) + rp->num);

	return;

fail:
	for (i = 0U; i < ARRAY_SIZE(allocated_channels); i++) {
		if (allocated_channels[i]) {
			channels[allocated_channels[i]->ident].in_use = false;
		}
	}
	tester_rsp(BTP_SERVICE_ID_L2CAP, L2CAP_CONNECT, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void disconnect(uint8_t *data, uint16_t len)
{
	const struct l2cap_disconnect_cmd *cmd = (void *) data;
	struct channel *chan = &channels[cmd->chan_id];
	uint8_t status;
	int err;

	err = bt_l2cap_chan_disconnect(&chan->le.chan);
	if (err) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	status = BTP_STATUS_SUCCESS;

rsp:
	tester_rsp(BTP_SERVICE_ID_L2CAP, L2CAP_DISCONNECT, CONTROLLER_INDEX,
		   status);
}

static void send_data(uint8_t *data, uint16_t len)
{
	const struct l2cap_send_data_cmd *cmd = (void *) data;
	struct channel *chan = &channels[cmd->chan_id];
	struct net_buf *buf;
	int ret;
	uint16_t data_len = sys_le16_to_cpu(cmd->data_len);

	/* FIXME: For now, fail if data length exceeds buffer length */
	if (data_len > DATA_MTU) {
		goto fail;
	}

	/* FIXME: For now, fail if data length exceeds remote's L2CAP SDU */
	if (data_len > chan->le.tx.mtu) {
		goto fail;
	}

	buf = net_buf_alloc(&data_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, cmd->data, data_len);
	ret = bt_l2cap_chan_send(&chan->le.chan, buf);
	if (ret < 0) {
		LOG_ERR("Unable to send data: %d", -ret);
		net_buf_unref(buf);
		goto fail;
	}

	tester_rsp(BTP_SERVICE_ID_L2CAP, L2CAP_SEND_DATA, CONTROLLER_INDEX,
			BTP_STATUS_SUCCESS);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_L2CAP, L2CAP_SEND_DATA, CONTROLLER_INDEX,
			BTP_STATUS_FAILED);
}

static struct bt_l2cap_server *get_free_server(void)
{
	uint8_t i;

	for (i = 0U; i < SERVERS ; i++) {
		if (servers[i].psm) {
			continue;
		}

		return &servers[i];
	}

	return NULL;
}

static bool is_free_psm(uint16_t psm)
{
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(servers); i++) {
		if (servers[i].psm == psm) {
			return false;
		}
	}

	return true;
}

static int accept(struct bt_conn *conn, struct bt_l2cap_chan **l2cap_chan)
{
	struct channel *chan;

	chan = get_free_channel();
	if (!chan) {
		return -ENOMEM;
	}

	if (bt_conn_enc_key_size(conn) < req_keysize) {
		/* TSPX_psm_encryption_key_size_required */
		req_keysize = 0;
		return -EPERM;
	} else if (authorize_flag) {
		/* TSPX_psm_authorization_required
		 * we never authorize this connection, so return error
		 * everytime this psm is used
		 */
		authorize_flag = false;
		return -EACCES;
	}

	chan->le.chan.ops = &l2cap_ops;
	chan->le.rx.mtu = DATA_MTU;

	*l2cap_chan = &chan->le.chan;

	return 0;
}

static void listen(uint8_t *data, uint16_t len)
{
	const struct l2cap_listen_cmd *cmd = (void *) data;
	struct bt_l2cap_server *server;

	/* TODO: Handle cmd->transport flag */

	if (!is_free_psm(cmd->psm)) {
		goto fail;
	}

	server = get_free_server();
	if (!server) {
		goto fail;
	}

	server->accept = accept;
	server->psm = cmd->psm;

	if (server->psm == 0x00F4) {
		/* TSPX_psm_encryption_key_size_required */
		req_keysize = 16;
	} else if (server->psm == 0x00F3) {
		/* TSPX_psm_authentication_required */
		authorize_flag = true;
	} else if (server->psm == 0x00F2) {
		/* TSPX_psm_authentication_required */
		server->sec_level = BT_SECURITY_L3;
	}

	if (bt_l2cap_server_register(server) < 0) {
		server->psm = 0U;
		goto fail;
	}

	tester_rsp(BTP_SERVICE_ID_L2CAP, L2CAP_LISTEN, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_L2CAP, L2CAP_LISTEN, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint8_t cmds[1];
	struct l2cap_read_supported_commands_rp *rp = (void *) cmds;

	(void)memset(cmds, 0, sizeof(cmds));

	tester_set_bit(cmds, L2CAP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(cmds, L2CAP_CONNECT);
	tester_set_bit(cmds, L2CAP_DISCONNECT);
	tester_set_bit(cmds, L2CAP_LISTEN);
	tester_set_bit(cmds, L2CAP_SEND_DATA);

	tester_send(BTP_SERVICE_ID_L2CAP, L2CAP_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, (uint8_t *) rp, sizeof(cmds));
}

void tester_handle_l2cap(uint8_t opcode, uint8_t index, uint8_t *data,
			 uint16_t len)
{
	switch (opcode) {
	case L2CAP_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	case L2CAP_CONNECT:
		connect(data, len);
		return;
	case L2CAP_DISCONNECT:
		disconnect(data, len);
		return;
	case L2CAP_SEND_DATA:
		send_data(data, len);
		return;
	case L2CAP_LISTEN:
		listen(data, len);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_L2CAP, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

uint8_t tester_init_l2cap(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_l2cap(void)
{
	return BTP_STATUS_SUCCESS;
}
