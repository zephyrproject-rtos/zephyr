/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_ICMPV6_LOG_LEVEL);

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>

#include <zephyr/tc_util.h>

#include <zephyr/net_buf.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>

#include "ipv6.h"
#include "net_private.h"
#include "icmpv6.h"
#include <zephyr/ztest.h>

static struct net_if *test_iface;
static int handler_called;
static int handler_status;

#define TEST_MSG "foobar devnull"

#define ICMPV6_MSG_SIZE 104

static uint8_t icmpv6_echo_req[] =
	"\x60\x02\xea\x12\x00\x40\x3a\x40\xfe\x80\x00\x00\x00\x00\x00\x00" \
	"\xda\xcb\x8a\xff\xfe\x34\xc8\xf3\xfe\x80\x00\x00\x00\x00\x00\x00" \
	"\xec\x88\x2d\x63\xfd\x67\x31\x66\x80\x00\xa4\x24\x0b\x95\x00\x01" \
	"\x97\x78\x0f\x5c\x00\x00\x00\x00\xf7\x72\x00\x00\x00\x00\x00\x00" \
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f" \
	"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f" \
	"\x30\x31\x32\x33\x34\x35\x36\x37";

static uint8_t icmpv6_echo_rep[] =
	"\x60\x09\x23\xa0\x00\x40\x3a\x40\xfe\x80\x00\x00\x00\x00\x00\x00" \
	"\xec\x88\x2d\x63\xfd\x67\x31\x66\xfe\x80\x00\x00\x00\x00\x00\x00" \
	"\xda\xcb\x8a\xff\xfe\x34\xc8\xf3\x81\x00\xa3\x24\x0b\x95\x00\x01" \
	"\x97\x78\x0f\x5c\x00\x00\x00\x00\xf7\x72\x00\x00\x00\x00\x00\x00" \
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f" \
	"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f" \
	"\x30\x31\x32\x33\x34\x35\x36\x37";

static uint8_t icmpv6_inval_chksum[] =
	"\x60\x09\x23\xa0\x00\x40\x3a\x40\xfe\x80\x00\x00\x00\x00\x00\x00" \
	"\xec\x88\x2d\x63\xfd\x67\x31\x66\xfe\x80\x00\x00\x00\x00\x00\x00" \
	"\xda\xcb\x8a\xff\xfe\x34\xc8\xf3\x00\x00\xa3\x24\x0b\x95\x00\x01" \
	"\x97\x78\x0f\x5c\x00\x00\x00\x00\xf7\x72\x00\x00\x00\x00\x00\x00" \
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f" \
	"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f" \
	"\x30\x31\x32\x33\x34\x35\x36\x37";

struct net_icmpv6_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static struct net_icmpv6_context net_icmpv6_context_data;

static int net_icmpv6_dev_init(const struct device *dev)
{
	struct net_icmpv6_context *net_icmpv6_context = dev->data;

	net_icmpv6_context = net_icmpv6_context;

	return 0;
}

static uint8_t *net_icmpv6_get_mac(const struct device *dev)
{
	struct net_icmpv6_context *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = 0x01;
	}

	return context->mac_addr;
}

static void net_icmpv6_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_icmpv6_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	net_pkt_unref(pkt);
	return 0;
}

static struct dummy_api net_icmpv6_if_api = {
	.iface_api.init = net_icmpv6_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_icmpv6_test, "net_icmpv6_test",
		net_icmpv6_dev_init, NULL,
		&net_icmpv6_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_icmpv6_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static int handle_test_msg(struct net_icmp_ctx *ctx,
			   struct net_pkt *pkt,
			   struct net_icmp_ip_hdr *hdr,
			   struct net_icmp_hdr *icmp_hdr,
			   void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	struct net_buf *last = net_buf_frag_last(pkt->buffer);
	int ret;

	if (last->len != ICMPV6_MSG_SIZE) {
		handler_status = -EINVAL;
		ret = -EINVAL;
	} else {
		handler_status = 0;
		ret = 0;
	}

	handler_called++;

	return ret;
}

static struct net_pkt *create_pkt(uint8_t *data, int len,
				  struct net_ipv6_hdr **hdr)
{
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(NULL, ICMPV6_MSG_SIZE,
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "Allocation failed");

	net_pkt_set_iface(pkt, test_iface);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	net_pkt_write(pkt, data, len);

	net_pkt_cursor_init(pkt);
	*hdr = net_pkt_cursor_get_pos(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, sizeof(struct net_ipv6_hdr));

	/* The cursor should be at the start of the ICMPv6 header */

	return pkt;
}

ZTEST(icmpv6_fn, test_icmpv6)
{
	struct net_icmp_ctx ctx1;
	struct net_icmp_ctx ctx2;
	struct net_ipv6_hdr *hdr;
	struct net_pkt *pkt;
	int ret;

	ret = net_icmp_init_ctx(&ctx1, NET_ICMPV6_ECHO_REPLY,
				0, handle_test_msg);
	zassert_equal(ret, 0, "Cannot register %s handler (%d)",
		      STRINGIFY(NET_ICMPV6_ECHO_REPLY), ret);

	ret = net_icmp_init_ctx(&ctx2, NET_ICMPV6_ECHO_REQUEST,
				0, handle_test_msg);
	zassert_equal(ret, 0, "Cannot register %s handler (%d)",
		      STRINGIFY(NET_ICMPV6_ECHO_REQUEST), ret);

	pkt = create_pkt(icmpv6_inval_chksum, ICMPV6_MSG_SIZE, &hdr);
	zassert_not_null(pkt, "Cannot create pkt");

	ret = net_icmpv6_input(pkt, hdr);

	/**TESTPOINT: Check input*/
	zassert_true(ret == NET_DROP, "Callback not called properly");

	handler_status = -1;

	pkt = create_pkt(icmpv6_echo_rep, ICMPV6_MSG_SIZE, &hdr);
	zassert_not_null(pkt, "Cannot create pkt");

	ret = net_icmpv6_input(pkt, hdr);

	/**TESTPOINT: Check input*/
	zassert_true(!(ret == NET_DROP || handler_status != 0),
		     "Callback not called properly");

	handler_status = -1;

	pkt = create_pkt(icmpv6_echo_req, ICMPV6_MSG_SIZE, &hdr);
	zassert_not_null(pkt, "Cannot create pkt");

	ret = net_icmpv6_input(pkt, hdr);

	/**TESTPOINT: Check input*/
	zassert_true(!(ret == NET_DROP || handler_status != 0),
			"Callback not called properly");

	/**TESTPOINT: Check input*/
	zassert_true(!(handler_called != 2), "Callbacks not called properly");

	ret = net_icmp_cleanup_ctx(&ctx1);
	zassert_equal(ret, 0, "Cannot unregister handler (%d)", ret);

	ret = net_icmp_cleanup_ctx(&ctx2);
	zassert_equal(ret, 0, "Cannot unregister handler (%d)", ret);
}

static void *setup(void)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

	test_iface = net_if_lookup_by_dev(DEVICE_GET(net_icmpv6_test));

	return NULL;
}

/**test case main entry*/
ZTEST_SUITE(icmpv6_fn, NULL, setup, NULL, NULL, NULL);
