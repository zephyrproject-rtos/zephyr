/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_CONTEXT_LOG_LEVEL);

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/random.h>

#include <zephyr/tc_util.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/udp.h>

#include "net_private.h"

#if defined(CONFIG_NET_CONTEXT_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct net_context *udp_v6_ctx;
static struct net_context *udp_v4_ctx;
static struct net_context *mcast_v6_ctx;

#if defined(CONFIG_NET_TCP)
static struct net_context *tcp_v6_ctx;
static struct net_context *tcp_v4_ctx;
#endif

static struct in6_addr in6addr_my = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct in_addr in4addr_my = { { { 192, 0, 2, 1 } } };

static char *test_data = "Test data to be sent";

static bool test_failed;
static bool cb_failure;
static bool expecting_cb_failure;
static bool data_failure;
static bool recv_cb_called;
static bool recv_cb_reconfig_called;
static bool recv_cb_timeout_called;
static bool test_sending;

static struct k_sem wait_data;

#define WAIT_TIME K_MSEC(250)
#define WAIT_TIME_LONG MSEC_PER_SEC
#define SENDING 93244
#define MY_PORT 1969
#define PEER_PORT 16233

ZTEST(net_context, test_net_ctx_get_fail)
{
	struct net_context *context;
	int ret;

	ret = net_context_get(AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, &context);
	zassert_equal(ret, -EAFNOSUPPORT,
		      "Invalid family test failed");

	ret = net_context_get(AF_INET6, 10, IPPROTO_UDP, &context);
	zassert_equal(ret, -EPROTOTYPE,
		      "Invalid context type test failed ");

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6, &context);
	zassert_equal(ret, -EPROTONOSUPPORT,
		      "Invalid context protocol test failed");

	ret = net_context_get(99, SOCK_DGRAM, IPPROTO_UDP, &context);
	zassert_equal(ret, -EAFNOSUPPORT,
		      "Invalid context family test failed");

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &context);
	zassert_equal(ret, -EPROTOTYPE,
		      "Invalid context proto type test failed");

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_TCP, &context);
	zassert_equal(ret, -EPROTONOSUPPORT,
		      "Invalid context proto value test failed");

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, NULL);
	zassert_equal(ret, -EINVAL,
		      "Invalid context value test failed ");
}

ZTEST(net_context, test_net_ctx_get_success)
{
	struct net_context *context = NULL;
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	zassert_equal(ret, 0,
		      "Context get test failed");
	zassert_not_null(context, "Got NULL context");

	ret = net_context_put(context);
	zassert_equal(ret, 0,
		      "Context put test failed");

	zassert_false(net_context_is_used(context),
		      "Context put check test failed");
}

ZTEST(net_context, test_net_ctx_get_all)
{
	struct net_context *contexts[CONFIG_NET_MAX_CONTEXTS];
	struct net_context *context;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		ret = net_context_get(AF_INET6, SOCK_DGRAM,
				      IPPROTO_UDP, &contexts[i]);
		zassert_equal(ret, 0,
			      "context get test failed");
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	zassert_equal(ret, -ENOENT,
		      "Context get extra test failed");

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		ret = net_context_put(contexts[i]);
		zassert_equal(ret, 0,
			      "Context put test failed");
	}
}

static void net_ctx_create(void)
{
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v6_ctx);
	zassert_equal(ret, 0,
		      "Context create IPv6 UDP test failed");

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &mcast_v6_ctx);
	zassert_equal(ret, 0,
		      "Context create IPv6 mcast test failed ");

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v4_ctx);
	zassert_equal(ret, 0,
		      "Context create IPv4 UDP test failed");

#if defined(CONFIG_NET_TCP)
	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
			      &tcp_v6_ctx);
	zassert_equal(ret, 0,
		      "Context create IPv6 TCP test failed");

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			      &tcp_v4_ctx);
	zassert_equal(ret, 0,
		      "Context create IPv4 TCP test failed");
#endif /* CONFIG_NET_TCP */
}

static void net_ctx_bind_fail(void)
{
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
	};
	int ret;

	ret = net_context_bind(udp_v6_ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, -ENOENT,
		      "Context bind failure test failed");
}

static void net_ctx_bind_uni_success_v6(void)
{
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(MY_PORT),
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	};
	int ret;

	ret = net_context_bind(udp_v6_ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0,
		      "Context bind IPv6 test failed");
}

