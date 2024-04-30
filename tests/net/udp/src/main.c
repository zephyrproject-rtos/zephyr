/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_UDP_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/udp.h>
#include <zephyr/random/random.h>

#include "ipv4.h"
#include "ipv6.h"

#include <zephyr/ztest.h>

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#include "udp_internal.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define NET_LOG_ENABLED 1
#endif
#include "net_private.h"
#include "ipv4.h"

static bool test_failed;
static bool fail = true;
static struct k_sem recv_lock;
static char payload[] = { 'f', 'o', 'o', 'b', 'a', 'r', '\0' };

struct net_udp_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_udp_dev_init(const struct device *dev)
{
	struct net_udp_context *net_udp_context = dev->data;

	net_udp_context = net_udp_context;

	return 0;
}

static uint8_t *net_udp_get_mac(const struct device *dev)
{
	struct net_udp_context *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand8_get();
	}

	return context->mac_addr;
}

static void net_udp_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_udp_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int send_status = -EINVAL;

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	DBG("Data was sent successfully\n");

	send_status = 0;

	return 0;
}

static inline struct in_addr *if_get_addr(struct net_if *iface)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->config.ip.ipv4->unicast[i].ipv4.is_used &&
		    iface->config.ip.ipv4->unicast[i].ipv4.address.family ==
								AF_INET &&
		    iface->config.ip.ipv4->unicast[i].ipv4.addr_state ==
							NET_ADDR_PREFERRED) {
			return
			    &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr;
		}
	}

	return NULL;
}

struct net_udp_context net_udp_context_data;

