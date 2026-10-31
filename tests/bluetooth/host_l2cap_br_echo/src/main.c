/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
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
#include "host/classic/l2cap_br_interface.h"

#define ECHO_DATA "ping"

#define L2CAP_HDR_SIZE 4U
#define SIG_HDR_SIZE   4U
#define CMD_REJECT     0x01U
#define ECHO_REQ       0x08U
#define ECHO_RSP       0x09U
#define INFO_RSP       0x0bU

/* No user data, so the L2CAP layer fails to send these buffers */
NET_BUF_POOL_FIXED_DEFINE(unsendable_pool, 3, BT_L2CAP_BR_ECHO_REQ_RESERVE + sizeof(ECHO_DATA), 0,
			  NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 4, BT_L2CAP_BR_ECHO_REQ_RESERVE + sizeof(ECHO_DATA),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
/* Two, as the stack keeps a received PDU it cannot process yet */
NET_BUF_POOL_FIXED_DEFINE(rx_pool, 2, 32, 0, NULL);

static struct bt_conn *conn;
/* Identifier of the Information Request sent on connection, left unanswered */
static uint8_t info_ident;
static uint32_t echo_rsp_count;

static void echo_rsp_cb(struct bt_conn *rsp_conn, struct net_buf *buf)
{
	ARG_UNUSED(rsp_conn);
	ARG_UNUSED(buf);

	echo_rsp_count++;
}

static struct bt_l2cap_br_echo_cb echo_cb = {
	.rsp = echo_rsp_cb,
};

static struct bt_l2cap_br_chan *sig_chan(void)
{
	struct bt_l2cap_chan *sig;

	sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);
	zassert_not_null(sig);

	return CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan);
}

static struct net_buf *alloc_echo_buf(struct net_buf_pool *pool)
{
	struct net_buf *buf;

	buf = net_buf_alloc(pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
	net_buf_add_mem(buf, ECHO_DATA, strlen(ECHO_DATA));

	return buf;
}

static struct net_buf *echo_buf(void)
{
	return alloc_echo_buf(&unsendable_pool);
}

/* Send an Echo Request and return its identifier, taken from the queued PDU */
static uint8_t send_echo_req(void)
{
	struct net_buf *buf = alloc_echo_buf(&tx_pool);
	int err;

	err = bt_l2cap_br_echo_req(conn, buf);
	zassert_equal(err, 0, "err %d", err);

	zassert_equal_ptr(sys_slist_peek_tail(&sig_chan()->_pdu_tx_queue), &buf->node);
	zassert_equal(buf->data[L2CAP_HDR_SIZE], ECHO_REQ);

	return buf->data[L2CAP_HDR_SIZE + 1U];
}

static struct net_buf *sig_pdu(uint8_t code, uint8_t ident, const void *data, uint16_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&rx_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_add_le16(buf, SIG_HDR_SIZE + len);
	net_buf_add_le16(buf, BT_L2CAP_CID_BR_SIG);
	net_buf_add_u8(buf, code);
	net_buf_add_u8(buf, ident);
	net_buf_add_le16(buf, len);
	net_buf_add_mem(buf, data, len);

	return buf;
}

static void recv_sig(uint8_t code, uint8_t ident, const void *data, uint16_t len)
{
	bt_l2cap_br_recv(conn, sig_pdu(code, ident, data, len));
}

static void recv_echo_rsp(uint8_t ident)
{
	recv_sig(ECHO_RSP, ident, ECHO_DATA, strlen(ECHO_DATA));
}

static void check_unchanged(const struct net_buf *buf)
{
	zassert_equal(net_buf_headroom(buf), BT_L2CAP_BR_ECHO_REQ_RESERVE);
	zassert_equal(buf->len, strlen(ECHO_DATA));
	zassert_mem_equal(buf->data, ECHO_DATA, strlen(ECHO_DATA));
}

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);

	zassert_ok(bt_l2cap_br_echo_cb_register(&echo_cb));

	bt_l2cap_br_connected(conn);

	info_ident = sig_chan()->ident;
	zassert_not_equal(info_ident, 0U, "no Information Request outstanding");

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	echo_rsp_count = 0U;
}

/* Release the PDUs queued for sending, as there is no controller */
static void release_tx_queue(void)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(&sig_chan()->_pdu_tx_queue)) != NULL) {
		net_buf_unref(CONTAINER_OF(node, struct net_buf, node));
	}
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	release_tx_queue();
}

/* The last PDU queued for sending on the signaling channel, if any */
static struct net_buf *sig_tx_tail(void)
{
	sys_snode_t *node;

	node = sys_slist_peek_tail(&sig_chan()->_pdu_tx_queue);

	return (node != NULL) ? CONTAINER_OF(node, struct net_buf, node) : NULL;
}

static void check_echo_rsp_without_data(const struct net_buf *rsp, uint8_t ident)
{
	zassert_not_null(rsp, "no Echo Response sent");
	zassert_equal(rsp->len, L2CAP_HDR_SIZE + SIG_HDR_SIZE);
	zassert_equal(rsp->data[L2CAP_HDR_SIZE], ECHO_RSP);
	zassert_equal(rsp->data[L2CAP_HDR_SIZE + 1U], ident);
	zassert_equal(sys_get_le16(&rsp->data[L2CAP_HDR_SIZE + 2U]), 0U);
}