static void net_ctx_bind_uni_success_v4(void)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(MY_PORT),
		.sin_addr = { { { 192, 0, 2, 1 } } },
	};
	int ret;

	ret = net_context_bind(udp_v4_ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in));
	zassert_equal(ret, 0,
		      "Context bind IPv4 test failed");
}

static void net_ctx_bind_mcast_success(void)
{
	int ret;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(MY_PORT),
		.sin6_addr = { { { 0 } } },
	};

	net_ipv6_addr_create_ll_allnodes_mcast(&addr.sin6_addr);

	ret = net_context_bind(mcast_v6_ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0,
		      "Context bind test failed ");
}

static void net_ctx_listen_v6(void)
{
#if defined(CONFIG_NET_TCP)
	zassert_true(net_context_listen(tcp_v6_ctx, 0),
		     "Context listen IPv6 TCP test failed");
#endif /* CONFIG_NET_TCP */
}

static void net_ctx_listen_v4(void)
{
#if defined(CONFIG_NET_TCP)
	zassert_true(net_context_listen(tcp_v4_ctx, 0),
		     "Context listen IPv4 TCP test failed");
#endif /* CONFIG_NET_TCP */
}

static void connect_cb(struct net_context *context, int status,
		       void *user_data)
{
	sa_family_t family = POINTER_TO_INT(user_data);

	if (net_context_get_family(context) != family) {
		TC_ERROR("Connect family mismatch %d should be %d\n",
		       net_context_get_family(context), family);
		cb_failure = true;
		return;
	}

	cb_failure = false;
}

static void net_ctx_connect_v6(void)
{
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(PEER_PORT),
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
	};
	int ret;

	ret = net_context_connect(udp_v6_ctx, (struct sockaddr *)&addr,
				  sizeof(struct sockaddr_in6), connect_cb,
				  K_NO_WAIT, INT_TO_POINTER(AF_INET6));
	zassert_false((ret || cb_failure),
		      "Context connect IPv6 UDP test failed");

#if defined(CONFIG_NET_TCP)
	ret = net_context_connect(tcp_v6_ctx, (struct sockaddr *)&addr,
				  sizeof(struct sockaddr_in6), connect_cb,
				  K_NO_WAIT, INT_TO_POINTER(AF_INET6));
	zassert_false((ret || cb_failure),
		      "Context connect IPv6 TCP test failed");

#endif /* CONFIG_NET_TCP */
}

static void net_ctx_connect_v4(void)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
		.sin_addr = { { { 192, 0, 2, 2 } } },
	};
	int ret;

	ret = net_context_connect(udp_v4_ctx, (struct sockaddr *)&addr,
				  sizeof(struct sockaddr_in), connect_cb,
				  K_NO_WAIT, INT_TO_POINTER(AF_INET));
	zassert_false((ret || cb_failure),
		      "Context connect IPv6 UDP test failed");

#if defined(CONFIG_NET_TCP)
	ret = net_context_connect(tcp_v4_ctx, (struct sockaddr *)&addr,
				  sizeof(struct sockaddr_in6), connect_cb,
				  K_NO_WAIT, INT_TO_POINTER(AF_INET));
	zassert_false((ret || cb_failure),
		      "Context connect IPv6 TCP test failed");
#endif /* CONFIG_NET_TCP */
}

#if defined(CONFIG_NET_TCP)
static void accept_cb(struct net_context *context,
		      struct sockaddr *addr,
		      socklen_t addrlen,
		      int status,
		      void *user_data)
{
	sa_family_t family = POINTER_TO_INT(user_data);

	if (net_context_get_family(context) != family) {
		TC_ERROR("Accept family mismatch %d should be %d\n",
		       net_context_get_family(context), family);
		cb_failure = true;
		return;
	}

	cb_failure = false;
}
#endif
static void net_ctx_accept_v6(void)
{
#if defined(CONFIG_NET_TCP)
	int ret;

	ret = net_context_accept(udp_v6_ctx, accept_cb, K_NO_WAIT,
				 INT_TO_POINTER(AF_INET6));
	zassert_false((ret != -EINVAL || cb_failure),
		      "Context accept IPv6 UDP test failed");
#endif
}

static void net_ctx_accept_v4(void)
{
#if defined(CONFIG_NET_TCP)
	int ret;

	ret = net_context_accept(udp_v4_ctx, accept_cb, K_NO_WAIT,
				 INT_TO_POINTER(AF_INET));
	zassert_false((ret != -EINVAL || cb_failure),
		      "Context accept IPv4 UDP test failed");
#endif
}