static struct dummy_api net_udp_if_api = {
	.iface_api.init = net_udp_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_udp_test, "net_udp_test",
		net_udp_dev_init, NULL,
		&net_udp_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_udp_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

struct ud {
	const struct sockaddr *remote_addr;
	const struct sockaddr *local_addr;
	uint16_t remote_port;
	uint16_t local_port;
	char *test;
	void *handle;
};

static struct ud *returned_ud;

static enum net_verdict test_ok(struct net_conn *conn,
				struct net_pkt *pkt,
				union net_ip_header *ip_hdr,
				union net_proto_header *proto_hdr,
				void *user_data)
{
	struct ud *ud = (struct ud *)user_data;

	k_sem_give(&recv_lock);

	if (!ud) {
		fail = true;

		DBG("Test %s failed.", ud->test);

		return NET_DROP;
	}

	fail = false;

	returned_ud = user_data;

	net_pkt_unref(pkt);

	return NET_OK;
}

static enum net_verdict test_fail(struct net_conn *conn,
				  struct net_pkt *pkt,
				  union net_ip_header *ip_hdr,
				  union net_proto_header *proto_hdr,
				  void *user_data)
{
	/* This function should never be called as there should not
	 * be a matching UDP connection.
	 */

	fail = true;
	return NET_DROP;
}

uint8_t ipv6_hop_by_hop_ext_hdr[] = {
/* Next header UDP */
0x11,
/* Length (multiple of 8 octets) */
0x0C,
/* Experimental extension */
0x3e,
/* Length in bytes */
0x20,
0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
0x49, 0x4A, 0x4B, 0x4C, 0x4E, 0x4F, 0x50, 0x51,
0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
0x5A, 0x5B, 0x5C, 0x5D, 0x5F, 0x60, 0x61, 0x62,
/* Another experimental extension */
0x3e,
/* Length in bytes */
0x20,
0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
0x6B, 0x6C, 0x6D, 0x6F, 0x70, 0x71, 0x72, 0x73,
0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B,
0x7C, 0x7D, 0x7E, 0x21, 0x22, 0x23, 0x24, 0x25,
/* Another experimental extension */
0x3e,
/* Length in bytes */
0x20,
0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
};

#define TIMEOUT K_MSEC(200)

static bool send_ipv6_udp_msg(struct net_if *iface,
			      struct in6_addr *src,
			      struct in6_addr *dst,
			      uint16_t src_port,
			      uint16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, 0, AF_INET6,
					IPPROTO_UDP, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	if (net_ipv6_create(pkt, src, dst) ||
	    net_udp_create(pkt, htons(src_port), htons(dst_port))) {
		printk("Cannot create IPv6 UDP pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_UDP);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		printk("Cannot recv pkt %p, ret %d\n", pkt, ret);
		zassert_true(0, "exiting");
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {

		/**TESTPOINT: Check for failure*/
		zassert_true(expect_failure, "Timeout, packet not received");
		return true;
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		printk("IPv6 wrong user data %p returned, expected %p\n",
		       returned_ud, ud);
		zassert_true(0, "exiting");
	}

	return !fail;
}

static bool send_ipv6_udp_long_msg(struct net_if *iface,
				   struct in6_addr *src,
				   struct in6_addr *dst,
				   uint16_t src_port,
				   uint16_t dst_port,
				   struct ud *ud,
				   bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(ipv6_hop_by_hop_ext_hdr) +
					sizeof(payload), AF_INET6,
					IPPROTO_UDP, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	if (net_ipv6_create(pkt, src, dst)) {
		printk("Cannot create IPv6  pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	if (net_pkt_write(pkt, (uint8_t *)ipv6_hop_by_hop_ext_hdr,
			      sizeof(ipv6_hop_by_hop_ext_hdr))) {
		printk("Cannot write IPv6 ext header pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_set_ipv6_ext_len(pkt, sizeof(ipv6_hop_by_hop_ext_hdr));
	net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);

	if (net_udp_create(pkt, htons(src_port), htons(dst_port))) {
		printk("Cannot create IPv6  pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	if (net_pkt_write(pkt, (uint8_t *)payload, sizeof(payload))) {
		printk("Cannot write IPv6 ext header pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_UDP);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		printk("Cannot recv pkt %p, ret %d\n", pkt, ret);
		zassert_true(0, "exiting");
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {
		/**TESTPOINT: Check for failure*/
		zassert_true(expect_failure, "Timeout, packet not received");
		return true;
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		printk("IPv6 wrong user data %p returned, expected %p\n",
		       returned_ud, ud);
		zassert_true(0, "exiting");
	}

	return !fail;
}

static bool send_ipv4_udp_msg(struct net_if *iface,
			      struct in_addr *src,
			      struct in_addr *dst,
			      uint16_t src_port,
			      uint16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, 0, AF_INET,
					IPPROTO_UDP, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	if (net_ipv4_create(pkt, src, dst) ||
	    net_udp_create(pkt, htons(src_port), htons(dst_port))) {
		printk("Cannot create IPv4 UDP pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, IPPROTO_UDP);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		printk("Cannot recv pkt %p, ret %d\n", pkt, ret);
		zassert_true(0, "exiting");
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {

		/**TESTPOINT: Check for failure*/
		zassert_true(expect_failure, "Timeout, packet not received");
		return true;
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		printk("IPv4 wrong user data %p returned, expected %p\n",
		       returned_ud, ud);
		zassert_true(0, "exiting");
	}

	return !fail;
}

static void set_port(sa_family_t family, struct sockaddr *raddr,
		     struct sockaddr *laddr, uint16_t rport,
		     uint16_t lport)
{
	if (family == AF_INET6) {
		if (raddr) {
			((struct sockaddr_in6 *)raddr)->
				sin6_port = htons(rport);
		}
		if (laddr) {
			((struct sockaddr_in6 *)laddr)->
				sin6_port = htons(lport);
		}
	} else if (family == AF_INET) {
		if (raddr) {
			((struct sockaddr_in *)raddr)->
				sin_port = htons(rport);
		}
		if (laddr) {
			((struct sockaddr_in *)laddr)->
				sin_port = htons(lport);
		}
	}
}

ZTEST(udp_fn_tests, test_udp)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

	test_failed = false;

	struct net_conn_handle *handlers[CONFIG_NET_MAX_CONN];
	struct net_if *iface;
	struct net_if_addr *ifaddr;
	struct ud *ud;
	int ret, i = 0;
	bool st;

	struct sockaddr_in6 any_addr6;
	const struct in6_addr in6addr_anyaddr = IN6ADDR_ANY_INIT;

	struct sockaddr_in6 my_addr6;
	struct in6_addr in6addr_my = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x1 } } };

	struct sockaddr_in6 peer_addr6;
	struct in6_addr in6addr_peer = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0x4e, 0x11, 0, 0, 0x2 } } };

	struct sockaddr_in any_addr4;
	const struct in_addr in4addr_any = { { { 0 } } };

	struct sockaddr_in my_addr4;
	struct in_addr in4addr_my = { { { 192, 0, 2, 1 } } };

	struct sockaddr_in peer_addr4;
	struct in_addr in4addr_peer = { { { 192, 0, 2, 9 } } };

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	net_ipaddr_copy(&any_addr6.sin6_addr, &in6addr_anyaddr);
	any_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
	my_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&peer_addr6.sin6_addr, &in6addr_peer);
	peer_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&any_addr4.sin_addr, &in4addr_any);
	any_addr4.sin_family = AF_INET;

