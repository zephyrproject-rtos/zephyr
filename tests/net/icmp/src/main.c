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
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/wifi_mgmt.h>

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

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
static struct test_icmp_context offload_ctx;
static struct net_if *offload_sender;

static struct in6_addr offload_send_addr_6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0x3 } } };
static struct in6_addr offload_recv_addr_6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0x4 } } };
static struct in_addr offload_send_addr_4 = { { { 192, 0, 2, 3 } } };
static struct in_addr offload_recv_addr_4 = { { { 192, 0, 2, 4 } } };
#endif

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

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
static int offload_dummy_get(sa_family_t family,
				enum net_sock_type type,
				enum net_ip_protocol ip_proto,
				struct net_context **context)
{
	return -1;
}

/* Placeholders, until Zephyr IP stack updated to handle a NULL net_offload */
static struct net_offload offload_dummy = {
	.get	       = offload_dummy_get,
	.bind	       = NULL,
	.listen	       = NULL,
	.connect       = NULL,
	.accept	       = NULL,
	.send	       = NULL,
	.sendto	       = NULL,
	.recv	       = NULL,
	.put	       = NULL,
};

static struct net_icmp_offload offload_data;

#if defined(CONFIG_NET_IPV4)
static int get_ipv4_reply(struct net_if *iface,
			  struct sockaddr *dst,
			  struct net_icmp_ping_params *params,
			  struct net_pkt **reply_pkt,
			  struct net_ipv4_hdr **hdr_ipv4,
			  struct net_icmp_hdr **hdr_icmp)
{
	struct net_ipv4_hdr *ipv4_hdr = NULL;
	struct net_icmp_hdr *icmp_hdr;
	const struct in_addr *dest4;
	struct net_pkt *reply;
	struct in_addr *src4;
	int ret;

	/* The code below should not be used in real life scenarios
	 * as it is missing filling the ICMP params etc. We just create
	 * a basic IPv4 header here in order to pass sanity checks
	 * in IP packet parsing.
	 */
	reply = net_pkt_alloc_with_buffer(iface, sizeof(struct net_ipv4_hdr) +
					  sizeof(struct net_icmp_hdr) +
					  params->data_size,
					  AF_INET, IPPROTO_ICMP,
					  PKT_WAIT_TIME);
	if (!reply) {
		NET_DBG("No buffer");
		return -ENOMEM;
	}

	dest4 = &offload_send_addr_4;
	src4 = &net_sin(dst)->sin_addr;

	ipv4_hdr = net_pkt_cursor_get_pos(reply);
	*hdr_ipv4 = ipv4_hdr;

	net_pkt_set_ipv4_ttl(reply, 1U);

	ret = net_ipv4_create_full(reply, src4, dest4, params->tc_tos,
				   params->identifier, 0, 0);
	if (ret < 0) {
		LOG_ERR("Cannot create IPv4 pkt (%d)", ret);
		return ret;
	}

	icmp_hdr = net_pkt_cursor_get_pos(reply);
	*hdr_icmp = icmp_hdr;

	ret = net_icmpv4_create(reply, NET_ICMPV4_ECHO_REPLY, 0);
	if (ret < 0) {
		LOG_ERR("Cannot create ICMPv4 pkt (%d)", ret);
		return ret;
	}

	ret = net_pkt_write(reply, params->data, params->data_size);
	if (ret < 0) {
		LOG_ERR("Cannot write payload (%d)", ret);
		return ret;
	}

	net_pkt_cursor_init(reply);
	net_ipv4_finalize(reply, IPPROTO_ICMP);

	*reply_pkt = reply;

	return 0;
}
#else
static int get_ipv4_reply(struct net_if *iface,
			  struct sockaddr *dst,
			  struct net_icmp_ping_params *params,
			  struct net_pkt **reply_pkt,
			  struct net_ipv4_hdr **hdr_ipv4,
			  struct net_icmp_hdr **hdr_icmp)
{
	return -ENOTSUP;
}
#endif

#if defined(CONFIG_NET_IPV6)
static int get_ipv6_reply(struct net_if *iface,
			  struct sockaddr *dst,
			  struct net_icmp_ping_params *params,
			  struct net_pkt **reply_pkt,
			  struct net_ipv6_hdr **hdr_ipv6,
			  struct net_icmp_hdr **hdr_icmp)
{
	struct net_ipv6_hdr *ipv6_hdr = NULL;
	struct net_icmp_hdr *icmp_hdr;
	const struct in6_addr *dest6;
	struct net_pkt *reply;
	struct in6_addr *src6;
	int ret;