static void net_ctx_setups_order_dependent(void)
{
	net_ctx_bind_fail();
	net_ctx_bind_uni_success_v6();
	net_ctx_bind_uni_success_v4();
	net_ctx_bind_mcast_success();
	net_ctx_listen_v6();
	net_ctx_listen_v4();
	net_ctx_connect_v6();
	net_ctx_connect_v4();
	net_ctx_accept_v6();
	net_ctx_accept_v4();
}

static void net_ctx_put(void)
{
	int ret;

	ret = net_context_put(udp_v6_ctx);
	zassert_equal(ret, 0,
		      "Context put IPv6 UDP test failed.");

	ret = net_context_put(mcast_v6_ctx);
	zassert_equal(ret, 0,
		      "Context put IPv6 mcast test failed");

	ret = net_context_put(udp_v4_ctx);
	zassert_equal(ret, 0,
		      "Context put IPv4 UDP test failed");

#if defined(CONFIG_NET_TCP)
	ret = net_context_put(tcp_v4_ctx);
	zassert_equal(ret, 0,
		      "Context put IPv4 TCP test failed");

	ret = net_context_put(tcp_v6_ctx);
	zassert_equal(ret, 0,
		      "Context put IPv6 TCP test failed");
#endif
}

static void send_cb(struct net_context *context, int status, void *user_data)
{
	sa_family_t family = POINTER_TO_INT(user_data);

	if (net_context_get_family(context) != family) {
		TC_ERROR("Send family mismatch %d should be %d\n",
		       net_context_get_family(context), family);
		cb_failure = true;
		return;
	}

	cb_failure = false;
}

static void net_ctx_send_v6(void)
{
	int ret;

	test_sending = true;

	ret = net_context_send(udp_v6_ctx, test_data, strlen(test_data),
			       send_cb, K_FOREVER, INT_TO_POINTER(AF_INET6));
	k_yield();

	zassert_false(((ret < 0) || cb_failure),
		     "Context send IPv6 UDP test failed");
}

static void net_ctx_send_v4(void)
{
	int ret;

	test_sending = true;

	ret = net_context_send(udp_v4_ctx, test_data, strlen(test_data),
			       send_cb, K_FOREVER, INT_TO_POINTER(AF_INET));
	k_yield();

	zassert_false(((ret < 0) || cb_failure),
		      "Context send IPv4 UDP test failed");
}

static void net_ctx_sendto_v6(void)
{
	int ret;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(PEER_PORT),
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
	};

	test_sending = true;

	ret = net_context_sendto(udp_v6_ctx, test_data, strlen(test_data),
				 (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in6), send_cb,
				 K_NO_WAIT, INT_TO_POINTER(AF_INET6));
	zassert_false(((ret < 0) || cb_failure),
		      "Context send IPv6 UDP test failed");
}

static void net_ctx_sendto_v4(void)
{
	int ret;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
		.sin_addr = { { { 192, 0, 2, 2 } } },
	};

	test_sending = true;

	ret = net_context_sendto(udp_v4_ctx, test_data, strlen(test_data),
				 (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in), send_cb,
				 K_NO_WAIT, INT_TO_POINTER(AF_INET));
	zassert_false(((ret < 0) || cb_failure),
		      "Context send IPv4 UDP test failed");
}

static void recv_cb(struct net_context *context,
		    struct net_pkt *pkt,
		    union net_ip_header *ip_hdr,
		    union net_proto_header *proto_hdr,
		    int status,
		    void *user_data)
{
	DBG("Data received.\n");

	recv_cb_called = true;
	k_sem_give(&wait_data);
}

ZTEST(net_context, test_net_ctx_recv_v6)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v6_ctx, recv_cb, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET6));
	zassert_false((ret || cb_failure),
		      "Context recv IPv6 UDP test failed");

	net_ctx_sendto_v6();

	k_sem_take(&wait_data, WAIT_TIME);

	zassert_true(recv_cb_called, "No data received on time, "
				"IPv6 recv test failed");
	recv_cb_called = false;

	net_ctx_put();
}

