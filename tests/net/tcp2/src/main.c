/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_TCP_LOG_LEVEL);

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <sys/printk.h>
#include <linker/sections.h>
#include <tc_util.h>

#include <net/ethernet.h>
#include <net/dummy.h>
#include <net/net_pkt.h>

#include "ipv4.h"
#include "ipv6.h"
#include "tcp2.h"
#include "tcp2_priv.h"

#include <ztest.h>

#define MY_PORT 4242
#define PEER_PORT 4242

static struct in_addr my_addr  = { { { 192, 0, 2, 1 } } };
static struct sockaddr_in my_addr_s = {
	.sin_family = AF_INET,
	.sin_port = htons(PEER_PORT),
	.sin_addr = { { { 192, 0, 2, 1 } } },
};

static struct in_addr peer_addr  = { { { 192, 0, 2, 2 } } };
static struct sockaddr_in peer_addr_s = {
	.sin_family = AF_INET,
	.sin_port = htons(PEER_PORT),
	.sin_addr = { { { 192, 0, 2, 2 } } },
};

static struct in6_addr my_addr_v6  = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct sockaddr_in6 my_addr_v6_s = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(PEER_PORT),
	.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0x1 } } },
};

static struct in6_addr peer_addr_v6  = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct sockaddr_in6 peer_addr_v6_s = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(PEER_PORT),
	.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
};

static struct net_if *iface;
static uint8_t test_case_no;
static uint32_t seq;
static uint32_t ack;

static K_SEM_DEFINE(test_sem, 0, 1);
static bool sem;

enum test_state {
	T_SYN = 0,
	T_SYN_ACK,
	T_DATA,
	T_DATA_ACK,
	T_FIN,
	T_FIN_ACK,
	T_FIN_2,
	T_CLOSING
};

static enum test_state t_state;

static struct k_delayed_work test_server;
static void test_server_timeout(struct k_work *work);

static int tester_send(struct device *dev, struct net_pkt *pkt);

static void handle_client_test(sa_family_t af, struct tcphdr *th);
static void handle_server_test(sa_family_t af, struct tcphdr *th);
static void handle_syn_resend(void);
static void handle_client_fin_wait_2_test(sa_family_t af, struct tcphdr *th);
static void handle_client_closing_test(sa_family_t af, struct tcphdr *th);

static void verify_flags(struct tcphdr *th, uint8_t flags,
			 const char *fun, int line)
{
	if (!(th && FL(&th->th_flags, ==, flags))) {
		zassert_true(false, "%s:%d flags mismatch", fun, line);
	}
}

#define test_verify_flags(_th, _flags) \
	verify_flags(_th, _flags, __func__, __LINE__)

struct net_tcp_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_tcp_dev_init(struct device *dev)
{
	struct net_tcp_context *net_tcp_context = dev->data;

	net_tcp_context = net_tcp_context;

	return 0;
}

