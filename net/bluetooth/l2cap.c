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

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/driver.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap.h"
#include "att.h"
#include "smp.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_L2CAP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

static struct bt_l2cap_chan *channels;
static struct bt_l2cap_server *servers;

static uint8_t get_ident(struct bt_conn *conn)
{
	conn->l2cap.ident++;

	/* handle integer overflow (0 is not valid) */
	if (!conn->l2cap.ident) {
		conn->l2cap.ident++;
	}

	return conn->l2cap.ident;
}

void bt_l2cap_chan_register(struct bt_l2cap_chan *chan)
{
	BT_DBG("CID 0x%04x\n", chan->cid);

	chan->_next = channels;
	channels = chan;
}

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

void bt_l2cap_connected(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	for (chan = channels; chan; chan = chan->_next) {
		if (chan->connected) {
			chan->connected(conn);
		}
	}
}

void bt_l2cap_disconnected(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	for (chan = channels; chan; chan = chan->_next) {
		if (chan->disconnected) {
			chan->disconnected(conn);
		}
	}
}

void bt_l2cap_encrypt_change(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	for (chan = channels; chan; chan = chan->_next) {
		if (chan->encrypt_change) {
			chan->encrypt_change(conn);
		}
	}
}

struct bt_buf *bt_l2cap_create_pdu(struct bt_conn *conn)
{
	size_t head_reserve = sizeof(struct bt_l2cap_hdr) +
				sizeof(struct bt_hci_acl_hdr) +
				bt_dev.drv->head_reserve;

	return bt_buf_get(BT_ACL_OUT, head_reserve);
}

void bt_l2cap_send(struct bt_conn *conn, uint16_t cid, struct bt_buf *buf)
{
	struct bt_l2cap_hdr *hdr;

	hdr = bt_buf_push(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));
	hdr->cid = sys_cpu_to_le16(cid);

	bt_conn_send(conn, buf);
}

static void rej_not_understood(struct bt_conn *conn, uint8_t ident)
{
	struct bt_l2cap_cmd_reject *rej;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_buf *buf;

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return;
	}

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CMD_REJECT;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rej));

	rej = bt_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(BT_L2CAP_REJ_NOT_UNDERSTOOD);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);
}

static void le_conn_param_rsp(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_l2cap_conn_param_rsp *rsp = (void *)buf->data;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE conn param rsp\n");
		return;
	}

	BT_DBG("LE conn param rsp result %u\n", sys_le16_to_cpu(rsp->result));
}

#if defined(CONFIG_BLUETOOTH_CENTRAL)
static void le_conn_param_update_req(struct bt_conn *conn, uint8_t ident,
				     struct bt_buf *buf)
{
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

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return;
	}

	params_valid = bt_le_conn_params_valid(min, max, latency, timeout);

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_PARAM_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = bt_buf_add(buf, sizeof(*rsp));
	if (params_valid) {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_ACCEPTED);
	} else {
		rsp->result = sys_cpu_to_le16(BT_L2CAP_CONN_PARAM_REJECTED);
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	if (params_valid) {
		bt_conn_le_conn_update(conn, min, max, latency, timeout);
	}
}
#endif /* CONFIG_BLUETOOTH_CENTRAL */

static void le_sig(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_l2cap_sig_hdr *hdr = (void *)buf->data;
	uint16_t len;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP LE signaling PDU\n");
		goto drop;
	}

	len = sys_le16_to_cpu(hdr->len);
	bt_buf_pull(buf, sizeof(*hdr));

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
		le_conn_param_rsp(conn, buf);
		break;
#if defined(CONFIG_BLUETOOTH_CENTRAL)
	case BT_L2CAP_CONN_PARAM_REQ:
		le_conn_param_update_req(conn, hdr->ident, buf);
		break;
#endif /* CONFIG_BLUETOOTH_CENTRAL */
	default:
		BT_WARN("Unknown L2CAP PDU code 0x%02x\n", hdr->code);
		rej_not_understood(conn, hdr->ident);
		break;
	}

drop:
	bt_buf_put(buf);
}

void bt_l2cap_recv(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_l2cap_hdr *hdr = (void *)buf->data;
	struct bt_l2cap_chan *chan;
	uint16_t cid;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP PDU received\n");
		bt_buf_put(buf);
		return;
	}

	cid = sys_le16_to_cpu(hdr->cid);
	bt_buf_pull(buf, sizeof(*hdr));

	BT_DBG("Packet for CID %u len %u\n", cid, buf->len);

	for (chan = channels; chan; chan = chan->_next) {
		if (chan->cid == cid) {
			break;
		}
	}

	if (!chan) {
		BT_WARN("Ignoring data for unknown CID 0x%04x\n", cid);
		bt_buf_put(buf);
		return;
	}

	chan->recv(conn, buf);
}

int bt_l2cap_update_conn_param(struct bt_conn *conn)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_param_req *req;
	struct bt_buf *buf;

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return -ENOBUFS;
	}

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_PARAM_REQ;
	hdr->ident = get_ident(conn);
	hdr->len = sys_cpu_to_le16(sizeof(*req));

	req = bt_buf_add(buf, sizeof(*req));
	req->min_interval = sys_cpu_to_le16(LE_CONN_MIN_INTERVAL);
	req->max_interval = sys_cpu_to_le16(LE_CONN_MAX_INTERVAL);
	req->latency = sys_cpu_to_le16(LE_CONN_LATENCY);
	req->timeout = sys_cpu_to_le16(LE_CONN_TIMEOUT);

	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	return 0;
}

int bt_l2cap_init(void)
{
	int err;

	static struct bt_l2cap_chan chan = {
		.cid	= BT_L2CAP_CID_LE_SIG,
		.recv	= le_sig,
	};

	bt_att_init();

	err = bt_smp_init();
	if (err) {
		return err;
	}

	bt_l2cap_chan_register(&chan);

	return 0;
}
