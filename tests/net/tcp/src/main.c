/* main.c - Application main entry point
 *
 * This is a self-contained test for exercising the TCP protocol stack. Both
 * the server and client side run on ths same device using a DUMMY interface
 * as a loopback of the IP packets.
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_TCP_LOG_LEVEL);

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>
#include <zephyr/tc_util.h>

#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_pkt.h>

#include "ipv4.h"
#include "ipv6.h"
#include "tcp_internal.h"
#include "net_stats.h"

#include <zephyr/ztest.h>

#define MY_PORT 4242
#define PEER_PORT 4242

/* Data (1280 bytes) to be sent */
static const char lorem_ipsum[] = LOREM_IPSUM;

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

static struct net_if *net_iface;
static uint32_t seq;
static uint32_t device_initial_seq;
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
	T_FIN_1,
	T_FIN_2,
	T_CLOSING,
	T_RST,
};

static enum test_case_no {
	TEST_CLIENT_IPV4 = 1,
	TEST_CLIENT_IPV6 = 2,
	TEST_SERVER_IPV4 = 3,
	TEST_SERVER_WITH_OPTIONS_IPV4 = 4,
	TEST_SERVER_IPV6 = 5,
	TEST_CLIENT_SYN_RESEND = 6,
	TEST_CLIENT_FIN_WAIT_2_IPV4 = 7,
	TEST_CLIENT_CLOSING_IPV6 = 8,
	TEST_SERVER_RECV_OUT_OF_ORDER_DATA = 9,
	TEST_CLIENT_FIN_WAIT_1_RETRANSMIT_IPV4 = 10,
	TEST_CLIENT_DATA_DURING_FIN_1_IPV4 = 11,
	TEST_CLIENT_SYN_RST_ACK = 12,
	TEST_SERVER_RST_ON_CLOSED_PORT = 13,
	TEST_SERVER_RST_ON_LISTENING_PORT_NO_ACTIVE_CONNECTION = 14,
	TEST_CLIENT_RST_ON_UNEXPECTED_ACK_ON_SYN = 15,
	TEST_CLIENT_CLOSING_FAILURE_IPV6 = 16,
	TEST_CLIENT_FIN_WAIT_2_IPV4_FAILURE = 17,
	TEST_CLIENT_FIN_ACK_WITH_DATA = 18,
	TEST_CLIENT_SEQ_VALIDATION = 19,
	TEST_SERVER_ACK_VALIDATION = 20,
	TEST_SERVER_FIN_ACK_AFTER_DATA = 21,
} test_case_no;

static enum test_state t_state;

static struct k_work_delayable test_server;
static void test_server_timeout(struct k_work *work);

static int tester_send(const struct device *dev, struct net_pkt *pkt);

static void handle_client_test(sa_family_t af, struct tcphdr *th);
static void handle_server_test(sa_family_t af, struct tcphdr *th);
static void handle_syn_resend(void);
static void handle_syn_rst_ack(sa_family_t af, struct tcphdr *th);
static void handle_client_fin_wait_2_test(sa_family_t af, struct tcphdr *th);
static void handle_client_fin_wait_2_failure_test(sa_family_t af, struct tcphdr *th);
static void handle_client_closing_test(sa_family_t af, struct tcphdr *th);
static void handle_client_closing_failure_test(sa_family_t af, struct tcphdr *th);
static void handle_data_fin1_test(sa_family_t af, struct tcphdr *th);
static void handle_data_during_fin1_test(sa_family_t af, struct tcphdr *th);
static void handle_server_recv_out_of_order(struct net_pkt *pkt);
static void handle_server_rst_on_closed_port(sa_family_t af, struct tcphdr *th);
static void handle_server_rst_on_listening_port(sa_family_t af, struct tcphdr *th);
static void handle_syn_invalid_ack(sa_family_t af, struct tcphdr *th);
static void handle_client_fin_ack_with_data_test(sa_family_t af, struct tcphdr *th);
static void handle_client_seq_validation_test(sa_family_t af, struct tcphdr *th);
static void handle_server_ack_validation_test(struct net_pkt *pkt);
static void handle_server_fin_ack_after_data_test(sa_family_t af, struct tcphdr *th);

static void verify_flags(struct tcphdr *th, uint8_t flags,
			 const char *fun, int line)
{
	if (!(th && FL(&th->th_flags, ==, flags))) {
		zassert_true(false,
			     "%s:%d flags mismatch (0x%04x vs 0x%04x)",
			     fun, line, th->th_flags, flags);
	}
}

#define test_verify_flags(_th, _flags) \
	verify_flags(_th, _flags, __func__, __LINE__)

struct net_tcp_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_tcp_dev_init(const struct device *dev)
{
	struct net_tcp_context *net_tcp_context = dev->data;

	net_tcp_context = net_tcp_context;

	return 0;
}

static uint8_t *net_tcp_get_mac(const struct device *dev)
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
		net_tcp_dev_init, NULL,
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

static void test_sem_take_failure(k_timeout_t timeout, int line)
{
	sem = true;

	if (k_sem_take(&test_sem, timeout) == 0) {
		zassert_true(false, "semaphore succeed out (line %d)", line);
	}
}

static uint8_t tcp_options[20] = {
	0x02, 0x04, 0x05, 0xb4, /* Max segment */
	0x04, 0x02, /* SACK */
	0x08, 0x0a, 0xc2, 0x7b, 0xef, 0x0f, 0x00, 0x00, 0x00, 0x00, /* Time */
	0x01, /* NOP */
	0x03, 0x03, 0x07 /* Win scale*/ };

static struct net_pkt *tester_prepare_tcp_pkt(sa_family_t af,
					      uint16_t src_port,
					      uint16_t dst_port,
					      uint8_t flags,
					      const uint8_t *data,
					      size_t len)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct tcphdr);
	struct net_pkt *pkt;
	struct tcphdr *th;
	uint8_t opts_len = 0;
	int ret = -EINVAL;

	if ((test_case_no == TEST_SERVER_WITH_OPTIONS_IPV4) && (flags & SYN)) {
		opts_len = sizeof(tcp_options);
	}

	/* Allocate buffer */
	pkt = net_pkt_alloc_with_buffer(net_iface,
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

	if ((test_case_no == TEST_SERVER_WITH_OPTIONS_IPV4) && (flags & SYN)) {
		th->th_off = 10U;
	} else {
		th->th_off = 5U;
	}

	th->th_flags = flags;
	th->th_win = htons(NET_IPV6_MTU);
	th->th_seq = htonl(seq);

	if (ACK & flags) {
		th->th_ack = htonl(ack);
	}

	ret = net_pkt_set_data(pkt, &tcp_access);
	if (ret < 0) {
		goto fail;
	}

	if ((test_case_no == TEST_SERVER_WITH_OPTIONS_IPV4) && (flags & SYN)) {
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

static struct net_pkt *prepare_rst_ack_packet(sa_family_t af, uint16_t src_port,
					      uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, RST | ACK,
				      NULL, 0U);
}

static struct net_pkt *prepare_ack_packet(sa_family_t af, uint16_t src_port,
					  uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, ACK, NULL, 0U);
}

static struct net_pkt *prepare_data_packet(sa_family_t af, uint16_t src_port,
					   uint16_t dst_port,
					   const uint8_t *data,
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

static struct net_pkt *prepare_rst_packet(sa_family_t af, uint16_t src_port,
					  uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, RST, NULL, 0U);
}

