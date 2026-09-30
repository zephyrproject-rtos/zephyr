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
#define INFO_RSP       0x0bU
#define INFO_FEAT_MASK 0x0002U
#define FEAT_CONNLESS  BIT(9)

/* No user data, so the L2CAP layer fails to send these buffers */
NET_BUF_POOL_FIXED_DEFINE(unsendable_pool, 2, BT_L2CAP_CONNLESS_RESERVE + sizeof(DATA), 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(rx_pool, 1, 32, 0, NULL);

static struct bt_conn *conn;

static struct net_buf *data_buf(size_t reserve)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&unsendable_pool, K_NO_WAIT);
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

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);

	bt_l2cap_br_connected(conn);
	recv_feat_mask();

	return NULL;
}

ZTEST_SUITE(l2cap_br_send, NULL, setup, NULL, NULL, NULL);

/* A send that fails leaves the buffer unchanged */
ZTEST(l2cap_br_send, test_send_failure)
{
	struct net_buf *buf = data_buf(BT_L2CAP_CHAN_SEND_RESERVE);
	int err;

	err = bt_l2cap_br_send_cb(conn, BT_L2CAP_CID_BR_SIG, buf, NULL, NULL);
	zassert_equal(err, -EINVAL, "err %d", err);
	check_unchanged(buf, BT_L2CAP_CHAN_SEND_RESERVE);

	net_buf_unref(buf);
}

/* A connectionless send that fails leaves the buffer unchanged */
ZTEST(l2cap_br_send, test_connless_send_failure)
{
	struct net_buf *buf = data_buf(BT_L2CAP_CONNLESS_RESERVE);
	int err;

	err = bt_l2cap_br_connless_send(conn, CONNLESS_PSM, buf);
	zassert_equal(err, -EINVAL, "err %d", err);
	check_unchanged(buf, BT_L2CAP_CONNLESS_RESERVE);

	net_buf_unref(buf);
}
