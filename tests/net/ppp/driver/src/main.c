/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_PPP_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/crc.h>

#include <ztest.h>

#include <net/ethernet.h>
#include <net/dummy.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

typedef enum net_verdict (*ppp_l2_callback_t)(struct net_if *iface,
					      struct net_pkt *pkt);
void ppp_l2_register_pkt_cb(ppp_l2_callback_t cb); /* found in ppp_l2.c */
void ppp_driver_feed_data(uint8_t *data, int data_len);

static struct net_if *iface;

static bool test_failed;
static bool test_started;
static struct k_sem wait_data;

#define WAIT_TIME 250
#define WAIT_TIME_LONG K_SECONDS(1)

/* If we receive this wire format PPP data, */
static uint8_t ppp_recv_data1[] = {
	0x7e, 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x21,
	0x7d, 0x21, 0x7d, 0x20, 0x7d, 0x34, 0x7d, 0x22,
	0x7d, 0x26, 0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20,
	0x7d, 0x20, 0x7d, 0x25, 0x7d, 0x26, 0x5d, 0x58,
	0xcf, 0x41, 0x7d, 0x27, 0x7d, 0x22, 0x7d, 0x28,
	0x7d, 0x22, 0xc4, 0xc9, 0x7e,
};

/* then we should see this FCS checked PPP data */
static uint8_t ppp_expect_data1[] = {
	0xc0, 0x21, 0x01, 0x01, 0x00, 0x14, 0x02, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x05, 0x06, 0x5d, 0x58,
	0xcf, 0x41, 0x07, 0x02, 0x08, 0x02,
};

static uint8_t ppp_recv_data2[] = {
	0x7e, 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x21,
	0x7d, 0x21, 0x7d, 0x20, 0x7d, 0x34, 0x7d, 0x22,
	0x7d, 0x26, 0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20,
	0x7d, 0x20, 0x7d, 0x25, 0x7d, 0x26, 0x5d, 0x58,
	0xcf, 0x41, 0x7d, 0x27, 0x7d, 0x22, 0x7d, 0x28,
	0x7d, 0x22, 0xc4, 0xc9,
	/* Second partial msg */
	0x7e, 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x21,
	0x7d, 0x21, 0x7d, 0x20, 0x7d, 0x34, 0x7d, 0x22,
};

/* This is HDLC encoded PPP message */
static uint8_t ppp_recv_data3[] = {
	0x7e, 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x22,
	0x7d, 0x21, 0x7d, 0x20, 0x7d, 0x24, 0x1c, 0x90, 0x7e
};

static uint8_t ppp_expect_data3[] = {
	0xc0, 0x21, 0x02, 0x01, 0x00, 0x04,
};

static uint8_t ppp_recv_data4[] = {
	/* There is FCS error in this packet */
	0x7e, 0xff, 0x7d, 0x5d, 0xe4, 0x7d, 0x23, 0xc0,
	0x21, 0x7d, 0x22, 0x7d, 0x21, 0x7d, 0x20, 0x7d,
	0x24, 0x7d, 0x3c, 0x90, 0x7e,
};

static uint8_t ppp_expect_data4[] = {
	0xff, 0x7d, 0xe4, 0x03, 0xc0, 0x21, 0x02, 0x01,
	0x00, 0x04, 0x1c, 0x90,
};

static uint8_t ppp_recv_data5[] = {
	/* Multiple valid packets here */
	/* 1st */
	0x7e, 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x21,
	0x7d, 0x23, 0x7d, 0x20, 0x7d, 0x34, 0x7d, 0x22,
	0x7d, 0x26, 0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20,
	0x7d, 0x20, 0x7d, 0x25, 0x7d, 0x26, 0x66, 0x7d,
	0x26, 0xbe, 0x70, 0x7d, 0x27, 0x7d, 0x22, 0x7d,
	0x28, 0x7d, 0x22, 0xf2, 0x47, 0x7e,
	/* 2nd */
	0x7e, 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x22,
	0x7d, 0x21, 0x7d, 0x20, 0x7d, 0x24, 0x7d, 0x3c,
	0x90, 0x7e,
	/* 3rd */
	0xff, 0x7d, 0x23, 0x80, 0xfd, 0x7d, 0x21, 0x7d,
	0x22, 0x7d, 0x20, 0x7d, 0x2f, 0x7d, 0x3a, 0x7d,
	0x24, 0x78, 0x7d, 0x20, 0x7d, 0x38, 0x7d, 0x24,
	0x78, 0x7d, 0x20, 0x7d, 0x35, 0x7d, 0x23, 0x2f,
	0x8f, 0x4e, 0x7e,
};

