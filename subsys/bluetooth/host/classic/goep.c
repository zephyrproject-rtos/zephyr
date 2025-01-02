/* goep.c - Bluetooth Generic Object Exchange Profile handling */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>

#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "obex_internal.h"

#define LOG_LEVEL CONFIG_BT_GOEP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_goep);

#define GOEP_MIN_MTU BT_OBEX_MIN_MTU

/* RFCOMM Server list */
static sys_slist_t goep_rfcomm_server;

static void goep_rfcomm_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_goep *goep = CONTAINER_OF(dlc, struct bt_goep, _transport.dlc);
	int err;

	err = bt_obex_recv(&goep->obex, buf);
	if (err) {
		LOG_WRN("Fail to handle OBEX packet (err %d)", err);
	}
}

static void goep_rfcomm_connected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_goep *goep = CONTAINER_OF(dlc, struct bt_goep, _transport.dlc);
	int err;

	LOG_DBG("RFCOMM %p connected", dlc);

	if (dlc->mtu < GOEP_MIN_MTU) {
		LOG_WRN("Invalid MTU %d", dlc->mtu);
		bt_rfcomm_dlc_disconnect(dlc);
		return;
	}
	goep->obex.rx.mtu = dlc->mtu;
	goep->obex.tx.mtu = dlc->mtu;

	/* Set MOPL of RX to MTU by default */
	goep->obex.rx.mopl = dlc->mtu;
	/* Set MOPL of TX to MTU by default to avoid the OBEX connect req cannot be sent. */
	goep->obex.tx.mopl = dlc->mtu;

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_CONNECTED);

	err = bt_obex_transport_connected(&goep->obex);
	if (err) {
		LOG_WRN("Notify OBEX (err %d). Disconnecting transport...", err);
		bt_rfcomm_dlc_disconnect(dlc);
		return;
	}

	if (goep->transport_ops->connected) {
		goep->transport_ops->connected(goep->_acl, goep);
	}
}

static void goep_rfcomm_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_goep *goep = CONTAINER_OF(dlc, struct bt_goep, _transport.dlc);
	int err;

	LOG_DBG("RFCOMM %p disconnected", dlc);

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_DISCONNECTED);

	err = bt_obex_transport_disconnected(&goep->obex);
	if (err) {
		LOG_WRN("Notify OBEX (err %d).", err);
	}

	if (goep->transport_ops->disconnected) {
		goep->transport_ops->disconnected(goep);
	}
}

static struct bt_rfcomm_dlc_ops goep_rfcomm_ops = {
	.recv = goep_rfcomm_recv,
	.connected = goep_rfcomm_connected,
	.disconnected = goep_rfcomm_disconnected,
};

static int goep_rfcomm_send(struct bt_obex *obex, struct net_buf *buf)
{
	struct bt_goep *goep = CONTAINER_OF(obex, struct bt_goep, obex);
	int err;

	if (goep->_goep_v2) {
		LOG_WRN("Invalid transport");
		return -EINVAL;
	}

	if (buf->len > obex->tx.mtu) {
		LOG_WRN("Packet size exceeds MTU");
		return -EMSGSIZE;
	}

	if (net_buf_tailroom(buf) < BT_RFCOMM_FCS_SIZE) {
		LOG_WRN("No tailroom for RFCOMM FCS field");
		return -EMSGSIZE;
	}

	err = bt_rfcomm_dlc_send(&goep->_transport.dlc, buf);
	if (err < 0) {
		return err;
	}

	return 0;
}

static struct net_buf *goep_rfcomm_alloc_buf(struct bt_obex *obex, struct net_buf_pool *pool)
{
	struct bt_goep *goep = CONTAINER_OF(obex, struct bt_goep, obex);

	if (goep->_goep_v2) {
		LOG_WRN("Invalid transport");
		return NULL;
	}

	return bt_goep_create_pdu(goep, pool);
}

static int geop_rfcomm_disconnect(struct bt_obex *obex)
{
	struct bt_goep *goep = CONTAINER_OF(obex, struct bt_goep, obex);

	if (goep->_goep_v2) {
		LOG_WRN("Invalid transport");
		return -EINVAL;
	}

	return bt_rfcomm_dlc_disconnect(&goep->_transport.dlc);
}

static const struct bt_obex_transport_ops goep_rfcomm_transport_ops = {
	.alloc_buf = goep_rfcomm_alloc_buf,
	.send = goep_rfcomm_send,
	.disconnect = geop_rfcomm_disconnect,
};

