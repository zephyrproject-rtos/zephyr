/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/classic/conn_br_internal.h"

#define L2CAP_HDR_LEN   4U
#define MAX_PAYLOAD_LEN 8U
/* One more PDU than bt_br_acl_recv() has views for, plus the last one */
#define MAX_PDUS        (BT_BUF_ACL_RX_COUNT + 2)
#define KEEP_ALL        SIZE_MAX

/* bt_br_acl_recv() only passes the connection on to bt_l2cap_recv() */
static uint8_t conn_storage;
#define TEST_CONN ((struct bt_conn *)&conn_storage)

struct rx_pdu {
	struct net_buf *buf;
	uint8_t *data;
	uint16_t len;
	uint8_t bytes[L2CAP_HDR_LEN + MAX_PAYLOAD_LEN];
};

static struct rx_pdu rx[MAX_PDUS];
static size_t rx_count;
/* How many of the first PDUs the receiver keeps, as an RFCOMM recv() returning
 * -EINPROGRESS does; the others are consumed during bt_l2cap_recv().
 */
static size_t rx_keep;
static size_t acl_freed;

void __wrap_bt_l2cap_recv(struct bt_conn *conn, struct net_buf *buf, bool complete)
{
	struct rx_pdu *pdu;

	zassert_equal_ptr(conn, TEST_CONN);
	zassert_true(complete);
	zassert_true(rx_count < ARRAY_SIZE(rx), "Too many PDUs delivered");
	zassert_true(buf->len <= sizeof(pdu->bytes));

	pdu = &rx[rx_count];
	pdu->buf = buf;
	pdu->data = buf->data;
	pdu->len = buf->len;
	memcpy(pdu->bytes, buf->data, buf->len);

	if (rx_count >= rx_keep) {
		net_buf_unref(buf);
	}

	rx_count++;
}

static void rx_freed(enum bt_buf_type type)
{
	if ((type & BT_BUF_ACL_IN) != 0) {
		acl_freed++;
	}
}

static uint8_t payload_byte(size_t pdu, size_t i)
{
	return (uint8_t)((pdu << 4) + i);
}

/* One HCI ACL packet payload holding an L2CAP PDU per entry of payload_lens */
static struct net_buf *acl_packet(const uint16_t *payload_lens, size_t count)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	zassert_not_null(buf);

	/* Drop the H4 packet type, as the HCI core does before L2CAP sees the buffer */
	(void)net_buf_pull_u8(buf);

	for (size_t pdu = 0; pdu < count; pdu++) {
		zassert_true(payload_lens[pdu] <= MAX_PAYLOAD_LEN);
		zassert_true(net_buf_tailroom(buf) >= L2CAP_HDR_LEN + payload_lens[pdu]);

		net_buf_add_le16(buf, payload_lens[pdu]);
		net_buf_add_le16(buf, 0x0040U + pdu);

		for (size_t i = 0; i < payload_lens[pdu]; i++) {
			net_buf_add_u8(buf, payload_byte(pdu, i));
		}
	}

	return buf;
}

static void check_pdu(size_t pdu, const uint8_t *data, uint16_t len, uint16_t payload_len)
{
	zassert_equal(len, L2CAP_HDR_LEN + payload_len, "PDU %zu: len %u", pdu, len);
	zassert_equal(sys_get_le16(&data[0]), payload_len, "PDU %zu: header", pdu);
	zassert_equal(sys_get_le16(&data[2]), 0x0040U + pdu, "PDU %zu: CID", pdu);

	for (size_t i = 0; i < payload_len; i++) {
		zassert_equal(data[L2CAP_HDR_LEN + i], payload_byte(pdu, i), "PDU %zu: byte %zu",
			      pdu, i);
	}
}

/* The PDU as bt_l2cap_recv() got it */
static void check_delivered(size_t pdu, uint16_t payload_len)
{
	check_pdu(pdu, rx[pdu].bytes, rx[pdu].len, payload_len);
}

