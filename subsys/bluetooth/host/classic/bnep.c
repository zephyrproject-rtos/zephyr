/* bnep.c - Bluetooth Network Encapsulation Protocol */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "bnep_internal.h"

#define LOG_LEVEL CONFIG_BT_BNEP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bnep);

#define BNEP_HDR_SIZE 1

NET_BUF_POOL_DEFINE(bnep_tx_pool, CONFIG_BT_BUF_ACL_TX_COUNT,
		    BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_TX_SIZE),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

NET_BUF_POOL_DEFINE(bnep_rx_pool, BT_BUF_ACL_RX_COUNT,
		    BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_RX_SIZE),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const struct bt_bnep_cb *bnep_cb;
static bt_bnep_accept_fn bnep_accept_fn;

static struct bt_bnep *bnep_from_chan(struct bt_l2cap_chan *chan)
{
	return (struct bt_bnep *)((char *)chan - offsetof(struct bt_bnep, chan.chan));
}

static struct net_buf *bnep_alloc_buf(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	return net_buf_alloc(&bnep_rx_pool, K_NO_WAIT);
}

static int bnep_send_control(struct bt_bnep *bnep, uint8_t control_type,
			     const void *data, size_t len)
{
	struct net_buf *buf;
	uint8_t *hdr;

	buf = net_buf_alloc(&bnep_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	hdr = net_buf_add(buf, BNEP_HDR_SIZE + 1 + len);
	hdr[0] = BNEP_TYPE_CONTROL;
	hdr[1] = control_type;
	if (len > 0) {
		memcpy(&hdr[2], data, len);
	}

	return bt_l2cap_br_chan_send(&bnep->chan.chan, buf);
}

static int bnep_send_setup_rsp(struct bt_bnep *bnep, uint16_t result)
{
	uint16_t rsp = sys_cpu_to_le16(result);

	return bnep_send_control(bnep, BNEP_CONTROL_SETUP_CONN_RSP, &rsp, sizeof(rsp));
}

static int bnep_send_setup_req(struct bt_bnep *bnep)
{
	uint8_t data[2];

	data[0] = bnep->remote_service;
	data[1] = bnep->local_service;

	return bnep_send_control(bnep, BNEP_CONTROL_SETUP_CONN_REQ, data, sizeof(data));
}

static int bnep_send_filter_rsp(struct bt_bnep *bnep, uint8_t control_type, uint16_t result)
{
	uint16_t rsp = sys_cpu_to_le16(result);

	return bnep_send_control(bnep, control_type, &rsp, sizeof(rsp));
}

static void bnep_connected_notify(struct bt_bnep *bnep)
{
	bnep->state = BT_BNEP_STATE_CONNECTED;

	if (bnep->cb && bnep->cb->connected) {
		bnep->cb->connected(bnep);
	}
}

static void bnep_handle_setup_req(struct bt_bnep *bnep, struct net_buf *buf)
{
	uint8_t src;

	if (buf->len < 2) {
		(void)bnep_send_setup_rsp(bnep, 0x0001);
		return;
	}

	/* Destination UUID size is unused; only the source service is stored */
	(void)net_buf_pull_u8(buf);
	src = net_buf_pull_u8(buf);

	LOG_DBG("setup req src 0x%02x", src);

	bnep->remote_service = src;
	(void)bnep_send_setup_rsp(bnep, BNEP_SETUP_SUCCESS);
	bnep_connected_notify(bnep);
}

static void bnep_handle_setup_rsp(struct bt_bnep *bnep, struct net_buf *buf)
{
	uint16_t result;

	if (buf->len < 2) {
		bt_bnep_disconnect(bnep);
		return;
	}

	result = net_buf_pull_le16(buf);
	if (result != BNEP_SETUP_SUCCESS) {
		LOG_WRN("setup failed 0x%04x", result);
		bt_bnep_disconnect(bnep);
		return;
	}

	bnep_connected_notify(bnep);
}

static void bnep_handle_control(struct bt_bnep *bnep, struct net_buf *buf)
{
	uint8_t control_type;

	if (buf->len < 1) {
		return;
	}

	control_type = net_buf_pull_u8(buf);

	switch (control_type) {
	case BNEP_CONTROL_SETUP_CONN_REQ:
		bnep_handle_setup_req(bnep, buf);
		break;
	case BNEP_CONTROL_SETUP_CONN_RSP:
		bnep_handle_setup_rsp(bnep, buf);
		break;
	case BNEP_CONTROL_FILTER_MULTI_ADDR_REQ:
		(void)bnep_send_filter_rsp(bnep, BNEP_CONTROL_FILTER_MULTI_ADDR_RSP,
					   BNEP_SETUP_SUCCESS);
		break;
	case BNEP_CONTROL_FILTER_NET_TYPE_REQ:
		(void)bnep_send_filter_rsp(bnep, BNEP_CONTROL_FILTER_NET_TYPE_RSP,
					   BNEP_SETUP_SUCCESS);
		break;
	default:
		LOG_DBG("unsupported control 0x%02x", control_type);
		break;
	}
}

static void bnep_handle_general(struct bt_bnep *bnep, struct net_buf *buf)
{
	if (bnep->state != BT_BNEP_STATE_CONNECTED) {
		return;
	}

	if (bnep->cb && bnep->cb->recv) {
		bnep->cb->recv(bnep, buf);
	}
}

static int bnep_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_bnep *bnep = bnep_from_chan(chan);
	uint8_t type;

	if (buf->len < 1) {
		return -EINVAL;
	}

	type = net_buf_pull_u8(buf) & ~BNEP_EXT_HEADER;

	switch (type) {
	case BNEP_TYPE_GENERAL_ETHERNET:
		bnep_handle_general(bnep, buf);
		break;
	case BNEP_TYPE_CONTROL:
		bnep_handle_control(bnep, buf);
		break;
	default:
		LOG_DBG("unsupported bnep type 0x%02x", type);
		break;
	}

	/* L2CAP owns and unrefs buf after recv returns (unless -EINPROGRESS). */
	return 0;
}

