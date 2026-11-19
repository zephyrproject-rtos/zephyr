/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-FileCopyrightText: Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_internal.h"

#define SIG_CID      0x0001U
#define CONNLESS_CID 0x0002U
#define ECHO_REQ     0x08U
#define ECHO_RSP     0x09U
#define CONNLESS_PSM 0x1001U
#define TEST_PSM     0x1003U
#define MAX_CALLS    2
#define MAX_DATA     8

/* What one callback invocation saw */
struct seen {
	uint8_t ident;
	uint16_t len;
	uint8_t data[MAX_DATA];
};

/* Calls seen by each of two registered callbacks */
struct cb_log {
	struct seen calls[MAX_CALLS];
	size_t count;
};

static struct cb_log echo_log[2];
static struct cb_log echo_rsp_log[2];
static struct cb_log connless_log[2];
static struct bt_conn *conn;
static size_t connected_count;
static size_t disconnected_count;
static struct bt_l2cap_chan *connected_chan;

NET_BUF_POOL_FIXED_DEFINE(pdu_pool, 1, 32, 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_L2CAP_BR_ECHO_REQ_RESERVE + MAX_DATA,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* Records the data and then consumes it, as a callback parsing it would */
static void record(struct cb_log *rec, uint8_t ident, struct net_buf *buf)
{
	struct seen *s;

	zassert_true(rec->count < ARRAY_SIZE(rec->calls), "Too many calls");
	zassert_true(buf->len <= sizeof(s->data));

	s = &rec->calls[rec->count++];
	s->ident = ident;
	s->len = buf->len;
	memcpy(s->data, buf->data, buf->len);

	(void)net_buf_pull(buf, buf->len);
}

static void check(const struct cb_log *rec, size_t call, uint8_t ident, const char *data)
{
	const struct seen *s = &rec->calls[call];

	zassert_equal(s->ident, ident, "call %zu: ident %u", call, s->ident);
	zassert_equal(s->len, strlen(data), "call %zu: len %u", call, s->len);
	zassert_mem_equal(s->data, data, strlen(data), "call %zu: data", call);
}

static void echo_req_0(struct bt_conn *acl, uint8_t ident, struct net_buf *buf)
{
	record(&echo_log[0], ident, buf);
}

static void echo_req_1(struct bt_conn *acl, uint8_t ident, struct net_buf *buf)
{
	record(&echo_log[1], ident, buf);
}

static void echo_rsp_0(struct bt_conn *acl, struct net_buf *buf)
{
	record(&echo_rsp_log[0], 0, buf);
}

static void echo_rsp_1(struct bt_conn *acl, struct net_buf *buf)
{
	record(&echo_rsp_log[1], 0, buf);
}

static struct bt_l2cap_br_echo_cb echo_cb[] = {
	{ .req = echo_req_0, .rsp = echo_rsp_0 },
	{ .req = echo_req_1, .rsp = echo_rsp_1 },
};

static void connless_recv_0(struct bt_conn *acl, uint16_t psm, struct net_buf *buf)
{
	zassert_equal(psm, CONNLESS_PSM);
	record(&connless_log[0], 0, buf);
}

static void connless_recv_1(struct bt_conn *acl, uint16_t psm, struct net_buf *buf)
{
	zassert_equal(psm, CONNLESS_PSM);
	record(&connless_log[1], 0, buf);
}

/* One callback for all PSMs and one for CONNLESS_PSM, so both get the frame */
static struct bt_l2cap_br_connless_cb connless_cb[] = {
	{ .psm = 0, .sec_level = BT_SECURITY_L0, .recv = connless_recv_0 },
	{ .psm = CONNLESS_PSM, .sec_level = BT_SECURITY_L0, .recv = connless_recv_1 },
};

static struct net_buf *l2cap_pdu(uint16_t cid)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&pdu_pool, K_NO_WAIT);
	zassert_not_null(buf);

	/* Length, filled in by send_pdu() */
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, cid);

	return buf;
}