static bool goep_find_rfcomm_server(sys_slist_t *servers,
				    struct bt_goep_transport_rfcomm_server *server)
{
	struct bt_goep_transport_rfcomm_server *rfcomm_server, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(servers, rfcomm_server, next, node) {
		if (rfcomm_server == server) {
			return true;
		}
	}
	return false;
}

static int goep_rfcomm_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			      struct bt_rfcomm_dlc **dlc)
{
	struct bt_goep_transport_rfcomm_server *rfcomm_server;
	struct bt_goep *goep;
	int err;

	rfcomm_server = CONTAINER_OF(server, struct bt_goep_transport_rfcomm_server, rfcomm);

	if (!goep_find_rfcomm_server(&goep_rfcomm_server, rfcomm_server)) {
		LOG_WRN("Invalid rfcomm server");
		return -ENOMEM;
	}

	err = rfcomm_server->accept(conn, rfcomm_server, &goep);
	if (err) {
		LOG_DBG("Incoming connection rejected");
		return err;
	}

	if (!goep || !goep->transport_ops || (goep->obex.rx.mtu < GOEP_MIN_MTU) ||
	    !goep->obex.server_ops || !goep->obex.server_ops->connect ||
	    !goep->obex.server_ops->disconnect) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	err = bt_obex_reg_transport(&goep->obex, &goep_rfcomm_transport_ops);
	if (err) {
		LOG_WRN("Fail to reg transport ops");
		return err;
	}

	goep->_goep_v2 = false;
	goep->_acl = conn;
	*dlc = &goep->_transport.dlc;
	goep->_transport.dlc.mtu = goep->obex.rx.mtu;
	goep->_transport.dlc.ops = &goep_rfcomm_ops;
	goep->_transport.dlc.required_sec_level = BT_SECURITY_L2;

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_CONNECTING);

	return 0;
}

int bt_goep_transport_rfcomm_server_register(struct bt_goep_transport_rfcomm_server *server)
{
	int err;

	if (!server || !server->accept) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	if (goep_find_rfcomm_server(&goep_rfcomm_server, server)) {
		LOG_WRN("RFCOMM server has been registered");
		return -EEXIST;
	}

	server->rfcomm.accept = goep_rfcomm_accept;
	err = bt_rfcomm_server_register(&server->rfcomm);
	if (err) {
		LOG_WRN("Fail to register RFCOMM Server %p", server);
		return err;
	}

	LOG_DBG("Register RFCOMM server %p", server);
	sys_slist_append(&goep_rfcomm_server, &server->node);

	return 0;
}

int bt_goep_transport_rfcomm_connect(struct bt_conn *conn, struct bt_goep *goep, uint8_t channel)
{
	int err;

	if (!conn || !goep || !goep->transport_ops || (goep->obex.rx.mtu < GOEP_MIN_MTU) ||
	    !goep->obex.client_ops || !goep->obex.client_ops->connect ||
	    !goep->obex.client_ops->disconnect) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	err = bt_obex_reg_transport(&goep->obex, &goep_rfcomm_transport_ops);
	if (err) {
		LOG_WRN("Fail to reg transport ops");
		return err;
	}

	goep->_goep_v2 = false;
	goep->_acl = conn;
	goep->_transport.dlc.mtu = goep->obex.rx.mtu;
	goep->_transport.dlc.ops = &goep_rfcomm_ops;
	goep->_transport.dlc.required_sec_level = BT_SECURITY_L2;

	err = bt_rfcomm_dlc_connect(conn, &goep->_transport.dlc, channel);
	if (err) {
		LOG_WRN("Fail to create RFCOMM connection");
		return err;
	}

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_CONNECTING);

	return 0;
}

int bt_goep_transport_rfcomm_disconnect(struct bt_goep *goep)
{
	int err;
	uint32_t state;

	if (!goep || goep->_goep_v2) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	state = atomic_get(&goep->_state);
	if (state != BT_GOEP_TRANSPORT_CONNECTED) {
		LOG_DBG("Invalid stats %d", state);
		return -ENOTCONN;
	}

	err = bt_rfcomm_dlc_disconnect(&goep->_transport.dlc);
	if (err) {
		LOG_WRN("Fail to disconnect RFCOMM DLC");
		return err;
	}

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_DISCONNECTING);

	return 0;
}

/* L2CAP Server list */
static sys_slist_t goep_l2cap_server;

