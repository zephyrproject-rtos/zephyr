/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_interface.h"

#define ECHO_DATA "ping"

/* No user data, so the L2CAP layer fails to send these buffers */
NET_BUF_POOL_FIXED_DEFINE(unsendable_pool, 3, BT_L2CAP_BR_ECHO_REQ_RESERVE + sizeof(ECHO_DATA), 0,
			  NULL);

static struct bt_conn *conn;

static struct net_buf *echo_buf(void)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&unsendable_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
	net_buf_add_mem(buf, ECHO_DATA, strlen(ECHO_DATA));

	return buf;
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

	bt_l2cap_br_connected(conn);

	return NULL;
}

static void before(void *fixture)
{
	struct bt_l2cap_chan *sig;

	ARG_UNUSED(fixture);

	/* No signaling request pending, such as the Information Request sent on connection */
	sig = bt_l2cap_br_lookup_rx_cid(conn, BT_L2CAP_CID_BR_SIG);
	zassert_not_null(sig);
	CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan)->ident = 0;
}

ZTEST_SUITE(l2cap_br_echo, NULL, setup, before, NULL, NULL);

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
