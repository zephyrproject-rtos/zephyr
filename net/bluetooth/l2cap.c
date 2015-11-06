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

#if !defined(CONFIG_BLUETOOTH_DEBUG_L2CAP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define L2CAP_LE_MIN_MTU		23
#define L2CAP_LE_MAX_CREDITS		(CONFIG_BLUETOOTH_ACL_IN_COUNT - 1)
#define L2CAP_LE_CREDITS_THRESHOLD	(L2CAP_LE_MAX_CREDITS / 2)

#define L2CAP_LE_DYN_CID_START	0x0040
#define L2CAP_LE_DYN_CID_END	0x007f

#define L2CAP_LE_PSM_START	0x0001
#define L2CAP_LE_PSM_END	0x00ff

/* Size of MTU is based on the maximum amount of data the buffer can hold
 * excluding ACL and driver headers.
 */
#define BT_L2CAP_MAX_LE_MPS	CONFIG_BLUETOOTH_L2CAP_IN_MTU
/* For now use MPS - SDU length to disable segmentation */
#define BT_L2CAP_MAX_LE_MTU	(BT_L2CAP_MAX_LE_MPS - 2)

#define l2cap_lookup_ident(conn, ident) __l2cap_lookup_ident(conn, ident, false)
#define l2cap_remove_ident(conn, ident) __l2cap_lookup_ident(conn, ident, true)

static struct bt_l2cap_fixed_chan *channels;
#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
static struct bt_l2cap_server *servers;
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */

/* Pool for outgoing LE signaling packets, MTU is 23 */
static struct nano_fifo le_sig;
static NET_BUF_POOL(le_pool, CONFIG_BLUETOOTH_MAX_CONN, BT_L2CAP_BUF_SIZE(23),
		    &le_sig, NULL, 0);

/* L2CAP signalling channel specific context */
struct bt_l2cap {
	/* The channel this context is associated with */
	struct bt_l2cap_chan	chan;

	uint8_t			ident;
};

static struct bt_l2cap bt_l2cap_pool[CONFIG_BLUETOOTH_MAX_CONN];

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

	/*
	 * No action needed if there's already a CID allocated, e.g. in
	 * the case of a fixed channel.
	 */
	if (chan->rx.cid > 0) {
		return;
	}

	/* TODO: Check conn type before assigning cid */
	for (cid = L2CAP_LE_DYN_CID_START; cid <= L2CAP_LE_DYN_CID_END; cid++) {
		if (!bt_l2cap_lookup_rx_cid(conn, cid)) {
			chan->rx.cid = cid;
			return;
		}
	}
}

static int l2cap_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan)
{
	l2cap_chan_alloc_cid(conn, chan);

	if (!chan->rx.cid) {
		BT_ERR("Unable to allocate L2CAP CID\n");
		return -ENOMEM;
	}

	/* Attach channel to the connection */
	chan->_next = conn->channels;
	conn->channels = chan;
	chan->conn = conn;

	BT_DBG("conn %p chan %p cid 0x%04x\n", conn, chan, chan->rx.cid);

	return 0;
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
		chan->tx.cid = fchan->cid;

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

struct net_buf *bt_l2cap_create_pdu(struct nano_fifo *fifo)
{
	return bt_conn_create_pdu(fifo, sizeof(struct bt_l2cap_hdr));
}

void bt_l2cap_send(struct bt_conn *conn, uint16_t cid, struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr;

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));
	hdr->cid = sys_cpu_to_le16(cid);

	bt_conn_send(conn, buf);
}

