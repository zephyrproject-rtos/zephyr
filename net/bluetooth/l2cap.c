/* l2cap.c - L2CAP handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/driver.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "att.h"
#include "smp.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_L2CAP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define L2CAP_LE_MIN_MTU	23
#define L2CAP_LE_MAX_CREDITS	1

#define L2CAP_CID_START		0x0040
#define L2CAP_LE_CID_END	0x007f

static struct bt_l2cap_fixed_chan *channels;
#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
static struct bt_l2cap_server *servers;
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */

/* L2CAP signalling channel specific context */
struct bt_l2cap {
	/* The channel this context is associated with */
	struct bt_l2cap_chan	chan;

	uint8_t			ident;
};

static struct bt_l2cap bt_l2cap_pool[CONFIG_BLUETOOTH_MAX_CONN];

#define BT_BUF_ACL_OUT_MAX 7
static struct nano_fifo avail_acl_out;
static NET_BUF_POOL(acl_out_pool, BT_BUF_ACL_OUT_MAX, BT_BUF_MAX_DATA,
		    &avail_acl_out, NULL, 0);

static struct bt_l2cap *l2cap_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_lookup_rx_cid(conn, BT_L2CAP_CID_LE_SIG);
	if (!chan) {
		BT_ERR("Unable to find L2CAP Signalling channel");
		return 0;
	}

	return CONTAINER_OF(chan, struct bt_l2cap, chan);
}

static uint8_t get_ident(struct bt_conn *conn)
{

	struct bt_l2cap *l2cap;

	l2cap = l2cap_chan_get(conn);
	if (!l2cap) {
		return 0;
	}

	l2cap->ident++;

	/* handle integer overflow (0 is not valid) */
	if (!l2cap->ident) {
		l2cap->ident++;
	}

	return l2cap->ident;
}

void bt_l2cap_fixed_chan_register(struct bt_l2cap_fixed_chan *chan)
{
	BT_DBG("CID 0x%04x\n", chan->cid);

	chan->_next = channels;
	channels = chan;
}

static void l2cap_chan_alloc_cid(struct bt_conn *conn,
				 struct bt_l2cap_chan *chan)
{
	uint16_t cid;

	if (chan->rx.cid > 0) {
		return;
	}

	/* Try matching dcid first */
	if (!bt_l2cap_lookup_rx_cid(conn, chan->rx.cid)) {
		chan->rx.cid = chan->tx.cid;
		return;
	}

	/* TODO: Check conn type before assigning cid */
	for (cid = L2CAP_CID_START; cid < L2CAP_LE_CID_END; cid++) {
		if (!bt_l2cap_lookup_rx_cid(conn, cid)) {
			chan->rx.cid = cid;
			return;
		}
	}
}

static void l2cap_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan)
{
	l2cap_chan_alloc_cid(conn, chan);

	/* Attach channel to the connection */
	chan->_next = conn->channels;
	conn->channels = chan;
	chan->conn = conn;

	BT_DBG("conn %p chan %p cid 0x%04x\n", conn, chan, chan->rx.cid);
}

void bt_l2cap_connected(struct bt_conn *conn)
{
	struct bt_l2cap_fixed_chan *fchan;
	struct bt_l2cap_chan *chan;

	for (fchan = channels; fchan; fchan = fchan->_next) {
		if (fchan->accept(conn, &chan) < 0) {
			continue;
		}

		chan->rx.cid = fchan->cid;

		l2cap_chan_add(conn, chan);

		if (chan->ops->connected) {
			chan->ops->connected(chan);
		}
	}
}

void bt_l2cap_disconnected(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	for (chan = conn->channels; chan;) {
		struct bt_l2cap_chan *next;

		/* prefetch since disconnected callback may cleanup */
		next = chan->_next;

		if (chan->ops->disconnected) {
			chan->ops->disconnected(chan);
		}

		chan->conn = NULL;
		chan = next;
	}

	conn->channels = NULL;
}

void bt_l2cap_encrypt_change(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	for (chan = conn->channels; chan; chan = chan->_next) {
		if (chan->ops->encrypt_change) {
			chan->ops->encrypt_change(chan);
		}
	}
}

struct net_buf *bt_l2cap_create_pdu(struct bt_conn *conn)
{
	size_t head_reserve = sizeof(struct bt_l2cap_hdr) +
				sizeof(struct bt_hci_acl_hdr) +
				bt_dev.drv->send_reserve;

	return net_buf_get(&avail_acl_out, head_reserve);
}

void bt_l2cap_send(struct bt_conn *conn, uint16_t cid, struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr;

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));
	hdr->cid = sys_cpu_to_le16(cid);

	bt_conn_send(conn, buf);
}

static void rej_not_understood(struct bt_conn *conn, uint8_t ident)
{
	struct bt_l2cap_cmd_reject *rej;
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CMD_REJECT;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rej));

	rej = net_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(BT_L2CAP_REJ_NOT_UNDERSTOOD);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static void le_conn_param_rsp(struct bt_l2cap *l2cap, struct net_buf *buf)
{
	struct bt_l2cap_conn_param_rsp *rsp = (void *)buf->data;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE conn param rsp\n");
		return;
	}

	BT_DBG("LE conn param rsp result %u\n", sys_le16_to_cpu(rsp->result));
}

