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

static const struct bt_avctp_event_cb *event_cb;

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
	bt_avctp_pkt_type_t pkt_type;
	bt_avctp_cr_t cr;
	int err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("invalid AVCTP header received");
		return -EINVAL;
	}

	tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(hdr);
	pkt_type = BT_AVCTP_HDR_GET_PACKET_TYPE(hdr);
	cr = BT_AVCTP_HDR_GET_CR(hdr);

	switch (pkt_type) {
	case BT_AVCTP_PKT_TYPE_SINGLE:
		break;
	case BT_AVCTP_PKT_TYPE_START:
	case BT_AVCTP_PKT_TYPE_CONTINUE:
	case BT_AVCTP_PKT_TYPE_END:
	default:
		LOG_ERR("fragmented AVCTP message is not supported, pkt_type = %d", pkt_type);
		return -EINVAL;
	}

	switch (hdr->pid) {
#if defined(CONFIG_BT_AVRCP)
	case sys_cpu_to_be16(BT_SDP_AV_REMOTE_SVCLASS):
		break;
#endif
	default:
		LOG_ERR("unsupported AVCTP PID received: 0x%04x", sys_be16_to_cpu(hdr->pid));
		if (cr == BT_AVCTP_CMD) {
			rsp = bt_avctp_create_pdu(session, BT_AVCTP_RESPONSE,
						  BT_AVCTP_PKT_TYPE_SINGLE, BT_AVCTP_IPID_INVALID,
						  tid, hdr->pid);
			if (!rsp) {
				return -ENOMEM;
			}

			err = bt_avctp_send(session, rsp);
			if (err < 0) {
				net_buf_unref(rsp);
				LOG_ERR("AVCTP send fail, err = %d", err);
				return err;
			}
		}
		return 0; /* No need to report to the upper layer */
	}

	return session->ops->recv(session, buf);
}

static const struct bt_l2cap_chan_ops ops = {
	.connected = avctp_l2cap_connected,
	.disconnected = avctp_l2cap_disconnected,
	.encrypt_change = avctp_l2cap_encrypt_changed,
	.recv = avctp_l2cap_recv,
};

int bt_avctp_connect(struct bt_conn *conn, struct bt_avctp *session)
{
	if (!session) {
		return -EINVAL;
	}

	session->br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	session->br_chan.chan.ops = &ops;
	session->br_chan.required_sec_level = BT_SECURITY_L2;

	return bt_l2cap_chan_connect(conn, &session->br_chan.chan, BT_L2CAP_PSM_AVCTP);
}

int bt_avctp_disconnect(struct bt_avctp *session)
{
	if (!session) {
		return -EINVAL;
	}

	LOG_DBG("session %p", session);

	return bt_l2cap_chan_disconnect(&session->br_chan.chan);
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
	BT_AVCTP_HDR_SET_TRANSACTION_LABLE(hdr, tid);
	BT_AVCTP_HDR_SET_PACKET_TYPE(hdr, pkt_type);
	BT_AVCTP_HDR_SET_CR(hdr, cr);
	BT_AVCTP_HDR_SET_IPID(hdr, ipid);
	hdr->pid = pid;

	LOG_DBG("cr:0x%lX, tid:0x%02lX", BT_AVCTP_HDR_GET_CR(hdr),
		BT_AVCTP_HDR_GET_TRANSACTION_LABLE(hdr));
	return buf;
}

int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf)
{
	return bt_l2cap_chan_send(&session->br_chan.chan, buf);
}

int bt_avctp_register(const struct bt_avctp_event_cb *cb)
{
	LOG_DBG("");

	if (event_cb) {
		return -EALREADY;
	}

	event_cb = cb;

	return 0;
}

static int avctp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			      struct bt_l2cap_chan **chan)
{
	struct bt_avctp *session = NULL;
	int err;

	LOG_DBG("conn %p", conn);

	if (!event_cb) {
		LOG_WRN("AVCTP server is unsupported");
		return -ENOTSUP;
	}

	/* Get the AVCTP session from upper layer */
	err = event_cb->accept(conn, &session);
	if (err < 0) {
		LOG_ERR("Get the AVCTP session failed %d", err);
		return err;
	}

	session->br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	session->br_chan.psm = BT_L2CAP_PSM_AVCTP;
	session->br_chan.chan.ops = &ops;
	*chan = &session->br_chan.chan;

	return 0;
}

int bt_avctp_init(void)
{
	int err;
	static struct bt_l2cap_server avctp_l2cap = {
		.psm = BT_L2CAP_PSM_AVCTP,
		.sec_level = BT_SECURITY_L2,
		.accept = avctp_l2cap_accept,
	};

	LOG_DBG("");

	/* Register AVCTP PSM with L2CAP */
	err = bt_l2cap_br_server_register(&avctp_l2cap);
	if (err < 0) {
		LOG_ERR("AVCTP L2CAP registration failed %d", err);
	}

	return err;
}
