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

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_pkt.h>

#include "ipv4.h"
#include "ipv6.h"
#include "tcp.h"
#include "tcp_private.h"
#include "net_stats.h"

#include <zephyr/ztest.h>

#define MY_PORT 4242
#define PEER_PORT 4242

/* Data (1280 bytes) to be sent */
static const char lorem_ipsum[] =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris id "
	"sodales lacus. Proin vel rhoncus sapien. Morbi semper, enim in "
	"ullamcorper luctus, urna mi efficitur ex, laoreet eleifend massa "
	"felis ac dui. Duis ut magna convallis, tristique leo eu, ornare "
	"eros. Mauris a neque dictum, lobortis quam ut, rutrum erat. Vivamus "
	"vulputate neque vel auctor porta. Duis consectetur justo ac molestie "
	"tristique. In hac habitasse platea dictumst. Cras augue metus, "
	"aliquet sodales elit eget suscipit rutrum tellus. Nulla ante purus, "
	"dictum id tellus at, mattis cursus lectus. Mauris fringilla eros "
	"lorem, in auctor erat consequat nec. Ut pharetra sollicitudin dolor "
	"vel laoreet. Praesent eu lectus a dolor fringilla aliquet varius "
	"eget erat. Ut vitae mauris commodo, feugiat arcu non vehicula nunc. "
	"Nam ac enim elit. Praesent sit amet erat massa. Suspendisse potenti. "
	"Etiam diam justo, tempus vel lobortis tincidunt, scelerisque vitae "
	"mauris. Aenean vestibulum venenatis dapibus. Curabitur id ullamcorper"
	" diam. Ut eu turpis mauris. Aliquam et ligula est. Proin a velit non "
	"velit interdum vulputate. Proin vehicula eleifend suscipit. Cras "
	"condimentum non massa egestas tempor. Donec quis scelerisque est, id "
	"suscipit neque. Ut lobortis cursus ultrices. Aenean malesuada, nibh "
	"ut laoreet.";

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
static uint8_t test_case_no;
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

	if ((test_case_no == 4U) && (flags & SYN)) {
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

static struct net_pkt *prepare_fin_packet(sa_family_t af, uint16_t src_port,
					  uint16_t dst_port)
{
	return tester_prepare_tcp_pkt(af, src_port, dst_port, FIN, NULL, 0U);
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
	case 9:
		handle_server_recv_out_of_order(pkt);
		break;
	case 10:
		handle_data_fin1_test(net_pkt_family(pkt), &th);
		break;
	case 11:
		handle_data_during_fin1_test(net_pkt_family(pkt), &th);
		break;
	case 12:
		handle_syn_rst_ack(net_pkt_family(pkt), &th);
		break;
	case 13:
		handle_server_rst_on_closed_port(net_pkt_family(pkt), &th);
		break;
	case 14:
		handle_server_rst_on_listening_port(net_pkt_family(pkt), &th);
		break;
	case 15:
		handle_syn_invalid_ack(net_pkt_family(pkt), &th);
		break;
	case 16:
		handle_client_closing_failure_test(net_pkt_family(pkt), &th);
		break;
	case 17:
		handle_client_fin_wait_2_failure_test(net_pkt_family(pkt), &th);
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
	if (test_case_no == 3 || test_case_no == 4 || test_case_no == 13 ||
	    test_case_no == 14) {
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
	test_case_no = 3;
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
	test_case_no = 4;
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
	test_case_no = 5;
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
	test_case_no = 6;
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
	test_case_no = 12;
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
	test_case_no = 17;
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
			     "%s:%d unexpected acknowlegdement in original FIN, got %d",
			     __func__, __LINE__, ntohl(th->th_ack));
		t_state = T_FIN_1;
		/* retransmit the data that we already send*/
		reply = prepare_data_packet(af, htons(MY_PORT),
					    th->th_sport, "A", 1U);
		seq++;
		break;
	case T_FIN_1:
		test_verify_flags(th, FIN | ACK);
		/* retransmitted FIN should have the same sequence number*/
		zassert_true(get_rel_seq(th) == 1,
			     "%s:%i unexpected sequence number in retransmitted FIN, got %d",
			     __func__, __LINE__, get_rel_seq(th));
		zassert_true(ntohl(th->th_ack) == 2,
			     "%s:%i unexpected acknowlegdement in retransmitted FIN, got %d",
			     __func__, __LINE__, ntohl(th->th_ack));
		ack = ack + 1U;
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
	test_case_no = 10;
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
			     "%s:%d unexpected acknowlegdement in original FIN, got %d",
			     __func__, __LINE__, ntohl(th->th_ack));

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
	test_case_no = 11;
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
		reply = prepare_fin_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_FIN_1:
		test_verify_flags(th, FIN | ACK);
		t_state = T_CLOSING;
		reply = prepare_fin_packet(af, htons(MY_PORT), th->th_sport);
		break;
	case T_CLOSING:
		test_verify_flags(th, FIN | ACK);
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
	test_case_no = 16;
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
	test_case_no = 5;
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
	{ 50,  6, 40, 0},
	{ 50,  3, 40, 0}, /* Discardable packet */
	{ 55,  5, 40, 0},
	{ 40, 10, 60, 0}, /* Some bigger data */
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
	test_case_no = 9;

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
		zassert_equal(ntohl(th->th_ack), seq, "Invalid ACK value");
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
	test_case_no = 13;
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
	test_case_no = 14;
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
	test_case_no = 15;
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

ZTEST_SUITE(net_tcp, NULL, presetup, NULL, NULL, NULL);