	net_ipaddr_copy(&my_addr4.sin_addr, &in4addr_my);
	my_addr4.sin_family = AF_INET;

	net_ipaddr_copy(&peer_addr4.sin_addr, &in4addr_peer);
	peer_addr4.sin_family = AF_INET;

	k_sem_init(&recv_lock, 0, UINT_MAX);

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv6_addr(&in6addr_my), iface);
		zassert_true(0, "exiting");
	}

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv4_addr(&in4addr_my), iface);
		zassert_true(0, "exiting");
	}

#define REGISTER(family, raddr, laddr, rport, lport)			\
	({								\
		static struct ud user_data;				\
									\
		user_data.remote_addr = (struct sockaddr *)raddr;	\
		user_data.local_addr =  (struct sockaddr *)laddr;	\
		user_data.remote_port = rport;				\
		user_data.local_port = lport;				\
		user_data.test = "DST="#raddr"-SRC="#laddr"-RP="#rport	\
			"-LP="#lport;					\
									\
		set_port(family, (struct sockaddr *)raddr,		\
			 (struct sockaddr *)laddr, rport, lport);	\
									\
		ret = net_udp_register(family,				\
				       (struct sockaddr *)raddr,	\
				       (struct sockaddr *)laddr,	\
				       rport, lport,			\
				       NULL, test_ok, &user_data,	\
				       &handlers[i]);			\
		if (ret) {						\
			printk("UDP register %s failed (%d)\n",		\
			       user_data.test, ret);			\
			zassert_true(0, "exiting");			\
		}							\
		user_data.handle = handlers[i++];			\
		&user_data;						\
	})

#define REGISTER_FAIL(raddr, laddr, rport, lport)			\
	ret = net_udp_register(AF_INET,					\
			       (struct sockaddr *)raddr,		\
			       (struct sockaddr *)laddr,		\
			       rport, lport,				\
			       NULL, test_fail, INT_TO_POINTER(0),	\
			       NULL);					\
	if (!ret) {							\
		printk("UDP register invalid match %s failed\n",	\
		       "DST="#raddr"-SRC="#laddr"-RP="#rport"-LP="#lport); \
		zassert_true(0, "exiting");				\
	}

#define UNREGISTER(ud)							\
	ret = net_udp_unregister(ud->handle);				\
	if (ret) {							\
		printk("UDP unregister %p failed (%d)\n", ud->handle,	\
		       ret);						\
		zassert_true(0, "exiting");				\
	}

#define TEST_IPV6_OK(ud, raddr, laddr, rport, lport)			\
	st = send_ipv6_udp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       false);					\
	if (!st) {							\
		printk("%d: UDP test \"%s\" fail\n", __LINE__,		\
		       ud->test);					\
		zassert_true(0, "exiting");				\
	}