static bool is_icmp_pkt(struct net_pkt *pkt)
{
	if (net_pkt_family(pkt) == AF_INET) {
		return NET_IPV4_HDR(pkt)->proto == IPPROTO_ICMP;
	} else {
		return NET_IPV6_HDR(pkt)->nexthdr == IPPROTO_ICMPV6;
	}
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

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct tcphdr th;
	int ret;

	/* Ignore ICMP explicitly */
	if (is_icmp_pkt(pkt)) {
		return 0;
	}

	ret = read_tcp_header(pkt, &th);
	if (ret < 0) {
		goto fail;
	}

	switch (test_case_no) {
	case TEST_CLIENT_IPV4:
	case TEST_CLIENT_IPV6:
		handle_client_test(net_pkt_family(pkt), &th);
		break;
	case TEST_SERVER_IPV4:
	case TEST_SERVER_WITH_OPTIONS_IPV4:
	case TEST_SERVER_IPV6:
		handle_server_test(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_SYN_RESEND:
		handle_syn_resend();
		break;
	case TEST_CLIENT_FIN_WAIT_2_IPV4:
		handle_client_fin_wait_2_test(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_CLOSING_IPV6:
		handle_client_closing_test(net_pkt_family(pkt), &th);
		break;
	case TEST_SERVER_RECV_OUT_OF_ORDER_DATA:
		handle_server_recv_out_of_order(pkt);
		break;
	case TEST_CLIENT_FIN_WAIT_1_RETRANSMIT_IPV4:
		handle_data_fin1_test(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_DATA_DURING_FIN_1_IPV4:
		handle_data_during_fin1_test(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_SYN_RST_ACK:
		handle_syn_rst_ack(net_pkt_family(pkt), &th);
		break;
	case TEST_SERVER_RST_ON_CLOSED_PORT:
		handle_server_rst_on_closed_port(net_pkt_family(pkt), &th);
		break;
	case TEST_SERVER_RST_ON_LISTENING_PORT_NO_ACTIVE_CONNECTION:
		handle_server_rst_on_listening_port(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_RST_ON_UNEXPECTED_ACK_ON_SYN:
		handle_syn_invalid_ack(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_CLOSING_FAILURE_IPV6:
		handle_client_closing_failure_test(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_FIN_WAIT_2_IPV4_FAILURE:
		handle_client_fin_wait_2_failure_test(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_FIN_ACK_WITH_DATA:
		handle_client_fin_ack_with_data_test(net_pkt_family(pkt), &th);
		break;
	case TEST_CLIENT_SEQ_VALIDATION:
		handle_client_seq_validation_test(net_pkt_family(pkt), &th);
		break;
	case TEST_SERVER_ACK_VALIDATION:
		handle_server_ack_validation_test(pkt);
		break;
	case TEST_SERVER_FIN_ACK_AFTER_DATA:
		handle_server_fin_ack_after_data_test(net_pkt_family(pkt), &th);
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
static void *presetup(void)
{
	struct net_if_addr *ifaddr;

	net_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	if (!net_iface) {
		zassert_true(false, "Interface not available");
	}

	ifaddr = net_if_ipv4_addr_add(net_iface, &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		zassert_true(false, "Failed to add IPv4 address");
	}

	ifaddr = net_if_ipv6_addr_add(net_iface, &my_addr_v6, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		zassert_true(false, "Failed to add IPv6 address");
	}

	k_work_init_delayable(&test_server, test_server_timeout);
	return NULL;
}

static void handle_client_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
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
		ack = ack + 1U;
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

	ret = net_recv_data(net_iface, reply);
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
ZTEST(net_tcp, test_client_ipv4)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_IPV4;
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

	/* Peer will release the semaphore after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);

	/* Peer will release the semaphore after it receives
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
ZTEST(net_tcp, test_client_ipv6)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_IPV6;
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

	/* Peer will release the semaphore after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);

	/* Peer will release the semaphore after it receives
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
		reply = prepare_syn_packet(af, htons(MY_PORT),
					   htons(PEER_PORT));
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, SYN | ACK);
		seq++;
		ack = ntohl(th->th_seq) + 1U;
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

	ret = net_recv_data(net_iface, reply);
	if (ret < 0) {
		goto fail;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

static void test_server_timeout(struct k_work *work)
{
	if (test_case_no == TEST_SERVER_IPV4 ||
	    test_case_no == TEST_SERVER_WITH_OPTIONS_IPV4 ||
	    test_case_no == TEST_SERVER_RST_ON_CLOSED_PORT ||
	    test_case_no == TEST_SERVER_RST_ON_LISTENING_PORT_NO_ACTIVE_CONNECTION) {
		handle_server_test(AF_INET, NULL);
	} else if (test_case_no == TEST_SERVER_IPV6) {
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

	if (pkt) {
		net_pkt_unref(pkt);
	}
}

static struct net_context *accepted_ctx;

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
	accepted_ctx = ctx;

	/* Ref the context on the app behalf. */
	net_context_ref(ctx);

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
ZTEST(net_tcp, test_server_ipv4)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_SERVER_IPV4;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

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
	k_work_reschedule(&test_server, K_NO_WAIT);

	ret = net_context_accept(ctx, test_tcp_accept_cb, K_FOREVER, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* test_tcp_accept_cb will release the semaphore after successful
	 * connection.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Trigger the peer to send DATA  */
	k_work_reschedule(&test_server, K_NO_WAIT);

	ret = net_context_recv(accepted_ctx, test_tcp_recv_cb, K_MSEC(200), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to recv data from peer");
	}

	/* Trigger the peer to send FIN after timeout */
	k_work_reschedule(&test_server, K_NO_WAIT);

	/* Let the receiving thread run */
	k_msleep(50);

	net_context_put(ctx);
	net_context_put(accepted_ctx);
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
ZTEST(net_tcp, test_server_with_options_ipv4)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_SERVER_WITH_OPTIONS_IPV4;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

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
	k_work_reschedule(&test_server, K_NO_WAIT);

	ret = net_context_accept(ctx, test_tcp_accept_cb, K_FOREVER, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* test_tcp_accept_cb will release the semaphore after successful
	 * connection.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Trigger the peer to send DATA  */
	k_work_reschedule(&test_server, K_NO_WAIT);

	ret = net_context_recv(accepted_ctx, test_tcp_recv_cb, K_MSEC(200), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to recv data from peer");
	}

	/* Trigger the peer to send FIN after timeout */
	k_work_reschedule(&test_server, K_NO_WAIT);

	/* Let the receiving thread run */
	k_msleep(50);

	net_context_put(ctx);
	net_context_put(accepted_ctx);
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
ZTEST(net_tcp, test_server_ipv6)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_SERVER_IPV6;
	seq = ack = 0;

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

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
	k_work_reschedule(&test_server, K_NO_WAIT);

	ret = net_context_accept(ctx, test_tcp_accept_cb, K_FOREVER, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* test_tcp_accept_cb will release the semaphore after successful
	 * connection.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Trigger the peer to send DATA  */
	k_work_reschedule(&test_server, K_NO_WAIT);

	ret = net_context_recv(accepted_ctx, test_tcp_recv_cb, K_MSEC(200), NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to recv data from peer");
	}

	/* Trigger the peer to send FIN after timeout */
	k_work_reschedule(&test_server, K_NO_WAIT);

	/* Let the receiving thread run */
	k_msleep(50);

	net_context_put(ctx);
	net_context_put(accepted_ctx);
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
ZTEST(net_tcp, test_client_syn_resend)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_SYN_RESEND;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
				  sizeof(struct sockaddr_in),
				  NULL,
				  K_MSEC(300 + 50), NULL);

	zassert_true(ret < 0, "Connect on no response from peer");

	/* test handler will release the sem once it receives SYN again */
	test_sem_take(K_MSEC(500), __LINE__);

	net_context_put(ctx);
}

static void handle_syn_rst_ack(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
		reply = prepare_rst_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		t_state = T_CLOSING;
		break;
	default:
		return;
	}

	ret = net_recv_data(net_iface, reply);
	if (ret < 0) {
		goto fail;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

/* Test case scenario IPv4
 *   send SYN,
 *   peer replies RST+ACK,
 *   net_context_connect should report an error
 */
ZTEST(net_tcp, test_client_syn_rst_ack)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_SYN_RST_ACK;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
				  sizeof(struct sockaddr_in),
				  NULL,
				  K_MSEC(1000), NULL);

	zassert_true(ret < 0, "Connect successful on RST+ACK");
	zassert_not_equal(net_context_get_state(ctx), NET_CONTEXT_CONNECTED,
			  "Context should not be connected");

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
		ack = ntohl(th->th_seq) + 1U;
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
		ack = ack + 1U;
		t_state = T_FIN_2;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_2:
		t_state = T_FIN_ACK;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_ACK:
		test_verify_flags(th, ACK);
		test_sem_give();
		return;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	ret = net_recv_data(net_iface, reply);
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
ZTEST(net_tcp, test_client_fin_wait_2_ipv4)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_FIN_WAIT_2_IPV4;
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

	/* Peer will release the semaphore after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);

	/* Peer will release the semaphore after it receives
	 * proper ACK to FIN | ACK
	 */
	test_sem_take(K_MSEC(300), __LINE__);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

static void handle_client_fin_wait_2_failure_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

send_next:
	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
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
		ack = ack + 1U;
		t_state = T_FIN_2;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_2:
		t_state = T_FIN_ACK;
		/* We do not send last FIN that would move the state to TIME_WAIT.
		 * Make sure that the stack can recover from this by closing the
		 * connection.
		 */
		reply = NULL;
		break;
	case T_FIN_ACK:
		reply = NULL;
		return;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	if (reply != NULL) {
		ret = net_recv_data(net_iface, reply);
		if (ret < 0) {
			goto fail;
		}
	}

	if (t_state == T_FIN_2) {
		goto send_next;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

static bool closed;
static K_SEM_DEFINE(wait_data, 0, 1);

static void connection_closed(struct tcp *conn, void *user_data)
{
	struct k_sem *my_sem = user_data;

	k_sem_give(my_sem);
	closed = true;
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
ZTEST(net_tcp, test_client_fin_wait_2_ipv4_failure)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_FIN_WAIT_2_IPV4_FAILURE;
	seq = ack = 0;
	closed = false;

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

	/* Peer will release the semaphore after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	k_sem_reset(&wait_data);
	k_sem_take(&wait_data, K_NO_WAIT);

	tcp_install_close_cb(ctx, connection_closed, &wait_data);

	net_context_put(ctx);

	/* The lock is released after we have closed the connection. */
	k_sem_take(&wait_data, K_MSEC(10000));
	zassert_equal(closed, true, "Connection was not closed!");
}

static uint32_t get_rel_seq(struct tcphdr *th)
{
	return ntohl(th->th_seq) - device_initial_seq;
}

static void handle_data_fin1_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

send_next:
	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		device_initial_seq = ntohl(th->th_seq);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
		reply = prepare_syn_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		seq++;
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		/* connection is success */
		reply = prepare_data_packet(af, htons(MY_PORT),
					    th->th_sport, "A", 1U);
		t_state = T_DATA_ACK;
		break;
	case T_DATA_ACK:
		test_verify_flags(th, ACK);
		test_sem_give();
		reply = NULL;
		t_state = T_FIN;
		return;
	case T_FIN:
		test_verify_flags(th, FIN | ACK);
		zassert_true(get_rel_seq(th) == 1,
			     "%s:%d unexpected sequence number in original FIN, got %d",
			     __func__, __LINE__, get_rel_seq(th));
		zassert_true(ntohl(th->th_ack) == 2,
			     "%s:%d unexpected acknowledgment in original FIN, got %d", __func__,
			     __LINE__, ntohl(th->th_ack));
		t_state = T_FIN_1;
		/* retransmit the data that we already send*/
		reply = prepare_data_packet(af, htons(MY_PORT),
					    th->th_sport, "A", 1U);
		seq++;
		break;
	case T_FIN_1:
		/* The FIN retransmit timer should not yet be expired */
		test_verify_flags(th, ACK);
		/* The ACK to an old data packet should be with the SND.NXT seq,
		 * and the RCV.NXT ack.
		 */
		zassert_true(get_rel_seq(th) == 2,
			     "%s:%i unexpected sequence number in retransmitted FIN, got %d",
			     __func__, __LINE__, get_rel_seq(th));
		zassert_true(ntohl(th->th_ack) == 2,
			     "%s:%i unexpected acknowledgment in retransmitted FIN, got %d",
			     __func__, __LINE__, ntohl(th->th_ack));
		ack = ack + 1U;
		t_state = T_FIN_2;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_2:
		t_state = T_FIN_ACK;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_ACK:
		test_verify_flags(th, ACK);
		test_sem_give();
		return;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	ret = net_recv_data(net_iface, reply);
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
 *   expect SYN,
 *   send SYN ACK,
 *   expect ACK,
 *   expect Data,
 *   send ACK,
 *   expect FIN,
 *   resend Data,
 *   expect FIN,
 *   send ACK,
 *   send FIN,
 *   expect ACK
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_client_fin_wait_1_retransmit_ipv4)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_FIN_WAIT_1_RETRANSMIT_IPV4;
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

	/* Do not perform a receive, as I cannot get it to work and it is not required to get the
	 * test functional, the receive functionality is covered by other tests.
	 */

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);

	/* Peer will release the semaphore after it receives
	 * proper ACK to FIN | ACK
	 */
	test_sem_take(K_MSEC(300), __LINE__);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

static void handle_data_during_fin1_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		device_initial_seq = ntohl(th->th_seq);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
		reply = prepare_syn_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		seq++;
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		test_sem_give();
		t_state = T_FIN;
		return;
	case T_FIN:
		test_verify_flags(th, FIN | ACK);
		zassert_true(get_rel_seq(th) == 1,
			     "%s:%d unexpected sequence number in original FIN, got %d",
			     __func__, __LINE__, get_rel_seq(th));
		zassert_true(ntohl(th->th_ack) == 1,
			     "%s:%d unexpected acknowledgment in original FIN, got %d", __func__,
			     __LINE__, ntohl(th->th_ack));

		ack = ack + 1U;

		/* retransmit the data that we already send / missed */
		reply = prepare_data_packet(af, htons(MY_PORT),
					    th->th_sport, "A", 1U);

		t_state = T_FIN_1;
		break;
	case T_FIN_1:
		test_verify_flags(th, RST);
		zassert_true(get_rel_seq(th) == 2,
			     "%s:%d unexpected sequence number in original FIN, got %d",
			     __func__, __LINE__, get_rel_seq(th));
		test_sem_give();
		return;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	ret = net_recv_data(net_iface, reply);
	if (ret < 0) {
		goto fail;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

/* Test case scenario IPv4
 *   expect SYN,
 *   send SYN ACK,
 *   expect ACK,
 *   expect Data,
 *   send ACK,
 *   expect FIN,
 *   resend Data,
 *   expect FIN,
 *   send ACK,
 *   send FIN,
 *   expect ACK
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_client_data_during_fin_1_ipv4)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_DATA_DURING_FIN_1_IPV4;
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

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);

	/* Peer will release the semaphore after it receives
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
		ack = ntohl(th->th_seq) + 1U;
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
		ack = ack + 1U;
		t_state = T_CLOSING;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_CLOSING:
		test_verify_flags(th, ACK);
		t_state = T_FIN_ACK;
		seq++;
		reply = prepare_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	ret = net_recv_data(net_iface, reply);
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
ZTEST(net_tcp, test_client_closing_ipv6)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_CLOSING_IPV6;
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

	/* Peer will release the semaphore after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);

	/* Peer will release the semaphore after it receives
	 * proper ACK to FIN | ACK
	 */
	test_sem_take(K_MSEC(300), __LINE__);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

/* In this test we check that things work properly if we do not receive
 * the final ACK that leads to TIME_WAIT state.
 */
static void handle_client_closing_failure_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply;
	int ret;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
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
		t_state = T_FIN_1;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_1:
		/* The FIN retransmit timer should not yet be expired */
		test_verify_flags(th, ACK);
		zassert_equal(ntohl(th->th_seq), ack + 1, "FIN seq was not correct!");
		zassert_equal(ntohl(th->th_ack), seq + 1, "FIN ack was not correct!");
		t_state = T_CLOSING;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_CLOSING:
		/* The FIN retransmit timer could have expired */
		if (th_flags(th) == (FIN | ACK)) {
			zassert_equal(ntohl(th->th_seq), ack, "FIN seq was not correct!");
			zassert_equal(ntohl(th->th_ack), seq + 1, "FIN ack was not correct!");
		} else if (th_flags(th) == ACK) {
			zassert_equal(ntohl(th->th_seq), ack + 1, "FIN seq was not correct!");
			zassert_equal(ntohl(th->th_ack), seq + 1, "FIN ack was not correct!");
		} else {
			zassert_true(false, "Wrong flag received: 0x%x", th_flags(th));
		}

		/* Simulate the case where we do not receive final ACK */
		reply = NULL;
		break;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	if (reply != NULL) {
		ret = net_recv_data(net_iface, reply);
		if (ret < 0) {
			goto fail;
		}
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
 *   expect ACK but fail to receive it
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_client_closing_failure_ipv6)
{
	struct net_context *ctx;
	uint8_t data = 0x41; /* "A" */
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_CLOSING_FAILURE_IPV6;
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

	/* Peer will release the semaphore after it receives
	 * proper ACK to SYN | ACK
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	ret = net_context_send(ctx, &data, 1, NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to send data to peer");
	}

	/* Peer will release the semaphore after it sends ACK for data */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);

	/* The lock is not released as we do not receive final ACK.
	 */
	test_sem_take_failure(K_MSEC(400), __LINE__);
}

static struct net_context *create_server_socket(uint32_t my_seq,
						uint32_t my_ack)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_SERVER_IPV6;
	seq = my_seq;
	ack = my_ack;

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

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
	k_work_reschedule(&test_server, K_NO_WAIT);

	ret = net_context_accept(ctx, test_tcp_accept_cb, K_FOREVER, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* test_tcp_accept_cb will release the semaphore after successful
	 * connection.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	return ctx;
}

static void check_rst_fail(uint32_t seq_value)
{
	struct net_pkt *reply;
	int rsterr_before, rsterr_after;
	int ret;

	rsterr_before = GET_STAT(net_iface, tcp.rsterr);

	/* Invalid seq in the RST packet */
	seq = seq_value;

	reply = prepare_rst_packet(AF_INET6, htons(MY_PORT), htons(PEER_PORT));

	ret = net_recv_data(net_iface, reply);
	zassert_true(ret == 0, "recv data failed (%d)", ret);

	/* Let the receiving thread run */
	k_msleep(50);

	rsterr_after = GET_STAT(net_iface, tcp.rsterr);

	zassert_equal(rsterr_before + 1, rsterr_after,
		      "RST packet not skipped (before %d, after %d)",
		      rsterr_before, rsterr_after);
}

static void check_rst_succeed(struct net_context *ctx,
			      uint32_t seq_value)
{
	struct net_pkt *reply;
	int rsterr_before, rsterr_after;
	int ret;

	/* Make sure that various other corner cases work */
	if (ctx == NULL) {
		ctx = create_server_socket(0, 0);
	}

	/* Another valid seq in the RST packet */
	seq = ack + seq_value;

	reply = prepare_rst_packet(AF_INET6, htons(MY_PORT), htons(PEER_PORT));

	rsterr_before = GET_STAT(net_iface, tcp.rsterr);

	ret = net_recv_data(net_iface, reply);
	zassert_true(ret == 0, "recv data failed (%d)", ret);

	/* Let the receiving thread run */
	k_msleep(50);

	rsterr_after = GET_STAT(net_iface, tcp.rsterr);

	zassert_equal(rsterr_before, rsterr_after,
		      "RST packet skipped (before %d, after %d)",
		      rsterr_before, rsterr_after);

	net_context_put(ctx);
	net_context_put(accepted_ctx);

	/* Let other threads run (so the TCP context is actually freed) */
	k_msleep(10);
}

ZTEST(net_tcp, test_client_invalid_rst)
{
	struct net_context *ctx;
	struct tcp *conn;
	uint16_t wnd;

	ctx = create_server_socket(0, 0);

	conn = ctx->tcp;
	wnd = conn->recv_win;

	/* Failure cases, the RST packets should be dropped */
	check_rst_fail(ack - 1);
	check_rst_fail(ack + wnd);

	/* Then send a valid seq in the RST packet */
	check_rst_succeed(ctx, wnd - 1);
	check_rst_succeed(NULL, 0);
	check_rst_succeed(NULL, 1);

	/* net_context is released within check_rst_succeed() */
}

#define MAX_DATA 100
#define OUT_OF_ORDER_SEQ_INIT -15
static uint32_t expected_ack;
static struct net_context *ooo_ctx;

static void handle_server_recv_out_of_order(struct net_pkt *pkt)
{
	struct tcphdr th;
	int ret;

	ret = read_tcp_header(pkt, &th);
	if (ret < 0) {
		goto fail;
	}

	/* Verify that we received all the queued data */
	zassert_equal(expected_ack, ntohl(th.th_ack),
		      "Not all pending data received. "
		      "Expected ACK %u but got %u",
		      expected_ack, ntohl(th.th_ack));

	test_sem_give();

	return;

fail:
	zassert_true(false, "%s failed", __func__);
	net_pkt_unref(pkt);
}

struct out_of_order_check_struct {
	int seq_offset;
	int length;
	int ack_offset;
	int delay_ms;
};

static struct out_of_order_check_struct out_of_order_check_list[] = {
	{ 30, 10, 0, 0}, /* First packet will be out-of-order */
	{ 20, 12, 0, 0},
	{ 10,  9, 0, 0}, /* Section with a gap */
	{ 0,  10, 10, 0},
	{ 10, 10, 40, 0}, /* First sequence complete */
	{ 32,  6, 40, 0}, /* Invalid seqnum (old) */
	{ 30, 16, 46, 0}, /* Partial data valid */
	{ 50,  6, 46, 0},
	{ 50,  3, 46, 0}, /* Discardable packet */
	{ 55,  5, 46, 0},
	{ 30, 20, 60, 0}, /* Partial data valid with pending data merge */
	{ 61,  2, 60, 0},
	{ 60,  5, 65, 0}, /* Over lapped incoming packet */
	{ 66,  4, 65, 0},
	{ 65,  5, 70, 0}, /* Over lapped incoming packet, at boundary */
	{ 72,  2, 70, 0},
	{ 71,  4, 70, 0},
	{ 70,  1, 75, 0}, /* Over lapped in out of order processing */
	{ 78,  2, 75, 0},
	{ 77,  3, 75, 0},
	{ 75,  2, 80, 0}, /* Over lapped in out of order processing, at boundary */
};

static void checklist_based_out_of_order_test(struct out_of_order_check_struct *check_list,
					      int num_checks, int sequence_base)
{
	const uint8_t *data = lorem_ipsum + 10;
	struct net_pkt *pkt;
	int ret, i;
	struct out_of_order_check_struct *check_ptr;

	for (i = 0; i < num_checks; i++) {
		check_ptr = &check_list[i];

		seq = sequence_base + check_ptr->seq_offset;
		pkt = prepare_data_packet(AF_INET6, htons(MY_PORT), htons(PEER_PORT),
				  &data[check_ptr->seq_offset], check_ptr->length);
		zassert_not_null(pkt, "Cannot create pkt");

		/* Initial ack for the last correctly received byte = SYN flag */
		expected_ack = sequence_base + check_ptr->ack_offset;

		ret = net_recv_data(net_iface, pkt);
		zassert_true(ret == 0, "recv data failed (%d)", ret);

		/* Let the IP stack to process the packet properly */
		k_yield();

		/* Peer will release the semaphore after it sends proper ACK to the
		 * queued data.
		 */
		test_sem_take(K_MSEC(1000), __LINE__);

		/* Optionally wait between transfers */
		if (check_ptr->delay_ms) {
			k_sleep(K_MSEC(check_ptr->delay_ms));
		}
	}
}

static void test_server_recv_out_of_order_data(void)
{
	/* Only run the tests if queueing is enabled */
	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT == 0) {
		return;
	}

	k_sem_reset(&test_sem);

	/* Start the sequence numbering so that we will wrap it (just for
	 * testing purposes)
	 */
	ooo_ctx = create_server_socket(OUT_OF_ORDER_SEQ_INIT, -15U);

	/* This will force the packet to be routed to our checker func
	 * handle_server_recv_out_of_order()
	 */
	test_case_no = TEST_SERVER_RECV_OUT_OF_ORDER_DATA;

	/* Run over the checklist to complete the test */
	checklist_based_out_of_order_test(out_of_order_check_list,
					  ARRAY_SIZE(out_of_order_check_list),
					  OUT_OF_ORDER_SEQ_INIT + 1);
}

struct out_of_order_check_struct reorder_timeout_list[] = {
	/* Wait more then the receive queue timeout */
	{ 90, 10, 80, CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT * 2},
	/* First message has been timeout, so only this is acknowledged */
	{ 80, 10, 90, 0},
};


/* This test expects that the system is in correct state after a call to
 * test_server_recv_out_of_order_data(), so this test must be run after that
 * test.
 */
static void test_server_timeout_out_of_order_data(void)
{
	struct net_pkt *rst;
	int ret;

	if (CONFIG_NET_TCP_RECV_QUEUE_TIMEOUT == 0) {
		return;
	}

	k_sem_reset(&test_sem);

	checklist_based_out_of_order_test(reorder_timeout_list,
					  ARRAY_SIZE(reorder_timeout_list),
					  OUT_OF_ORDER_SEQ_INIT + 1);

	/* Just send a RST packet to abort the underlying connection, so that
	 * the testcase does not need to implement full TCP closing handshake.
	 */
	seq = expected_ack + 1;
	rst = prepare_rst_packet(AF_INET6, htons(MY_PORT), htons(PEER_PORT));

	ret = net_recv_data(net_iface, rst);
	zassert_true(ret == 0, "recv data failed (%d)", ret);

	/* Let the receiving thread run */
	k_msleep(50);

	net_context_put(ooo_ctx);
	net_context_put(accepted_ctx);
}

ZTEST(net_tcp, test_server_out_of_order_data)
{
	test_server_recv_out_of_order_data();
	test_server_timeout_out_of_order_data();
}

static void handle_server_rst_on_closed_port(sa_family_t af, struct tcphdr *th)
{
	switch (t_state) {
	case T_SYN_ACK:
		/* Port was closed so expect RST instead of SYN */
		test_verify_flags(th, RST | ACK);
		zassert_equal(ntohl(th->th_seq), 0, "Invalid SEQ value");
		zassert_equal(ntohl(th->th_ack), seq + 1, "Invalid ACK value");
		t_state = T_CLOSING;
		test_sem_give();
		break;
	default:
		return;
	}
}

/* Test case scenario
 *   Send SYN
 *   expect RST ACK (closed port)
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_server_rst_on_closed_port)
{
	t_state = T_SYN;
	test_case_no = TEST_SERVER_RST_ON_CLOSED_PORT;
	seq = ack = 0;

	k_sem_reset(&test_sem);

	/* Trigger the peer to send SYN */
	k_work_reschedule(&test_server, K_NO_WAIT);

	/* Peer will release the semaphore after it receives RST */
	test_sem_take(K_MSEC(100), __LINE__);
}

static void handle_server_rst_on_listening_port(sa_family_t af, struct tcphdr *th)
{
	switch (t_state) {
	case T_DATA_ACK:
		/* No active connection so expect RST instead of ACK */
		test_verify_flags(th, RST);
		zassert_equal(ntohl(th->th_seq), ack, "Invalid SEQ value");
		t_state = T_CLOSING;
		test_sem_give();
		break;
	default:
		return;
	}
}

static void dummy_accept_cb(struct net_context *ctx, struct sockaddr *addr,
			    socklen_t addrlen, int status, void *user_data)
{
	/* Should not ever be called. */
	zassert_unreachable("Should not have called dummy accept cb");
}

/* Test case scenario
 *   Open listening port
 *   Send DATA packet to the listening port
 *   expect RST (no matching connection)
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_server_rst_on_listening_port_no_active_connection)
{
	struct net_context *ctx;
	int ret;

	t_state = T_DATA;
	test_case_no = TEST_SERVER_RST_ON_LISTENING_PORT_NO_ACTIVE_CONNECTION;
	seq = ack = 200;

	k_sem_reset(&test_sem);

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_bind(ctx, (struct sockaddr *)&my_addr_s,
			       sizeof(struct sockaddr_in));
	if (ret < 0) {
		zassert_true(false, "Failed to bind net_context");
	}

	ret = net_context_listen(ctx, 1);
	if (ret < 0) {
		zassert_true(false, "Failed to listen on net_context");
	}

	ret = net_context_accept(ctx, dummy_accept_cb, K_NO_WAIT, NULL);
	if (ret < 0) {
		zassert_true(false, "Failed to set accept on net_context");
	}

	/* Trigger the peer to send data to the listening port w/o active connection */
	k_work_reschedule(&test_server, K_NO_WAIT);

	/* Peer will release the semaphore after it receives RST */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);
}

/* Test case scenario
 *   Expect SYN
 *   Send ACK (wrong ACK number),
 *   expect RST,
 *   expect SYN (retransmission),
 *   send SYN ACK,
 *   expect ACK (handshake success),
 *   expect FIN,
 *   send FIN ACK,
 *   expect ACK.
 *   any failures cause test case to fail.
 */
static void handle_syn_invalid_ack(sa_family_t af, struct tcphdr *th)
{
	static bool invalid_ack = true;
	struct net_pkt *reply;
	int ret;


	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		if (invalid_ack) {
			ack = ntohl(th->th_seq) + 1000U;
			reply = prepare_ack_packet(af, htons(MY_PORT),
						   th->th_sport);
			/* Expect RST now. */
			t_state = T_RST;
			invalid_ack = false;
		} else {
			ack = ntohl(th->th_seq) + 1U;
			reply = prepare_syn_ack_packet(af, htons(MY_PORT),
						       th->th_sport);
			/* Proceed with the handshake on second attempt. */
			t_state = T_SYN_ACK;
		}

		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		/* connection is successful */
		t_state = T_FIN;
		return;
	case T_FIN:
		test_verify_flags(th, FIN | ACK);
		seq++;
		ack++;
		t_state = T_FIN_ACK;
		reply = prepare_fin_ack_packet(af, htons(MY_PORT),
					       th->th_sport);
		break;
	case T_FIN_ACK:
		test_verify_flags(th, ACK);
		test_sem_give();
		return;
	case T_RST:
		test_verify_flags(th, RST);
		zassert_equal(ntohl(th->th_seq), ack, "Invalid SEQ value");

		/* Wait for SYN retransmission. */
		t_state = T_SYN;
		return;
	default:
		return;
	}

	ret = net_recv_data(net_iface, reply);
	if (ret < 0) {
		goto fail;
	}

	return;
fail:
	zassert_true(false, "%s failed", __func__);
}

ZTEST(net_tcp, test_client_rst_on_unexpected_ack_on_syn)
{
	struct net_context *ctx;
	int ret;

	t_state = T_SYN;
	test_case_no = TEST_CLIENT_RST_ON_UNEXPECTED_ACK_ON_SYN;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	if (ret < 0) {
		zassert_true(false, "Failed to get net_context");
	}

	net_context_ref(ctx);

	ret = net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
				  sizeof(struct sockaddr_in),
				  NULL, K_MSEC(1000), NULL);

	zassert_equal(ret, 0, "Connect failed");
	zassert_equal(net_context_get_state(ctx), NET_CONTEXT_CONNECTED,
			  "Context should be connected");

	/* Just wait for the connection teardown. */
	net_context_put(ctx);

	/* Peer will release the semaphore after it receives final ACK */
	test_sem_take(K_MSEC(100), __LINE__);
}

#define TEST_FIN_DATA "test_data"

static enum fin_data_variant {
	FIN_DATA_FIN_ACK,
	FIN_DATA_FIN_ACK_PSH,
} test_fin_data_variant;

static struct k_work_delayable test_fin_data_work;

/* In this test we check that FIN packet containing data is handled correctly
 * by the TCP stack.
 */
static void handle_client_fin_ack_with_data_test(sa_family_t af, struct tcphdr *th)
{
	static uint16_t peer_port;
	struct net_pkt *reply;
	uint8_t flags = 0;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		device_initial_seq = ntohl(th->th_seq);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
		peer_port = th->th_sport;
		reply = prepare_syn_ack_packet(af, htons(MY_PORT), peer_port);
		seq++;
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		t_state = T_DATA;

		/* FIN packet with DATA needs to be rescheduled for later - if
		 * we send it here, the net_context_recv() won't have a chance
		 * to execute, hence no callback will be registered and data
		 * will be dropped.
		 */
		k_work_reschedule(&test_fin_data_work, K_MSEC(1));
		return;
	case T_DATA:
		switch (test_fin_data_variant) {
		case FIN_DATA_FIN_ACK:
			flags = FIN | ACK;
			t_state = T_FIN_ACK;
			break;
		case FIN_DATA_FIN_ACK_PSH:
			flags = FIN | ACK | PSH;
			t_state = T_FIN_ACK;
			break;
		}

		reply = tester_prepare_tcp_pkt(af, htons(MY_PORT), peer_port,
					       flags, TEST_FIN_DATA,
					       strlen(TEST_FIN_DATA));
		seq += strlen(TEST_FIN_DATA) + 1;

		break;
	case T_FIN:
		test_verify_flags(th, ACK);
		zassert_equal(get_rel_seq(th), 1, "Unexpected SEQ number in T_FIN, got %d",
			      get_rel_seq(th));
		zassert_equal(ntohl(th->th_ack), seq, "Unexpected ACK in T_FIN, got %d",
			      ntohl(th->th_ack));

		t_state = T_CLOSING;
		return;
	case T_FIN_ACK:
		test_verify_flags(th, FIN | ACK);
		zassert_equal(get_rel_seq(th), 1, "Unexpected SEQ number in T_FIN_ACK, got %d",
			      get_rel_seq(th));
		zassert_equal(ntohl(th->th_ack), seq, "Unexpected ACK in T_FIN_ACK, got %d",
			      ntohl(th->th_ack));

		ack++;
		reply = prepare_ack_packet(af, htons(MY_PORT), peer_port);
		t_state = T_SYN;
		break;

	case T_CLOSING:
		test_verify_flags(th, FIN | ACK);
		zassert_equal(get_rel_seq(th), 1, "Unexpected SEQ number in T_CLOSING, got %d",
			      get_rel_seq(th));

		ack++;
		reply = prepare_ack_packet(af, htons(MY_PORT), peer_port);
		t_state = T_SYN;
		break;

	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	zassert_ok(net_recv_data(net_iface, reply), "%s failed", __func__);
}


static void test_fin_data_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	handle_client_fin_ack_with_data_test(AF_INET, NULL);
}

static void test_fin_ack_data_recv_cb(struct net_context *context,
				      struct net_pkt *pkt,
				      union net_ip_header *ip_hdr,
				      union net_proto_header *proto_hdr,
				      int status,
				      void *user_data)
{
	if (status) {
		zassert_true(false, "failed to recv the data");
	}

	if (pkt) {
		uint8_t buf[sizeof(TEST_FIN_DATA)] = { 0 };
		int data_len = net_pkt_remaining_data(pkt);

		zassert_equal(data_len, strlen(TEST_FIN_DATA),
			      "Invalid packet length, %d", data_len);
		zassert_ok(net_pkt_read(pkt, buf, data_len));
		zassert_mem_equal(buf, TEST_FIN_DATA, data_len);

		net_pkt_unref(pkt);
	}

	test_sem_give();
}

/* Test case scenario IPv4
 *   expect SYN,
 *   send SYN ACK,
 *   expect ACK,
 *   send FIN/FIN,ACK/FIN,ACK,PSH with Data,
 *   expect FIN/FIN,ACK/ACK,
 *   send ACK
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_client_fin_ack_with_data)
{
	struct net_context *ctx;

	test_case_no = TEST_CLIENT_FIN_ACK_WITH_DATA;

	k_work_init_delayable(&test_fin_data_work, test_fin_data_handler);

	for (enum fin_data_variant variant = FIN_DATA_FIN_ACK;
	     variant <= FIN_DATA_FIN_ACK_PSH; variant++) {
		test_fin_data_variant = variant;
		t_state = T_SYN;
		seq = ack = 0;

		zassert_ok(net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx),
			   "Failed to get net_context");

		net_context_ref(ctx);

		zassert_ok(net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
					       sizeof(struct sockaddr_in), NULL,
					       K_MSEC(1000), NULL),
			   "Failed to connect to peer");
		zassert_ok(net_context_recv(ctx, test_fin_ack_data_recv_cb,
					    K_NO_WAIT, NULL),
			   "Failed to recv data from peer");

		/* Take sem twice, one for data packet, second for conn close
		 * (NULL net_pkt).
		 */
		test_sem_take(K_MSEC(100), __LINE__);
		test_sem_take(K_MSEC(100), __LINE__);

		net_context_put(ctx);

		/* Connection is in TIME_WAIT state, context will be released
		 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
		 */
		k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
	}
}

static struct k_work_delayable test_seq_val_work;

static void handle_client_seq_validation_test(sa_family_t af, struct tcphdr *th)
{
	static uint16_t peer_port;
	struct net_pkt *reply = NULL;
	static uint8_t round;
	static uint16_t wnd;
	static int rsterr;
	const uint8_t *data = lorem_ipsum;
	size_t len = 500;

	switch (t_state) {
	case T_SYN:
		test_verify_flags(th, SYN);
		device_initial_seq = ntohl(th->th_seq);
		seq = 0U;
		ack = ntohl(th->th_seq) + 1U;
		peer_port = th->th_sport;
		reply = prepare_syn_ack_packet(af, htons(MY_PORT), peer_port);
		seq++;
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, ACK);
		t_state = T_DATA;
		wnd = ntohs(th_win(th));
		round = 0;

		k_work_reschedule(&test_seq_val_work, K_MSEC(1));
		return;
	case T_DATA:
		/* Test seqnum on case 'recv_win > 0'. Some scenarios
		 * have been covered by out-of-order test. Here we only
		 * cover the remaining ones.
		 */

		switch (round) {
		case 0: {
			uint32_t t_seq = seq;

			/* len == 0, with seqnum bigger than the scope. */
			seq = 66000;
			reply = prepare_ack_packet(af, htons(MY_PORT), peer_port);
			seq = t_seq;
			round++;

			/* The ACK with an invalid seqnum would be silently dropped */
			k_work_reschedule(&test_seq_val_work, K_MSEC(1));
		}
		break;
		case 1: {
			uint32_t t_seq = seq;

			/* len == 0, with seqnum bigger than the scope. */
			seq = 66000;
			reply = prepare_data_packet(af, htons(MY_PORT), peer_port, data, 10);
			seq = t_seq;
			round++;
		}
		break;
		case 2: {
			uint32_t t_seq = seq;

			test_verify_flags(th, ACK);
			zassert_equal(get_rel_seq(th), 1,
				      "Unexpected SEQ number in T_DATA_ACK round %d, got %d",
				      round, get_rel_seq(th));
			zassert_equal(ntohl(th->th_ack), seq,
				      "Unexpected ACK in T_DATA_ACK round %d, got %d",
				      round, ntohl(th->th_ack));

			seq = wnd + 10;
			reply = prepare_data_packet(af, htons(MY_PORT), peer_port, data, 1);
			seq = t_seq;
			round++;
		}
		break;
		case 3:
			test_verify_flags(th, ACK);
			zassert_equal(get_rel_seq(th), 1,
				      "Unexpected SEQ number in T_DATA round %d, got %d",
				      round, get_rel_seq(th));
			zassert_equal(ntohl(th->th_ack), seq,
				      "Unexpected ACK in T_DATA round %d, got %d",
				      round, ntohl(th->th_ack));
			zassert_equal(ntohs(th_win(th)), wnd,
				      "Unexpected recv_win in T_DATA round %d, got %d",
				      round, ntohs(th_win(th)));

			len = len > wnd ? wnd : len;
			reply = prepare_data_packet(af, htons(MY_PORT), peer_port, data, len);
			seq += len;
			wnd -= len;
			if (wnd == 0) {
				t_state = T_DATA_ACK;
				round++;
			}
			break;
		default:
			zassert_true(false, "T_DATA, %d round should never happen", round);
			break;
		}
		break;
	case T_DATA_ACK:
		if (th) {
			test_verify_flags(th, ACK);
			zassert_equal(get_rel_seq(th), 1,
				      "Unexpected SEQ number in T_DATA_ACK round %d, got %d",
				      round, get_rel_seq(th));
			zassert_equal(ntohl(th->th_ack), seq,
				      "Unexpected ACK in T_DATA_ACK round %d, got %d",
				      round, ntohl(th->th_ack));
			zassert_equal(ntohs(th_win(th)), wnd,
				      "Unexpected recv_win in T_DATA_ACK round %d, got %d",
				      round, ntohs(th_win(th)));
		}

		switch (round) {
		case 4:
			zassert_true(wnd == 0, "Peer's recv_win is not zero");
			/* test data in 0 window, expect duplicate ACK */
			reply = prepare_data_packet(af, htons(MY_PORT), peer_port, data, 1);
			round++;
			break;
		case 5:
			/* test RST with different seqnum, expect no reply */
			seq--;
			reply = prepare_rst_ack_packet(af, htons(MY_PORT), peer_port);
			seq++;
			round++;

			rsterr = GET_STAT(net_iface, tcp.rsterr);

			/* reschedule thread due to no reply */
			k_work_reschedule(&test_seq_val_work, K_MSEC(1));
			break;
		case 6: {
			int rsterr_after = GET_STAT(net_iface, tcp.rsterr);

			zassert_equal(rsterr + 1, rsterr_after,
				      "RST packet not skipped (before %d, after %d)",
				      rsterr, rsterr_after);

			/* test RST with correct seqnum, expect ERSET in recv_cb. */
			reply = prepare_rst_ack_packet(af, htons(MY_PORT), peer_port);

		}
		break;
		default:
			zassert_true(false, "T_DATA_ACK, %d round should never happen", round);
			break;
		}
		break;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	zassert_ok(net_recv_data(net_iface, reply), "%s failed", __func__);
}

static void test_seq_val_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	handle_client_seq_validation_test(AF_INET, NULL);
}