static void send_pdu(struct net_buf *buf)
{
	sys_put_le16(buf->len - 4U, buf->data);
	bt_l2cap_br_recv(conn, buf);
}

static void add_sig_cmd(struct net_buf *buf, uint8_t code, uint8_t ident, const void *data,
			size_t len)
{
	net_buf_add_u8(buf, code);
	net_buf_add_u8(buf, ident);
	net_buf_add_le16(buf, len);
	net_buf_add_mem(buf, data, len);
}

static void send_sig_cmd(uint8_t code, uint8_t ident, const void *data, size_t len)
{
	struct net_buf *buf = l2cap_pdu(SIG_CID);

	add_sig_cmd(buf, code, ident, data, len);
	send_pdu(buf);
}

static struct bt_l2cap_br_chan *sig_chan(void)
{
	struct bt_l2cap_chan *sig = bt_l2cap_br_lookup_rx_cid(conn, SIG_CID);

	zassert_not_null(sig);

	return CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan);
}

static void complete_sig_cmd(struct net_buf *buf)
{
	bt_conn_tx_cb_t cb = closure_cb(buf->user_data);

	if (cb != NULL) {
		cb(conn, closure_data(buf->user_data), 0);
	}

	net_buf_unref(buf);
}

static struct net_buf *recv_sig_cmd(uint8_t code)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(&sig_chan()->_pdu_tx_queue)) != NULL) {
		struct net_buf *buf = CONTAINER_OF(node, struct net_buf, node);
		struct bt_l2cap_sig_hdr *hdr;

		zassert_true(buf->len >= BT_L2CAP_HDR_SIZE + sizeof(*hdr));
		net_buf_pull(buf, BT_L2CAP_HDR_SIZE);
		hdr = (void *)buf->data;
		zassert_equal(sys_le16_to_cpu(hdr->len), buf->len - sizeof(*hdr));
		if (hdr->code == code) {
			return buf;
		}

		complete_sig_cmd(buf);
	}

	zassert_unreachable("No signaling command with code 0x%02x", code);
	return NULL;
}

static void chan_connected(struct bt_l2cap_chan *chan)
{
	connected_chan = chan;
	connected_count++;
}

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
	disconnected_count++;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.connected = chan_connected,
	.disconnected = chan_disconnected,
};

static uint8_t start_connect(struct bt_l2cap_br_chan *chan, uint16_t *local_cid)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_req *req;
	struct net_buf *buf;
	uint8_t ident;

	zassert_ok(bt_l2cap_br_chan_connect(conn, &chan->chan, TEST_PSM));
	buf = recv_sig_cmd(BT_L2CAP_CONN_REQ);
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	ident = hdr->ident;
	zassert_equal(buf->len, sizeof(*req));
	req = (void *)buf->data;
	zassert_equal(sys_le16_to_cpu(req->psm), TEST_PSM);
	*local_cid = sys_le16_to_cpu(req->scid);
	complete_sig_cmd(buf);

	return ident;
}

static uint8_t respond_connect(uint16_t local_cid, uint16_t peer_cid, uint8_t ident)
{
	struct bt_l2cap_conn_rsp rsp = {
		.dcid = sys_cpu_to_le16(peer_cid),
		.scid = sys_cpu_to_le16(local_cid),
		.result = sys_cpu_to_le16(BT_L2CAP_BR_SUCCESS),
	};
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conf_req *req;
	struct net_buf *buf;

	send_sig_cmd(BT_L2CAP_CONN_RSP, ident, &rsp, sizeof(rsp));
	buf = recv_sig_cmd(BT_L2CAP_CONF_REQ);
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	ident = hdr->ident;
	zassert_true(buf->len >= sizeof(*req));
	req = (void *)buf->data;
	zassert_equal(sys_le16_to_cpu(req->dcid), peer_cid);
	complete_sig_cmd(buf);

	return ident;
}

static struct net_buf *send_conf_req(uint16_t local_cid, uint16_t peer_cid, uint8_t ident)
{
	struct bt_l2cap_conf_req req = {
		.dcid = sys_cpu_to_le16(local_cid),
	};
	struct bt_l2cap_conf_rsp *rsp;
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *buf;

