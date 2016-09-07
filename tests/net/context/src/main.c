/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <sections.h>

#include <tc_util.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_context.h>

#include "net_private.h"

#if defined(CONFIG_NET_DEBUG_CONTEXT)
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
static bool data_failure;
static bool recv_cb_called;
static bool recv_cb_reconfig_called;
static bool recv_cb_timeout_called;
static int test_token, timeout_token;

static struct nano_sem wait_data;

#define WAIT_TIME (sys_clock_ticks_per_sec / 4)
#define WAIT_TIME_LONG (sys_clock_ticks_per_sec)
#define SENDING 93244
#define MY_PORT 1969
#define PEER_PORT 16233

static bool net_ctx_get_fail(void)
{
	struct net_context *context;
	int ret;

	ret = net_context_get(AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (ret != -EAFNOSUPPORT) {
		TC_ERROR("Invalid family test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(AF_INET6, 10, IPPROTO_UDP, &context);
	if (ret != -EPROTOTYPE) {
		TC_ERROR("Invalid context type test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6, &context);
	if (ret != -EPROTONOSUPPORT) {
		TC_ERROR("Invalid context protocol test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(1, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (ret != -EAFNOSUPPORT) {
		TC_ERROR("Invalid context family test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &context);
	if (ret != -EPROTOTYPE) {
		TC_ERROR("Invalid context proto type test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_TCP, &context);
	if (ret != -EPROTONOSUPPORT) {
		TC_ERROR("Invalid context proto value test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, NULL);
	if (ret != -EINVAL) {
		TC_ERROR("Invalid context value test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_get_success(void)
{
	struct net_context *context = NULL;
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (ret != 0) {
		TC_ERROR("Context get test failed.\n");
		return false;
	}

	if (!context) {
		TC_ERROR("Got NULL context\n");
		return false;
	}

	ret = net_context_put(context);
	if (ret != 0) {
		TC_ERROR("Context put test failed.\n");
		return false;
	}

	if (net_context_is_used(context)) {
		TC_ERROR("Context put check test failed.\n");
		return false;
	}

	return true;
}

static bool net_ctx_get_all(void)
{
	struct net_context *contexts[CONFIG_NET_MAX_CONTEXTS];
	struct net_context *context;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		ret = net_context_get(AF_INET6, SOCK_DGRAM,
				      IPPROTO_UDP, &contexts[i]);
		if (ret != 0) {
			TC_ERROR("Context get [%d] test failed.\n", i);
			return false;
		}
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (ret != -ENOENT) {
		TC_ERROR("Context get extra test failed.\n");
		return false;
	}

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		ret = net_context_put(contexts[i]);
		if (ret != 0) {
			TC_ERROR("Context put [%d] test failed.\n", i);
			return false;
		}
	}

	return true;
}

static bool net_ctx_create(void)
{
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context create IPv6 UDP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &mcast_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context create IPv6 mcast test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context create IPv4 UDP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}

#if defined(CONFIG_NET_TCP)
	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
			      &tcp_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context create IPv6 TCP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			      &tcp_v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context create IPv4 TCP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}
#endif /* CONFIG_NET_TCP */

	return true;
}

static bool net_ctx_bind_fail(void)
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
	if (ret != -ENOENT) {
		TC_ERROR("Context bind failure test failed.\n");
		return false;
	}

	return true;
}

static bool net_ctx_bind_uni_success_v6(void)
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
	if (ret != 0) {
		TC_ERROR("Context bind IPv6 test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_bind_uni_success_v4(void)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(MY_PORT),
		.sin_addr = { { { 192, 0, 2, 1 } } },
	};
	int ret;

	ret = net_context_bind(udp_v4_ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in));
	if (ret != 0) {
		TC_ERROR("Context bind IPv4 test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_bind_mcast_success(void)
{
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(MY_PORT),
		.sin6_addr = { { { 0 } } },
	};
	int ret;

	net_ipv6_addr_create_ll_allnodes_mcast(&addr.sin6_addr);

	ret = net_context_bind(mcast_v6_ctx, (struct sockaddr *)&addr,
			       sizeof(struct sockaddr_in6));
	if (ret != 0) {
		TC_ERROR("Context bind test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_listen_v6(void)
{
	int ret;

	ret = net_context_listen(udp_v6_ctx, 0);
	if (!ret) {
		TC_ERROR("Context listen IPv6 UDP test failed (%d)\n", ret);
		return false;
	}

#if defined(CONFIG_NET_TCP)
	ret = net_context_listen(tcp_v6_ctx, 0);
	if (!ret) {
		TC_ERROR("Context listen IPv6 TCP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}
#endif /* CONFIG_NET_TCP */

	return true;
}

static bool net_ctx_listen_v4(void)
{
	int ret;

	ret = net_context_listen(udp_v4_ctx, 0);
	if (!ret) {
		TC_ERROR("Context listen IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

#if defined(CONFIG_NET_TCP)
	ret = net_context_listen(tcp_v4_ctx, 0);
	if (!ret) {
		TC_ERROR("Context listen IPv4 TCP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}
#endif /* CONFIG_NET_TCP */

	return true;
}

static void connect_cb(struct net_context *context, void *user_data)
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

static bool net_ctx_connect_v6(void)
{
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(PEER_PORT),
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
	};
	int ret;

	ret = net_context_connect(udp_v6_ctx, (struct sockaddr *)&addr,
				  sizeof(struct sockaddr_in6),
				  connect_cb, 0, INT_TO_POINTER(AF_INET6));
	if (ret || cb_failure) {
		TC_ERROR("Context connect IPv6 UDP test failed (%d)\n", ret);
		return false;
	}

#if defined(CONFIG_NET_TCP)
	ret = net_context_listen(tcp_v6_ctx, (struct sockaddr *)&addr,
				 connect_cb, 0, INT_TO_POINTER(AF_INET6));
	if (ret || cb_failure) {
		TC_ERROR("Context connect IPv6 TCP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}
#endif /* CONFIG_NET_TCP */

	return true;
}

static bool net_ctx_connect_v4(void)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
		.sin_addr = { { { 192, 0, 2, 2 } } },
	};
	int ret;

	ret = net_context_connect(udp_v4_ctx, (struct sockaddr *)&addr,
				  sizeof(struct sockaddr_in),
				  connect_cb, 0, INT_TO_POINTER(AF_INET));
	if (ret || cb_failure) {
		TC_ERROR("Context connect IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

#if defined(CONFIG_NET_TCP)
	ret = net_context_listen(tcp_v4_ctx, (struct sockaddr *)&addr,
				 connect_cb, 0, INT_TO_POINTER(AF_INET));
	if (ret || cb_failure) {
		TC_ERROR("Context connect IPv6 TCP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}
#endif /* CONFIG_NET_TCP */

	return true;
}

static void accept_cb(struct net_context *context,
		      struct sockaddr *addr,
		      socklen_t addrlen,
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

static bool net_ctx_accept_v6(void)
{
	int ret;

	ret = net_context_accept(udp_v6_ctx, accept_cb, 0,
				 INT_TO_POINTER(AF_INET6));
	if (ret != -EINVAL || cb_failure) {
		TC_ERROR("Context accept IPv6 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_accept_v4(void)
{
	int ret;

	ret = net_context_accept(udp_v4_ctx, accept_cb, 0,
				 INT_TO_POINTER(AF_INET));
	if (ret != -EINVAL || cb_failure) {
		TC_ERROR("Context accept IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static void send_cb(struct net_context *context, int status,
		    void *token, void *user_data)
{
	sa_family_t family = POINTER_TO_INT(user_data);

	if (net_context_get_family(context) != family) {
		TC_ERROR("Send family mismatch %d should be %d\n",
		       net_context_get_family(context), family);
		cb_failure = true;
		return;
	}

	if (POINTER_TO_INT(token) != test_token) {
		TC_ERROR("Token mismatch %d should be %d\n",
		       POINTER_TO_INT(token), test_token);
		cb_failure = true;
		return;
	}

	cb_failure = false;
	test_token = 0;
}

static bool net_ctx_send_v6(void)
{
	int ret, len;
	struct net_buf *buf, *frag;

	buf = net_nbuf_get_tx(udp_v6_ctx);
	frag = net_nbuf_get_data(udp_v6_ctx);

	net_buf_frag_add(buf, frag);

	len = strlen(test_data);

	memcpy(net_buf_add(frag, len), test_data, len);

	net_nbuf_set_appdatalen(buf, len);

	test_token = SENDING;

	ret = net_context_send(buf, send_cb, 0, INT_TO_POINTER(test_token),
			       INT_TO_POINTER(AF_INET6));
	if (ret || cb_failure) {
		TC_ERROR("Context send IPv6 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_send_v4(void)
{
	int ret, len;
	struct net_buf *buf, *frag;

	buf = net_nbuf_get_tx(udp_v4_ctx);
	frag = net_nbuf_get_data(udp_v4_ctx);

	net_buf_frag_add(buf, frag);

	len = strlen(test_data);

	memcpy(net_buf_add(frag, len), test_data, len);

	net_nbuf_set_appdatalen(buf, len);

	test_token = SENDING;

	ret = net_context_send(buf, send_cb, 0, INT_TO_POINTER(test_token),
			       INT_TO_POINTER(AF_INET));
	if (ret || cb_failure) {
		TC_ERROR("Context send IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_sendto_v6(void)
{
	int ret, len;
	struct net_buf *buf, *frag;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(PEER_PORT),
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
	};

	buf = net_nbuf_get_tx(udp_v6_ctx);
	frag = net_nbuf_get_data(udp_v6_ctx);

	net_buf_frag_add(buf, frag);

	len = strlen(test_data);

	memcpy(net_buf_add(frag, len), test_data, len);

	net_nbuf_set_appdatalen(buf, len);

	test_token = SENDING;

	ret = net_context_sendto(buf, (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in6),
				 send_cb, 0,
				 INT_TO_POINTER(test_token),
				 INT_TO_POINTER(AF_INET6));
	if (ret || cb_failure) {
		TC_ERROR("Context sendto IPv6 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_sendto_v4(void)
{
	int ret, len;
	struct net_buf *buf, *frag;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
		.sin_addr = { { { 192, 0, 2, 2 } } },
	};

	buf = net_nbuf_get_tx(udp_v4_ctx);
	frag = net_nbuf_get_data(udp_v4_ctx);

	net_buf_frag_add(buf, frag);

	len = strlen(test_data);

	memcpy(net_buf_add(frag, len), test_data, len);

	net_nbuf_set_appdatalen(buf, len);

	test_token = SENDING;

	ret = net_context_sendto(buf, (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in),
				 send_cb, 0,
				 INT_TO_POINTER(test_token),
				 INT_TO_POINTER(AF_INET));
	if (ret || cb_failure) {
		TC_ERROR("Context send IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static void recv_cb(struct net_context *context,
		    struct net_buf *buf,
		    int status,
		    void *user_data)
{
	DBG("Data received.\n");

	recv_cb_called = true;
	nano_sem_give(&wait_data);
}

static bool net_ctx_recv_v6(void)
{
	int ret;

	ret = net_context_recv(udp_v6_ctx, recv_cb, 0,
			       INT_TO_POINTER(AF_INET6));
	if (ret || cb_failure) {
		TC_ERROR("Context recv IPv6 UDP test failed (%d)\n", ret);
		return false;
	}

	net_ctx_sendto_v6();

	nano_sem_take(&wait_data, WAIT_TIME);

	if (!recv_cb_called) {
		TC_ERROR("No data received on time, IPv6 recv test failed\n");
		return false;
	}

	recv_cb_called = false;

	return true;
}

static bool net_ctx_recv_v4(void)
{
	int ret;

	ret = net_context_recv(udp_v4_ctx, recv_cb, 0,
			       INT_TO_POINTER(AF_INET));
	if (ret || cb_failure) {
		TC_ERROR("Context recv IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

	net_ctx_sendto_v4();

	nano_sem_take(&wait_data, WAIT_TIME);

	if (!recv_cb_called) {
		TC_ERROR("No data received on time, IPv4 recv test failed\n");
		return false;
	}

	recv_cb_called = false;

	return true;
}

static bool net_ctx_sendto_v6_wrong_src(void)
{
	int ret, len;
	struct net_buf *buf, *frag;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(PEER_PORT),
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x3 } } },
	};

	buf = net_nbuf_get_tx(udp_v6_ctx);
	frag = net_nbuf_get_data(udp_v6_ctx);

	net_buf_frag_add(buf, frag);

	len = strlen(test_data);

	memcpy(net_buf_add(frag, len), test_data, len);

	net_nbuf_set_appdatalen(buf, len);

	test_token = SENDING;

	ret = net_context_sendto(buf, (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in6),
				 send_cb, 0,
				 INT_TO_POINTER(test_token),
				 INT_TO_POINTER(AF_INET6));
	if (ret || cb_failure) {
		TC_ERROR("Context sendto IPv6 UDP wrong src "
			 "test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_recv_v6_fail(void)
{
	net_ctx_sendto_v6_wrong_src();

	if (nano_sem_take(&wait_data, WAIT_TIME)) {
		TC_ERROR("Semaphore triggered but should not\n");
		return false;
	}

	if (recv_cb_called) {
		TC_ERROR("Data received but should not have, "
			 "IPv6 recv test failed\n");
		return false;
	}

	recv_cb_called = false;

	return true;
}

static bool net_ctx_sendto_v4_wrong_src(void)
{
	int ret, len;
	struct net_buf *buf, *frag;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
		.sin_addr = { { { 192, 0, 2, 3 } } },
	};

	buf = net_nbuf_get_tx(udp_v4_ctx);
	frag = net_nbuf_get_data(udp_v4_ctx);

	net_buf_frag_add(buf, frag);

	len = strlen(test_data);

	memcpy(net_buf_add(frag, len), test_data, len);

	net_nbuf_set_appdatalen(buf, len);

	test_token = SENDING;

	ret = net_context_sendto(buf, (struct sockaddr *)&addr,
				 sizeof(struct sockaddr_in),
				 send_cb, 0,
				 INT_TO_POINTER(test_token),
				 INT_TO_POINTER(AF_INET));
	if (ret || cb_failure) {
		TC_ERROR("Context send IPv4 UDP test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_ctx_recv_v4_fail(void)
{
	net_ctx_sendto_v4_wrong_src();

	if (nano_sem_take(&wait_data, WAIT_TIME)) {
		TC_ERROR("Semaphore triggered but should not\n");
		return false;
	}

	if (recv_cb_called) {
		TC_ERROR("Data received but should not have, "
			 "IPv4 recv test failed\n");
		return false;
	}

	recv_cb_called = false;

	return true;
}

static bool net_ctx_recv_v6_again(void)
{
	net_ctx_sendto_v6();

	nano_sem_take(&wait_data, WAIT_TIME);

	if (!recv_cb_called) {
		TC_ERROR("No data received on time 2nd time, "
			 "IPv6 recv test failed\n");
		return false;
	}

	recv_cb_called = false;

	return true;
}

static bool net_ctx_recv_v4_again(void)
{
	net_ctx_sendto_v4();

	nano_sem_take(&wait_data, WAIT_TIME);

	if (!recv_cb_called) {
		TC_ERROR("No data received on time 2nd time, "
			 "IPv4 recv test failed\n");
		return false;
	}

	recv_cb_called = false;

	return true;
}

static void recv_cb_another(struct net_context *context,
			    struct net_buf *buf,
			    int status,
			    void *user_data)
{
	DBG("Data received in another callback.\n");

	recv_cb_reconfig_called = true;
	nano_sem_give(&wait_data);
}

static bool net_ctx_recv_v6_reconfig(void)
{
	int ret;

	ret = net_context_recv(udp_v6_ctx, recv_cb_another, 0,
			       INT_TO_POINTER(AF_INET6));
	if (ret || cb_failure) {
		TC_ERROR("Context recv reconfig IPv6 UDP test failed (%d)\n",
			 ret);
		return false;
	}

	net_ctx_sendto_v6();

	nano_sem_take(&wait_data, WAIT_TIME);

	if (!recv_cb_reconfig_called) {
		TC_ERROR("No data received on time, "
			 "IPv6 recv reconfig test failed\n");
		return false;
	}

	recv_cb_reconfig_called = false;

	return true;
}

static bool net_ctx_recv_v4_reconfig(void)
{
	int ret;

	ret = net_context_recv(udp_v4_ctx, recv_cb_another, 0,
			       INT_TO_POINTER(AF_INET));
	if (ret || cb_failure) {
		TC_ERROR("Context recv reconfig IPv4 UDP test failed (%d)\n",
			 ret);
		return false;
	}

	net_ctx_sendto_v4();

	nano_sem_take(&wait_data, WAIT_TIME);

	if (!recv_cb_reconfig_called) {
		TC_ERROR("No data received on time, "
			 "IPv4 recv reconfig test failed\n");
		return false;
	}

	recv_cb_reconfig_called = false;

	return true;
}

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 1024
char __noinit __stack fiberStack[STACKSIZE];
#endif

static void recv_cb_timeout(struct net_context *context,
			    struct net_buf *buf,
			    int status,
			    void *user_data)
{
	DBG("Data received after a timeout.\n");

	recv_cb_timeout_called = true;
	nano_sem_give(&wait_data);
}

void timeout_fiber(struct net_context *ctx, sa_family_t family)
{
	int ret;

	ret = net_context_recv(ctx, recv_cb_timeout, WAIT_TIME_LONG,
			       INT_TO_POINTER((int)family));

	if (ret || cb_failure) {
		TC_ERROR("Context recv UDP timeout test failed (%d)\n", ret);
		cb_failure = true;
		return;
	}

	if (!recv_cb_timeout_called) {
		TC_ERROR("No data received on time, recv test failed\n");
		cb_failure = true;
		return;
	}

	fiber_abort();
}

static void start_timeout_v6_fiber(void)
{
#if defined(CONFIG_MICROKERNEL)
	timeout_fiber(udp_v6_ctx, AF_INET6);
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)timeout_fiber,
			 (int)udp_v6_ctx, AF_INET6, 7, 0);
#endif
}

static void start_timeout_v4_fiber(void)
{
#if defined(CONFIG_MICROKERNEL)
	timeout_fiber(udp_v4_ctx, AF_INET);
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)timeout_fiber,
			 (int)udp_v4_ctx, AF_INET, 7, 0);
#endif
}

static bool net_ctx_recv_v6_timeout(void)
{
	cb_failure = false;

	/* Start a fiber that will send data to receiver. */
	start_timeout_v6_fiber();

	net_ctx_send_v6();
	timeout_token = SENDING;

	DBG("Sent data\n");

	nano_sem_take(&wait_data, TICKS_UNLIMITED);

	return !cb_failure;
}

static bool net_ctx_recv_v4_timeout(void)
{
	cb_failure = false;

	/* Start a fiber that will send data to receiver. */
	start_timeout_v4_fiber();

	net_ctx_send_v4();
	timeout_token = SENDING;

	nano_sem_take(&wait_data, TICKS_UNLIMITED);

	return !cb_failure;
}

static bool net_ctx_put(void)
{
	int ret;

	ret = net_context_put(udp_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context put IPv6 UDP test failed.\n");
		return false;
	}

	ret = net_context_put(mcast_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context put IPv6 mcast test failed.\n");
		return false;
	}

	ret = net_context_put(udp_v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context put IPv4 UDP test failed.\n");
		return false;
	}

#if defined(CONFIG_NET_TCP)
	ret = net_context_put(tcp_v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context put IPv4 TCP test failed.\n");
		return false;
	}

	ret = net_context_put(tcp_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context put IPv6 TCP test failed.\n");
		return false;
	}
#endif

	return true;
}

struct net_context_test {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_context_dev_init(struct device *dev)
{
	return 0;
}

static uint8_t *net_context_get_mac(struct device *dev)
{
	struct net_context_test *context = dev->driver_data;

	if (context->mac_addr[0] == 0x00) {
		/* 10-00-00-00-00 to 10-00-00-00-FF Documentation RFC7042 */
		context->mac_addr[0] = 0x10;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x00;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x00;
		context->mac_addr[5] = sys_rand32_get();
	}

	return context->mac_addr;
}

static void net_context_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_context_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr));
}

static int tester_send(struct net_if *iface, struct net_buf *buf)
{
	if (!buf->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	if (test_token == SENDING || timeout_token == SENDING) {
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

		if (net_nbuf_family(buf) == AF_INET6) {
			struct in6_addr addr;

			net_ipaddr_copy(&addr, &NET_IPV6_BUF(buf)->src);
			net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
					&NET_IPV6_BUF(buf)->dst);
			net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, &addr);
		} else {
			struct in_addr addr;

			net_ipaddr_copy(&addr, &NET_IPV4_BUF(buf)->src);
			net_ipaddr_copy(&NET_IPV4_BUF(buf)->src,
					&NET_IPV4_BUF(buf)->dst);
			net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, &addr);
		}

		port = NET_UDP_BUF(buf)->src_port;
		NET_UDP_BUF(buf)->src_port = NET_UDP_BUF(buf)->dst_port;
		NET_UDP_BUF(buf)->dst_port = port;

		if (net_recv_data(iface, buf) < 0) {
			TC_ERROR("Data receive failed.");
			goto out;
		}

		timeout_token = 0;

		return 0;
	}

out:
	net_nbuf_unref(buf);

	if (data_failure) {
		test_failed = true;
	}

	return 0;
}

struct net_context_test net_context_data;

static struct net_if_api net_context_if_api = {
	.init = net_context_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_context_test, "net_context_test",
		net_context_dev_init, &net_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_context_if_api, _ETH_L2_LAYER,
		_ETH_L2_CTX_TYPE, 127);

static bool test_init(void)
{
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = net_if_get_default();

	if (!iface) {
		TC_ERROR("Interface is NULL\n");
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_my));
		return false;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&in4addr_my));
		return false;
	}

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface, &in6addr_mcast);
	if (!maddr) {
		TC_ERROR("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		return false;
	}

	/* The semaphore is there to wait the data to be received. */
	nano_sem_init(&wait_data);

	return true;
}

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "test init", test_init },
	{ "net_context_get failures", net_ctx_get_fail },
	{ "net_context_get all", net_ctx_get_all },
	{ "net_context_get", net_ctx_get_success },
	{ "net_context_get create", net_ctx_create },
	{ "net_context_bind fail", net_ctx_bind_fail },
	{ "net_context_bind IPv6", net_ctx_bind_uni_success_v6 },
	{ "net_context_bind IPv4", net_ctx_bind_uni_success_v4 },
	{ "net_context_bind mcast", net_ctx_bind_mcast_success },
	{ "net_context_listen IPv6", net_ctx_listen_v6 },
	{ "net_context_listen IPv4", net_ctx_listen_v4 },
	{ "net_context_connect IPv6", net_ctx_connect_v6 },
	{ "net_context_connect IPv4", net_ctx_connect_v4 },
	{ "net_context_accept IPv6", net_ctx_accept_v6 },
	{ "net_context_accept IPv4", net_ctx_accept_v4 },
	{ "net_context_send IPv6", net_ctx_send_v6 },
	{ "net_context_send IPv4", net_ctx_send_v4 },
	{ "net_context_sendto IPv6", net_ctx_sendto_v6 },
	{ "net_context_sendto IPv4", net_ctx_sendto_v4 },
	{ "net_context_recv IPv6", net_ctx_recv_v6 },
	{ "net_context_recv IPv4", net_ctx_recv_v4 },
	{ "net_context_recv IPv6 fail", net_ctx_recv_v6_fail },
	{ "net_context_recv IPv4 fail", net_ctx_recv_v4_fail },
	{ "net_context_recv IPv6 again", net_ctx_recv_v6_again },
	{ "net_context_recv IPv4 again", net_ctx_recv_v4_again },
	{ "net_context_recv IPv6 reconfig", net_ctx_recv_v6_reconfig },
	{ "net_context_recv IPv4 reconfig", net_ctx_recv_v4_reconfig },
	{ "net_context_recv IPv6 timeout", net_ctx_recv_v6_timeout },
	{ "net_context_recv IPv4 timeout", net_ctx_recv_v4_timeout },
	{ "net_context_put", net_ctx_put },
};

void main(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);
		test_failed = false;
		if (!tests[count].func() || test_failed) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}