static void test_seq_val_recv_cb(struct net_context *context,
				 struct net_pkt *pkt,
				 union net_ip_header *ip_hdr,
				 union net_proto_header *proto_hdr,
				 int status,
				 void *user_data)
{
	if (status) {
		zassert_equal(status, -ECONNRESET, "failed to recv data");
		test_sem_give();
	}

	if (pkt) {
		static int i;

		net_pkt_unref(pkt);

		if (i == 0) {
			test_sem_give();
		}
		i++;
	}
}

/* Test case scenario IPv4
 *   expect SYN,
 *   send SYN ACK,
 *   expect ACK,
 *   send Data with seq valid,
 *   expect ACK,
 *   send Data with seq invalid,
 *   expect same ACK,
 *   send Data to make server 0 recv_wnd,
 *   expect ACK with 0 window,
 *   send Data, valid RST,
 *   expect duplicate ACK on Data, check RST stat
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_client_seq_validation)
{
	struct net_context *ctx;

	test_case_no = TEST_CLIENT_SEQ_VALIDATION;

	k_work_init_delayable(&test_seq_val_work, test_seq_val_handler);

	t_state = T_SYN;
	seq = ack = 0;

	zassert_ok(net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx),
		   "Failed to get net_context");

	net_context_ref(ctx);

	zassert_ok(net_context_connect(ctx, (struct sockaddr *)&peer_addr_s,
				       sizeof(struct sockaddr_in), NULL,
				       K_MSEC(1000), NULL),
		   "Failed to connect to peer");
	zassert_ok(net_context_recv(ctx, test_seq_val_recv_cb,
				    K_NO_WAIT, NULL),
		   "Failed to recv data from peer");

	/* Take sem twice, one for data packet, second for conn reset. */
	test_sem_take(K_MSEC(300), __LINE__);
	test_sem_take(K_MSEC(300), __LINE__);

	net_context_put(ctx);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

