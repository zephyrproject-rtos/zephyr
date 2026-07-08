/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test_icmpv6_send_error, CONFIG_NET_ICMPV6_LOG_LEVEL);

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/ztest.h>

#include "ipv6.h"
#include "icmpv6.h"

#define TEST_IPPROTO_NONE 59

static struct net_if *test_iface;

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);

	net_pkt_unref(pkt);

	return 0;
}

struct send_error_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
};

static struct send_error_context send_error_context_data;

static int send_error_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint8_t *send_error_get_mac(const struct device *dev)
{
	struct send_error_context *context = dev->data;

	context->mac_addr[0] = 0x00;
	context->mac_addr[1] = 0x00;
	context->mac_addr[2] = 0x5E;
	context->mac_addr[3] = 0x00;
	context->mac_addr[4] = 0x53;
	context->mac_addr[5] = 0x03;

	return context->mac_addr;
}

static void send_error_iface_init(struct net_if *iface)
{
	uint8_t *mac = send_error_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr), NET_LINK_ETHERNET);
}

static struct dummy_api send_error_if_api = {
	.iface_api.init = send_error_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_send_error_test, "net_send_error_test",
		send_error_dev_init, NULL,
		&send_error_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&send_error_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static struct net_pkt *create_orig_pkt(const struct net_in6_addr *src,
					const struct net_in6_addr *dst)
{
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(test_iface, sizeof(struct net_ipv6_hdr),
					NET_AF_INET6, TEST_IPPROTO_NONE,
					K_SECONDS(1));
	zassert_not_null(pkt, "Cannot allocate pkt");

	zassert_ok(net_ipv6_create(pkt, src, dst), "Cannot create IPv6 pkt");

	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv6_finalize(pkt, TEST_IPPROTO_NONE), "Cannot finalize IPv6 pkt");

	return pkt;
}

struct send_error_test_case {
	const char *name;
	const char *src;
	const char *dst;
	uint8_t type;
	uint8_t code;
	bool expect_sent;
};

static const struct send_error_test_case send_error_cases[] = {
	{
		.name = "unicast src/dst is sent (baseline)",
		.src = "2001:db8::1",
		.dst = "2001:db8::2",
		.type = NET_ICMPV6_DST_UNREACH,
		.code = NET_ICMPV6_DST_UNREACH_NO_ROUTE,
		.expect_sent = true,
	},
	{
		.name = "unspecified src is dropped (e.6)",
		.src = "::",
		.dst = "2001:db8::2",
		.type = NET_ICMPV6_DST_UNREACH,
		.code = NET_ICMPV6_DST_UNREACH_NO_ROUTE,
		.expect_sent = false,
	},
	{
		.name = "multicast src is dropped (e.6)",
		.src = "ff02::1",
		.dst = "2001:db8::2",
		.type = NET_ICMPV6_DST_UNREACH,
		.code = NET_ICMPV6_DST_UNREACH_NO_ROUTE,
		.expect_sent = false,
	},
	{
		.name = "multicast dst + Dst Unreach is dropped (e.3)",
		.src = "2001:db8::1",
		.dst = "ff02::1",
		.type = NET_ICMPV6_DST_UNREACH,
		.code = NET_ICMPV6_DST_UNREACH_NO_ROUTE,
		.expect_sent = false,
	},
	{
		.name = "multicast dst + Time Exceeded is dropped (e.3)",
		.src = "2001:db8::1",
		.dst = "ff02::1",
		.type = NET_ICMPV6_TIME_EXCEEDED,
		.code = 0,
		.expect_sent = false,
	},
	{
		.name = "multicast dst + Param Problem code 0 is dropped (e.3)",
		.src = "2001:db8::1",
		.dst = "ff02::1",
		.type = NET_ICMPV6_PARAM_PROBLEM,
		.code = NET_ICMPV6_PARAM_PROB_HEADER,
		.expect_sent = false,
	},
	{
		.name = "multicast dst + Param Problem code 1 is dropped (e.3)",
		.src = "2001:db8::1",
		.dst = "ff02::1",
		.type = NET_ICMPV6_PARAM_PROBLEM,
		.code = NET_ICMPV6_PARAM_PROB_NEXTHEADER,
		.expect_sent = false,
	},
	{
		.name = "multicast dst + Packet Too Big is sent (e.3 exception 1)",
		.src = "2001:db8::1",
		.dst = "ff02::1",
		.type = NET_ICMPV6_PACKET_TOO_BIG,
		.code = 0,
		.expect_sent = true,
	},
	{
		.name = "multicast dst + Param Problem code 2 is sent (e.3 exception 2)",
		.src = "2001:db8::1",
		.dst = "ff02::1",
		.type = NET_ICMPV6_PARAM_PROBLEM,
		.code = NET_ICMPV6_PARAM_PROB_OPTION,
		.expect_sent = true,
	},
};

ZTEST(icmpv6_send_error_fn, test_send_error_rfc4443_matrix)
{
	for (size_t i = 0; i < ARRAY_SIZE(send_error_cases); i++) {
		const struct send_error_test_case *tc = &send_error_cases[i];
		struct net_in6_addr src;
		struct net_in6_addr dst;
		struct net_pkt *pkt;
		int ret;

		zassert_ok(net_addr_pton(NET_AF_INET6, tc->src, &src),
			   "%s: bad src address", tc->name);
		zassert_ok(net_addr_pton(NET_AF_INET6, tc->dst, &dst),
			   "%s: bad dst address", tc->name);

		pkt = create_orig_pkt(&src, &dst);

		ret = net_icmpv6_send_error(pkt, tc->type, tc->code, 0);

		if (tc->expect_sent) {
			zassert_equal(ret, 0, "%s: expected send, got %d",
				      tc->name, ret);
		} else {
			zassert_equal(ret, -EINVAL, "%s: expected -EINVAL, got %d",
				      tc->name, ret);
		}

		net_pkt_unref(pkt);
	}
}

static void *send_error_setup(void)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				      K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

	test_iface = net_if_lookup_by_dev(DEVICE_GET(net_send_error_test));

	return NULL;
}

ZTEST_SUITE(icmpv6_send_error_fn, NULL, send_error_setup, NULL, NULL, NULL);
