/** @file
 *  @brief Audio Video Control Transport Protocol
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <strings.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "avctp_internal.h"
#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"

#define LOG_LEVEL CONFIG_BT_AVCTP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_avctp);

#define AVCTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avctp, br_chan.chan)
/* L2CAP Server list */
static sys_slist_t avctp_l2cap_server = SYS_SLIST_STATIC_INIT(&avctp_l2cap_server);
static struct k_sem avctp_server_lock;

static void avctp_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avctp *session;

	if (!chan) {
		LOG_ERR("Invalid AVCTP chan");
		return;
	}

	session = AVCTP_CHAN(chan);
	LOG_DBG("chan %p session %p", chan, session);

	if (session->ops && session->ops->connected) {
		session->ops->connected(session);
	}
}

static void avctp_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avctp *session;

	if (!chan) {
		LOG_ERR("Invalid AVCTP chan");
		return;
	}

	session = AVCTP_CHAN(chan);
	LOG_DBG("chan %p session %p", chan, session);
	session->br_chan.chan.conn = NULL;

	if (session->ops && session->ops->disconnected) {
		session->ops->disconnected(session);
	}
}

static void avctp_l2cap_encrypt_changed(struct bt_l2cap_chan *chan, uint8_t status)
{
	LOG_DBG("");
}

static int avctp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct net_buf *rsp;
	struct bt_avctp *session = AVCTP_CHAN(chan);
	struct bt_avctp_header *hdr = (void *)buf->data;
	uint8_t tid;
	bt_avctp_cr_t cr;
	int err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("invalid AVCTP header received");
		return -EINVAL;
	}

	tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(hdr);
	cr = BT_AVCTP_HDR_GET_CR(hdr);

	LOG_DBG("AVCTP msg received, cr:0x%X, tid:0x%X, pid: 0x%04X",
		cr, tid, sys_be16_to_cpu(hdr->pid));

	if (sys_be16_to_cpu(hdr->pid) == session->pid) {
		return session->ops->recv(session, buf);
	}

	LOG_ERR("unsupported AVCTP PID received: 0x%04x", sys_be16_to_cpu(hdr->pid));
	if (cr == BT_AVCTP_CMD) {
		rsp = bt_avctp_create_pdu(session, BT_AVCTP_RESPONSE,
					  BT_AVCTP_PKT_TYPE_SINGLE, BT_AVCTP_IPID_INVALID,
					  tid, hdr->pid);
		if (rsp == NULL) {
			__ASSERT(0, "Failed to create AVCTP response PDU");
			return -ENOMEM;
		}

		err = bt_avctp_send(session, rsp);
		if (err < 0) {
			net_buf_unref(rsp);
			LOG_ERR("AVCTP send fail, err = %d", err);
			bt_avctp_disconnect(session);
			return err;
		}
	}
	return 0; /* No need to report to the upper layer */
}

static const struct bt_l2cap_chan_ops ops = {
	.connected = avctp_l2cap_connected,
	.disconnected = avctp_l2cap_disconnected,
	.encrypt_change = avctp_l2cap_encrypt_changed,
	.recv = avctp_l2cap_recv,
};

int bt_avctp_connect(struct bt_conn *conn, uint16_t psm, struct bt_avctp *session)
{
	if (!session) {
		return -EINVAL;
	}

	session->br_chan.chan.ops = &ops;
	return bt_l2cap_chan_connect(conn, &session->br_chan.chan, psm);
}

int bt_avctp_disconnect(struct bt_avctp *session)
{
	if (!session) {
		return -EINVAL;
	}

	LOG_DBG("session %p", session);

	return bt_l2cap_chan_disconnect(&session->br_chan.chan);
}

void bt_avctp_set_header(struct bt_avctp_header *avctp_hdr, bt_avctp_cr_t cr,
			 bt_avctp_pkt_type_t pkt_type, bt_avctp_ipid_t ipid,
			 uint8_t tid, uint16_t pid)
{
	LOG_DBG("");

	BT_AVCTP_HDR_SET_TRANSACTION_LABLE(avctp_hdr, tid);
	BT_AVCTP_HDR_SET_PACKET_TYPE(avctp_hdr, pkt_type);
	BT_AVCTP_HDR_SET_CR(avctp_hdr, cr);
	BT_AVCTP_HDR_SET_IPID(avctp_hdr, ipid);
	avctp_hdr->pid = pid;
}

struct net_buf *bt_avctp_create_pdu(struct bt_avctp *session, bt_avctp_cr_t cr,
				    bt_avctp_pkt_type_t pkt_type, bt_avctp_ipid_t ipid,
				    uint8_t tid, uint16_t pid)
{
	struct net_buf *buf;
	struct bt_avctp_header *hdr;

	LOG_DBG("");

	buf = bt_l2cap_create_pdu(NULL, 0);
	if (!buf) {
		LOG_ERR("No buff available");
		return buf;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	bt_avctp_set_header(hdr, cr, pkt_type, ipid, tid, pid);

	LOG_DBG("cr:0x%lX, tid:0x%02lX", BT_AVCTP_HDR_GET_CR(hdr),
		BT_AVCTP_HDR_GET_TRANSACTION_LABLE(hdr));
	return buf;
}

int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf)
{
	return bt_l2cap_chan_send(&session->br_chan.chan, buf);
}

static int avctp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			      struct bt_l2cap_chan **chan)
{
	struct bt_avctp *session = NULL;
	struct bt_avctp_server *avctp_server;
	int err;

	LOG_DBG("conn %p", conn);

	avctp_server = CONTAINER_OF(server, struct bt_avctp_server, l2cap);

	k_sem_take(&avctp_server_lock, K_FOREVER);

	if (!sys_slist_find(&avctp_l2cap_server, &avctp_server->node, NULL)) {
		LOG_WRN("Invalid l2cap server");
		k_sem_give(&avctp_server_lock);
		return -EINVAL;
	}

	k_sem_give(&avctp_server_lock);

	/* Get the AVCTP session from upper layer */
	err = avctp_server->accept(conn, &session);
	if (err < 0) {
		LOG_ERR("Incoming connection rejected");
		return err;
	}

	session->br_chan.chan.ops = &ops;
	*chan = &session->br_chan.chan;

	return 0;
}

int bt_avctp_server_register(struct bt_avctp_server *server)
{
	int err;
	LOG_DBG("");

	if ((server == NULL) || (server->accept == NULL)) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	k_sem_take(&avctp_server_lock, K_FOREVER);

	if (sys_slist_find(&avctp_l2cap_server, &server->node, NULL)) {
		LOG_WRN("L2CAP server has been registered");
		k_sem_give(&avctp_server_lock);
		return -EEXIST;
	}

	server->l2cap.accept = avctp_l2cap_accept;
	err = bt_l2cap_br_server_register(&server->l2cap);
	if (err < 0) {
		LOG_ERR("AVCTP L2CAP registration failed %d", err);
		k_sem_give(&avctp_server_lock);
		return err;
	}

	LOG_DBG("Register L2CAP server %p", server);
	sys_slist_append(&avctp_l2cap_server, &server->node);

	k_sem_give(&avctp_server_lock);

	return err;
}

int bt_avctp_init(void)
{
	LOG_DBG("Initializing AVCTP");
	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&avctp_server_lock, 1, 1);
	return 0;
}
