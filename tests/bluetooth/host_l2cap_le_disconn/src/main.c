/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

/* The first LE dynamic SPSM */
#define PSM        0x0080U
/* The smallest MTU and MPS of an LE credit-based channel */
#define LE_MIN_MTU 23U
/* The peer's endpoint of the channel, chosen apart from the CIDs that the Host
 * allocates from 0x0040 up so that a swapped CID shows
 */
#define PEER_CID   0x0050U

/* Identifiers of the peer's requests: the connection, then the disconnection
 * under test and the one that tears a channel down after a test
 */
#define CONN_IDENT    0x10U
#define DISCONN_IDENT 0x11U
#define CLEANUP_IDENT 0x12U

/* One signaling PDU from the peer at a time, of at most an LE Credit Based
 * Connection Request
 */
NET_BUF_POOL_FIXED_DEFINE(pdu_pool, 1,
			  sizeof(struct bt_l2cap_hdr) + sizeof(struct bt_l2cap_sig_hdr) +
				  sizeof(struct bt_l2cap_le_conn_req),
			  0, NULL);

static struct bt_conn *conn;
static struct bt_l2cap_le_chan chan;
static size_t disconnected_count;

static int chan_recv(struct bt_l2cap_chan *ch, struct net_buf *buf)
{
	ARGS_UNUSED(ch, buf);

	return 0;
}

static void chan_disconnected(struct bt_l2cap_chan *ch)
{
	zassert_equal_ptr(ch, &chan.chan);
	disconnected_count++;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.recv = chan_recv,
	.disconnected = chan_disconnected,
};

static int server_accept(struct bt_conn *acl, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **ch)
{
	ARGS_UNUSED(acl, server);

	*ch = &chan.chan;

	return 0;
}

/* Deliver a signaling command from the peer. The Host handles it before this
 * returns, as the signaling channel is a fixed channel.
 */
static void recv_sig(uint8_t code, uint8_t ident, const void *data, uint16_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&pdu_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_add_le16(buf, (uint16_t)(sizeof(struct bt_l2cap_sig_hdr) + len));
	net_buf_add_le16(buf, BT_L2CAP_CID_LE_SIG);
	(void)net_buf_add_u8(buf, code);
	(void)net_buf_add_u8(buf, ident);
	net_buf_add_le16(buf, len);
	(void)net_buf_add_mem(buf, data, len);

	bt_l2cap_recv(conn, buf, true);
}

static void recv_disconn_req(uint8_t ident, uint16_t dcid, uint16_t scid)
{
	const struct bt_l2cap_disconn_req req = {
		.dcid = sys_cpu_to_le16(dcid),
		.scid = sys_cpu_to_le16(scid),
	};

	recv_sig(BT_L2CAP_DISCONN_REQ, ident, &req, sizeof(req));
}

static struct k_fifo *sig_tx_queue(void)
{
	struct bt_l2cap_chan *sig = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_LE_SIG);

	zassert_not_null(sig);

	return &BT_L2CAP_LE_CHAN(sig)->tx_queue;
}

/* The next signaling command that the Host queued, with its header pulled.
 * There is no controller, so the PDU stays on the signaling channel's queue.
 */
static struct net_buf *tx_sig(uint8_t code, uint8_t ident)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *buf;

	buf = k_fifo_get(sig_tx_queue(), K_NO_WAIT);
	zassert_not_null(buf, "Nothing sent");

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(hdr->code, code, "code 0x%02x", hdr->code);
	zassert_equal(hdr->ident, ident, "ident 0x%02x", hdr->ident);
	zassert_equal(sys_le16_to_cpu(hdr->len), buf->len);

	return buf;
}

static void check_connected(void)
{
	zassert_equal(chan.state, BT_L2CAP_CONNECTED);
	zassert_equal_ptr(bt_l2cap_le_lookup_rx_cid(conn, chan.rx.cid), &chan.chan);
	zassert_equal(disconnected_count, 0U);
}

/* The Host rejected the Disconnection Request with the given CIDs as naming no
 * channel, and the channel is still connected
 */
static void check_rejected(uint16_t dcid, uint16_t scid)
{
	struct net_buf *buf = tx_sig(BT_L2CAP_CMD_REJECT, DISCONN_IDENT);

	zassert_equal(buf->len, sizeof(struct bt_l2cap_cmd_reject) +
					sizeof(struct bt_l2cap_cmd_reject_cid_data));
	zassert_equal(net_buf_pull_le16(buf), BT_L2CAP_REJ_INVALID_CID);
	/* The local endpoint comes first, which is the destination of the request */
	zassert_equal(net_buf_pull_le16(buf), dcid);
	zassert_equal(net_buf_pull_le16(buf), scid);
	net_buf_unref(buf);

	check_connected();
}

