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
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_internal.h"

/* A dynamic PSM for the server that accepts the peer's channel */
#define SERVER_PSM 0x1001U
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

/* One signaling PDU from the peer at a time: a Connection Request or a
 * Disconnection Request, which have the same size
 */
BUILD_ASSERT(sizeof(struct bt_l2cap_conn_req) == sizeof(struct bt_l2cap_disconn_req));
NET_BUF_POOL_FIXED_DEFINE(pdu_pool, 1,
			  sizeof(struct bt_l2cap_hdr) + sizeof(struct bt_l2cap_sig_hdr) +
				  sizeof(struct bt_l2cap_disconn_req),
			  0, NULL);

static struct bt_conn *conn;
static struct bt_l2cap_br_chan br_chan;
static size_t disconnected_count;

static int chan_recv(struct bt_l2cap_chan *ch, struct net_buf *buf)
{
	ARGS_UNUSED(ch, buf);

	return 0;
}

static void chan_disconnected(struct bt_l2cap_chan *ch)
{
	zassert_equal_ptr(ch, &br_chan.chan);
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

	*ch = &br_chan.chan;

	return 0;
}

/* Deliver a signaling command from the peer, which the Host handles before
 * this returns
 */
static void recv_sig(uint8_t code, uint8_t ident, const void *data, uint16_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&pdu_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_add_le16(buf, (uint16_t)(sizeof(struct bt_l2cap_sig_hdr) + len));
	net_buf_add_le16(buf, BT_L2CAP_CID_BR_SIG);
	(void)net_buf_add_u8(buf, code);
	(void)net_buf_add_u8(buf, ident);
	net_buf_add_le16(buf, len);
	(void)net_buf_add_mem(buf, data, len);

	bt_l2cap_br_recv(conn, buf);
}

static void recv_disconn_req(uint8_t ident, uint16_t dcid, uint16_t scid)
{
	const struct bt_l2cap_disconn_req req = {
		.dcid = sys_cpu_to_le16(dcid),
		.scid = sys_cpu_to_le16(scid),
	};

	recv_sig(BT_L2CAP_DISCONN_REQ, ident, &req, sizeof(req));
}

/* There is no controller, so what the Host sends stays on this queue */
static sys_slist_t *sig_tx_queue(void)
{
	struct bt_l2cap_chan *sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);

	zassert_not_null(sig);

	return &BR_CHAN(sig)->_pdu_tx_queue;
}

static void purge_sent(void)
{
	sys_snode_t *node = sys_slist_get(sig_tx_queue());

	while (node != NULL) {
		net_buf_unref(CONTAINER_OF(node, struct net_buf, node));
		node = sys_slist_get(sig_tx_queue());
	}
}

/* The next signaling command that the Host queued, with the L2CAP and the
 * signaling headers pulled
 */
static struct net_buf *tx_sig(uint8_t code, uint8_t ident)
{
	struct bt_l2cap_sig_hdr *sig;
	struct bt_l2cap_hdr *hdr;
	struct net_buf *buf;
	sys_snode_t *node;

	node = sys_slist_get(sig_tx_queue());
	zassert_not_null(node, "Nothing sent");
	buf = CONTAINER_OF(node, struct net_buf, node);

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	zassert_equal(sys_le16_to_cpu(hdr->cid), BT_L2CAP_CID_BR_SIG);
	zassert_equal(sys_le16_to_cpu(hdr->len), buf->len);

	sig = net_buf_pull_mem(buf, sizeof(*sig));
	zassert_equal(sig->code, code, "code 0x%02x", sig->code);
	zassert_equal(sig->ident, ident, "ident 0x%02x", sig->ident);
	zassert_equal(sys_le16_to_cpu(sig->len), buf->len);

	return buf;
}

static void check_still_there(void)
{
	zassert_equal(br_chan.state, BT_L2CAP_CONFIG);
	zassert_equal_ptr(bt_l2cap_br_lookup_rx_cid(conn, br_chan.rx.cid), &br_chan.chan);
	zassert_equal(disconnected_count, 0U);
}

/* The Host rejected the Disconnection Request with the given CIDs as naming no
 * channel, and the channel is still there
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

	check_still_there();
}

/* The Host discarded the Disconnection Request without an answer, and the
 * channel is still there
 */
static void check_discarded(void)
{
	zassert_true(sys_slist_is_empty(sig_tx_queue()), "Answered");

	check_still_there();
}