static int goep_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_goep *goep = CONTAINER_OF(chan, struct bt_goep, _transport.chan.chan);
	int err;

	err = bt_obex_recv(&goep->obex, buf);
	if (err) {
		LOG_WRN("Fail to handle OBEX packet (err %d)", err);
	}
	return err;
}

static void goep_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_goep *goep = CONTAINER_OF(chan, struct bt_goep, _transport.chan.chan);
	int err;

	LOG_DBG("L2CAP channel %p connected", chan);

	if ((goep->_transport.chan.tx.mtu < GOEP_MIN_MTU) ||
	    (goep->_transport.chan.rx.mtu < GOEP_MIN_MTU)) {
		LOG_WRN("Invalid MTU (local %d, peer %d", goep->_transport.chan.rx.mtu,
			goep->_transport.chan.tx.mtu);
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	goep->obex.rx.mtu = goep->_transport.chan.rx.mtu;
	goep->obex.tx.mtu = goep->_transport.chan.tx.mtu;

	/* Set MOPL of RX to MTU by default */
	goep->obex.rx.mopl = goep->_transport.chan.rx.mtu;
	/* Set MOPL of TX to MTU by default to avoid the OBEX connect req cannot be sent. */
	goep->obex.tx.mopl = goep->_transport.chan.tx.mtu;

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_CONNECTED);

	err = bt_obex_transport_connected(&goep->obex);
	if (err) {
		LOG_WRN("Notify OBEX (err %d). Disconnecting transport...", err);
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	if (goep->transport_ops->connected) {
		goep->transport_ops->connected(goep->_acl, goep);
	}
}

static void goep_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_goep *goep = CONTAINER_OF(chan, struct bt_goep, _transport.chan.chan);
	int err;

	LOG_DBG("L2CAP channel %p disconnected", chan);

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_DISCONNECTED);

	err = bt_obex_transport_disconnected(&goep->obex);
	if (err) {
		LOG_WRN("Notify OBEX (err %d).", err);
	}

	if (goep->transport_ops->disconnected) {
		goep->transport_ops->disconnected(goep);
	}
}

static const struct bt_l2cap_chan_ops goep_l2cap_ops = {
	.recv = goep_l2cap_recv,
	.connected = goep_l2cap_connected,
	.disconnected = goep_l2cap_disconnected,
};

static int goep_l2cap_send(struct bt_obex *obex, struct net_buf *buf)
{
	struct bt_goep *goep = CONTAINER_OF(obex, struct bt_goep, obex);

	if (!goep->_goep_v2) {
		LOG_WRN("Invalid transport");
		return -EINVAL;
	}

	if (buf->len > obex->tx.mtu) {
		LOG_WRN("Packet size exceeds MTU");
		return -EMSGSIZE;
	}

	return bt_l2cap_chan_send(&goep->_transport.chan.chan, buf);
}

static struct net_buf *goep_l2cap_alloc_buf(struct bt_obex *obex, struct net_buf_pool *pool)
{
	struct bt_goep *goep = CONTAINER_OF(obex, struct bt_goep, obex);

	if (goep->_goep_v2) {
		LOG_WRN("Invalid transport");
		return NULL;
	}

	return bt_goep_create_pdu(goep, pool);
}

static int geop_l2cap_disconnect(struct bt_obex *obex)
{
	struct bt_goep *goep = CONTAINER_OF(obex, struct bt_goep, obex);

	if (goep->_goep_v2) {
		LOG_WRN("Invalid transport");
		return -EINVAL;
	}

	return bt_l2cap_chan_disconnect(&goep->_transport.chan.chan);
}

static const struct bt_obex_transport_ops goep_l2cap_transport_ops = {
	.alloc_buf = goep_l2cap_alloc_buf,
	.send = goep_l2cap_send,
	.disconnect = geop_l2cap_disconnect,
};

static bool goep_find_l2cap_server(sys_slist_t *servers,
				   struct bt_goep_transport_l2cap_server *server)
{
	struct bt_goep_transport_l2cap_server *l2cap_server, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(servers, l2cap_server, next, node) {
		if (l2cap_server == server) {
			return true;
		}
	}
	return false;
}

