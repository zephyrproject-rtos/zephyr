/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/classic/l2cap_br_interface.h"

#define BASIC_CID  0x003eU
#define STREAM_CID 0x003fU
#define PAYLOAD    0xabcdU

struct test_chan {
	struct bt_l2cap_br_chan br;
	struct net_buf *kept;
	size_t recv_count;
	/* Whether recv() keeps the buffer and returns -EINPROGRESS */
	bool keep;
};

static struct test_chan basic_chan;
static struct test_chan stream_chan;
static struct bt_conn *conn;
static size_t freed;

static void pdu_destroy(struct net_buf *buf)
{
	freed++;
	net_buf_destroy(buf);
}

NET_BUF_POOL_FIXED_DEFINE(pdu_pool, 1, 16, 0, pdu_destroy);

static int test_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct test_chan *tc = CONTAINER_OF(chan, struct test_chan, br.chan);

	tc->recv_count++;
	zassert_equal(buf->len, sizeof(uint16_t));
	zassert_equal(sys_get_le16(buf->data), PAYLOAD);

	if (tc->keep) {
		tc->kept = buf;
		return -EINPROGRESS;
	}

	return 0;
}

static const struct bt_l2cap_chan_ops test_ops = {
	.recv = test_recv,
};

static int basic_accept(struct bt_conn *acl, struct bt_l2cap_chan **chan)
{
	basic_chan.br.chan.ops = &test_ops;
	*chan = &basic_chan.br.chan;

	return 0;
}

BT_L2CAP_BR_CHANNEL_DEFINE(test_basic_chan, BASIC_CID, basic_accept);

#if defined(CONFIG_BT_L2CAP_RET_FC)
static int stream_accept(struct bt_conn *acl, struct bt_l2cap_chan **chan)
{
	stream_chan.br.chan.ops = &test_ops;
	stream_chan.br.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
	stream_chan.br.rx.fcs = BT_L2CAP_BR_FCS_NO;
	stream_chan.br.rx.mps = 16;
	*chan = &stream_chan.br.chan;

	return 0;
}

BT_L2CAP_BR_CHANNEL_DEFINE(test_stream_chan, STREAM_CID, stream_accept);
#endif /* CONFIG_BT_L2CAP_RET_FC */

/* An L2CAP PDU carrying PAYLOAD, as an unsegmented I-frame when i_frame is set */
static struct net_buf *pdu(uint16_t cid, bool i_frame)
{
	struct net_buf *buf;
	uint16_t len = sizeof(uint16_t);

	buf = net_buf_alloc(&pdu_pool, K_NO_WAIT);
	zassert_not_null(buf);

	if (i_frame) {
		len += sizeof(uint16_t);
	}

	net_buf_add_le16(buf, len);
	net_buf_add_le16(buf, cid);

	if (i_frame) {
		/* TxSeq 0, ReqSeq 0, unsegmented */
		net_buf_add_le16(buf, 0U);
	}

	net_buf_add_le16(buf, PAYLOAD);

	return buf;
}

static void recv_kept(struct test_chan *tc, uint16_t cid, bool i_frame)
{
	tc->keep = true;
	bt_l2cap_br_recv(conn, pdu(cid, i_frame));

	zassert_equal(tc->recv_count, 1);
	zassert_not_null(tc->kept);
	zassert_equal(freed, 0, "Buffer freed while the receiver owns it");
	zassert_equal(sys_get_le16(tc->kept->data), PAYLOAD);

	/* Without CONFIG_BT_L2CAP_RET_FC this returns -ENOTSUP for BR/EDR, but it
	 * releases the buffer either way.
	 */
	(void)bt_l2cap_chan_recv_complete(&tc->br.chan, tc->kept);
	zassert_equal(freed, 1);
}

static void recv_consumed(struct test_chan *tc, uint16_t cid, bool i_frame)
{
	bt_l2cap_br_recv(conn, pdu(cid, i_frame));

	zassert_equal(tc->recv_count, 1);
	zassert_equal(freed, 1);
}

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);

	bt_l2cap_br_connected(conn);

	return NULL;
}

static void reset(struct test_chan *tc)
{
	tc->kept = NULL;
	tc->recv_count = 0;
	tc->keep = false;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset(&basic_chan);
	reset(&stream_chan);
	freed = 0;
}

ZTEST_SUITE(l2cap_br_recv, NULL, setup, before, NULL, NULL);

ZTEST(l2cap_br_recv, test_basic_kept)
{
	recv_kept(&basic_chan, BASIC_CID, false);
}

ZTEST(l2cap_br_recv, test_basic_consumed)
{
	recv_consumed(&basic_chan, BASIC_CID, false);
}

ZTEST(l2cap_br_recv, test_stream_kept)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_L2CAP_RET_FC);

	recv_kept(&stream_chan, STREAM_CID, true);
}

ZTEST(l2cap_br_recv, test_stream_consumed)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_L2CAP_RET_FC);

	recv_consumed(&stream_chan, STREAM_CID, true);
}