ZTEST_SUITE(l2cap_br_echo, NULL, setup, before, after, NULL);

/* A request that fails to send leaves the buffer unchanged */
ZTEST(l2cap_br_echo, test_echo_req_buf_unchanged)
{
	struct net_buf *buf = echo_buf();
	int err;

	err = bt_l2cap_br_echo_req(conn, buf);
	zassert_equal(err, -EINVAL, "err %d", err);
	check_unchanged(buf);

	net_buf_unref(buf);
}

/* A request that fails to send does not wait for a response */
ZTEST(l2cap_br_echo, test_echo_req_not_pending)
{
	struct net_buf *buf = echo_buf();
	int err;

	err = bt_l2cap_br_echo_req(conn, buf);
	zassert_equal(err, -EINVAL, "err %d", err);
	net_buf_unref(buf);

	/* -EBUSY would mean the first request is still waiting for a response */
	buf = echo_buf();
	err = bt_l2cap_br_echo_req(conn, buf);
	zassert_equal(err, -EINVAL, "err %d", err);
	net_buf_unref(buf);
}

/* A response that fails to send leaves the buffer unchanged */
ZTEST(l2cap_br_echo, test_echo_rsp_send_failure)
{
	struct net_buf *buf = echo_buf();
	int err;

	err = bt_l2cap_br_echo_rsp(conn, 1, buf);
	zassert_equal(err, -EINVAL, "err %d", err);
	check_unchanged(buf);

	net_buf_unref(buf);
}

/* An Echo Request can be outstanding together with an Information Request */
ZTEST(l2cap_br_echo, test_echo_req_with_info_req)
{
	static const uint8_t feat_mask_rsp[] = {
		/* Type: extended features mask, result: success, no features */
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	struct net_buf *buf;
	uint8_t echo_ident;
	int err;

	zassert_equal(sig_chan()->ident, info_ident, "Information Request not outstanding");

	echo_ident = send_echo_req();
	zassert_not_equal(echo_ident, info_ident);

	/* The Information Response does not end the Echo Request */
	recv_sig(INFO_RSP, info_ident, feat_mask_rsp, sizeof(feat_mask_rsp));

	buf = alloc_echo_buf(&tx_pool);
	err = bt_l2cap_br_echo_req(conn, buf);
	zassert_equal(err, -EBUSY, "err %d", err);
	net_buf_unref(buf);

	recv_echo_rsp(echo_ident);
	zassert_equal(echo_rsp_count, 1U);
}

/* An Echo Response with another identifier is ignored */
ZTEST(l2cap_br_echo, test_echo_rsp_ident_mismatch)
{
	uint8_t echo_ident;

	echo_ident = send_echo_req();

	recv_echo_rsp((uint8_t)(echo_ident + 1U));
	zassert_equal(echo_rsp_count, 0U);

	recv_echo_rsp(echo_ident);
	zassert_equal(echo_rsp_count, 1U);

	/* The response ended the request, so another one can be sent */
	recv_echo_rsp(send_echo_req());
	zassert_equal(echo_rsp_count, 2U);
}

/* A rejected Echo Request ends, and the signaling channel stays connected */
ZTEST(l2cap_br_echo, test_echo_req_rejected)
{
	static const uint8_t reason[] = {0x00, 0x00};
	uint8_t echo_ident;

	echo_ident = send_echo_req();

	recv_sig(CMD_REJECT, echo_ident, reason, sizeof(reason));
	zassert_equal(sig_chan()->state, BT_L2CAP_CONNECTED);

	recv_echo_rsp(send_echo_req());
	zassert_equal(echo_rsp_count, 1U);
}

/* Without a req callback, the stack answers an Echo Request */
ZTEST(l2cap_br_echo, test_echo_req_answered_by_stack)
{
	recv_sig(ECHO_REQ, 0x42U, ECHO_DATA, strlen(ECHO_DATA));

	check_echo_rsp_without_data(sig_tx_tail(), 0x42U);
}

/* An Echo Request received while out of TX buffers is kept, to be answered later */
ZTEST(l2cap_br_echo, test_echo_req_kept_without_buf)
{
	struct net_buf *prev = NULL;
	struct net_buf *tail;
	struct net_buf *req;
	uint8_t ident;
	bool kept;

	/* Without a controller every response stays queued, until no buffer is left */
	for (ident = 1U; ident < UINT8_MAX; ident++) {
		req = net_buf_ref(sig_pdu(ECHO_REQ, ident, ECHO_DATA, strlen(ECHO_DATA)));
		bt_l2cap_br_recv(conn, req);
		kept = (req->ref > 1U);
		net_buf_unref(req);

		tail = sig_tx_tail();
		if (tail == prev) {
			break;
		}

		zassert_false(kept);
		check_echo_rsp_without_data(tail, ident);
		prev = tail;
	}

	zassert_true(ident < UINT8_MAX, "TX buffers not exhausted");
	/* Kept for the retry work, which needs the Bluetooth workqueue not run here */
	zassert_true(kept, "Echo Request dropped");

	/* Drop the kept request with the signaling channel */
	release_tx_queue();
	bt_l2cap_br_disconnected(conn);
	bt_l2cap_br_connected(conn);
	info_ident = sig_chan()->ident;
	zassert_not_equal(info_ident, 0U, "no Information Request outstanding");
}