static void l2cap_send_reject(struct bt_conn *conn, uint8_t ident,
			      uint16_t reason)
{
	struct bt_l2cap_cmd_reject *rej;
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(&le_sig);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CMD_REJECT;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rej));

	rej = net_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(reason);

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
		l2cap_send_reject(conn, ident, BT_L2CAP_REJ_NOT_UNDERSTOOD);
		return;
	}

	min = sys_le16_to_cpu(req->min_interval);
	max = sys_le16_to_cpu(req->max_interval);
	latency = sys_le16_to_cpu(req->latency);
	timeout = sys_le16_to_cpu(req->timeout);

	BT_DBG("min 0x%4.4x max 0x%4.4x latency: 0x%4.4x timeout: 0x%4.4x",
	       min, max, latency, timeout);

	buf = bt_l2cap_create_pdu(&le_sig);
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
	if (server->psm < L2CAP_LE_PSM_START ||
	    server->psm > L2CAP_LE_PSM_END || !server->accept) {
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

	buf = bt_l2cap_create_pdu(&le_sig);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_LE_CONN_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	memset(rsp, 0, sizeof(*rsp));

	/* Check if there is a server registered */
	server = l2cap_server_lookup_psm(psm);
	if (!psm) {
		rsp->result = BT_L2CAP_ERR_PSM_NOT_SUPP;
		goto rsp;
	}

	/* TODO: Add security check */

	if (scid < L2CAP_LE_DYN_CID_START || scid > L2CAP_LE_DYN_CID_END) {
		rsp->result = BT_L2CAP_ERR_INVALID_SCID;
		goto rsp;
	}

	chan = bt_l2cap_lookup_tx_cid(conn, scid);
	if (chan) {
		rsp->result = BT_L2CAP_ERR_SCID_IN_USE;
		goto rsp;
	}

	/* Request server to accept the new connection and allocate the
	 * channel.
	 *
	 * TODO: Handle different errors, it may be required to respond async.
	 */
	if (server->accept(conn, &chan) < 0) {
		rsp->result = BT_L2CAP_ERR_NO_RESOURCES;
		goto rsp;
	}

	/* Init TX parameters */
	chan->tx.cid = scid;
	chan->tx.mps = mps;
	chan->tx.mtu = mtu;
	chan->tx.credits = credits;

	/* Init RX parameters */
	chan->rx.mps = BT_L2CAP_MAX_LE_MPS;
	/* TODO: Once segmentation is supported these can be different */
	chan->rx.mtu = BT_L2CAP_MAX_LE_MTU;
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

static struct bt_l2cap_chan *l2cap_remove_tx_cid(struct bt_conn *conn,
						 uint16_t cid)
{
	struct bt_l2cap_chan *chan, *prev;

	for (chan = conn->channels, prev = NULL; chan;
	     prev = chan, chan = chan->_next) {
		if (chan->tx.cid != cid) {
			continue;
		}

		if (!prev) {
			conn->channels = chan->_next;
		} else {
			prev->_next = chan->_next;
		}

		return chan;
	}

	return NULL;
}

static void l2cap_chan_del(struct bt_l2cap_chan *chan)
{
	BT_DBG("conn %p chan %p cid 0x%04x\n", chan->conn, chan, chan->rx.cid);

	chan->conn = NULL;

	if (chan->ops->disconnected) {
		chan->ops->disconnected(chan);
	}
}

static void le_disconn_req(struct bt_l2cap *l2cap, uint8_t ident,
			   struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_disconn_req *req = (void *)buf->data;
	struct bt_l2cap_disconn_rsp *rsp;
	struct bt_l2cap_sig_hdr *hdr;
	uint16_t scid, dcid;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small LE conn req packet size\n");
		return;
	}

	dcid = sys_le16_to_cpu(req->dcid);
	scid = sys_le16_to_cpu(req->scid);

	BT_DBG("scid 0x%04x dcid 0x%04x\n", dcid, scid);

	chan = l2cap_remove_tx_cid(conn, scid);
	if (!chan) {
		l2cap_send_reject(conn, ident, BT_L2CAP_REJ_INVALID_CID);
		return;
	}

	buf = bt_l2cap_create_pdu(&le_sig);
	if (!buf) {
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_DISCONN_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->dcid = sys_cpu_to_le16(chan->rx.cid);
	rsp->scid = sys_cpu_to_le16(chan->tx.cid);

	l2cap_chan_del(chan);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static struct bt_l2cap_chan *__l2cap_lookup_ident(struct bt_conn *conn,
						  uint16_t ident, bool remove)
{
	struct bt_l2cap_chan *chan, *prev;

	for (chan = conn->channels, prev = NULL; chan;
	     prev = chan, chan = chan->_next) {
		if (chan->_ident != ident) {
			continue;
		}

		if (!remove) {
			return chan;
		}

		if (!prev) {
			conn->channels = chan->_next;
		} else {
			prev->_next = chan->_next;
		}

		return chan;
	}

	return NULL;
}

static void le_conn_rsp(struct bt_l2cap *l2cap, uint8_t ident,
			struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_le_conn_rsp *rsp = (void *)buf->data;
	uint16_t dcid, mtu, mps, credits, result;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE conn rsp packet size\n");
		return;
	}

	dcid = sys_le16_to_cpu(rsp->dcid);
	mtu = sys_le16_to_cpu(rsp->mtu);
	mps = sys_le16_to_cpu(rsp->mps);
	credits = sys_le16_to_cpu(rsp->credits);
	result = sys_le16_to_cpu(rsp->result);

	BT_DBG("dcid 0x%04x mtu %u mps %u credits %u result 0x%04x\n", dcid,
	       mtu, mps, credits, result);

	if (result == BT_L2CAP_SUCCESS) {
		chan = l2cap_lookup_ident(conn, ident);
	} else {
		chan = l2cap_remove_ident(conn, ident);
	}

	if (!chan) {
		BT_ERR("Cannot find channel for ident %u\n", ident);
		return;
	}

	switch (result) {
	case BT_L2CAP_SUCCESS:
		/* Reset _ident since it is no longer pending */
		chan->_ident = 0;
		chan->tx.cid = dcid;
		chan->tx.mtu = mtu;
		chan->tx.mps = mps;
		chan->tx.credits = credits;

		if (chan->ops->connected) {
			chan->ops->connected(chan);
		}

		break;
	/* TODO: Retry on Authentication and Encryption errors */
	default:
		l2cap_chan_del(chan);
	}
}

static void le_disconn_rsp(struct bt_l2cap *l2cap, uint8_t ident,
			   struct net_buf *buf)
{
	struct bt_conn *conn = l2cap->chan.conn;
	struct bt_l2cap_chan *chan;
	struct bt_l2cap_disconn_rsp *rsp = (void *)buf->data;
	uint16_t dcid, scid;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE disconn rsp packet size\n");
		return;
	}

	dcid = sys_le16_to_cpu(rsp->dcid);
	scid = sys_le16_to_cpu(rsp->scid);

	BT_DBG("dcid 0x%04x scid 0x%04x\n", dcid, scid);

	chan = l2cap_remove_tx_cid(conn, scid);
	if (!chan) {
		return;
	}

	l2cap_chan_del(chan);
}
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */

