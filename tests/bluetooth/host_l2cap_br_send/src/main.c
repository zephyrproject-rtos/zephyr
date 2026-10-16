/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_interface.h"

#define DATA           "data"
#define CONNLESS_PSM   0x1001U
#define SDP_PSM        0x0001U
#define ECHO_REQ       0x08U
#define INFO_RSP       0x0bU
#define INFO_FEAT_MASK 0x0002U
#define FEAT_CONNLESS  BIT(9)

/* No user data, so the L2CAP layer fails to send these buffers */
NET_BUF_POOL_FIXED_DEFINE(unsendable_pool, 2, BT_L2CAP_CONNLESS_RESERVE + sizeof(DATA), 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(sendable_pool, 1, BT_L2CAP_CHAN_SEND_RESERVE + sizeof(DATA),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(rx_pool, 1, 32, 0, NULL);

static struct bt_conn *conn;
/* A link past Disconnection Complete, not yet cleaned up */
static struct bt_conn *lost;
static struct bt_l2cap_br_chan lost_chan;
static int echo_reqs;

static struct net_buf *data_buf(struct net_buf_pool *pool, size_t reserve)
{
	struct net_buf *buf;

	buf = net_buf_alloc(pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_reserve(buf, reserve);
	net_buf_add_mem(buf, DATA, strlen(DATA));

	return buf;
}

static void check_unchanged(const struct net_buf *buf, size_t reserve)
{
	zassert_equal(net_buf_headroom(buf), reserve, "headroom %zu", net_buf_headroom(buf));
	zassert_equal(buf->len, strlen(DATA));
	zassert_mem_equal(buf->data, DATA, strlen(DATA));
}

/* Answer the Information Request sent on connection with a feature mask
 * that has connectionless reception, as a peer supporting it would.
 */
static void recv_feat_mask(void)
{
	struct bt_l2cap_chan *sig;
	struct net_buf *buf;

	sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);
	zassert_not_null(sig);

	buf = net_buf_alloc(&rx_pool, K_NO_WAIT);
	zassert_not_null(buf);

	/* L2CAP header */
	net_buf_add_le16(buf, 12U);
	net_buf_add_le16(buf, BT_L2CAP_CID_BR_SIG);
	/* Signaling command header */
	net_buf_add_u8(buf, INFO_RSP);
	net_buf_add_u8(buf, CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan)->ident);
	net_buf_add_le16(buf, 8U);
	/* Information Response: type, result, data */
	net_buf_add_le16(buf, INFO_FEAT_MASK);
	net_buf_add_le16(buf, 0U);
	net_buf_add_le32(buf, FEAT_CONNLESS);

	bt_l2cap_br_recv(conn, buf);
}

static void recv_echo_req(struct bt_conn *c)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&rx_pool, K_NO_WAIT);
	zassert_not_null(buf);

	/* L2CAP header */
	net_buf_add_le16(buf, 4U);
	net_buf_add_le16(buf, BT_L2CAP_CID_BR_SIG);
	/* Signaling command header, no data */
	net_buf_add_u8(buf, ECHO_REQ);
	net_buf_add_u8(buf, 1U);
	net_buf_add_le16(buf, 0U);

	bt_l2cap_br_recv(c, buf);
}

static void echo_req(struct bt_conn *c, uint8_t identifier, struct net_buf *buf)
{
	echo_reqs++;
}

static struct bt_l2cap_br_echo_cb echo_cb = {
	.req = echo_req,
};

static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return 0;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.recv = chan_recv,
};

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };
	static const bt_addr_t lost_addr = { .val = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 } };

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);

	/* As on Connection Complete, before L2CAP attaches its channels */
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_br_connected(conn);
	recv_feat_mask();

	lost = bt_conn_add_br(&lost_addr);
	zassert_not_null(lost);

	bt_conn_set_state(lost, BT_CONN_CONNECTED);
	bt_l2cap_br_connected(lost);

	lost_chan.chan.ops = &chan_ops;
	zassert_ok(bt_l2cap_chan_connect(lost, &lost_chan.chan, SDP_PSM));

	/* As the priority handling of Disconnection Complete does, ahead of the RX work and
	 * the link cleanup on bt_workq
	 */
	bt_conn_set_state(lost, BT_CONN_DISCONNECT_COMPLETE);

	return NULL;
}