static void *setup(void)
{
	static const bt_addr_t addr = {.val = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
	static struct bt_l2cap_server server = {
		.psm = SERVER_PSM,
		.sec_level = BT_SECURITY_L1,
		.accept = server_accept,
	};

	zassert_ok(bt_l2cap_br_server_register(&server));

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);

	bt_l2cap_br_connected(conn);

	return NULL;
}

/* The peer connects a channel to the server, which the Host accepts and goes
 * on to configure
 */
static void before(void *fixture)
{
	const struct bt_l2cap_conn_req req = {
		.psm = sys_cpu_to_le16(SERVER_PSM),
		.scid = sys_cpu_to_le16(PEER_CID),
	};
	struct bt_l2cap_conn_rsp *rsp;
	struct net_buf *buf;

	ARG_UNUSED(fixture);

	(void)memset(&br_chan, 0, sizeof(br_chan));
	br_chan.chan.ops = &chan_ops;
	br_chan.rx.mtu = BT_L2CAP_RX_MTU;
	disconnected_count = 0U;

	/* Including the Information Request that the Host sends on a new link */
	purge_sent();

	recv_sig(BT_L2CAP_CONN_REQ, CONN_IDENT, &req, sizeof(req));

	buf = tx_sig(BT_L2CAP_CONN_RSP, CONN_IDENT);
	rsp = net_buf_pull_mem(buf, sizeof(*rsp));
	zassert_equal(sys_le16_to_cpu(rsp->result), BT_L2CAP_BR_SUCCESS);
	zassert_equal(sys_le16_to_cpu(rsp->dcid), br_chan.rx.cid);
	zassert_equal(sys_le16_to_cpu(rsp->scid), PEER_CID);
	net_buf_unref(buf);

	/* The configuration that the Host starts plays no part here */
	purge_sent();

	zassert_equal(br_chan.state, BT_L2CAP_CONFIG);
	zassert_equal(br_chan.tx.cid, PEER_CID);
}

/* The peer disconnects a channel that a test left behind, and nothing that the
 * Host sent stays queued
 */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (br_chan.state != BT_L2CAP_DISCONNECTED) {
		recv_disconn_req(CLEANUP_IDENT, br_chan.rx.cid, br_chan.tx.cid);
	}

	purge_sent();
}

ZTEST_SUITE(l2cap_br_disconn, NULL, setup, before, after, NULL);

/* A request with both CIDs of the channel disconnects it */
static ZTEST(l2cap_br_disconn, test_disconn_req)
{
	uint16_t rx_cid = br_chan.rx.cid;
	struct net_buf *buf;

	recv_disconn_req(DISCONN_IDENT, rx_cid, PEER_CID);

	buf = tx_sig(BT_L2CAP_DISCONN_RSP, DISCONN_IDENT);
	zassert_equal(buf->len, sizeof(struct bt_l2cap_disconn_rsp));
	zassert_equal(net_buf_pull_le16(buf), rx_cid);
	zassert_equal(net_buf_pull_le16(buf), PEER_CID);
	net_buf_unref(buf);

	zassert_equal(disconnected_count, 1U);
	zassert_is_null(bt_l2cap_br_lookup_rx_cid(conn, rx_cid));
}

/* The source CID is not the peer's endpoint of the channel */
static ZTEST(l2cap_br_disconn, test_disconn_req_wrong_scid)
{
	recv_disconn_req(DISCONN_IDENT, br_chan.rx.cid, PEER_CID + 1U);
	check_discarded();
}

/* The source CID is a fixed channel */
static ZTEST(l2cap_br_disconn, test_disconn_req_fixed_scid)
{
	recv_disconn_req(DISCONN_IDENT, br_chan.rx.cid, BT_L2CAP_CID_BR_SIG);
	check_discarded();
}

/* The destination CID names no channel, while the source CID is the peer's
 * endpoint of one
 */
static ZTEST(l2cap_br_disconn, test_disconn_req_unknown_dcid)
{
	recv_disconn_req(DISCONN_IDENT, br_chan.rx.cid + 1U, PEER_CID);
	check_rejected(br_chan.rx.cid + 1U, PEER_CID);
}

/* The destination CID is the signaling channel, which is never disconnected */
static ZTEST(l2cap_br_disconn, test_disconn_req_fixed_dcid)
{
	recv_disconn_req(DISCONN_IDENT, BT_L2CAP_CID_BR_SIG, PEER_CID);
	check_rejected(BT_L2CAP_CID_BR_SIG, PEER_CID);
}