static void l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap *l2cap = CONTAINER_OF(chan, struct bt_l2cap, chan);
	struct bt_l2cap_sig_hdr *hdr = (void *)buf->data;
	uint16_t len;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP LE signaling PDU\n");
		return;
	}

	len = sys_le16_to_cpu(hdr->len);
	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("LE signaling code 0x%02x ident %u len %u\n", hdr->code,
	       hdr->ident, len);

	if (buf->len != len) {
		BT_ERR("L2CAP length mismatch (%u != %u)\n", buf->len, len);
		return;
	}

	if (!hdr->ident) {
		BT_ERR("Invalid ident value in L2CAP PDU\n");
		return;
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
	case BT_L2CAP_LE_CONN_RSP:
		le_conn_rsp(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_DISCONN_REQ:
		le_disconn_req(l2cap, hdr->ident, buf);
		break;
	case BT_L2CAP_DISCONN_RSP:
		le_disconn_rsp(l2cap, hdr->ident, buf);
		break;
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */
	default:
		BT_WARN("Unknown L2CAP PDU code 0x%02x\n", hdr->code);
		l2cap_send_reject(chan->conn, hdr->ident,
				  BT_L2CAP_REJ_NOT_UNDERSTOOD);
		break;
	}
}

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
static void l2cap_chan_update_credits(struct bt_l2cap_chan *chan)
{
	struct net_buf *buf;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_le_credits *ev;
	uint16_t credits;

	/* Only give more credits if it went bellow the defined threshold */
	if (chan->rx.credits > L2CAP_LE_CREDITS_THRESHOLD) {
		goto done;
	}

	/* Restore credits */
	credits = L2CAP_LE_MAX_CREDITS - chan->rx.credits;
	chan->rx.credits = L2CAP_LE_MAX_CREDITS;

	buf = bt_l2cap_create_pdu(&le_sig);
	if (!buf) {
		BT_ERR("Unable to send credits\n");
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_LE_CREDITS;
	hdr->ident = get_ident(chan->conn);
	hdr->len = sys_cpu_to_le16(sizeof(*ev));

	ev = net_buf_add(buf, sizeof(*ev));
	ev->cid = sys_cpu_to_le16(chan->tx.cid);
	ev->credits = sys_cpu_to_le16(credits);

	bt_l2cap_send(chan->conn, BT_L2CAP_CID_LE_SIG, buf);

done:
	BT_DBG("chan %p credits %u\n", chan, chan->rx.credits);
}

static void l2cap_chan_le_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	uint16_t sdu_len;

	if (!chan->rx.credits) {
		BT_ERR("No credits to receive packet\n");
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	chan->rx.credits--;

	/* TODO: SDU length is only sent in the first packet, for now this is
	 * ok because MPS is the same as MTU so no segmentation should happen.
	 */
	sdu_len = net_buf_pull_le16(buf);

	BT_DBG("chan %p len %u sdu_len %u\n", chan, buf->len, sdu_len);

	if (sdu_len > chan->rx.mtu) {
		BT_ERR("Invalid SDU length\n");
		bt_l2cap_chan_disconnect(chan);
		return;
	}

	chan->ops->recv(chan, buf);

	l2cap_chan_update_credits(chan);
}
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */

static void l2cap_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
	/* TODO: Check the conn type to differentiate BR/EDR and LE or
	 * introduce a mode.
	 */
	if (chan->rx.cid >= L2CAP_LE_DYN_CID_START &&
	    chan->rx.cid <= L2CAP_LE_DYN_CID_END) {
		l2cap_chan_le_recv(chan, buf);
		return;
	}
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */

	BT_DBG("chan %p len %u\n", chan, buf->len);

	chan->ops->recv(chan, buf);
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

	l2cap_chan_recv(chan, buf);

	net_buf_unref(buf);
}