ZTEST_SUITE(l2cap_br_send, NULL, setup, NULL, NULL, NULL);

/* A send that fails leaves the buffer unchanged */
ZTEST(l2cap_br_send, test_send_failure)
{
	struct net_buf *buf = data_buf(&unsendable_pool, BT_L2CAP_CHAN_SEND_RESERVE);
	int err;

	err = bt_l2cap_br_send_cb(conn, BT_L2CAP_CID_BR_SIG, buf, NULL, NULL);
	zassert_equal(err, -EINVAL, "err %d", err);
	check_unchanged(buf, BT_L2CAP_CHAN_SEND_RESERVE);

	net_buf_unref(buf);
}

/* A connectionless send that fails leaves the buffer unchanged */
ZTEST(l2cap_br_send, test_connless_send_failure)
{
	struct net_buf *buf = data_buf(&unsendable_pool, BT_L2CAP_CONNLESS_RESERVE);
	int err;

	err = bt_l2cap_br_connless_send(conn, CONNLESS_PSM, buf);
	zassert_equal(err, -EINVAL, "err %d", err);
	check_unchanged(buf, BT_L2CAP_CONNLESS_RESERVE);

	net_buf_unref(buf);
}

/* Signalling is handled on a connected link, which the lost link test relies on */
ZTEST(l2cap_br_send, test_signalling)
{
	echo_reqs = 0;
	zassert_ok(bt_l2cap_br_echo_cb_register(&echo_cb));

	recv_echo_req(conn);
	zassert_equal(echo_reqs, 1);

	zassert_ok(bt_l2cap_br_echo_cb_unregister(&echo_cb));
}

/* A send on a lost link fails and queues nothing, also with a buffer that could
 * be sent: the PDU would never leave the queue.
 */
ZTEST(l2cap_br_send, test_send_on_lost_link)
{
	struct bt_l2cap_chan *sig = bt_l2cap_br_lookup_tx_cid(lost, BT_L2CAP_CID_BR_SIG);
	struct net_buf *buf = data_buf(&sendable_pool, BT_L2CAP_CHAN_SEND_RESERVE);
	sys_slist_t *queue;
	size_t queued;
	int err;

	zassert_not_null(sig);
	queue = &CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan)->_pdu_tx_queue;
	queued = sys_slist_len(queue);

	err = bt_l2cap_br_send_cb(lost, BT_L2CAP_CID_BR_SIG, buf, NULL, NULL);
	zassert_equal(err, -ENOTCONN, "err %d", err);
	zassert_equal(sys_slist_len(queue), queued);
	check_unchanged(buf, BT_L2CAP_CHAN_SEND_RESERVE);

	net_buf_unref(buf);
}

/* Signalling on a lost link is not handled */
ZTEST(l2cap_br_send, test_signalling_on_lost_link)
{
	echo_reqs = 0;
	zassert_ok(bt_l2cap_br_echo_cb_register(&echo_cb));

	recv_echo_req(lost);
	zassert_equal(echo_reqs, 0);

	zassert_ok(bt_l2cap_br_echo_cb_unregister(&echo_cb));
}

/* No Disconnection Request is attempted on a lost link: the signalling pool is
 * exhausted, and its buffers come back only when the link is cleaned up.
 */
ZTEST(l2cap_br_send, test_disconnect_on_lost_link)
{
	int err;

	err = bt_l2cap_chan_disconnect(&lost_chan.chan);
	zassert_equal(err, -ENOTCONN, "err %d", err);
}