/* The Host discarded the Disconnection Request without an answer, and the
 * channel is still connected
 */
static void check_discarded(void)
{
	zassert_true(k_fifo_is_empty(sig_tx_queue()), "Answered");

	check_connected();
}

static void *setup(void)
{
	static const bt_addr_le_t addr = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = {.val = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}},
	};
	static struct bt_l2cap_server server = {
		.psm = PSM,
		.sec_level = BT_SECURITY_L1,
		.accept = server_accept,
	};

	zassert_ok(bt_l2cap_server_register(&server));

	/* As on Connection Complete */
	conn = bt_conn_add_le(BT_ID_DEFAULT, &addr);
	zassert_not_null(conn);
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_connected(conn);

	return NULL;
}

/* The peer connects a channel to the server */
static void before(void *fixture)
{
	const struct bt_l2cap_le_conn_req req = {
		.psm = sys_cpu_to_le16(PSM),
		.scid = sys_cpu_to_le16(PEER_CID),
		.mtu = sys_cpu_to_le16(LE_MIN_MTU),
		.mps = sys_cpu_to_le16(LE_MIN_MTU),
		/* The Host sends nothing on the channel */
		.credits = sys_cpu_to_le16(0U),
	};
	struct bt_l2cap_le_conn_rsp *rsp;
	struct net_buf *buf;

	ARG_UNUSED(fixture);

	(void)memset(&chan, 0, sizeof(chan));
	chan.chan.ops = &chan_ops;
	chan.rx.mtu = LE_MIN_MTU;
	disconnected_count = 0U;

	recv_sig(BT_L2CAP_LE_CONN_REQ, CONN_IDENT, &req, sizeof(req));

	buf = tx_sig(BT_L2CAP_LE_CONN_RSP, CONN_IDENT);
	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	zassert_equal(sys_le16_to_cpu(rsp->result), BT_L2CAP_LE_SUCCESS);
	zassert_equal(sys_le16_to_cpu(rsp->dcid), chan.rx.cid);
	net_buf_unref(buf);

	zassert_equal(chan.state, BT_L2CAP_CONNECTED);
	zassert_equal(chan.tx.cid, PEER_CID);
}

/* The peer disconnects a channel that a test left connected, and nothing that
 * the Host sent stays queued
 */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (chan.state != BT_L2CAP_DISCONNECTED) {
		recv_disconn_req(CLEANUP_IDENT, chan.rx.cid, chan.tx.cid);
	}

	net_buf_fifo_purge(sig_tx_queue());
}

ZTEST_SUITE(l2cap_le_disconn, NULL, setup, before, after, NULL);

/* A request with both CIDs of the channel disconnects it */
static ZTEST(l2cap_le_disconn, test_disconn_req)
{
	uint16_t rx_cid = chan.rx.cid;
	struct net_buf *buf;

	recv_disconn_req(DISCONN_IDENT, rx_cid, PEER_CID);

	buf = tx_sig(BT_L2CAP_DISCONN_RSP, DISCONN_IDENT);
	zassert_equal(buf->len, sizeof(struct bt_l2cap_disconn_rsp));
	zassert_equal(net_buf_pull_le16(buf), rx_cid);
	zassert_equal(net_buf_pull_le16(buf), PEER_CID);
	net_buf_unref(buf);

	zassert_equal(disconnected_count, 1U);
	zassert_is_null(bt_l2cap_le_lookup_rx_cid(conn, rx_cid));
}

/* The source CID is not the peer's endpoint of the channel */
static ZTEST(l2cap_le_disconn, test_disconn_req_wrong_scid)
{
	recv_disconn_req(DISCONN_IDENT, chan.rx.cid, PEER_CID + 1U);
	check_discarded();
}

/* The source CID is a fixed channel */
static ZTEST(l2cap_le_disconn, test_disconn_req_fixed_scid)
{
	recv_disconn_req(DISCONN_IDENT, chan.rx.cid, BT_L2CAP_CID_ATT);
	check_discarded();
}

/* The destination CID names no channel */
static ZTEST(l2cap_le_disconn, test_disconn_req_unknown_dcid)
{
	recv_disconn_req(DISCONN_IDENT, chan.rx.cid + 1U, PEER_CID);
	check_rejected(chan.rx.cid + 1U, PEER_CID);
}

/* The destination CID is the signaling channel, which is never disconnected */
static ZTEST(l2cap_le_disconn, test_disconn_req_fixed_dcid)
{
	recv_disconn_req(DISCONN_IDENT, BT_L2CAP_CID_LE_SIG, PEER_CID);
	check_rejected(BT_L2CAP_CID_LE_SIG, PEER_CID);
}