struct ack_check_struct {
	uint32_t s_data_len;
	uint32_t c_ack;
	uint32_t c_exp_seq;
};

/* The Congestion Window is by default enabled to be small (e.g. 67B),
 * so DONT set the s_data_len bigger than it or the expected_seq will
 * fail the check.
 */
static struct ack_check_struct ack_check_list[] = {
	{10, 20, 10},
	{20, 9, 30},
	{30, -2000, 60},
	{50, 111, 110},
};

static int ack_check_index;
static uint32_t expected_seq;
static uint32_t svr_seq_base;
static int ackerr;

static void handle_server_ack_validation_test(struct net_pkt *pkt)
{
	struct tcphdr th;
	struct net_pkt *reply;
	int ret;

	ret = read_tcp_header(pkt, &th);
	if (ret < 0) {
		goto fail;
	}

	if (FL(&th.th_flags, &, PSH)) {
		/* If server data packet, then return with c_ack in the ack_check_list[] */
		uint32_t old_ack = ack;

		ack = svr_seq_base + ack_check_list[ack_check_index].c_ack;
		reply = prepare_ack_packet(net_pkt_family(pkt), htons(MY_PORT), htons(PEER_PORT));
		zassert_not_null(reply, "Cannot create pkt");
		ack = old_ack;

		expected_seq = svr_seq_base + ack_check_list[ack_check_index].c_exp_seq;
		ackerr = GET_STAT(net_iface, tcp.ackerr);

		ret = net_recv_data(net_iface, reply);
		zassert_true(ret == 0, "recv data failed (%d)", ret);

		/* Let the IP stack to process the packet properly */
		k_yield();
	} else {
		int ackerr_after;

		/* For all non-data packets, we assume it is ACK-only because we use RST to
		 * close the TCP connection later.
		 */
		test_verify_flags(&th, ACK);
		/* Verify that we received the ACK-only packet with the expected seqnum */
		zassert_equal(expected_seq, ntohl(th.th_seq),
			      "Wrong seqnum received from server. "
			      "Expected SEQ %u but got %u",
			      expected_seq, ntohl(th.th_seq));

		ackerr_after = GET_STAT(net_iface, tcp.ackerr);
		zassert_equal(ackerr + 1, ackerr_after,
			      "Wrong ACK number not detected (before %d, after %d)", ackerr,
			      ackerr_after);

		/* send valid ACK to server to avoid retransmission */
		ack += ack_check_list[ack_check_index].s_data_len;
		reply = prepare_ack_packet(net_pkt_family(pkt), htons(MY_PORT), htons(PEER_PORT));
		zassert_not_null(reply, "Cannot create pkt");

		ret = net_recv_data(net_iface, reply);
		zassert_true(ret == 0, "recv data failed (%d)", ret);
		k_yield();

		test_sem_give();
	}

	return;

fail:
	zassert_true(false, "%s failed", __func__);
	net_pkt_unref(pkt);
}