#define TEST_IPV6_LONG_OK(ud, raddr, laddr, rport, lport)		\
	st = send_ipv6_udp_long_msg(iface, raddr, laddr, rport, lport, ud, \
			       false);					\
	if (!st) {							\
		printk("%d: UDP long test \"%s\" fail\n", __LINE__,	\
		       ud->test);					\
		zassert_true(0, "exiting");				\
	}

#define TEST_IPV4_OK(ud, raddr, laddr, rport, lport)			\
	st = send_ipv4_udp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       false);					\
	if (!st) {							\
		printk("%d: UDP test \"%s\" fail\n", __LINE__,		\
		       ud->test);					\
		zassert_true(0, "exiting");				\
	}

#define TEST_IPV6_FAIL(ud, raddr, laddr, rport, lport)			\
	st = send_ipv6_udp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       true);					\
	if (!st) {							\
		printk("%d: UDP neg test \"%s\" fail\n", __LINE__,	\
		       ud->test);					\
		zassert_true(0, "exiting");				\
	}

#define TEST_IPV4_FAIL(ud, raddr, laddr, rport, lport)			\
	st = send_ipv4_udp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       true);					\
	if (!st) {							\
		printk("%d: UDP neg test \"%s\" fail\n", __LINE__,	\
		       ud->test);					\
		zassert_true(0, "exiting");				\
	}

	ud = REGISTER(AF_INET6, &any_addr6, &any_addr6, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	UNREGISTER(ud);

	ud = REGISTER(AF_INET, &any_addr4, &any_addr4, 1234, 4242);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 4242);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 4242);
	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 1234, 4325);
	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 1234, 4325);
	UNREGISTER(ud);

	ud = REGISTER(AF_INET6, &any_addr6, NULL, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	UNREGISTER(ud);

	ud = REGISTER(AF_INET6, NULL, &any_addr6, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	UNREGISTER(ud);

	ud = REGISTER(AF_INET6, &peer_addr6, &my_addr6, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 4243);

	ud = REGISTER(AF_INET, &peer_addr4, &my_addr4, 1234, 4242);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 4242);
	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 1234, 4243);

	ud = REGISTER(AF_UNSPEC, NULL, NULL, 1234, 42423);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 42423);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 42423);

	ud = REGISTER(AF_UNSPEC, NULL, NULL, 1234, 0);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 42422);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 42422);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 42422);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 42422);

	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 12345, 42421);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 12345, 42421);

	ud = REGISTER(AF_UNSPEC, NULL, NULL, 0, 0);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 12345, 42421);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 12345, 42421);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 12345, 42421);

	/* Remote addr same as local addr, these two will never match */
	REGISTER(AF_INET6, &my_addr6, NULL, 1234, 4242);
	REGISTER(AF_INET, &my_addr4, NULL, 1234, 4242);

	/* IPv4 remote addr and IPv6 remote addr, impossible combination */
	REGISTER_FAIL(&my_addr4, &my_addr6, 1234, 4242);

	/**TESTPOINT: Check if tests passed*/
	zassert_false(fail, "Tests failed");

	i--;
	while (i) {
		ret = net_udp_unregister(handlers[i]);
		if (ret < 0 && ret != -ENOENT) {
			printk("Cannot unregister udp %d\n", i);
			zassert_true(0, "exiting");
		}

		i--;
	}

	zassert_true((net_udp_unregister(NULL) < 0), "Unregister udp failed");
	zassert_false(test_failed, "udp tests failed");
}

ZTEST_SUITE(udp_fn_tests, NULL, NULL, NULL, NULL, NULL);