#if defined(CONFIG_BLUETOOTH_CENTRAL)
static void le_conn_param_update_req(struct bt_l2cap *l2cap, uint8_t ident,
				     struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.conn;
	uint16_t min, max, latency, timeout;
	bool params_valid;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_param_rsp *rsp;
	struct bt_l2cap_conn_param_req *req = (void *)buf->data;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small LE conn update param req\n");
		return;
	}

	if (conn->role != BT_HCI_ROLE_MASTER) {
		rej_not_understood(conn, ident);
		return;
	}

	min = sys_le16_to_cpu(req->min_interval);
	max = sys_le16_to_cpu(req->max_interval);
	latency = sys_le16_to_cpu(req->latency);
	timeout = sys_le16_to_cpu(req->timeout);

	BT_DBG("min 0x%4.4x max 0x%4.4x latency: 0x%4.4x timeout: 0x%4.4x",
	       min, max, latency, timeout);

	buf = bt_l2cap_create_pdu(l2cap->chan.conn);
	if (!buf) {
		return;
	}

	params_valid = bt_le_conn_params_valid(min, max, latency, timeout);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_PARAM_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	if (params_valid) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_ACCEPTED);
	} else {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_REJECTED);
	}

	bt_l2cap_send(l2cap->chan.conn, BT_L2CAP_CID_LE_SIG, buf);

	if (params_valid) {
		bt_conn_le_conn_update(conn, min, max, latency, timeout);
	}
}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
static struct bt_l2cap_server *l2cap_server_lookup_psm(uint16_t psm)
{
	struct bt_l2cap_server *server;

	for (server = servers; server; server = server->_next) {
		if (server->psm == psm) {
			return server;
		}
	}

	return NULL;
}

int bt_l2cap_server_register(struct bt_l2cap_server *server)
{
	if (server->psm < 0x0080 || server->psm > 0x00ff || !server->accept) {
		return -EINVAL;
	}

	/* Check if given PSM is already in use */
	if (l2cap_server_lookup_psm(server->psm)) {
		BT_DBG("PSM already registered\n");
		return -EADDRINUSE;
	}

	BT_DBG("PSM 0x%04x\n", server->psm);

	server->_next = servers;
	servers = server;

	return 0;
}

static void le_conn_req(struct bt_l2cap *l2cap, uint8_t ident,
			struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_server *server;
	struct bt_l2cap_le_conn_req *req = (void *)buf->data;
	struct bt_l2cap_le_conn_rsp *rsp;
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t psm, scid, mtu, mps, credits;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small LE conn req packet size\n");
		return;
	}

	psm = sys_le16_to_cpu(req->psm);
	scid = sys_le16_to_cpu(req->scid);
	mtu = sys_le16_to_cpu(req->mtu);
	mps = sys_le16_to_cpu(req->mps);
	credits = sys_le16_to_cpu(req->credits);

	BT_DBG("psm 0x%02x scid 0x%04x mtu %u mps %u credits %u\n", psm, scid,
	       mtu, mps, credits);

	if (mtu < L2CAP_LE_MIN_MTU || mps < L2CAP_LE_MIN_MTU) {
		BT_ERR("Invalid LE-Conn Req params\n");
		return;
	}

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_LE_CONN_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	mtu = min(mtu, net_buf_tailroom(buf));

	rsp = net_buf_add(buf, sizeof(*rsp));
	memset(rsp, 0, sizeof(*rsp));

	/* Check if there is a server registered */
	server = l2cap_server_lookup_psm(psm);
	if (!psm) {
		rsp->dcid = req->scid;
		rsp->result = BT_L2CAP_ERR_PSM_NOT_SUPP;
		goto rsp;
	}

	/* TODO: Add security check */

	chan = bt_l2cap_lookup_tx_cid(conn, scid);
	if (chan) {
		rsp->dcid = req->scid;
		rsp->result = BT_L2CAP_ERR_NO_RESOURCES;
		goto rsp;
	}

	/* Request server to accept the new connection and allocate the
	 * channel.
	 *
	 * TODO: Handle different errors, it may be required to respond async.
	 */
	if (server->accept(conn, &chan) < 0) {
		rsp->dcid = req->scid;
		rsp->result = BT_L2CAP_ERR_NO_RESOURCES;
		goto rsp;
	}

	/* Init TX parameters */
	chan->tx.cid = scid;
	chan->tx.mps = mps;
	chan->tx.mtu = mtu;
	chan->tx.credits = credits;

	/* Init RX parameters */
	chan->rx.mps = net_buf_tailroom(buf) + sizeof(*rsp);
	/* TODO: Once segmentation is supported these can be different */
	chan->rx.mtu = chan->rx.mps;
	chan->rx.credits = L2CAP_LE_MAX_CREDITS;

	l2cap_chan_add(conn, chan);

	if (chan->ops->connected) {
		chan->ops->connected(chan);
	}

	rsp->dcid = sys_cpu_to_le16(chan->rx.cid);
	rsp->mps = sys_cpu_to_le16(chan->rx.mps);
	rsp->mtu = sys_cpu_to_le16(chan->rx.mtu);
	rsp->credits = sys_cpu_to_le16(chan->rx.credits);
	rsp->result = BT_L2CAP_SUCCESS;