static uint8_t *net_tcp_get_mac(struct device *dev)
{
	struct net_tcp_context *context = dev->data;

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

static void net_tcp_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_tcp_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

struct net_tcp_context net_tcp_context_data;

static struct dummy_api net_tcp_if_api = {
	.iface_api.init = net_tcp_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_tcp_test, "net_tcp_test",
		net_tcp_dev_init, device_pm_control_nop,
		&net_tcp_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_tcp_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static void test_sem_give(void)
{
	sem = false;
	k_sem_give(&test_sem);
}

static void test_sem_take(k_timeout_t timeout, int line)
{
	sem = true;

	if (k_sem_take(&test_sem, timeout) != 0) {
		zassert_true(false, "semaphore timed out (line %d)", line);
	}
}

static uint8_t tcp_options[20] = {
	0x02, 0x04, 0x05, 0xb4, /* Max segment */
	0x04, 0x02, /* SACK */
	0x08, 0x0a, 0xc2, 0x7b, 0xef, 0x0f, 0x00, 0x00, 0x00, 0x00, /* Time */
	0x01, /* NOP */
	0x03, 0x03, 0x07 /* Win scale*/ };

static struct net_pkt *tester_prepare_tcp_pkt(sa_family_t af,
					      uint16_t src_port, uint16_t dst_port,
					      uint8_t flags, uint8_t *data,
					      size_t len)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct tcphdr);
	struct net_pkt *pkt;
	struct tcphdr *th;
	uint8_t opts_len = 0;
	int ret = -EINVAL;

	if ((test_case_no == 4U) && (flags & SYN)) {
		opts_len = sizeof(tcp_options);
	}

	/* Allocate buffer */
	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct tcphdr) + len + opts_len,
					af, IPPROTO_TCP, K_NO_WAIT);
	if (!pkt) {
		return NULL;
	}

	/* Create IP header */
	if (af == AF_INET) {
		ret = net_ipv4_create(pkt, &peer_addr, &my_addr);
	} else if (af == AF_INET6) {
		ret = net_ipv6_create(pkt, &peer_addr_v6, &my_addr_v6);
	} else {
		goto fail;
	}

	/* Create TCP header */
	if (ret < 0) {
		goto fail;
	}

	th = (struct tcphdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!th) {
		goto fail;
	}

	memset(th, 0U, sizeof(struct tcphdr));

	th->th_sport = src_port;
	th->th_dport = dst_port;

	if ((test_case_no == 4U) && (flags & SYN)) {
		th->th_off = 10U;
	} else {
		th->th_off = 5U;
	}

	th->th_flags = flags;
	th->th_win = NET_IPV6_MTU;
	th->th_seq = htonl(seq);

	if (ACK & flags) {
		th->th_ack = htonl(ack);
	}

	ret = net_pkt_set_data(pkt, &tcp_access);
	if (ret < 0) {
		goto fail;
	}

	if ((test_case_no == 4U) && (flags & SYN)) {
		/* Add TCP Options */
		ret = net_pkt_write(pkt, tcp_options, opts_len);
		if (ret < 0) {
			goto fail;
		}
	}

	if (data && len) {
		ret = net_pkt_write(pkt, data, len);
		if (ret < 0) {
			goto fail;
		}
	}

	net_pkt_cursor_init(pkt);

	if (af == AF_INET) {
		ret = net_ipv4_finalize(pkt, IPPROTO_TCP);
	} else if (af == AF_INET6) {
		ret = net_ipv6_finalize(pkt, IPPROTO_TCP);
	} else {
		goto fail;
	}

	if (ret < 0) {
		goto fail;
	}

	return pkt;
fail:
	net_pkt_unref(pkt);
	return NULL;
}

static struct net_pkt *prepare_syn_packet(sa_family_t af, uint16_t src_port,
					  uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, SYN, NULL, 0U);
}

static struct net_pkt *prepare_syn_ack_packet(sa_family_t af, uint16_t src_port,
					      uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, SYN | ACK,
				      NULL, 0U);
}

static struct net_pkt *prepare_ack_packet(sa_family_t af, uint16_t src_port,
					  uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, ACK, NULL, 0U);
}

static struct net_pkt *prepare_data_packet(sa_family_t af, uint16_t src_port,
					   uint16_t dst_port, uint8_t *data,
					   size_t len)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, PSH | ACK, data,
				      len);
}

static struct net_pkt *prepare_fin_ack_packet(sa_family_t af, uint16_t src_port,
					      uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, FIN | ACK,
				      NULL, 0U);
}

static struct net_pkt *prepare_fin_packet(sa_family_t af, uint16_t src_port,
					  uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, FIN, NULL, 0U);
}

static int read_tcp_header(struct net_pkt *pkt, struct tcphdr *th)
{
	int ret;

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	ret = net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			   net_pkt_ip_opts_len(pkt));
	if (ret < 0) {
		goto fail;
	}

	ret = net_pkt_read(pkt, th, sizeof(struct tcphdr));
	if (ret < 0) {
		goto fail;
	}

	net_pkt_cursor_init(pkt);

	return 0;
fail:
	return -EINVAL;
}

static int tester_send(struct device *dev, struct net_pkt *pkt)
{
	struct tcphdr th;
	int ret;

	ret = read_tcp_header(pkt, &th);
	if (ret < 0) {
		goto fail;
	}

	switch (test_case_no) {
	case 1:
	case 2:
		handle_client_test(net_pkt_family(pkt), &th);
		break;
	case 3:
	case 4:
	case 5:
		handle_server_test(net_pkt_family(pkt), &th);
		break;
	case 6:
		handle_syn_resend();
		break;
	case 7:
		handle_client_fin_wait_2_test(net_pkt_family(pkt), &th);
		break;
	case 8:
		handle_client_closing_test(net_pkt_family(pkt), &th);
		break;
	default:
		zassert_true(false, "Undefined test case");
	}

	return 0;
fail:
	zassert_true(false, "%s failed", __func__);

	net_pkt_unref(pkt);

	return -EINVAL;
}