	send_sig_cmd(BT_L2CAP_CONF_REQ, ident, &req, sizeof(req));
	buf = recv_sig_cmd(BT_L2CAP_CONF_RSP);
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(hdr->ident, ident);
	zassert_equal(buf->len, sizeof(*rsp));
	rsp = (void *)buf->data;
	zassert_equal(sys_le16_to_cpu(rsp->scid), peer_cid);
	zassert_equal(sys_le16_to_cpu(rsp->result), BT_L2CAP_CONF_SUCCESS);

	return buf;
}

static void send_conf_rsp(uint16_t local_cid, uint8_t ident)
{
	struct bt_l2cap_conf_rsp rsp = {
		.scid = sys_cpu_to_le16(local_cid),
		.result = sys_cpu_to_le16(BT_L2CAP_CONF_SUCCESS),
	};

	send_sig_cmd(BT_L2CAP_CONF_RSP, ident, &rsp, sizeof(rsp));
}

/* Send an Echo Request and return its identifier, taken from the queued PDU */
static uint8_t send_echo_req(void)
{
	struct net_buf *buf;
	int err;

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);

	err = bt_l2cap_br_echo_req(conn, buf);
	zassert_equal(err, 0, "err %d", err);
	zassert_equal_ptr(sys_slist_peek_tail(&sig_chan()->_pdu_tx_queue), &buf->node);

	/* L2CAP header, then the signaling header code and identifier */
	return buf->data[5];
}

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };

	ARRAY_FOR_EACH_PTR(echo_cb, cb) {
		zassert_ok(bt_l2cap_br_echo_cb_register(cb));
	}

	ARRAY_FOR_EACH_PTR(connless_cb, cb) {
		zassert_ok(bt_l2cap_br_connless_register(cb));
	}

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(echo_log, 0, sizeof(echo_log));
	memset(echo_rsp_log, 0, sizeof(echo_rsp_log));
	memset(connless_log, 0, sizeof(connless_log));
	connected_count = 0;
	disconnected_count = 0;
	connected_chan = NULL;

	bt_l2cap_br_connected(conn);
}

/* Release the PDUs queued for sending, as there is no controller */
static void after(void *fixture)
{
	sys_snode_t *node;

	ARG_UNUSED(fixture);

	while ((node = sys_slist_get(&sig_chan()->_pdu_tx_queue)) != NULL) {
		net_buf_unref(CONTAINER_OF(node, struct net_buf, node));
	}

	bt_l2cap_br_disconnected(conn);
}

ZTEST_SUITE(l2cap_br_sig, NULL, setup, before, after, NULL);