int bt_l2cap_update_conn_param(struct bt_conn *conn)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_param_req *req;
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(&le_sig);
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

void bt_l2cap_init(void)
{
	static struct bt_l2cap_fixed_chan chan = {
		.cid	= BT_L2CAP_CID_LE_SIG,
		.accept	= l2cap_accept,
	};

	net_buf_pool_init(le_pool);

	bt_l2cap_fixed_chan_register(&chan);
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

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
static int l2cap_le_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			    uint16_t psm)
{
	struct net_buf *buf;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_le_conn_req *req;

	if (psm < L2CAP_LE_PSM_START || psm > L2CAP_LE_PSM_END) {
		return -EINVAL;
	}

	if (l2cap_chan_add(conn, chan) < 0) {
		return -ENOMEM;
	}

	buf = bt_l2cap_create_pdu(&le_sig);
	if (!buf) {
		BT_ERR("Unable to send L2CP connection request\n");
		return -ENOMEM;
	}

	/* Use existing MTU if defined */
	if (!chan->rx.mtu) {
		chan->rx.mtu = BT_L2CAP_MAX_LE_MTU;
	}

	chan->rx.mps = BT_L2CAP_MAX_LE_MPS;
	chan->_ident = get_ident(chan->conn);
	memset(&chan->tx, 0, sizeof(chan->tx));

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_LE_CONN_REQ;
	hdr->ident = chan->_ident;
	hdr->len = sys_cpu_to_le16(sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->psm = sys_cpu_to_le16(psm);
	req->scid = sys_cpu_to_le16(chan->rx.cid);
	req->mtu = sys_cpu_to_le16(chan->rx.mtu);
	req->mps = sys_cpu_to_le16(chan->rx.mps);
	req->credits = sys_cpu_to_le16(L2CAP_LE_MAX_CREDITS);

	bt_l2cap_send(chan->conn, BT_L2CAP_CID_LE_SIG, buf);

	return 0;
}

int bt_l2cap_chan_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			  uint16_t psm)
{
	BT_DBG("conn %p chan %p psm 0x%04x\n", conn, chan, psm);

	if (!conn || conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (!chan) {
		return -EINVAL;
	}

	/* TODO: Check conn/address type when BR/EDR is introduced */
	return l2cap_le_connect(conn, chan, psm);
}

int bt_l2cap_chan_disconnect(struct bt_l2cap_chan *chan)
{
	struct bt_conn *conn = chan->conn;
	struct net_buf *buf;
	struct bt_l2cap_disconn_req *req;
	struct bt_l2cap_sig_hdr *hdr;

	BT_DBG("chan %p scid 0x%04x dcid 0x%04x\n", chan, chan->rx.cid,
	       chan->tx.cid);

	if (!conn) {
		return -ENOTCONN;
	}

	buf = bt_l2cap_create_pdu(&le_sig);
	if (!buf) {
		BT_ERR("Unable to send L2CP disconnect request\n");
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_DISCONN_REQ;
	hdr->ident = get_ident(conn);
	hdr->len = sys_cpu_to_le16(sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->dcid = sys_cpu_to_le16(chan->tx.cid);
	req->scid = sys_cpu_to_le16(chan->rx.cid);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	return 0;
}
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */
