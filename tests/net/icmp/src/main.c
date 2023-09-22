/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Use highest log level if both IPv4 and IPv6 are defined */
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)

#if CONFIG_NET_ICMPV4_LOG_LEVEL > CONFIG_NET_ICMPV6_LOG_LEVEL
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV4_LOG_LEVEL
#else
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV6_LOG_LEVEL
#endif

#elif defined(CONFIG_NET_IPV4)
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV4_LOG_LEVEL
#elif defined(CONFIG_NET_IPV6)
#define ICMP_LOG_LEVEL CONFIG_NET_ICMPV6_LOG_LEVEL
#else
#define ICMP_LOG_LEVEL LOG_LEVEL_INF
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, ICMP_LOG_LEVEL);

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/net/buf.h>
#include <zephyr/ztest.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_pkt.h>

#include "net_private.h"
#include "icmpv4.h"
#include "icmpv6.h"
#include "ipv4.h"
#include "ipv6.h"

#define PKT_WAIT_TIME K_SECONDS(1)
#define SEM_WAIT_TIME K_SECONDS(1)
#define TEST_DATA "dummy test data"

static struct in6_addr send_addr_6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr recv_addr_6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct in_addr send_addr_4 = { { { 192, 0, 2, 1 } } };
static struct in_addr recv_addr_4 = { { { 192, 0, 2, 2 } } };

static struct net_if *sender, *receiver;

static struct test_icmp_context {
	uint8_t mac[sizeof(struct net_eth_addr)];
	struct net_if *iface;
	uint8_t test_data[sizeof(TEST_DATA)];
	struct k_sem tx_sem;
	bool req_received;
} send_ctx, recv_ctx;

static void test_iface_init(struct net_if *iface)
{
	struct test_icmp_context *ctx = net_if_get_device(iface)->data;
	static int counter;

	/* Generate and assign MAC. */
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	ctx->mac[0] = 0x00;
	ctx->mac[1] = 0x00;
	ctx->mac[2] = 0x5E;
	ctx->mac[3] = 0x00;
	ctx->mac[4] = 0x53;
	ctx->mac[5] = ++counter;

	net_if_set_link_addr(iface, ctx->mac, sizeof(ctx->mac), NET_LINK_ETHERNET);

	ctx->iface = iface;
}

static int test_sender(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *send_pkt;

	send_pkt = net_pkt_clone(pkt, PKT_WAIT_TIME);

	net_pkt_set_iface(send_pkt, recv_ctx.iface);

	(void)net_recv_data(recv_ctx.iface, send_pkt);

	net_pkt_unref(pkt);

	return 0;
}

static int test_receiver(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *send_pkt;

	send_pkt = net_pkt_clone(pkt, PKT_WAIT_TIME);

	net_pkt_set_iface(send_pkt, send_ctx.iface);

	(void)net_recv_data(send_ctx.iface, send_pkt);

	net_pkt_unref(pkt);

	return 0;
}

static struct dummy_api send_if_api = {
	.iface_api.init = test_iface_init,
	.send = test_sender,
};

static struct dummy_api recv_if_api = {
	.iface_api.init = test_iface_init,
	.send = test_receiver,
};

NET_DEVICE_INIT(test_sender_icmp, "test_sender_icmp", NULL, NULL, &send_ctx, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &send_if_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), NET_IPV6_MTU);

NET_DEVICE_INIT(test_receiver_icmp, "test_receiver_icmp", NULL, NULL, &recv_ctx, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &recv_if_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), NET_IPV6_MTU);

static int icmp_handler(struct net_icmp_ctx *ctx,
			struct net_pkt *pkt,
			struct net_icmp_ip_hdr *hdr,
			struct net_icmp_hdr *icmp_hdr,
			void *user_data)
{
	struct test_icmp_context *test = user_data;

	if (hdr->family == AF_INET) {
		struct net_ipv4_hdr *ip_hdr = hdr->ipv4;

		NET_DBG("Received Echo reply from %s to %s",
			net_sprint_ipv4_addr(&ip_hdr->src),
			net_sprint_ipv4_addr(&ip_hdr->dst));

	} else if (hdr->family == AF_INET6) {
		struct net_ipv6_hdr *ip_hdr = hdr->ipv6;

		NET_DBG("Received Echo Reply from %s to %s",
			net_sprint_ipv6_addr(&ip_hdr->src),
			net_sprint_ipv6_addr(&ip_hdr->dst));
	} else {
		return -ENOENT;
	}

	test->req_received = true;
	k_sem_give(&test->tx_sem);

	net_pkt_unref(pkt);

	return 0;
}

ZTEST(icmp_tests, test_icmpv6_echo_request)
{
	struct sockaddr_in6 dst6 = { 0 };
	struct net_icmp_ping_params params;
	struct net_icmp_ctx ctx;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		return;
	}

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV6_ECHO_REPLY, 0, icmp_handler);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	dst6.sin6_family = AF_INET6;

#if defined(CONFIG_NET_IPV6)
	memcpy(&dst6.sin6_addr, &recv_addr_6, sizeof(recv_addr_6));
#endif

	params.identifier = 1234;
	params.sequence = 5678;
	params.tc_tos = 1;
	params.priority = 2;
	params.data = send_ctx.test_data;
	params.data_size = sizeof(send_ctx.test_data);

	ret = net_icmp_send_echo_request(&ctx, sender,
					 (struct sockaddr *)&dst6,
					 &params,
					 &send_ctx);
	zassert_equal(ret, 0, "Cannot send ICMP Echo-Request (%d)", ret);

	k_sem_take(&send_ctx.tx_sem, SEM_WAIT_TIME);

	zassert_true(send_ctx.req_received, "Did not receive Echo-Request");

	ret = net_icmp_cleanup_ctx(&ctx);
	zassert_equal(ret, 0, "Cannot cleanup ICMP (%d)", ret);

	send_ctx.req_received = false;
}

ZTEST(icmp_tests, test_icmpv4_echo_request)
{
	struct sockaddr_in dst4 = { 0 };
	struct net_icmp_ping_params params;
	struct net_icmp_ctx ctx;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		return;
	}

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV4_ECHO_REPLY, 0, icmp_handler);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	dst4.sin_family = AF_INET;

#if defined(CONFIG_NET_IPV4)
	memcpy(&dst4.sin_addr, &recv_addr_4, sizeof(recv_addr_4));
#endif

	params.identifier = 1234;
	params.sequence = 5678;
	params.tc_tos = 1;
	params.priority = 2;
	params.data = send_ctx.test_data;
	params.data_size = sizeof(send_ctx.test_data);

	ret = net_icmp_send_echo_request(&ctx, sender,
					 (struct sockaddr *)&dst4,
					 &params,
					 &send_ctx);
	zassert_equal(ret, 0, "Cannot send ICMP Echo-Request (%d)", ret);

	k_sem_take(&send_ctx.tx_sem, SEM_WAIT_TIME);

	zassert_true(send_ctx.req_received, "Did not receive Echo-Request");

	ret = net_icmp_cleanup_ctx(&ctx);
	zassert_equal(ret, 0, "Cannot cleanup ICMP (%d)", ret);

	send_ctx.req_received = false;
}

static void *setup(void)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

#if defined(CONFIG_NET_IPV6)
	(void)net_if_ipv6_addr_add(send_ctx.iface, &send_addr_6, NET_ADDR_MANUAL, 0);
	(void)net_if_ipv6_addr_add(recv_ctx.iface, &recv_addr_6, NET_ADDR_MANUAL, 0);
#else
	ARG_UNUSED(send_addr_6);
	ARG_UNUSED(recv_addr_6);
#endif

#if defined(CONFIG_NET_IPV4)
	(void)net_if_ipv4_addr_add(send_ctx.iface, &send_addr_4, NET_ADDR_MANUAL, 0);
	(void)net_if_ipv4_addr_add(recv_ctx.iface, &recv_addr_4, NET_ADDR_MANUAL, 0);
#else
	ARG_UNUSED(send_addr_4);
	ARG_UNUSED(recv_addr_4);
#endif

	memcpy(send_ctx.test_data, &(TEST_DATA), sizeof(TEST_DATA));
	memcpy(recv_ctx.test_data, &(TEST_DATA), sizeof(TEST_DATA));

	k_sem_init(&send_ctx.tx_sem, 0, 1);
	k_sem_init(&recv_ctx.tx_sem, 0, 1);

	sender = net_if_lookup_by_dev(DEVICE_GET(test_sender_icmp));
	zassert_equal(sender, send_ctx.iface, "Invalid interface (%p vs %p)",
		      sender, send_ctx.iface);

	receiver = net_if_lookup_by_dev(DEVICE_GET(test_receiver_icmp));
	zassert_equal(receiver, recv_ctx.iface, "Invalid interface (%p vs %p)",
		      receiver, recv_ctx.iface);

	return NULL;
}

ZTEST_SUITE(icmp_tests, NULL, setup, NULL, NULL, NULL);
