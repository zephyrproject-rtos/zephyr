/* l2cap.c - L2CAP handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap.h"
#include "att.h"
#include "smp.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_L2CAP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define LE_CONN_MIN_INTERVAL	0x0028
#define LE_CONN_MAX_INTERVAL	0x0038
#define LE_CONN_LATENCY		0x0000
#define LE_CONN_TIMEOUT		0x002a

#define BT_L2CAP_CONN_PARAM_ACCEPTED 0
#define BT_L2CAP_CONN_PARAM_REJECTED 1

static struct bt_l2cap_chan *channels;

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

static uint16_t le_validate_conn_params(uint16_t min, uint16_t max,
					uint16_t latency, uint16_t timeout)
{
	uint16_t max_latency;

	if (min > max || min < 6 || max > 3200) {
		return BT_L2CAP_CONN_PARAM_REJECTED;
	}

	if (timeout < 10 || timeout > 3200) {
		return BT_L2CAP_CONN_PARAM_REJECTED;
	}

	/* calculation based on BT spec 4.2 [Vol3, PartA, 4.20]
	 * max_latency = ((timeout * 10)/(max * 1.25 * 2)) - 1;
	 */
	max_latency = (timeout * 4 / max) - 1;
	if (latency > 499 || latency > max_latency) {
		return BT_L2CAP_CONN_PARAM_REJECTED;
	}

	return BT_L2CAP_CONN_PARAM_ACCEPTED;
}

static void le_conn_param_update_req(struct bt_conn *conn, uint8_t ident,
				     struct bt_buf *buf)
{
	uint16_t min, max, latency, timeout, result;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_param_rsp *rsp;
	struct bt_l2cap_conn_param_req *req = (void *)buf->data;

	if (buf->len < sizeof(*req)) {
		BT_ERR("Too small LE conn update param req\n");
		return;
	}

	if (conn->role != BT_HCI_ROLE_MASTER) {
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

	result = le_validate_conn_params(min, max, latency, timeout);

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_PARAM_RSP;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rsp));

	rsp = bt_buf_add(buf, sizeof(*rsp));
	memset(rsp, 0, sizeof(*rsp));
	rsp->result = sys_cpu_to_le16(result);
	bt_l2cap_send(conn, BT_L2CAP_CID_LE_SIG, buf);

	if (result == BT_L2CAP_CONN_PARAM_ACCEPTED) {
		bt_conn_le_conn_update(conn, min, max, latency, timeout);
	}
}

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
	case BT_L2CAP_CONN_PARAM_REQ:
		le_conn_param_update_req(conn, hdr->ident, buf);
		break;
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

void bt_l2cap_update_conn_param(struct bt_conn *conn)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_param_req *req;
	struct bt_buf *buf;

	/* Check if we need to update anything */
	if (conn->le_conn_interval >= LE_CONN_MIN_INTERVAL &&
	    conn->le_conn_interval <= LE_CONN_MAX_INTERVAL) {
		return;
	}

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return;
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
}

void bt_l2cap_init(void)
{
	static struct bt_l2cap_chan chan = {
		.cid	= BT_L2CAP_CID_LE_SIG,
		.recv	= le_sig,
	};

	bt_att_init();
	bt_smp_init();

	bt_l2cap_chan_register(&chan);
}