static void checklist_based_ack_val_test(struct ack_check_struct *list, int num)
{
	const uint8_t *data = lorem_ipsum + 10;
	int ret;
	struct ack_check_struct *ptr;

	/* Server sends data and client ACKs data */
	for (ack_check_index = 0; ack_check_index < num; ack_check_index++) {
		ptr = &list[ack_check_index];
		ret = net_context_send(accepted_ctx, data, ptr->s_data_len, NULL, K_NO_WAIT, NULL);
		if (ret < 0) {
			zassert_true(false, "Failed to send data to peer %d", ret);
		}

		/* Peer will release the semaphore after the valid ACK for data */
		test_sem_take(K_MSEC(2000), __LINE__);
	}
}

ZTEST(net_tcp, test_server_ack_validation)
{
	struct net_pkt *rst;
	struct net_context *ctx;
	int ret;

	k_sem_reset(&test_sem);

	ctx = create_server_socket(0, 0);
	svr_seq_base = ack;

	/* This will force the packet to be routed to our checker func
	 * handle_server_ack_validation_test()
	 */
	test_case_no = TEST_SERVER_ACK_VALIDATION;

	/* Run over the checklist to complete the test */
	checklist_based_ack_val_test(ack_check_list, ARRAY_SIZE(ack_check_list));

	/* Just send a RST packet to abort the underlying connection, so that
	 * the testcase does not need to implement full TCP closing handshake.
	 */
	rst = tester_prepare_tcp_pkt(AF_INET6, htons(MY_PORT), htons(PEER_PORT), RST, NULL, 0);
	zassert_not_null(rst, "Cannot create pkt");

	ret = net_recv_data(net_iface, rst);
	zassert_true(ret == 0, "recv data failed (%d)", ret);

	/* Let the receiving thread run */
	k_msleep(50);

	net_context_put(ctx);
	net_context_put(accepted_ctx);
}