	reply = net_pkt_alloc_with_buffer(iface, sizeof(struct net_ipv6_hdr) +
					  sizeof(struct net_icmp_hdr) +
					  params->data_size,
					  AF_INET6, IPPROTO_ICMP,
					  PKT_WAIT_TIME);
	if (!reply) {
		NET_DBG("No buffer");
		return -ENOMEM;
	}

	dest6 = &offload_send_addr_6;
	src6 = &net_sin6(dst)->sin6_addr;

	ipv6_hdr = net_pkt_cursor_get_pos(reply);
	*hdr_ipv6 = ipv6_hdr;

	ret = net_ipv6_create(reply, src6, dest6);
	if (ret < 0) {
		LOG_ERR("Cannot create IPv6 pkt (%d)", ret);
		return ret;
	}

	icmp_hdr = net_pkt_cursor_get_pos(reply);
	*hdr_icmp = icmp_hdr;

	ret = net_icmpv6_create(reply, NET_ICMPV6_ECHO_REPLY, 0);
	if (ret < 0) {
		LOG_ERR("Cannot create ICMPv6 pkt (%d)", ret);
		return ret;
	}

	ret = net_pkt_write(reply, params->data, params->data_size);
	if (ret < 0) {
		LOG_ERR("Cannot write payload (%d)", ret);
		return ret;
	}

	net_pkt_cursor_init(reply);
	net_ipv6_finalize(reply, IPPROTO_ICMP);

	*reply_pkt = reply;

	return 0;
}
#else
static int get_ipv6_reply(struct net_if *iface,
			  struct sockaddr *dst,
			  struct net_icmp_ping_params *params,
			  struct net_pkt **reply_pkt,
			  struct net_ipv6_hdr **hdr_ipv6,
			  struct net_icmp_hdr **hdr_icmp)
{
	return -ENOTSUP;
}
#endif

static int offload_ping_handler(struct net_icmp_ctx *ctx,
				struct net_if *iface,
				struct sockaddr *dst,
				struct net_icmp_ping_params *params,
				void *user_data)
{
	struct net_icmp_offload *icmp_offload_ctx = &offload_data;
	struct net_icmp_hdr *icmp_hdr = NULL;
	struct net_pkt *reply = NULL;
	struct net_icmp_ip_hdr ip_hdr;
	struct net_ipv4_hdr *ipv4_hdr;
	struct net_ipv6_hdr *ipv6_hdr;
	net_icmp_handler_t resp_handler;
	int ret;

	ret = net_icmp_get_offload_rsp_handler(icmp_offload_ctx, &resp_handler);
	if (ret < 0) {
		LOG_ERR("Cannot get offload response handler.");
		return -ENOENT;
	}

	/* So in real life scenario, we should here send a Echo-Request via
	 * some offloaded way to peer. When the response is received, we
	 * should then return that information to the ping caller by
	 * calling the response handler function.
	 * Here we just simulate a reply as there is no need to actually
	 * send anything anywhere.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4) && dst->sa_family == AF_INET) {
		ret = get_ipv4_reply(iface, dst, params, &reply,
				     &ipv4_hdr, &icmp_hdr);
		if (ret < 0) {
			LOG_ERR("Cannot create reply pkt (%d)", ret);
			return ret;
		}

		ip_hdr.family = AF_INET;
		ip_hdr.ipv4 = ipv4_hdr;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && dst->sa_family == AF_INET6) {
		ret = get_ipv6_reply(iface, dst, params, &reply,
				     &ipv6_hdr, &icmp_hdr);
		if (ret < 0) {
			LOG_ERR("Cannot create reply pkt (%d)", ret);
			return ret;
		}

		ip_hdr.family = AF_INET6;
		ip_hdr.ipv6 = ipv6_hdr;
	}

	ret = resp_handler(ctx, reply, &ip_hdr, icmp_hdr, user_data);
	if (ret < 0) {
		LOG_ERR("Cannot send response (%d)", ret);
	}

	return ret;
}

static void offload_iface_init(struct net_if *iface)
{
	struct test_icmp_context *ctx = net_if_get_device(iface)->data;
	int ret;

	/* Generate and assign MAC. */
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	ctx->mac[0] = 0x00;
	ctx->mac[1] = 0x00;
	ctx->mac[2] = 0x5E;
	ctx->mac[3] = 0x00;
	ctx->mac[4] = 0x53;
	ctx->mac[5] = 0xF0;

	net_if_set_link_addr(iface, ctx->mac, sizeof(ctx->mac), NET_LINK_ETHERNET);

	/* A dummy placeholder to allow network stack to pass offloaded data to our interface */
	iface->if_dev->offload = &offload_dummy;

	/* This will cause ping requests to be re-directed to our offload handler */
	ret = net_icmp_register_offload_ping(&offload_data, iface, offload_ping_handler);
	if (ret < 0) {
		LOG_ERR("Cannot register offload ping handler (%d)", ret);
	}

	ctx->iface = iface;
}