/* Initial setup for the tests */
static void test_presetup(void)
{
	struct net_if_addr *ifaddr;

	iface = net_if_get_default();
	if (!iface) {
		zassert_true(false, "Interface not available");
	}

	ifaddr = net_if_ipv4_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		zassert_true(false, "Failed to add IPv4 address");
	}

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr_v6, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		zassert_true(false, "Failed to add IPv6 address");
	}

	k_delayed_work_init(&test_server, test_server_timeout);
}

static void handle_client_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		seq = 0U;
		ack = ntohs(th->th_seq) + 1U;
		reply = prepare_syn_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		/* connection is success */
		t_state = T_DATA;
		test_sem_give();
		return;
	case T_DATA:
		test_verify_flags(th, PSH | ACK);
		seq++;
		ack = ack + 1U;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		t_state = T_FIN;
		test_sem_give();
		break;
	case T_FIN:
		test_verify_flags(th, FIN | ACK);
		ack = ntohs(th->th_seq) + 1U;
		t_state = T_FIN_ACK;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		break;
	case T_FIN_ACK:
		test_verify_flags(th, ACK);
		test_sem_give();
		return;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	ret = net_recv_data(iface, reply);
	if (ret < 0) {
		goto fail;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

/* Test case scenario IPv4
 *   send SYN,
 *   expect SYN ACK,
 *   send ACK,
 *   send Data,
 *   expect ACK,
 *   send FIN,
 *   expect FIN ACK,
 *   send ACK.
 *   any failures cause test case to fail.
 */
static void test_client_ipv4(void)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = 1;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
				  sizeof(struct sockaddr_in),
				  NULL,
				  K_MSEC(100), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to connect to peer");
	}

	/* Peer will release the semaphone after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphone after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_tcp_put(ctx);

	/* Peer will release the semaphone after it receives
	 * proper ACK to FIN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

/* Test case scenario IPv6
 *   send SYN,
 *   expect SYN ACK,
 *   send ACK,
 *   send Data,
 *   expect ACK,
 *   send FIN,
 *   expect FIN ACK,
 *   send ACK.
 *   any failures cause test case to fail.
 */
static void test_client_ipv6(void)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = 2;
	seq = ack = 0;

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_v6_s,
				  sizeof(struct sockaddr_in6),
				  NULL,
				  K_MSEC(100), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to connect to peer");
	}

	/* Peer will release the semaphone after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphone after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_tcp_put(ctx);

	/* Peer will release the semaphone after it receives
	 * proper ACK to FIN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

static void handle_server_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

	switch (t_state) {
	case T_SYN:
		seq = 0U;
		ack = 0U;
		reply = prepare_syn_packet(af, htons(MY_PORT),
					   htons(PEER_PORT));
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, SYN | ACK);
		seq++;
		ack = ntohs(th->th_seq) + 1U;
		reply = prepare_ack_packet(af, htons(MY_PORT),
					   htons(PEER_PORT));
		t_state = T_DATA;
		break;
	case T_DATA:
		reply = prepare_data_packet(af, htons(MY_PORT),
					    htons(PEER_PORT), "A", 1U);
		t_state = T_DATA_ACK;
		break;
	case T_DATA_ACK:
		test_verify_flags(th, ACK);
		seq++;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT),
					       htons(PEER_PORT));
		t_state = T_FIN;
		break;
	case T_FIN:
		test_verify_flags(th, FIN | ACK);
		seq++;
		ack++;
		reply = prepare_ack_packet(af, htons(MY_PORT),
					   htons(PEER_PORT));
		t_state = T_FIN_ACK;
		break;
	case T_FIN_ACK:
		return;
	default:
		zassert_true(false, "%s: unexpected state", __func__);
		return;
	}

	ret = net_recv_data(iface, reply);
	if (ret < 0) {
		goto fail;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

static void test_server_timeout(struct k_work *work)
{
	if (test_case_no == 3 || test_case_no == 4) {
		handle_server_test(AF_INET, NULL);
	} else if (test_case_no == 5) {
		handle_server_test(AF_INET6, NULL);
	} else {
		zassert_true(false, "Invalid test case");
	}
}

static void test_tcp_recv_cb(struct net_context *context,
			     struct net_pkt *pkt,
			     union net_ip_header *ip_hdr,
			     union net_proto_header *proto_hdr,
			     int status,
			     void *user_data)
{
	if (status && status != -ECONNRESET) {
		zassert_true(false, "failed to recv the data");
	}
}

static void test_tcp_accept_cb(struct net_context *ctx,
			       struct sockaddr *addr,
			       socklen_t addrlen,
			       int status,
			       void *user_data)
{
	if (status) {
		zassert_true(false, "failed to accept the conn");
	}

	/* set callback on newly created context */
	ctx->recv_cb = test_tcp_recv_cb;

	test_sem_give();
}