#define TEST_FIN_ACK_AFTER_DATA_REQ "request"
#define TEST_FIN_ACK_AFTER_DATA_RSP "test data response"

/* In this test we check that FIN,ACK packet acknowledging latest data is
 * handled correctly by the TCP stack.
 */
static void handle_server_fin_ack_after_data_test(sa_family_t af, struct tcphdr *th)
{
	struct net_pkt *reply = NULL;

	zassert_false(th == NULL && t_state != T_SYN,
		     "NULL pkt only expected in T_SYN state");

	switch (t_state) {
	case T_SYN:
		reply = prepare_syn_packet(af, htons(MY_PORT), htons(PEER_PORT));
		seq++;
		t_state = T_SYN_ACK;
		break;
	case T_SYN_ACK:
		test_verify_flags(th, SYN | ACK);
		zassert_equal(ntohl(th->th_ack), seq,
			      "Unexpected ACK in T_SYN_ACK, got %d, expected %d",
			      ntohl(th->th_ack), seq);
		device_initial_seq = ntohl(th->th_seq);
		ack = ntohl(th->th_seq) + 1U;
		t_state = T_DATA_ACK;

		/* Dummy "request" packet */
		reply = prepare_data_packet(af, htons(MY_PORT), htons(PEER_PORT),
					    TEST_FIN_ACK_AFTER_DATA_REQ,
					    sizeof(TEST_FIN_ACK_AFTER_DATA_REQ) - 1);
		seq += sizeof(TEST_FIN_ACK_AFTER_DATA_REQ) - 1;
		break;
	case T_DATA_ACK:
		test_verify_flags(th, ACK);
		t_state = T_DATA;
		zassert_equal(ntohl(th->th_seq), ack,
			      "Unexpected SEQ in T_DATA_ACK, got %d, expected %d",
			      get_rel_seq(th), ack);
		zassert_equal(ntohl(th->th_ack), seq,
			      "Unexpected ACK in T_DATA_ACK, got %d, expected %d",
			      ntohl(th->th_ack), seq);
		break;
	case T_DATA:
		test_verify_flags(th, PSH | ACK);
		zassert_equal(ntohl(th->th_seq), ack,
			      "Unexpected SEQ in T_DATA, got %d, expected %d",
			      get_rel_seq(th), ack);
		zassert_equal(ntohl(th->th_ack), seq,
			      "Unexpected ACK in T_DATA, got %d, expected %d",
			      ntohl(th->th_ack), seq);
		ack += sizeof(TEST_FIN_ACK_AFTER_DATA_RSP) - 1;
		t_state = T_FIN_ACK;

		reply = prepare_fin_ack_packet(af, htons(MY_PORT), htons(PEER_PORT));
		seq++;
		break;
	case T_FIN_ACK:
		test_verify_flags(th, FIN | ACK);
		zassert_equal(ntohl(th->th_seq), ack,
			      "Unexpected SEQ in T_FIN_ACK, got %d, expected %d",
			      get_rel_seq(th), ack);
		zassert_equal(ntohl(th->th_ack), seq,
			      "Unexpected ACK in T_FIN_ACK, got %d, expected %d",
			      ntohl(th->th_ack), seq);

		ack++;
		t_state = T_CLOSING;

		reply = prepare_ack_packet(af, htons(MY_PORT), htons(PEER_PORT));
		seq++;
		break;
	case T_CLOSING:
		zassert_true(false, "Should not receive anything after final ACK");
		break;
	default:
		zassert_true(false, "%s unexpected state", __func__);
		return;
	}

	if (reply != NULL) {
		zassert_ok(net_recv_data(net_iface, reply), "%s failed", __func__);
	}
}