static enum offloaded_net_if_types offload_get_type(void)
{
	return L2_OFFLOADED_NET_IF_TYPE_WIFI;
}

static const struct net_wifi_mgmt_offload offload_api = {
	.wifi_iface.iface_api.init = offload_iface_init,
	.wifi_iface.get_type = offload_get_type,
};

NET_DEVICE_OFFLOAD_INIT(test_offload, "test_offload", NULL, NULL, &offload_ctx, NULL,
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &offload_api, 1500);
#endif /* CONFIG_NET_OFFLOADING_SUPPORT */

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

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)
#if defined(CONFIG_NET_IPV4)
ZTEST(icmp_tests, test_offload_icmpv4_echo_request)
{
	struct sockaddr_in dst4 = { 0 };
	struct net_icmp_ping_params params;
	struct net_icmp_ctx ctx;
	int ret;

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV4_ECHO_REPLY, 0, icmp_handler);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	dst4.sin_family = AF_INET;

	memcpy(&dst4.sin_addr, &offload_recv_addr_4, sizeof(offload_recv_addr_4));

	params.identifier = 1234;
	params.sequence = 5678;
	params.tc_tos = 1;
	params.priority = 2;
	params.data = offload_ctx.test_data;
	params.data_size = sizeof(offload_ctx.test_data);

	ret = net_icmp_send_echo_request(&ctx, offload_sender,
					 (struct sockaddr *)&dst4,
					 &params,
					 &offload_ctx);
	zassert_equal(ret, 0, "Cannot send ICMP Echo-Request (%d)", ret);

	k_sem_take(&offload_ctx.tx_sem, SEM_WAIT_TIME);

	zassert_true(offload_ctx.req_received, "Did not receive Echo-Request");

	ret = net_icmp_cleanup_ctx(&ctx);
	zassert_equal(ret, 0, "Cannot cleanup ICMP (%d)", ret);

	offload_ctx.req_received = false;
}
#endif

#if defined(CONFIG_NET_IPV6)
ZTEST(icmp_tests, test_offload_icmpv6_echo_request)
{
	struct sockaddr_in6 dst6 = { 0 };
	struct net_icmp_ping_params params;
	struct net_icmp_ctx ctx;
	int ret;

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV6_ECHO_REPLY, 0, icmp_handler);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	dst6.sin6_family = AF_INET6;

	memcpy(&dst6.sin6_addr, &offload_recv_addr_6, sizeof(offload_recv_addr_6));

	params.identifier = 1234;
	params.sequence = 5678;
	params.tc_tos = 1;
	params.priority = 2;
	params.data = offload_ctx.test_data;
	params.data_size = sizeof(offload_ctx.test_data);

	ret = net_icmp_send_echo_request(&ctx, offload_sender,
					 (struct sockaddr *)&dst6,
					 &params,
					 &offload_ctx);
	zassert_equal(ret, 0, "Cannot send ICMP Echo-Request (%d)", ret);

	k_sem_take(&offload_ctx.tx_sem, SEM_WAIT_TIME);

	zassert_true(offload_ctx.req_received, "Did not receive Echo-Request");

	ret = net_icmp_cleanup_ctx(&ctx);
	zassert_equal(ret, 0, "Cannot cleanup ICMP (%d)", ret);

	offload_ctx.req_received = false;
}
#endif
#endif /* CONFIG_NET_OFFLOADING_SUPPORT */

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

#if defined(CONFIG_NET_OFFLOADING_SUPPORT)

#if defined(CONFIG_NET_IPV6)
	(void)net_if_ipv6_addr_add(offload_ctx.iface, &offload_send_addr_6, NET_ADDR_MANUAL, 0);
#else
	ARG_UNUSED(offload_send_addr_6);
	ARG_UNUSED(offload_recv_addr_6);
#endif

#if defined(CONFIG_NET_IPV4)
	(void)net_if_ipv4_addr_add(offload_ctx.iface, &offload_send_addr_4, NET_ADDR_MANUAL, 0);
#else
	ARG_UNUSED(offload_send_addr_4);
	ARG_UNUSED(offload_recv_addr_4);
#endif

	memcpy(offload_ctx.test_data, &(TEST_DATA), sizeof(TEST_DATA));
	k_sem_init(&offload_ctx.tx_sem, 0, 1);

	offload_sender = net_if_lookup_by_dev(DEVICE_GET(test_offload));
	zassert_equal(offload_sender, offload_ctx.iface, "Invalid interface (%p vs %p)",
		      offload_sender, offload_ctx.iface);
#endif

	return NULL;
}

ZTEST_SUITE(icmp_tests, NULL, setup, NULL, NULL, NULL);