static uint8_t ppp_expect_data5[] = {
	0xc0, 0x21, 0x01, 0x03, 0x00, 0x14, 0x02, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x05, 0x06, 0x66, 0x06,
	0xbe, 0x70, 0x07, 0x02, 0x08, 0x02,
};

static uint8_t ppp_recv_data6[] = {
	0x7e, 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x7d, 0x22,
	0x7d, 0x21, 0x7d, 0x20, 0x7d, 0x24, 0x7d, 0x3c,
	0x90, 0x7e,
};

static uint8_t ppp_expect_data6[] = {
	0xc0, 0x21, 0x02, 0x01, 0x00, 0x04,
};

static uint8_t ppp_recv_data7[] = {
	0x7e, 0xff, 0x7d, 0x23, 0x80, 0x21, 0x7d, 0x22,
	0x7d, 0x22, 0x7d, 0x20, 0x7d, 0x2a, 0x7d, 0x23,
	0x7d, 0x26, 0xc0, 0x7d, 0x20, 0x7d, 0x22, 0x7d,
	0x22, 0x06, 0xa1, 0x7e,
};

static uint8_t ppp_expect_data7[] = {
	0x80, 0x21, 0x02, 0x02, 0x00, 0x0a, 0x03, 0x06,
	0xc0, 0x00, 0x02, 0x02,
};

static uint8_t ppp_recv_data8[] = {
	0x7e, 0xff, 0x7d, 0x23, 0x00, 0x57, 0x60, 0x7d,
	0x20, 0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20, 0x7d,
	0x2c, 0x3a, 0x40, 0xfe, 0x80, 0x7d, 0x20, 0x7d,
	0x20, 0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20, 0x7d,
	0x20, 0x7d, 0x20, 0x7d, 0x20, 0x5e, 0xff, 0xfe,
	0x7d, 0x20, 0x53, 0x44, 0xfe, 0x80, 0x7d, 0x20,
	0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20, 0x7d, 0x20,
	0x7d, 0x20, 0xa1, 0x41, 0x6d, 0x45, 0xbf, 0x28,
	0x7d, 0x25, 0xd1, 0x80, 0x7d, 0x20, 0x7d, 0x28,
	0x6c, 0x7d, 0x5e, 0x32, 0x7d, 0x20, 0x7d, 0x22,
	0x5b, 0x2c, 0x7d, 0x3d, 0x25, 0x20, 0x11, 0x7e,
};

static uint8_t ppp_expect_data8[] = {
	0x60, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x3a, 0x40,
	0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x5e, 0xff, 0xfe, 0x00, 0x53, 0x44,
	0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xa1, 0x41, 0x6d, 0x45, 0xbf, 0x28, 0x05, 0xd1,
	0x80, 0x00, 0x08, 0x6c, 0x7e, 0x32, 0x00, 0x02,
	0x5b, 0x2c, 0x1d, 0x25,
};

static uint8_t *receiving, *expecting;

static enum net_verdict ppp_l2_recv(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		NET_DBG("No data to recv!");
		return NET_DROP;
	}

	if (test_started) {
		struct net_buf *buf = pkt->buffer;
		int pos = 0;

		while (buf) {
			if (memcmp(expecting + pos, buf->data, buf->len)) {
				LOG_HEXDUMP_DBG(expecting + pos,
						buf->len,
						"expecting");
				LOG_HEXDUMP_DBG(buf->data, buf->len,
						"received");
				test_failed = true;
				break;
			}

			pos += buf->len;
			buf = buf->frags;
		}
	}

	if (test_failed) {
		net_pkt_hexdump(pkt, "received");
	}

	k_sem_give(&wait_data);

	return NET_DROP;
}

static void test_iface_setup(void)
{
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	zassert_not_null(iface, "PPP interface not found!");

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	ppp_l2_register_pkt_cb(ppp_l2_recv);

	net_if_up(iface);

	test_failed = false;
	test_started = true;
}