/* Test case scenario IPv4
 *   Expect SYN
 *   send SYN ACK,
 *   expect ACK,
 *   expect DATA,
 *   send ACK,
 *   expect FIN,
 *   send FIN ACK,
 *   expect ACK.
 *   any failures cause test case to fail.
 */
static void test_server_ipv4(void)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = 3;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	ret = net_context_bind(ctx, (struct sockaddr *)&my_addr_s,
			       sizeof(struct sockaddr_in));
	if (ret < 0) {
		zassert_true(false, "Failed to bind net_context");
	}

	ret = net_context_listen(ctx, 1);
	if (ret < 0) {
		zassert_true(false, "Failed to listen on net_context");
	}

	/* Trigger the peer to send SYN */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	ret = net_context_accept(ctx, test_tcp_accept_cb, K_FOREVER, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* test_tcp_accept_cb will release the semaphone after succesfull
	 * connection.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Trigger the peer to send DATA  */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	ret = net_context_recv(ctx, test_tcp_recv_cb, K_MSEC(200), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to recv data from peer");
	}

	/* Trigger the peer to send FIN after timeout */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	net_context_put(ctx);
}

/* Test case scenario IPv4
 *   Expect SYN with TCP options
 *   send SYN ACK,
 *   expect ACK,
 *   expect DATA,
 *   send ACK,
 *   expect FIN,
 *   send FIN ACK,
 *   expect ACK.
 *   any failures cause test case to fail.
 */
static void test_server_with_options_ipv4(void)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = 4;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	ret = net_context_bind(ctx, (struct sockaddr *)&my_addr_s,
			       sizeof(struct sockaddr_in));
	if (ret < 0) {
		zassert_true(false, "Failed to bind net_context");
	}

	ret = net_context_listen(ctx, 1);
	if (ret < 0) {
		zassert_true(false, "Failed to listen on net_context");
	}

	/* Trigger the peer to send SYN */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	ret = net_context_accept(ctx, test_tcp_accept_cb, K_FOREVER, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* test_tcp_accept_cb will release the semaphone after succesfull
	 * connection.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Trigger the peer to send DATA  */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	ret = net_context_recv(ctx, test_tcp_recv_cb, K_MSEC(200), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to recv data from peer");
	}

	/* Trigger the peer to send FIN after timeout */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	net_context_put(ctx);
}

/* Test case scenario IPv6
 *   Expect SYN
 *   send SYN ACK,
 *   expect ACK,
 *   expect DATA,
 *   send ACK,
 *   expect FIN,
 *   send FIN ACK,
 *   expect ACK.
 *   any failures cause test case to fail.
 */
static void test_server_ipv6(void)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = 5;
	seq = ack = 0;

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	ret = net_context_bind(ctx, (struct sockaddr *)&my_addr_v6_s,
			       sizeof(struct sockaddr_in6));
	if (ret < 0) {
		zassert_true(false, "Failed to bind net_context");
	}

	ret = net_context_listen(ctx, 1);
	if (ret < 0) {
		zassert_true(false, "Failed to listen on net_context");
	}

	/* Trigger the peer to send SYN  */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	ret = net_context_accept(ctx, test_tcp_accept_cb, K_FOREVER, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* test_tcp_accept_cb will release the semaphone after succesfull
	 * connection.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Trigger the peer to send DATA  */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	ret = net_context_recv(ctx, test_tcp_recv_cb, K_MSEC(200), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to recv data from peer");
	}

	/* Trigger the peer to send FIN after timeout */
	k_delayed_work_submit(&test_server, K_NO_WAIT);

	net_context_put(ctx);
}

static void handle_syn_resend(void)
{
	static uint8_t syn_times;

	syn_times++;

	if (syn_times == 2) {
		test_sem_give();
	}
}

/* Test case scenario IPv4
 *   send SYN,
 *   peer doesn't reply SYN ACK,
 *   send SYN again,
 *   any failures cause test case to fail.
 */
