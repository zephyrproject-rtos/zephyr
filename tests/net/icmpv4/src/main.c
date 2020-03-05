/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_ICMPV4_LOG_LEVEL);

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <sys/printk.h>
#include <linker/sections.h>

#include <tc_util.h>

#include <net/buf.h>

#include "net_private.h"
#include "icmpv4.h"

#include <ztest.h>

static int handler_called;
static int handler_status;

static const unsigned char icmpv4_echo_req[] = {
	/* IPv4 Header */
	0x45, 0x00, 0x00, 0x54, 0xea, 0x8c, 0x40, 0x00,
	0x40, 0x01, 0xcc, 0x18, 0xc0, 0x00, 0x02, 0x02,
	0xc0, 0x00, 0x02, 0x01,
	/* ICMP Header (Echo Request) */
	0x08, 0x00, 0xe3, 0x7c,
	0x10, 0x63, 0x00, 0x01,
	/* Payload */
	0xb8, 0xa4, 0x8c, 0x5d, 0x00, 0x00, 0x00, 0x00,
	0xfc, 0x49, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 };

static const unsigned char icmpv4_echo_rep[] = {
	/* IPv4 Header */
	0x45, 0x00, 0x00, 0x20, 0x75, 0xac, 0x00, 0x00,
	0x40, 0x01, 0x81, 0x2d, 0xc0, 0x00, 0x02, 0x02,
	0xc0, 0x00, 0x02, 0x01,
	/* ICMP Header (Echo Reply)*/
	0x00, 0x00, 0x91, 0x12,
	0x16, 0x50, 0x00, 0x00, 0x01, 0xfd, 0x56, 0xa0 };

static const unsigned char icmpv4_echo_req_opt[] = {
	/* IPv4 Header */
	0x4e, 0x00, 0x00, 0x78, 0xe1, 0x4b, 0x40, 0x00,
	0x40, 0x01, 0x9a, 0x83, 0xc0, 0x00, 0x02, 0x02,
	0xc0, 0x00, 0x02, 0x01,
	/* IPv4 Header Options (Timestamp) */
	0x44, 0x24, 0x0d, 0x01, 0xc0, 0x00, 0x02, 0x02,
	0x02, 0x4d, 0x1c, 0x3d, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	/* ICMP Header (Echo Request) */
	0x08, 0x00, 0x35, 0xbf,
	0x5d, 0xe7, 0x00, 0x01,
	0xcf, 0xe7, 0x8d, 0x5d, 0x00, 0x00, 0x00, 0x00,
	/* Payload */
	0x3a, 0x40, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 };

static const unsigned char icmpv4_echo_req_opt_bad[] = {
	/* IPv4 Header */
	0x46, 0x00, 0x00, 0xa0, 0xf8, 0x6c, 0x00, 0x00,
	0x64, 0x01, 0x56, 0xa8, 0xc0, 0x00, 0x02, 0x02,
	0xc0, 0x00, 0x02, 0x01,

	/* IPv4 Header Options (Wrong length) */
	0x41, 0x03, 0x41, 0x41,

	/* ICMP Header (Echo Request) */
	0x08, 0x00, 0x06, 0xb8, 0x30, 0x31, 0x32, 0x07,
	/* Payload */
	0x80, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00 };

static enum net_verdict handle_reply_msg(struct net_pkt *pkt,
					 struct net_ipv4_hdr *ip_hdr,
					 struct net_icmp_hdr *icmp_hdr)
{
	enum net_verdict ret;

	handler_called++;

	if (net_pkt_get_len(pkt) != sizeof(icmpv4_echo_rep)) {
		goto fail;
	}

	handler_status = 0;
	ret = NET_OK;

	return ret;

fail:
	handler_status = -EINVAL;
	ret = NET_DROP;

	return ret;
}

static enum net_verdict handle_request_msg(struct net_pkt *pkt,
					   struct net_ipv4_hdr *ip_hdr,
					   struct net_icmp_hdr *icmp_hdr)
{
	enum net_verdict ret;

	handler_called++;

	if (net_pkt_get_len(pkt) != sizeof(icmpv4_echo_req)) {
		goto fail;
	}

	handler_status = 0;
	ret = NET_OK;

	return ret;

fail:
	handler_status = -EINVAL;
	ret = NET_DROP;

	return ret;
}

static enum net_verdict handle_request_opt_msg(struct net_pkt *pkt,
					       struct net_ipv4_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr)
{
	enum net_verdict ret;

	handler_called++;

	if (net_pkt_get_len(pkt) != sizeof(icmpv4_echo_req_opt)) {
		goto fail;
	}

	handler_status = 0;
	ret = NET_OK;

	return ret;

fail:
	handler_status = -EINVAL;
	ret = NET_DROP;

	return ret;
}

static struct net_icmpv4_handler echo_rep_handler = {
	.type = NET_ICMPV4_ECHO_REPLY,
	.code = 0,
	.handler = handle_reply_msg,
};