ZTEST(net_context, test_net_ctx_recv_v4)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v4_ctx, recv_cb, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET));
	zassert_false((ret || cb_failure),
		      "Context recv IPv4 UDP test failed");

	net_ctx_sendto_v4();

	k_sem_take(&wait_data, WAIT_TIME);

	zassert_true(recv_cb_called, "No data received on time, "
				"IPv4 recv test failed");

	recv_cb_called = false;

	net_ctx_put();
}

static bool net_ctx_sendto_v6_wrong_src(void)
{
	int ret;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(PEER_PORT),
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x3 } } },
	};

	test_sending = true;

	ret = net_context_sendto(udp_v6_ctx, test_data, strlen(test_data),
				 (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in6), send_cb,
				 K_NO_WAIT, INT_TO_POINTER(AF_INET6));
	if ((ret < 0) || cb_failure) {
		TC_ERROR("Context sendto IPv6 UDP wrong src "
			 "test failed (%d)\n", ret);
		return false;
	}

	return true;
}

ZTEST(net_context, test_net_ctx_recv_v6_fail)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v6_ctx, recv_cb, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET6));
	zassert_false((ret || cb_failure),
		      "Context recv IPv6 UDP test failed");

	net_ctx_sendto_v6_wrong_src();

	zassert_true(k_sem_take(&wait_data, WAIT_TIME),
		     "Semaphore triggered but should not");

	zassert_false(recv_cb_called, "Data received but should not have, "
			     "IPv6 recv test failed");

	recv_cb_called = false;

	net_ctx_put();
}

static bool net_ctx_sendto_v4_wrong_src(void)
{
	int ret;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
		.sin_addr = { { { 192, 0, 2, 3 } } },
	};

	test_sending = true;

	ret = net_context_sendto(udp_v4_ctx, test_data, strlen(test_data),
				 (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in), send_cb,
				 K_NO_WAIT, INT_TO_POINTER(AF_INET));
	if ((ret < 0) || cb_failure) {
		TC_ERROR("Context send IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

ZTEST(net_context, test_net_ctx_recv_v4_fail)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v4_ctx, recv_cb, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET));
	zassert_false((ret || cb_failure),
		      "Context recv IPv4 UDP test failed");

	net_ctx_sendto_v4_wrong_src();

	zassert_true(k_sem_take(&wait_data, WAIT_TIME),
		     "Semaphore triggered but should not");

	zassert_false(recv_cb_called, "Data received but should not have, "
		      "IPv4 recv test failed");

	recv_cb_called = false;

	net_ctx_put();
}

ZTEST(net_context, test_net_ctx_recv_v6_again)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v6_ctx, recv_cb, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET6));
	zassert_false((ret || cb_failure),
		      "Context recv IPv6 UDP test failed");

	net_ctx_sendto_v6();

	k_sem_take(&wait_data, WAIT_TIME);

	net_ctx_sendto_v6();

	k_sem_take(&wait_data, WAIT_TIME);

	zassert_true(recv_cb_called, "No data received on time 2nd time, "
		     "IPv6 recv test failed");

	recv_cb_called = false;

	net_ctx_put();
}

ZTEST(net_context, test_net_ctx_recv_v4_again)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v4_ctx, recv_cb, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET));
	zassert_false((ret || cb_failure),
		      "Context recv IPv4 UDP test failed");

	net_ctx_sendto_v4();

	k_sem_take(&wait_data, WAIT_TIME);

	net_ctx_sendto_v4();

	k_sem_take(&wait_data, WAIT_TIME);

	zassert_true(recv_cb_called,
		     "No data received on time 2nd time, "
		     "IPv4 recv test failed");

	recv_cb_called = false;

	net_ctx_put();
}

static void recv_cb_another(struct net_context *context,
			    struct net_pkt *pkt,
			    union net_ip_header *ip_hdr,
			    union net_proto_header *proto_hdr,
			    int status,
			    void *user_data)
{
	DBG("Data received in another callback.\n");

	recv_cb_reconfig_called = true;
	k_sem_give(&wait_data);
}

ZTEST(net_context, test_net_ctx_recv_v6_reconfig)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v6_ctx, recv_cb_another, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET6));
	zassert_false((ret || cb_failure),
		      "Context recv reconfig IPv6 UDP test failed");

	net_ctx_sendto_v6();

	k_sem_take(&wait_data, WAIT_TIME);

	zassert_true(recv_cb_reconfig_called,
		     "No data received on time, "
		     "IPv6 recv reconfig test failed");

	recv_cb_reconfig_called = false;

	net_ctx_put();
}