rsp:
	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */

static void l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap *l2cap = CONTAINER_OF(chan, struct bt_l2cap, chan);
	struct bt_l2cap_sig_hdr *hdr = (void *)buf->data;
	uint16_t len;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP LE signaling PDU\n");
		goto drop;
	}

	len = sys_le16_to_cpu(hdr->len);
	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("LE signaling code 0x%02x ident %u len %u\n", hdr->code,
	       hdr->ident, len);

	if (buf->len != len) {
		BT_ERR("L2CAP length mismatch (%u != %u)\n", buf->len, len);
		goto drop;
	}

	if (!hdr->ident) {
		BT_ERR("Invalid ident value in L2CAP PDU\n");
		goto drop;
	}

	switch (hdr->code) {
	case BT_L2CAP_CONN_PARAM_RSP:
		le_conn_param_rsp(l2cap, buf);
		break;
#if defined(CONFIG_BLUETOOTH_CENTRAL)
	case BT_L2CAP_CONN_PARAM_REQ:
		le_conn_param_update_req(l2cap, hdr->ident, buf);
		break;
#endif /* CONFIG_BLUETOOTH_CENTRAL */
#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
	case BT_L2CAP_LE_CONN_REQ:
		le_conn_req(l2cap, hdr->ident, buf);
		break;
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */
	default:
		BT_WARN("Unknown L2CAP PDU code 0x%02x\n", hdr->code);
		rej_not_understood(chan->conn, hdr->ident);
		break;
	}

drop:
	net_buf_unref(buf);
}

void bt_l2cap_recv(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr = (void *)buf->data;
	struct bt_l2cap_chan *chan;
	uint16_t cid;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP PDU received\n");
		net_buf_unref(buf);
		return;
	}

	cid = sys_le16_to_cpu(hdr->cid);
	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("Packet for CID %u len %u\n", cid, buf->len);

	chan = bt_l2cap_lookup_rx_cid(conn, cid);
	if (!chan) {
		BT_WARN("Ignoring data for unknown CID 0x%04x\n", cid);
		net_buf_unref(buf);
		return;
	}

	chan->ops->recv(chan, buf);
}

int bt_l2cap_update_conn_param(struct bt_conn *conn)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_param_req *req;
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return -ENOBUFS;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_PARAM_REQ;
	hdr->ident = get_ident(conn);
	hdr->len = sys_cpu_to_le16(sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->min_interval = sys_cpu_to_le16(LE_CONN_MIN_INTERVAL);
	req->max_interval = sys_cpu_to_le16(LE_CONN_MAX_INTERVAL);
	req->latency = sys_cpu_to_le16(LE_CONN_LATENCY);
	req->timeout = sys_cpu_to_le16(LE_CONN_TIMEOUT);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	BT_DBG("chan %p cid 0x%04x\n", chan, chan->rx.cid);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	BT_DBG("chan %p cid 0x%04x\n", chan, chan->rx.cid);
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_connected,
		.disconnected = l2cap_disconnected,
		.recv = l2cap_recv,
	};

	BT_DBG("conn %p handle %u\n", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_l2cap_pool); i++) {
		struct bt_l2cap *l2cap = &bt_l2cap_pool[i];

		if (l2cap->chan.conn) {
			continue;
		}

		l2cap->chan.ops = &ops;

		*chan = &l2cap->chan;

		return 0;
	}

	BT_ERR("No available L2CAP context for conn %p\n", conn);

	return -ENOMEM;
}

int bt_l2cap_init(void)
{
	int err;

	static struct bt_l2cap_fixed_chan chan = {
		.cid	= BT_L2CAP_CID_LE_SIG,
		.accept	= l2cap_accept,
	};

	bt_att_init();

	err = bt_smp_init();
	if (err) {
		return err;
	}

	net_buf_pool_init(acl_out_pool);

	bt_l2cap_fixed_chan_register(&chan);

	return 0;
}

struct bt_l2cap_chan *bt_l2cap_lookup_tx_cid(struct bt_conn *conn,
					     uint16_t cid)
{
	struct bt_l2cap_chan *chan;

	for (chan = conn->channels; chan; chan = chan->_next) {
		if (chan->tx.cid == cid)
			return chan;
	}

	return NULL;
}

struct bt_l2cap_chan *bt_l2cap_lookup_rx_cid(struct bt_conn *conn,
					     uint16_t cid)
{
	struct bt_l2cap_chan *chan;

	for (chan = conn->channels; chan; chan = chan->_next) {
		if (chan->rx.cid == cid) {
			return chan;
		}
	}

	return NULL;
}