static bool send_iface(struct net_if *iface,
		       uint8_t *recv, size_t recv_len,
		       uint8_t *expect, size_t expect_len)
{
	receiving = recv;
	expecting = expect;
	test_failed = false;

	NET_DBG("Feeding %zd bytes data", recv_len);

	/* ToDo: Check the expected buffer value */

	ppp_driver_feed_data(recv, recv_len);

	zassert_false(test_failed, "Test failed");

	return true;
}

static void test_send_ppp_pkt_with_escapes(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data1, sizeof(ppp_recv_data1),
			 ppp_expect_data1, sizeof(ppp_expect_data1));

	zassert_true(ret, "iface");
}

static void test_send_ppp_pkt_with_full_and_partial(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data2, sizeof(ppp_recv_data2),
			 ppp_expect_data1, sizeof(ppp_expect_data1));

	zassert_true(ret, "iface");
}

static bool check_fcs(struct net_pkt *pkt, uint16_t *fcs)
{
	struct net_buf *buf;
	uint16_t crc;

	buf = pkt->buffer;
	if (!buf) {
		return false;
	}

	crc = crc16_ccitt(0xffff, buf->data, buf->len);

	buf = buf->frags;

	while (buf) {
		crc = crc16_ccitt(crc, buf->data, buf->len);
		buf = buf->frags;
	}

	*fcs = crc;

	if (crc != 0xf0b8) {
		return false;
	}

	return true;
}

static uint8_t unescape(uint8_t **ptr)
{
	uint8_t *pos = *ptr;
	uint8_t val;

	if (*pos == 0x7d) {
		pos++;
		val = (*pos) ^ 0x20;

		*ptr += 2;

		return val;
	}

	*ptr += 1;

	return *pos;
}

static void ppp_verify_fcs(uint8_t *buf, int len)
{
	struct net_pkt *pkt;
	uint16_t addr_and_ctrl;
	uint16_t fcs = 0;
	uint8_t *ptr;
	bool ret;

	pkt = net_pkt_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "Cannot create pkt");

	ptr = buf;
	while (ptr < &buf[len]) {
		net_pkt_write_u8(pkt, unescape(&ptr));
	}

	/* Remove sync byte 0x7e */
	(void)net_buf_pull_u8(pkt->buffer);

	net_pkt_cursor_init(pkt);

	net_pkt_read_be16(pkt, &addr_and_ctrl);

	/* Currently we do not support compressed Address and Control
	 * fields so they must always be present.
	 */
	if (addr_and_ctrl != (0xff << 8 | 0x03)) {
		zassert_true(false, "Invalid address / control bytes");
	}

	/* Skip sync byte (1) */
	net_buf_frag_last(pkt->buffer)->len -= 1;

	ret = check_fcs(pkt, &fcs);
	zassert_true(ret, "FCS calc failed, expecting 0x%x got 0x%x",
		     0xf0b8, fcs);

	net_pkt_unref(pkt);
}

static void test_ppp_verify_fcs_1(void)
{
	ppp_verify_fcs(ppp_recv_data1, sizeof(ppp_recv_data1));
}

static bool calc_fcs(struct net_pkt *pkt, uint16_t *fcs)
{
	struct net_buf *buf;
	uint16_t crc;

	buf = pkt->buffer;
	if (!buf) {
		return false;
	}

	crc = crc16_ccitt(0xffff, buf->data, buf->len);

	buf = buf->frags;

	while (buf) {
		crc = crc16_ccitt(crc, buf->data, buf->len);
		buf = buf->frags;
	}

	crc ^= 0xffff;

	*fcs = crc;

	return true;
}