ZTEST(net_context, test_net_ctx_recv_v4_reconfig)
{
	int ret;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	ret = net_context_recv(udp_v4_ctx, recv_cb_another, K_NO_WAIT,
			       INT_TO_POINTER(AF_INET));
	zassert_false((ret || cb_failure),
		      "Context recv reconfig IPv4 UDP test failed");


	net_ctx_sendto_v4();

	k_sem_take(&wait_data, WAIT_TIME);

	zassert_true(recv_cb_reconfig_called, "No data received on time, "
		     "IPv4 recv reconfig test failed");

	recv_cb_reconfig_called = false;

	net_ctx_put();
}

#define STACKSIZE 1024
K_THREAD_STACK_DEFINE(thread_stack, STACKSIZE);
static struct k_thread thread_data;

static void recv_cb_timeout(struct net_context *context,
			    struct net_pkt *pkt,
			    union net_ip_header *ip_hdr,
			    union net_proto_header *proto_hdr,
			    int status,
			    void *user_data)
{
	if (expecting_cb_failure) {
		DBG("Data received after a timeout.\n");
	}

	recv_cb_timeout_called = true;
	k_sem_give(&wait_data);

	net_pkt_unref(pkt);
}

void timeout_thread(void *p1, void *p2, void *p3)
{
	struct net_context *ctx = p1;
	int family = POINTER_TO_INT(p2);
	int32_t timeout = POINTER_TO_INT(p3);
	int ret;

	ret = net_context_recv(ctx, recv_cb_timeout, K_MSEC(timeout),
			       INT_TO_POINTER(family));
	if (ret != -ETIMEDOUT && expecting_cb_failure) {
		zassert_true(expecting_cb_failure,
			     "Context recv UDP timeout test failed");
		cb_failure = true;
		return;
	}

	if (!recv_cb_timeout_called) {
		DBG("Data received on time, recv test failed\n");
		cb_failure = true;
		return;
	}

	DBG("Timeout %s\n", family == AF_INET ? "IPv4" : "IPv6");

	k_sem_give(&wait_data);
}

static k_tid_t start_timeout_v6_thread(int32_t timeout)
{
	return k_thread_create(&thread_data, thread_stack, STACKSIZE,
			       timeout_thread,
			       udp_v6_ctx, INT_TO_POINTER(AF_INET6),
			       INT_TO_POINTER(timeout),
			       K_PRIO_COOP(7), 0, K_NO_WAIT);
}

static k_tid_t start_timeout_v4_thread(int32_t timeout)
{
	return k_thread_create(&thread_data, thread_stack, STACKSIZE,
			       timeout_thread,
			       udp_v4_ctx, INT_TO_POINTER(AF_INET),
			       INT_TO_POINTER(timeout),
			       K_PRIO_COOP(7), 0, K_NO_WAIT);
}

ZTEST(net_context, test_net_ctx_recv_v6_timeout)
{
	k_tid_t tid;

	cb_failure = false;
	expecting_cb_failure = true;
	recv_cb_timeout_called = false;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	/* Start a thread that will send data to receiver. */
	tid = start_timeout_v6_thread(WAIT_TIME_LONG);

	k_sem_reset(&wait_data);
	k_sem_take(&wait_data, K_MSEC(WAIT_TIME_LONG * 2));

	net_ctx_send_v6();

	DBG("Sent data\n");

	k_sem_take(&wait_data, K_FOREVER);

	k_thread_abort(tid);

	expecting_cb_failure = false;
	recv_cb_timeout_called = false;

	zassert_true(!cb_failure);

	net_ctx_put();
}

ZTEST(net_context, test_net_ctx_recv_v4_timeout)
{
	k_tid_t tid;

	cb_failure = false;
	expecting_cb_failure = true;
	recv_cb_timeout_called = false;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	/* Start a thread that will send data to receiver. */
	tid = start_timeout_v4_thread(WAIT_TIME_LONG);

	k_sem_reset(&wait_data);
	k_sem_take(&wait_data, K_MSEC(WAIT_TIME_LONG * 2));

	net_ctx_send_v4();

	DBG("Sent data\n");

	k_sem_take(&wait_data, K_FOREVER);

	k_thread_abort(tid);

	expecting_cb_failure = false;
	recv_cb_timeout_called = false;

	zassert_true(!cb_failure);

	net_ctx_put();
}