static struct net_icmpv4_handler echo_req_handler = {
	.type = NET_ICMPV4_ECHO_REQUEST,
	.code = 0,
	.handler = handle_request_msg,
};

static struct net_icmpv4_handler echo_req_opt_handler = {
	.type = NET_ICMPV4_ECHO_REQUEST,
	.code = 0,
	.handler = handle_request_opt_msg,
};

void test_icmpv4(void)
{
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(7));

	struct net_ipv4_hdr *hdr;
	struct net_pkt *pkt;
	int ret;

	/* ================ Echo Request ================= */
	net_icmpv4_register_handler(&echo_req_handler);

	pkt = net_pkt_alloc_with_buffer(NULL, sizeof(icmpv4_echo_req),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "Allocation failed");

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_write(pkt, icmpv4_echo_req, sizeof(icmpv4_echo_req));

	net_pkt_cursor_init(pkt);

	net_pkt_set_ipv4_opts_len(pkt, 0);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_skip(pkt, sizeof(struct net_ipv4_hdr));

	hdr = (struct net_ipv4_hdr *)pkt->buffer->data;

	ret = net_icmpv4_input(pkt, hdr);

	/** TESTPOINT: Check input */
	zassert_true(!(ret == NET_DROP || handler_status != 0),
			"Callback not called properly");

	/** TESTPOINT: Check input */
	zassert_true(!(handler_called != 1), "Callbacks not called properly");

	net_icmpv4_unregister_handler(&echo_req_handler);

	net_pkt_unref(pkt);

	/* ================ Echo Reply ================= */
	net_icmpv4_register_handler(&echo_rep_handler);

	pkt = net_pkt_alloc_with_buffer(NULL, sizeof(icmpv4_echo_rep),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "Allocation failed");

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_write(pkt, icmpv4_echo_rep, sizeof(icmpv4_echo_rep));

	net_pkt_cursor_init(pkt);

	net_pkt_set_ipv4_opts_len(pkt, 0);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_skip(pkt, sizeof(struct net_ipv4_hdr));

	hdr = (struct net_ipv4_hdr *)pkt->buffer->data;

	ret = net_icmpv4_input(pkt, hdr);

	/** TESTPOINT: Check input */
	zassert_true(!(ret == NET_DROP || handler_status != 0),
			"Callback not called properly");

	/** TESTPOINT: Check input */
	zassert_true(!(handler_called != 2), "Callbacks not called properly");

	net_icmpv4_unregister_handler(&echo_rep_handler);

	net_pkt_unref(pkt);

	/* ================ Echo Request with Options  ================= */
	net_icmpv4_register_handler(&echo_req_opt_handler);

	pkt = net_pkt_alloc_with_buffer(NULL, sizeof(icmpv4_echo_req_opt),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "Allocation failed");

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_write(pkt, icmpv4_echo_req_opt, sizeof(icmpv4_echo_req_opt));

	net_pkt_cursor_init(pkt);

	net_pkt_set_ipv4_opts_len(pkt, 36);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_skip(pkt, sizeof(struct net_ipv4_hdr)); /* Header*/
	net_pkt_skip(pkt, 36); /* Options length */

	hdr = (struct net_ipv4_hdr *)pkt->buffer->data;

	ret = net_icmpv4_input(pkt, hdr);

	/** TESTPOINT: Check input */
	zassert_true(!(ret == NET_DROP || handler_status != 0),
			"Callback not called properly");

	/** TESTPOINT: Check input */
	zassert_true(!(handler_called != 3), "Callbacks not called properly");

	net_icmpv4_unregister_handler(&echo_req_opt_handler);

	net_pkt_unref(pkt);

	/* ================ Echo Request with Bad Options  ================= */
	net_icmpv4_register_handler(&echo_req_opt_handler);

	pkt = net_pkt_alloc_with_buffer(NULL, sizeof(icmpv4_echo_req_opt_bad),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "Allocation failed");

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_write(pkt, icmpv4_echo_req_opt_bad,
		      sizeof(icmpv4_echo_req_opt_bad));

	net_pkt_cursor_init(pkt);

	net_pkt_set_ipv4_opts_len(pkt, 4);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_skip(pkt, sizeof(struct net_ipv4_hdr)); /* Header*/
	net_pkt_skip(pkt, 4); /* Options length */

	hdr = (struct net_ipv4_hdr *)pkt->buffer->data;

	ret = net_icmpv4_input(pkt, hdr);

	/** TESTPOINT: Check input */
	zassert_true((ret == NET_DROP), "Packet should drop");

	net_icmpv4_unregister_handler(&echo_req_opt_handler);

	net_pkt_unref(pkt);
}

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_icmpv4_fn,
		ztest_unit_test(test_icmpv4));
	ztest_run_test_suite(test_icmpv4_fn);
}