static int goep_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			     struct bt_l2cap_chan **chan)
{
	struct bt_goep_transport_l2cap_server *l2cap_server;
	struct bt_goep *goep;
	int err;

	l2cap_server = CONTAINER_OF(server, struct bt_goep_transport_l2cap_server, l2cap);

	if (!goep_find_l2cap_server(&goep_l2cap_server, l2cap_server)) {
		LOG_WRN("Invalid l2cap server");
		return -ENOMEM;
	}

	err = l2cap_server->accept(conn, l2cap_server, &goep);
	if (err) {
		LOG_DBG("Incoming connection rejected");
		return err;
	}

	if (!goep || !goep->transport_ops || (goep->obex.rx.mtu < GOEP_MIN_MTU) ||
	    !goep->obex.server_ops || !goep->obex.server_ops->connect ||
	    !goep->obex.server_ops->disconnect) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	err = bt_obex_reg_transport(&goep->obex, &goep_l2cap_transport_ops);
	if (err) {
		LOG_WRN("Fail to reg transport ops");
		return err;
	}

	goep->_goep_v2 = true;
	goep->_acl = conn;
	*chan = &goep->_transport.chan.chan;
	goep->_transport.chan.rx.mtu = goep->obex.rx.mtu;
	goep->_transport.chan.chan.ops = &goep_l2cap_ops;
	goep->_transport.chan.required_sec_level = BT_SECURITY_L2;

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_CONNECTING);

	return 0;
}

int bt_goep_transport_l2cap_server_register(struct bt_goep_transport_l2cap_server *server)
{
	int err;

	if (!server || !server->accept) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	if (goep_find_l2cap_server(&goep_l2cap_server, server)) {
		LOG_WRN("L2CAP server has been registered");
		return -EEXIST;
	}

	server->l2cap.accept = goep_l2cap_accept;
	err = bt_l2cap_br_server_register(&server->l2cap);
	if (err) {
		LOG_WRN("Fail to register L2CAP Server %p", server);
		return err;
	}

	LOG_DBG("Register L2CAP server %p", server);
	sys_slist_append(&goep_l2cap_server, &server->node);

	return 0;
}

int bt_goep_transport_l2cap_connect(struct bt_conn *conn, struct bt_goep *goep, uint16_t psm)
{
	int err;
	uint32_t state;

	if (!conn || !goep || !goep->transport_ops || (goep->obex.rx.mtu < GOEP_MIN_MTU) ||
	    !goep->obex.client_ops || !goep->obex.client_ops->connect ||
	    !goep->obex.client_ops->disconnect) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	state = atomic_get(&goep->_state);
	if (state != BT_GOEP_TRANSPORT_DISCONNECTED) {
		LOG_DBG("Invalid stats %d", state);
		return -EBUSY;
	}

	err = bt_obex_reg_transport(&goep->obex, &goep_l2cap_transport_ops);
	if (err) {
		LOG_WRN("Fail to reg transport ops");
		return err;
	}

	goep->_goep_v2 = true;
	goep->_acl = conn;
	goep->_transport.chan.rx.mtu = goep->obex.rx.mtu;
	goep->_transport.chan.chan.ops = &goep_l2cap_ops;
	goep->_transport.chan.required_sec_level = BT_SECURITY_L2;

	err = bt_l2cap_chan_connect(conn, &goep->_transport.chan.chan, psm);
	if (err) {
		LOG_WRN("Fail to create L2CAP connection");
		return err;
	}

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_CONNECTING);

	return 0;
}

int bt_goep_transport_l2cap_disconnect(struct bt_goep *goep)
{
	int err;
	uint32_t state;

	if (!goep || !goep->_goep_v2) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	state = atomic_get(&goep->_state);
	if (state != BT_GOEP_TRANSPORT_CONNECTED) {
		LOG_DBG("Invalid stats %d", state);
		return -ENOTCONN;
	}

	err = bt_l2cap_chan_disconnect(&goep->_transport.chan.chan);
	if (err) {
		LOG_WRN("Fail to disconnect L2CAP channel");
		return err;
	}

	atomic_set(&goep->_state, BT_GOEP_TRANSPORT_DISCONNECTING);

	return 0;
}

struct net_buf *bt_goep_create_pdu(struct bt_goep *goep, struct net_buf_pool *pool)
{
	struct net_buf *buf;
	size_t len;

	if (!goep) {
		LOG_WRN("Invalid parameter");
		return NULL;
	}

	if (!goep->_goep_v2) {
		buf = bt_rfcomm_create_pdu(pool);
	} else {
		buf = bt_conn_create_pdu(pool, sizeof(struct bt_l2cap_hdr));
	}

	if (buf) {
		len = net_buf_headroom(buf);
		net_buf_reserve(buf, len + BT_OBEX_SEND_BUF_RESERVE);
	}
	return buf;
}