ZTEST(net_context, test_net_ctx_recv_v6_timeout_forever)
{
	k_tid_t tid;

	cb_failure = false;
	expecting_cb_failure = false;
	recv_cb_timeout_called = false;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	/* Start a thread that will send data to receiver. */
	tid = start_timeout_v6_thread(SYS_FOREVER_MS);

	/* Wait a bit so that we see if recv waited or not */
	k_sleep(WAIT_TIME);

	net_ctx_send_v6();

	DBG("Sent data\n");

	k_sem_take(&wait_data, K_FOREVER);

	k_thread_abort(tid);

	expecting_cb_failure = false;
	recv_cb_timeout_called = false;

	net_ctx_put();
}

ZTEST(net_context, test_net_ctx_recv_v4_timeout_forever)
{
	k_tid_t tid;

	cb_failure = false;
	expecting_cb_failure = false;
	recv_cb_timeout_called = false;

	net_ctx_create();
	net_ctx_setups_order_dependent();

	/* Start a thread that will send data to receiver. */
	tid = start_timeout_v4_thread(SYS_FOREVER_MS);

	/* Wait a bit so that we see if recv waited or not */
	k_sleep(WAIT_TIME);

	net_ctx_send_v4();

	DBG("Sent data\n");

	k_sem_take(&wait_data, K_FOREVER);

	k_thread_abort(tid);

	expecting_cb_failure = false;
	recv_cb_timeout_called = false;

	net_ctx_put();
}

struct net_context_test {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_context_dev_init(const struct device *dev)
{
	return 0;
}

static uint8_t *net_context_get_mac(const struct device *dev)
{
	struct net_context_test *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand32_get();
	}

	return context->mac_addr;
}

static void net_context_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_context_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_udp_hdr hdr, *udp_hdr;

	if (!pkt->buffer) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	if (test_sending) {
		/* We are now about to send data to outside but in this
		 * test we just check what would be sent. In real life
		 * one would not do something like this in the sending
		 * side.
		 */

		/* In this test we feed the data back to us
		 * in order to test the recv functionality.
		 */
		/* We need to swap the IP addresses because otherwise
		 * the packet will be dropped.
		 */
		uint16_t port;

		if (net_pkt_family(pkt) == AF_INET6) {
			struct in6_addr addr;

			net_ipv6_addr_copy_raw((uint8_t *)&addr, NET_IPV6_HDR(pkt)->src);
			net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->src,
					       NET_IPV6_HDR(pkt)->dst);
			net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->dst, (uint8_t *)&addr);
		} else {
			struct in_addr addr;

			net_ipv4_addr_copy_raw((uint8_t *)&addr, NET_IPV4_HDR(pkt)->src);
			net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->src,
					       NET_IPV4_HDR(pkt)->dst);
			net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->dst, (uint8_t *)&addr);
		}

		udp_hdr = net_udp_get_hdr(pkt, &hdr);
		if (!udp_hdr) {
			TC_ERROR("UDP data receive failed.");
			goto out;
		}

		port = udp_hdr->src_port;
		udp_hdr->src_port = udp_hdr->dst_port;
		udp_hdr->dst_port = port;
		net_udp_set_hdr(pkt, udp_hdr);

		if (net_recv_data(net_pkt_iface(pkt),
				  net_pkt_clone(pkt, K_NO_WAIT)) < 0) {
			TC_ERROR("Data receive failed.");
			goto out;
		}

		test_sending = false;

		return 0;
	}

out:
	if (data_failure) {
		test_failed = true;
	}

	return 0;
}


struct net_context_test net_context_data;

static struct dummy_api net_context_if_api = {
	.iface_api.init = net_context_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_context_test, "net_context_test",
		net_context_dev_init, NULL,
		&net_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_context_if_api, _ETH_L2_LAYER,
		_ETH_L2_CTX_TYPE, 127);

static void *iface_add_address(void)
{
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface is NULL");

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address ");

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv4 address");

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface, &in6addr_mcast);
	zassert_not_null(maddr, "Cannot add multicast IPv6 address");

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	return NULL;
}

static void iface_remove_addr(void *dummy)
{
	struct net_if *iface;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	net_if_ipv6_addr_rm(iface, &in6addr_my);
	net_if_ipv4_addr_rm(iface, &in4addr_my);
	net_if_ipv6_maddr_rm(iface, &in6addr_mcast);
}

ZTEST_SUITE(net_context, NULL, iface_add_address, NULL, NULL, iface_remove_addr);