static void ppp_calc_fcs(uint8_t *buf, int len)
{
	struct net_pkt *pkt;
	uint16_t addr_and_ctrl;
	uint16_t pkt_fcs, fcs = 0;
	uint8_t *ptr;
	bool ret;

	pkt = net_pkt_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "Cannot create pkt");

	ptr = buf;
	while (ptr < &buf[len]) {
		net_pkt_write_u8(pkt, unescape(&ptr));
	}

	/* Remove sync byte 0x7e */
	(void)net_buf_pull_u8(pkt->buffer);

	net_pkt_cursor_init(pkt);

	net_pkt_read_be16(pkt, &addr_and_ctrl);

	/* Currently we do not support compressed Address and Control
	 * fields so they must always be present.
	 */
	if (addr_and_ctrl != (0xff << 8 | 0x03)) {
		zassert_true(false, "Invalid address / control bytes");
	}

	len = net_pkt_get_len(pkt);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, len - sizeof(uint16_t) - (2 + 1));
	net_pkt_read_le16(pkt, &pkt_fcs);

	/* Skip FCS and sync bytes (2 + 1) */
	net_buf_frag_last(pkt->buffer)->len -= 2 + 1;

	ret = calc_fcs(pkt, &fcs);
	zassert_true(ret, "FCS calc failed");

	zassert_equal(pkt_fcs, fcs, "FCS calc failed, expecting 0x%x got 0x%x",
		     pkt_fcs, fcs);

	net_pkt_unref(pkt);
}

static void test_ppp_calc_fcs_1(void)
{
	ppp_calc_fcs(ppp_recv_data1, sizeof(ppp_recv_data1));
}

static void test_ppp_verify_fcs_3(void)
{
	ppp_verify_fcs(ppp_recv_data3, sizeof(ppp_recv_data3));
}

static void test_send_ppp_3(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data3, sizeof(ppp_recv_data3),
			 ppp_expect_data3, sizeof(ppp_expect_data3));

	zassert_true(ret, "iface");

	if (k_sem_take(&wait_data, WAIT_TIME_LONG)) {
		zassert_true(false, "Timeout, packet not received");
	}
}

static void test_send_ppp_4(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data4, sizeof(ppp_recv_data4),
			 ppp_expect_data4, sizeof(ppp_expect_data4));

	zassert_true(ret, "iface");

	if (k_sem_take(&wait_data, WAIT_TIME_LONG)) {
		zassert_true(false, "Timeout, packet not received");
	}
}

static void test_send_ppp_5(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data5, sizeof(ppp_recv_data5),
			 ppp_expect_data5, sizeof(ppp_expect_data5));

	zassert_true(ret, "iface");

	if (k_sem_take(&wait_data, WAIT_TIME_LONG)) {
		zassert_true(false, "Timeout, packet not received");
	}
}

static void test_send_ppp_6(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data6, sizeof(ppp_recv_data6),
			 ppp_expect_data6, sizeof(ppp_expect_data6));

	zassert_true(ret, "iface");

	if (k_sem_take(&wait_data, WAIT_TIME_LONG)) {
		zassert_true(false, "Timeout, packet not received");
	}
}

static void test_send_ppp_7(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data7, sizeof(ppp_recv_data7),
			 ppp_expect_data7, sizeof(ppp_expect_data7));

	zassert_true(ret, "iface");

	if (k_sem_take(&wait_data, WAIT_TIME_LONG)) {
		zassert_true(false, "Timeout, packet not received");
	}
}

static void test_send_ppp_8(void)
{
	bool ret;

	NET_DBG("Sending data to iface %p", iface);

	ret = send_iface(iface, ppp_recv_data8, sizeof(ppp_recv_data8),
			 ppp_expect_data8, sizeof(ppp_expect_data8));

	zassert_true(ret, "iface");

	if (k_sem_take(&wait_data, WAIT_TIME_LONG)) {
		zassert_true(false, "Timeout, packet not received");
	}
}

void test_main(void)
{
	ztest_test_suite(net_ppp_test,
			 ztest_unit_test(test_iface_setup),
			 ztest_unit_test(test_send_ppp_pkt_with_escapes),
			 ztest_unit_test(test_send_ppp_pkt_with_full_and_partial),
			 ztest_unit_test(test_ppp_verify_fcs_1),
			 ztest_unit_test(test_ppp_calc_fcs_1),
			 ztest_unit_test(test_ppp_verify_fcs_3),
			 ztest_unit_test(test_send_ppp_3),
			 ztest_unit_test(test_send_ppp_4),
			 ztest_unit_test(test_send_ppp_5),
			 ztest_unit_test(test_send_ppp_6),
			 ztest_unit_test(test_send_ppp_7),
			 ztest_unit_test(test_send_ppp_8)
		);

	ztest_run_test_suite(net_ppp_test);
}