static void test_dynamic_chan_reuse_tx_cid(bool a_first)
{
	static struct bt_l2cap_br_chan chan_a = {
		.chan.ops = &chan_ops,
	};
	static struct bt_l2cap_br_chan chan_c = {
		.chan.ops = &chan_ops,
	};
	const uint16_t first_peer_b_cid = 0x0040U;
	const uint16_t peer_d_cid = first_peer_b_cid;
	const uint16_t second_peer_b_cid = 0x0041U;
	struct bt_l2cap_disconn_req disconn_req;
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *pending_conf_rsp;
	struct net_buf *buf;
	uint16_t a_cid;
	uint16_t c_cid;
	uint8_t a_conn_ident;
	uint8_t a_conf_ident;
	uint8_t c_conn_ident;
	uint8_t c_conf_ident;

	a_conn_ident = start_connect(&chan_a, &a_cid);
	a_conf_ident = respond_connect(a_cid, first_peer_b_cid, a_conn_ident);
	complete_sig_cmd(send_conf_req(a_cid, first_peer_b_cid, 0x40U));
	send_conf_rsp(a_cid, a_conf_ident);
	zassert_equal(connected_count, 1);
	zassert_equal_ptr(connected_chan, &chan_a.chan);

	disconn_req.dcid = sys_cpu_to_le16(a_cid);
	disconn_req.scid = sys_cpu_to_le16(first_peer_b_cid);
	send_sig_cmd(BT_L2CAP_DISCONN_REQ, 0x41U, &disconn_req, sizeof(disconn_req));
	buf = recv_sig_cmd(BT_L2CAP_DISCONN_RSP);
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(hdr->ident, 0x41U);
	zassert_equal(buf->len, sizeof(struct bt_l2cap_disconn_rsp));
	zassert_mem_equal(buf->data, &disconn_req, sizeof(disconn_req));
	complete_sig_cmd(buf);
	zassert_equal(disconnected_count, 1);

	bt_l2cap_br_disconnected(conn);
	bt_l2cap_br_connected(conn);

	if (a_first) {
		a_conn_ident = start_connect(&chan_a, &a_cid);
	}

	c_conn_ident = start_connect(&chan_c, &c_cid);
	c_conf_ident = respond_connect(c_cid, peer_d_cid, c_conn_ident);
	pending_conf_rsp = send_conf_req(c_cid, peer_d_cid, 0x42U);

	if (!a_first) {
		a_conn_ident = start_connect(&chan_a, &a_cid);
	}

	/* Keep A's connection response pending until C connects. */
	zassert_equal(connected_count, 1);
	complete_sig_cmd(pending_conf_rsp);
	send_conf_rsp(c_cid, c_conf_ident);
	zassert_equal(connected_count, 2, "C/D did not finish configuring");
	zassert_equal_ptr(connected_chan, &chan_c.chan, "C/D connected the wrong local channel");

	a_conf_ident = respond_connect(a_cid, second_peer_b_cid, a_conn_ident);
	complete_sig_cmd(send_conf_req(a_cid, second_peer_b_cid, 0x43U));
	send_conf_rsp(a_cid, a_conf_ident);
	zassert_equal(connected_count, 3, "A/B did not finish configuring");
	zassert_equal_ptr(connected_chan, &chan_a.chan, "A/B connected the wrong local channel");
}

ZTEST(l2cap_br_sig, test_dynamic_chan_reuse_tx_cid_a_first)
{
	test_dynamic_chan_reuse_tx_cid(true);
}

ZTEST(l2cap_br_sig, test_dynamic_chan_reuse_tx_cid_c_first)
{
	test_dynamic_chan_reuse_tx_cid(false);
}

/* Each Echo Request callback sees exactly the data of that request, although
 * another request follows in the same signaling packet and the other callback
 * consumes the data.
 */
ZTEST(l2cap_br_sig, test_echo_req)
{
	struct net_buf *buf = l2cap_pdu(SIG_CID);

	add_sig_cmd(buf, ECHO_REQ, 1, "abc", 3);
	add_sig_cmd(buf, ECHO_REQ, 2, "xy", 2);
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(echo_log, rec) {
		zassert_equal(rec->count, 2);
		check(rec, 0, 1, "abc");
		check(rec, 1, 2, "xy");
	}
}

/* Each Echo Response callback sees exactly the data of the response */
ZTEST(l2cap_br_sig, test_echo_rsp)
{
	struct net_buf *buf;
	uint8_t ident;

	ident = send_echo_req();

	buf = l2cap_pdu(SIG_CID);
	add_sig_cmd(buf, ECHO_RSP, ident, "pq", 2);
	add_sig_cmd(buf, ECHO_REQ, (uint8_t)(ident + 1U), "z", 1);
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(echo_rsp_log, rec) {
		zassert_equal(rec->count, 1);
		check(rec, 0, 0, "pq");
	}
}

/* Each connectionless callback sees the whole data, although the other one consumes it */
ZTEST(l2cap_br_sig, test_connless_recv)
{
	struct net_buf *buf = l2cap_pdu(CONNLESS_CID);

	net_buf_add_le16(buf, CONNLESS_PSM);
	net_buf_add_mem(buf, "hello", strlen("hello"));
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(connless_log, rec) {
		zassert_equal(rec->count, 1);
		check(rec, 0, 0, "hello");
	}
}