/* Receive callback to be installed in the accept handler */
static void test_fin_ack_after_data_recv_cb(struct net_context *context,
					    struct net_pkt *pkt,
					    union net_ip_header *ip_hdr,
					    union net_proto_header *proto_hdr,
					    int status,
					    void *user_data)
{
	zassert_ok(status, "failed to recv the data");

	if (pkt != NULL) {
		uint8_t buf[sizeof(TEST_FIN_ACK_AFTER_DATA_REQ)] = { 0 };
		int data_len = net_pkt_remaining_data(pkt);

		zassert_equal(data_len, sizeof(TEST_FIN_ACK_AFTER_DATA_REQ) - 1,
			      "Invalid packet length, %d", data_len);
		zassert_ok(net_pkt_read(pkt, buf, data_len));
		zassert_mem_equal(buf, TEST_FIN_ACK_AFTER_DATA_REQ, data_len);

		net_pkt_unref(pkt);
	}

	test_sem_give();
}

static void test_fin_ack_after_data_accept_cb(struct net_context *ctx,
					      struct sockaddr *addr,
					      socklen_t addrlen,
					      int status,
					      void *user_data)
{
	int ret;

	zassert_ok(status, "failed to accept the conn");

	/* set callback on newly created context */
	accepted_ctx = ctx;
	ret = net_context_recv(ctx, test_fin_ack_after_data_recv_cb,
			       K_NO_WAIT, NULL);
	zassert_ok(ret, "Failed to recv data from peer");

	/* Ref the context on the app behalf. */
	net_context_ref(ctx);
}