/* The buffer a kept PDU arrived in still shows that PDU */
static void check_kept(size_t pdu, uint16_t payload_len)
{
	struct net_buf *buf = rx[pdu].buf;

	zassert_equal_ptr(buf->data, rx[pdu].data, "PDU %zu: cursor moved", pdu);
	check_pdu(pdu, buf->data, buf->len, payload_len);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(rx, 0, sizeof(rx));
	rx_count = 0;
	rx_keep = 0;
	acl_freed = 0;
	bt_buf_rx_freed_cb_set(rx_freed);
}

ZTEST_SUITE(br_acl_recv, NULL, NULL, before, NULL, NULL);

/* A receiver that keeps the only PDU of a packet sees it after bt_br_acl_recv() returns */
ZTEST(br_acl_recv, test_single_pdu_kept)
{
	static const uint16_t lens[] = { 5 };
	struct net_buf *acl = acl_packet(lens, ARRAY_SIZE(lens));

	rx_keep = KEEP_ALL;
	bt_br_acl_recv(TEST_CONN, acl, true);

	zassert_equal(rx_count, 1);
	zassert_equal_ptr(rx[0].buf, acl);
	check_kept(0, lens[0]);
	zassert_equal(acl_freed, 0);

	net_buf_unref(rx[0].buf);
	zassert_equal(acl_freed, 1);
}

/* Each PDU of a packet carrying several keeps its own buffer, and the ACL
 * buffer is freed exactly once, after the last of them is released.
 */
ZTEST(br_acl_recv, test_multiple_pdus_kept)
{
	static const uint16_t lens[] = { 3, 1, 7 };
	struct net_buf *acl = acl_packet(lens, ARRAY_SIZE(lens));

	rx_keep = KEEP_ALL;
	bt_br_acl_recv(TEST_CONN, acl, true);

	zassert_equal(rx_count, ARRAY_SIZE(lens));
	zassert_equal_ptr(rx[ARRAY_SIZE(lens) - 1].buf, acl, "Last PDU not in the ACL buffer");
	zassert_not_equal(rx[0].buf, rx[1].buf);

	for (size_t pdu = 0; pdu < ARRAY_SIZE(lens); pdu++) {
		check_kept(pdu, lens[pdu]);
	}

	net_buf_unref(rx[2].buf);
	net_buf_unref(rx[0].buf);
	zassert_equal(acl_freed, 0);

	net_buf_unref(rx[1].buf);
	zassert_equal(acl_freed, 1);
}

/* A receiver that consumes each PDU synchronously sees every PDU once */
ZTEST(br_acl_recv, test_multiple_pdus_released)
{
	static const uint16_t lens[] = { 2, 4, 6 };
	struct net_buf *acl = acl_packet(lens, ARRAY_SIZE(lens));

	bt_br_acl_recv(TEST_CONN, acl, true);

	zassert_equal(rx_count, ARRAY_SIZE(lens));

	for (size_t pdu = 0; pdu < ARRAY_SIZE(lens); pdu++) {
		check_delivered(pdu, lens[pdu]);
	}

	zassert_equal(acl_freed, 1);
}

/* With every view kept by earlier receivers, later PDUs are still delivered */
ZTEST(br_acl_recv, test_views_exhausted)
{
	uint16_t lens[MAX_PDUS];
	struct net_buf *acl;
	size_t views = MAX_PDUS - 2;

	for (size_t pdu = 0; pdu < ARRAY_SIZE(lens); pdu++) {
		lens[pdu] = 1;
	}

	acl = acl_packet(lens, ARRAY_SIZE(lens));

	rx_keep = views;
	bt_br_acl_recv(TEST_CONN, acl, true);

	zassert_equal(rx_count, ARRAY_SIZE(lens));

	for (size_t pdu = 0; pdu < ARRAY_SIZE(lens); pdu++) {
		check_delivered(pdu, lens[pdu]);
	}

	for (size_t pdu = 0; pdu < views; pdu++) {
		zassert_not_equal(rx[pdu].buf, acl);
		check_kept(pdu, lens[pdu]);
	}

	zassert_equal(acl_freed, 0);

	for (size_t pdu = 0; pdu < views; pdu++) {
		net_buf_unref(rx[pdu].buf);
	}

	zassert_equal(acl_freed, 1);
}