static void test_client_syn_resend(void)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = 6;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
				  sizeof(struct sockaddr_in),
				  NULL,
				  K_MSEC(1000), NULL);

	zassert_true(ret < 0, "Connect on no response from peer");

	/* test handler will release the sem once it receives SYN again */
	test_sem_take(K_MSEC(500), __LINE__);

	net_context_put(ctx);
}

static void handle_client_fin_wait_2_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

send_next:
	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		seq = 0U;
		ack = ntohs(th->th_seq) + 1U;
		reply = prepare_syn_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		/* connection is success */
		t_state = T_DATA;
		test_sem_give();
		return;
	case T_DATA:
		test_verify_flags(th, PSH | ACK);
		seq++;
		ack = ack + 1U;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		t_state = T_FIN;
		test_sem_give();
		break;
	case T_FIN:
		test_verify_flags(th, FIN | ACK);
		ack = ntohs(th->th_seq) + 1U;
		t_state = T_FIN_2;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_2:
		t_state = T_FIN_ACK;
		reply = prepare_fin_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_ACK:
		test_verify_flags(th, ACK);
		test_sem_give();
		return;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	ret = net_recv_data(iface, reply);
	if (ret < 0) {
		goto fail;
	}

	if (t_state == T_FIN_2) {
		goto send_next;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

/* Test case scenario IPv4
 *   send SYN,
 *   expect SYN ACK,
 *   send ACK,
 *   send Data,
 *   expect ACK,
 *   send FIN,
 *   expect ACK,
 *   expect FIN,
 *   send ACK,
 *   any failures cause test case to fail.
 */
static void test_client_fin_wait_2_ipv4(void)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = 7;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
				  sizeof(struct sockaddr_in),
				  NULL,
				  K_MSEC(100), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to connect to peer");
	}

	/* Peer will release the semaphone after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphone after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_tcp_put(ctx);

	/* Peer will release the semaphone after it receives
	 * proper ACK to FIN | ACK
	 */
	test_sem_take(K_MSEC(300), __LINE__);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

static void handle_client_closing_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		seq = 0U;
		ack = ntohs(th->th_seq) + 1U;
		reply = prepare_syn_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		/* connection is success */
		t_state = T_DATA;
		test_sem_give();
		return;
	case T_DATA:
		test_verify_flags(th, PSH | ACK);
		seq++;
		ack = ack + 1U;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		t_state = T_FIN;
		test_sem_give();
		break;
	case T_FIN:
		test_verify_flags(th, FIN | ACK);
		ack = ntohs(th->th_seq) + 1U;
		t_state = T_CLOSING;
		reply = prepare_fin_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_CLOSING:
		test_verify_flags(th, ACK);
		t_state = T_FIN_ACK;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	ret = net_recv_data(iface, reply);
	if (ret < 0) {
		goto fail;
	}

	if (t_state == T_FIN_ACK) {
		test_sem_give();
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

/* Test case scenario IPv6
 *   send SYN,
 *   expect SYN ACK,
 *   send ACK,
 *   send Data,
 *   expect ACK,
 *   send FIN,
 *   expect FIN,
 *   send ACK,
 *   expect ACK,
 *   any failures cause test case to fail.
 */
static void test_client_closing_ipv6(void)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = 8;
	seq = ack = 0;

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_v6_s,
				  sizeof(struct sockaddr_in6),
				  NULL,
				  K_MSEC(100), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to connect to peer");
	}

	/* Peer will release the semaphone after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphone after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_tcp_put(ctx);

	/* Peer will release the semaphone after it receives
	 * proper ACK to FIN | ACK
	 */
	test_sem_take(K_MSEC(300), __LINE__);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

/** Test case main entry */
void test_main(void)
{
	ztest_test_suite(test_tcp_fn,
			 ztest_unit_test(test_presetup),
			 ztest_unit_test(test_client_ipv4),
			 ztest_unit_test(test_client_ipv6),
			 ztest_unit_test(test_server_ipv4),
			 ztest_unit_test(test_server_with_options_ipv4),
			 ztest_unit_test(test_server_ipv6),
			 ztest_unit_test(test_client_syn_resend),
			 ztest_unit_test(test_client_fin_wait_2_ipv4),
			 ztest_unit_test(test_client_closing_ipv6)
			 );

	ztest_run_test_suite(test_tcp_fn);
}