/* Verify that the TCP stack replies with a valid FIN,ACK after the peer
 * acknowledges the latest data in the FIN packet.
 * Test case scenario IPv4
 *   send SYN,
 *   expect SYN ACK,
 *   send ACK with Data,
 *   expect ACK,
 *   expect Data,
 *   send FIN,ACK
 *   expect FIN,ACK
 *   send ACK
 *   any failures cause test case to fail.
 */
ZTEST(net_tcp, test_server_fin_ack_after_data)
{
	struct net_context *ctx;
	int ret;

	test_case_no = TEST_SERVER_FIN_ACK_AFTER_DATA;

	t_state = T_SYN;
	seq = ack = 0;

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &ctx);
	zassert_ok(ret, "Failed to get net_context");

	net_context_ref(ctx);

	ret = net_context_bind(ctx, (struct sockaddr *)&my_addr_s,
			       sizeof(struct sockaddr_in));
	zassert_ok(ret, "Failed to bind net_context");

	/* Put context into listening mode and install accept cb */
	ret = net_context_listen(ctx, 1);
	zassert_ok(ret, "Failed to listen on net_context");

	ret = net_context_accept(ctx, test_fin_ack_after_data_accept_cb,
				 K_NO_WAIT, NULL);
	zassert_ok(ret, "Failed to set accept on net_context");

	/* Trigger the peer to send SYN */
	handle_server_fin_ack_after_data_test(AF_INET, NULL);

	/* test_fin_ack_after_data_recv_cb will release the semaphore after
	 * dummy request is read.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	/* Send dummy "response" */
	ret = net_context_send(accepted_ctx, TEST_FIN_ACK_AFTER_DATA_RSP,
			       sizeof(TEST_FIN_ACK_AFTER_DATA_RSP) - 1, NULL,
			       K_NO_WAIT, NULL);
	zassert_equal(ret, sizeof(TEST_FIN_ACK_AFTER_DATA_RSP) - 1,
		      "Failed to send data to peer %d", ret);

	/* test_fin_ack_after_data_recv_cb will release the semaphore after
	 * the connection is marked closed.
	 */
	test_sem_take(K_MSEC(100), __LINE__);

	net_context_put(ctx);
	net_context_put(accepted_ctx);

	/* Connection is in TIME_WAIT state, context will be released
	 * after K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY), so wait for it.
	 */
	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

ZTEST_SUITE(net_tcp, NULL, presetup, NULL, NULL, NULL);