static void bnep_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_bnep *bnep = bnep_from_chan(chan);

	LOG_DBG("bnep connected initiator %d", bnep->initiator);

	bnep->state = BT_BNEP_STATE_CONNECTING;

	if (bnep->initiator) {
		(void)bnep_send_setup_req(bnep);
	}
}

static void bnep_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_bnep *bnep = bnep_from_chan(chan);

	LOG_DBG("bnep disconnected");

	bnep->state = BT_BNEP_STATE_DISCONNECTED;
	bnep->initiator = false;

	if (bnep->cb && bnep->cb->disconnected) {
		bnep->cb->disconnected(bnep);
	}
}

static const struct bt_l2cap_chan_ops bnep_l2cap_ops = {
	.alloc_buf = bnep_alloc_buf,
	.recv = bnep_l2cap_recv,
	.connected = bnep_l2cap_connected,
	.disconnected = bnep_l2cap_disconnected,
};

void bt_bnep_init(void)
{
	static bool initialized;
	static struct bt_l2cap_server bnep_l2cap = {
		.psm = BT_L2CAP_PSM_BNEP,
		.sec_level = BT_SECURITY_L2,
		.accept = bt_bnep_l2cap_accept,
	};
	int err;

	if (initialized) {
		return;
	}

	err = bt_l2cap_br_server_register(&bnep_l2cap);
	if ((err < 0) && (err != -EEXIST)) {
		LOG_ERR("BNEP L2CAP registration failed %d", err);
		return;
	}

	initialized = true;
}

int bt_bnep_register_cb(const struct bt_bnep_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	bnep_cb = cb;
	return 0;
}

int bt_bnep_register_accept(bt_bnep_accept_fn accept)
{
	if (accept == NULL) {
		return -EINVAL;
	}

	bnep_accept_fn = accept;
	return 0;
}

int bt_bnep_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	struct bt_bnep *bnep;
	int err;

	ARG_UNUSED(server);

	if (bnep_accept_fn == NULL) {
		return -EINVAL;
	}

	err = bnep_accept_fn(conn, &bnep);
	if (err != 0) {
		return err;
	}

	*chan = &bnep->chan.chan;
	return 0;
}

static void bnep_chan_init(struct bt_bnep *bnep, struct bt_conn *conn)
{
	memset(bnep, 0, sizeof(*bnep));
	bnep->conn = conn;
	bnep->chan.chan.conn = conn;
	bnep->chan.chan.ops = &bnep_l2cap_ops;
	bnep->chan.rx.mtu = BT_L2CAP_RX_MTU;
	bnep->chan.required_sec_level = BT_SECURITY_L2;
	bnep->cb = bnep_cb;
	bnep->state = BT_BNEP_STATE_DISCONNECTED;
}

int bt_bnep_connect(struct bt_conn *conn, struct bt_bnep *bnep, uint8_t local_service,
		    uint8_t remote_service)
{
	int err;

	if (conn == NULL || bnep == NULL || bnep_cb == NULL) {
		return -EINVAL;
	}

	if (bnep->chan.chan.conn != NULL) {
		return -EALREADY;
	}

	bnep_chan_init(bnep, conn);
	bnep->local_service = local_service;
	bnep->remote_service = remote_service;
	bnep->initiator = true;

	err = bt_l2cap_br_chan_connect(conn, &bnep->chan.chan, BT_L2CAP_PSM_BNEP);
	if (err != 0) {
		memset(bnep, 0, sizeof(*bnep));
		return err;
	}

	return 0;
}

int bt_bnep_accept(struct bt_conn *conn, struct bt_bnep *bnep, uint8_t local_service)
{
	if (conn == NULL || bnep == NULL || bnep_cb == NULL) {
		return -EINVAL;
	}

	if (bnep->chan.chan.conn != NULL) {
		return -EALREADY;
	}

	bnep_chan_init(bnep, conn);
	bnep->local_service = local_service;
	bnep->initiator = false;

	return 0;
}

int bt_bnep_disconnect(struct bt_bnep *bnep)
{
	if (bnep == NULL || bnep->chan.chan.conn == NULL) {
		return -ENOTCONN;
	}

	return bt_l2cap_br_chan_disconnect(&bnep->chan.chan);
}

int bt_bnep_send(struct bt_bnep *bnep, struct net_buf *buf)
{
	if (bnep == NULL || bnep->state != BT_BNEP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (buf == NULL) {
		return -EINVAL;
	}

	net_buf_push_u8(buf, BNEP_TYPE_GENERAL_ETHERNET);

	return bt_l2cap_br_chan_send(&bnep->chan.chan, buf);
}

struct net_buf *bt_bnep_alloc_buf(size_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&bnep_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		return NULL;
	}

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE + BNEP_HDR_SIZE);

	if (net_buf_tailroom(buf) < len) {
		net_buf_unref(buf);
		return NULL;
	}

	return buf;
}
