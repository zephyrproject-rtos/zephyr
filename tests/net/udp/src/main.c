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
#include <zephyr/net_buf.h>
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
	ARG_UNUSED(dev);

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
	const struct net_sockaddr *remote_addr;
	const struct net_sockaddr *local_addr;
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
	k_sem_give(&recv_lock);

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
			      struct net_in6_addr *src,
			      struct net_in6_addr *dst,
			      uint16_t src_port,
			      uint16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, 0, NET_AF_INET6,
					NET_IPPROTO_UDP, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	if (net_ipv6_create(pkt, src, dst) ||
	    net_udp_create(pkt, net_htons(src_port), net_htons(dst_port))) {
		printk("Cannot create IPv6 UDP pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_UDP);

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
				   struct net_in6_addr *src,
				   struct net_in6_addr *dst,
				   uint16_t src_port,
				   uint16_t dst_port,
				   struct ud *ud,
				   bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(ipv6_hop_by_hop_ext_hdr) +
					sizeof(payload), NET_AF_INET6,
					NET_IPPROTO_UDP, K_SECONDS(1));
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

	if (net_udp_create(pkt, net_htons(src_port), net_htons(dst_port))) {
		printk("Cannot create IPv6  pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	if (net_pkt_write(pkt, (uint8_t *)payload, sizeof(payload))) {
		printk("Cannot write IPv6 ext header pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_UDP);

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
			      struct net_in_addr *src,
			      struct net_in_addr *dst,
			      uint16_t src_port,
			      uint16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, 0, NET_AF_INET,
					NET_IPPROTO_UDP, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	if (net_ipv4_create(pkt, src, dst) ||
	    net_udp_create(pkt, net_htons(src_port), net_htons(dst_port))) {
		printk("Cannot create IPv4 UDP pkt %p", pkt);
		zassert_true(0, "exiting");
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

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

static void set_port(net_sa_family_t family, struct net_sockaddr *raddr,
		     struct net_sockaddr *laddr, uint16_t rport,
		     uint16_t lport)
{
	if (family == NET_AF_INET6) {
		if (raddr) {
			((struct net_sockaddr_in6 *)raddr)->
				sin6_port = net_htons(rport);
		}
		if (laddr) {
			((struct net_sockaddr_in6 *)laddr)->
				sin6_port = net_htons(lport);
		}
	} else if (family == NET_AF_INET) {
		if (raddr) {
			((struct net_sockaddr_in *)raddr)->
				sin_port = net_htons(rport);
		}
		if (laddr) {
			((struct net_sockaddr_in *)laddr)->
				sin_port = net_htons(lport);
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

	struct net_sockaddr_in6 any_addr6;
	const struct net_in6_addr in6addr_anyaddr = NET_IN6ADDR_ANY_INIT;

	struct net_sockaddr_in6 my_addr6;
	struct net_in6_addr in6addr_my = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x1 } } };

	struct net_sockaddr_in6 peer_addr6;
	struct net_in6_addr in6addr_peer = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0x4e, 0x11, 0, 0, 0x2 } } };

	struct net_sockaddr_in any_addr4;
	const struct net_in_addr in4addr_any = { { { 0 } } };

	struct net_sockaddr_in my_addr4;
	struct net_in_addr in4addr_my = { { { 192, 0, 2, 1 } } };

	struct net_sockaddr_in peer_addr4;
	struct net_in_addr in4addr_peer = { { { 192, 0, 2, 9 } } };

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	net_ipaddr_copy(&any_addr6.sin6_addr, &in6addr_anyaddr);
	any_addr6.sin6_family = NET_AF_INET6;

	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
	my_addr6.sin6_family = NET_AF_INET6;

	net_ipaddr_copy(&peer_addr6.sin6_addr, &in6addr_peer);
	peer_addr6.sin6_family = NET_AF_INET6;

	net_ipaddr_copy(&any_addr4.sin_addr, &in4addr_any);
	any_addr4.sin_family = NET_AF_INET;

	net_ipaddr_copy(&my_addr4.sin_addr, &in4addr_my);
	my_addr4.sin_family = NET_AF_INET;

	net_ipaddr_copy(&peer_addr4.sin_addr, &in4addr_peer);
	peer_addr4.sin_family = NET_AF_INET;

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
		user_data.remote_addr = (struct net_sockaddr *)raddr;	\
		user_data.local_addr =  (struct net_sockaddr *)laddr;	\
		user_data.remote_port = rport;				\
		user_data.local_port = lport;				\
		user_data.test = "DST="#raddr"-SRC="#laddr"-RP="#rport	\
			"-LP="#lport;					\
									\
		set_port(family, (struct net_sockaddr *)raddr,		\
			 (struct net_sockaddr *)laddr, rport, lport);	\
									\
		ret = net_udp_register(family,				\
				       (struct net_sockaddr *)raddr,	\
				       (struct net_sockaddr *)laddr,	\
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
	ret = net_udp_register(NET_AF_INET,					\
			       (struct net_sockaddr *)raddr,		\
			       (struct net_sockaddr *)laddr,		\
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

	ud = REGISTER(NET_AF_INET6, &any_addr6, &any_addr6, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	UNREGISTER(ud);

	ud = REGISTER(NET_AF_INET, &any_addr4, &any_addr4, 1234, 4242);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 4242);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 4242);
	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 1234, 4325);
	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 1234, 4325);
	UNREGISTER(ud);

	ud = REGISTER(NET_AF_INET6, &any_addr6, NULL, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	UNREGISTER(ud);

	ud = REGISTER(NET_AF_INET6, NULL, &any_addr6, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 61400);
	UNREGISTER(ud);

	ud = REGISTER(NET_AF_INET6, &peer_addr6, &my_addr6, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 1234, 4243);

	ud = REGISTER(NET_AF_INET, &peer_addr4, &my_addr4, 1234, 4242);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 4242);
	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 1234, 4243);

	ud = REGISTER(NET_AF_UNSPEC, NULL, NULL, 1234, 42423);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 42423);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 42423);

	ud = REGISTER(NET_AF_UNSPEC, NULL, NULL, 1234, 0);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 42422);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 42422);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 1234, 42422);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 42422);

	TEST_IPV4_FAIL(ud, &in4addr_peer, &in4addr_my, 12345, 42421);
	TEST_IPV6_FAIL(ud, &in6addr_peer, &in6addr_my, 12345, 42421);

	ud = REGISTER(NET_AF_UNSPEC, NULL, NULL, 0, 0);
	TEST_IPV4_OK(ud, &in4addr_peer, &in4addr_my, 12345, 42421);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 12345, 42421);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 12345, 42421);

	/* Remote addr same as local addr, these two will never match */
	REGISTER(NET_AF_INET6, &my_addr6, NULL, 1234, 4242);
	REGISTER(NET_AF_INET, &my_addr4, NULL, 1234, 4242);

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

#if defined(CONFIG_NET_UDP_OPTIONS)
/* Overwrite the UDP checksum field (offset 6 in the UDP header) so that the
 * OCS finalization / parsing treats the checksum as present (non-zero).
 */
static void set_v4_udp_checksum(struct net_pkt *pkt, uint16_t chksum)
{
	struct net_pkt_cursor backup;
	bool ow = net_pkt_is_being_overwritten(pkt);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) + 6U),
		   "skip to UDP checksum failed");
	zassert_ok(net_pkt_write_be16(pkt, chksum), "write UDP checksum failed");
	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, ow);
}

/* Build an IPv4 UDP packet with four bytes of user data followed by a surplus
 * area (RFC 9868). The surplus is [OCS(2)][tlvs...] (the four-byte data keeps
 * the surplus 2-byte aligned so no pad byte is needed). With @valid_ocs a
 * correct OCS is computed over a non-zero UDP checksum; otherwise the OCS bytes
 * are left as @ocs_raw.
 */
static struct net_pkt *build_v4_udp_opt_pkt(const uint8_t *tlvs, size_t tlv_len,
					    bool valid_ocs, uint16_t ocs_raw)
{
	static const uint8_t data[4] = { 'D', 'A', 'T', 'A' };
	struct net_in_addr src = { { { 192, 0, 2, 2 } } };
	struct net_in_addr dst = { { { 192, 0, 2, 1 } } };
	uint16_t surplus_len = (uint16_t)(sizeof(uint16_t) + tlv_len);
	uint16_t surplus_offset;
	uint8_t ocs_bytes[2];
	struct net_pkt *pkt;

	ocs_bytes[0] = ocs_raw >> 8;
	ocs_bytes[1] = ocs_raw & 0xffU;

	pkt = net_pkt_alloc_with_buffer(net_if_get_default(),
					sizeof(data) + surplus_len, NET_AF_INET,
					NET_IPPROTO_UDP, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	zassert_ok(net_ipv4_create(pkt, &src, &dst), "create v4 failed");
	zassert_ok(net_udp_create(pkt, net_htons(1234), net_htons(4242)), "create udp failed");
	zassert_ok(net_pkt_write(pkt, data, sizeof(data)), "write data failed");
	zassert_ok(net_pkt_write(pkt, ocs_bytes, sizeof(ocs_bytes)), "write ocs failed");
	if (tlv_len > 0U) {
		zassert_ok(net_pkt_write(pkt, tlvs, tlv_len), "write tlvs failed");
	}

	net_pkt_set_udp_opt_surplus_len(pkt, surplus_len);

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

	surplus_offset = net_pkt_ip_hdr_len(pkt) + NET_UDPH_LEN + sizeof(data);

	if (valid_ocs) {
		/* Force a non-zero UDP checksum so the OCS must be validated,
		 * then compute a correct OCS over the surplus area.
		 */
		set_v4_udp_checksum(pkt, 0x1234);
		zassert_ok(net_udp_opt_finalize_ocs(pkt, surplus_offset, surplus_len),
			   "finalize ocs failed");
	} else {
		/* Deterministic zero UDP checksum for the invalid/unused-OCS
		 * scenarios (the interface would otherwise compute one).
		 */
		set_v4_udp_checksum(pkt, 0);
	}

	net_pkt_cursor_init(pkt);

	return pkt;
}

ZTEST(udp_fn_tests, test_udp_opt_bad_ocs_still_delivered)
{
	/* RFC 9868: a failed OCS (or otherwise malformed surplus area) MUST NOT
	 * cause the datagram to be dropped; the user data is delivered and the
	 * options are ignored.
	 */
	static const uint8_t mds_tlv[4] = { NET_UDP_OPT_KIND_MDS, 0x04U, 0x04U, 0xb0U };
	struct net_in_addr my = { { { 192, 0, 2, 1 } } };
	struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(4242),
	};
	struct net_if *iface = net_if_get_default();
	struct net_conn_handle *handle = NULL;
	struct ud opt_ud = { 0 };
	struct net_pkt *pkt;
	int ret;

	net_ipaddr_copy(&local.sin_addr, &my);
	(void)net_if_ipv4_addr_add(iface, &my, NET_ADDR_MANUAL, 0);

	ret = net_udp_register(NET_AF_INET, NULL, (struct net_sockaddr *)&local,
			       0, 4242, NULL, test_ok, &opt_ud, &handle);
	zassert_ok(ret, "register failed (%d)", ret);

	/* Non-zero, deliberately wrong OCS value. */
	pkt = build_v4_udp_opt_pkt(mds_tlv, sizeof(mds_tlv), false, 0x0001U);

	fail = true;
	ret = net_recv_data(iface, pkt);
	zassert_ok(ret, "recv failed (%d)", ret);

	zassert_ok(k_sem_take(&recv_lock, TIMEOUT),
		   "datagram with a bad OCS was dropped instead of delivered");
	zassert_false(fail, "handler reported failure");

	(void)net_udp_unregister(handle);
}

ZTEST(udp_fn_tests, test_udp_opt_parse_known_ok)
{
	/* Positive control: a valid MDS option with a correct OCS parses and
	 * is reported through the option info.
	 */
	static const uint8_t mds_tlv[4] = { NET_UDP_OPT_KIND_MDS, 0x04U, 0x04U, 0xb0U };
	struct net_udp_opt_info info;
	struct net_pkt *pkt;
	int ret;

	pkt = build_v4_udp_opt_pkt(mds_tlv, sizeof(mds_tlv), true, 0U);
	ret = net_udp_opt_parse(pkt, &info);
	zassert_ok(ret, "valid options rejected (ret %d)", ret);
	zassert_true((info.present & NET_UDP_OPT_F_MDS) != 0U, "MDS not parsed");
	zassert_equal(info.mds, 0x04b0U, "MDS value mismatch (%u)", info.mds);

	net_pkt_unref(pkt);
}

ZTEST(udp_fn_tests, test_udp_opt_unknown_unsafe_discarded)
{
	/* RFC 9868 Section 7: an unsupported option in the UNSAFE range
	 * (192-255) must cause all options to be discarded, i.e. parsing must
	 * report an error.
	 */
	static const uint8_t unsafe_tlv[4] = { NET_UDP_OPT_KIND_UCMP, 0x04U, 0x00U, 0x00U };
	struct net_udp_opt_info info;
	struct net_pkt *pkt;
	int ret;

	pkt = build_v4_udp_opt_pkt(unsafe_tlv, sizeof(unsafe_tlv), true, 0U);
	ret = net_udp_opt_parse(pkt, &info);
	zassert_true(ret < 0, "UNSAFE option was not rejected (ret %d)", ret);

	net_pkt_unref(pkt);
}

ZTEST(udp_fn_tests, test_udp_opt_uexp_kind_is_unsafe)
{
	/* RFC 9868: UNSAFE Experimental is kind 254 (255 is Reserved). Both
	 * fall in the UNSAFE range and must cause all options to be discarded.
	 */
	static const uint8_t uexp_tlv[4] = { NET_UDP_OPT_KIND_UEXP, 0x04U, 0x00U, 0x00U };
	static const uint8_t reserved_tlv[4] = { 255U, 0x04U, 0x00U, 0x00U };
	struct net_udp_opt_info info;
	struct net_pkt *pkt;

	zassert_equal(NET_UDP_OPT_KIND_UEXP, 254, "UEXP kind must be 254");

	pkt = build_v4_udp_opt_pkt(uexp_tlv, sizeof(uexp_tlv), true, 0U);
	zassert_true(net_udp_opt_parse(pkt, &info) < 0, "UEXP (254) not rejected");
	net_pkt_unref(pkt);

	pkt = build_v4_udp_opt_pkt(reserved_tlv, sizeof(reserved_tlv), true, 0U);
	zassert_true(net_udp_opt_parse(pkt, &info) < 0, "reserved UNSAFE (255) not rejected");
	net_pkt_unref(pkt);
}

ZTEST(udp_fn_tests, test_udp_opt_wrong_length_discarded)
{
	/* RFC 9868 Section 7: a known option whose length is not the value
	 * defined for its kind must cause all options to be discarded. MDS
	 * (kind 4) has a fixed total length of 4; present it with length 5.
	 */
	static const uint8_t bad_mds[5] = { NET_UDP_OPT_KIND_MDS, 0x05U, 0x00U, 0x00U, 0x00U };
	struct net_udp_opt_info info;
	struct net_pkt *pkt;
	int ret;

	pkt = build_v4_udp_opt_pkt(bad_mds, sizeof(bad_mds), true, 0U);
	ret = net_udp_opt_parse(pkt, &info);
	zassert_true(ret < 0, "wrong-length option was not rejected (ret %d)", ret);

	net_pkt_unref(pkt);
}

ZTEST(udp_fn_tests, test_udp_opt_ocs_unused_parsed)
{
	/* RFC 9868: with a zero UDP checksum the OCS may be unused (zero);
	 * the options must still be parsed and processed.
	 */
	static const uint8_t mds_tlv[4] = { NET_UDP_OPT_KIND_MDS, 0x04U, 0x04U, 0xb0U };
	struct net_udp_opt_info info;
	struct net_pkt *pkt;
	int ret;

	/* valid_ocs = false leaves the OCS bytes zero and the UDP checksum
	 * zero (the test disables UDP checksums), exercising the unused-OCS
	 * path.
	 */
	pkt = build_v4_udp_opt_pkt(mds_tlv, sizeof(mds_tlv), false, 0U);
	ret = net_udp_opt_parse(pkt, &info);
	zassert_ok(ret, "options not parsed with an unused OCS (ret %d)", ret);
	zassert_true((info.present & NET_UDP_OPT_F_MDS) != 0U, "MDS not parsed");
	zassert_true(info.ocs_valid, "ocs_valid should be true when OCS is unused");
	zassert_equal(info.mds, 0x04b0U, "MDS value mismatch (%u)", info.mds);

	net_pkt_unref(pkt);
}

ZTEST(udp_fn_tests, test_udp_opt_extended_length)
{
	/* RFC 9868 Section 5: an option with Length == 255 uses the extended
	 * format (Kind, 0xff, 2-byte Extended Length counting the whole
	 * option). Use an experimental (SAFE, unknown) option in extended form
	 * followed by a known MDS option, and check the MDS is still parsed -
	 * proving the extended option was skipped by the correct length.
	 */
	static const uint8_t tlvs[] = {
		/* EXP option, extended format, total length 6 (2 payload bytes) */
		NET_UDP_OPT_KIND_EXP, 0xffU, 0x00U, 0x06U, 0xaaU, 0xbbU,
		/* MDS option */
		NET_UDP_OPT_KIND_MDS, 0x04U, 0x04U, 0xb0U,
	};
	struct net_udp_opt_info info;
	struct net_pkt *pkt;
	int ret;

	pkt = build_v4_udp_opt_pkt(tlvs, sizeof(tlvs), true, 0U);
	ret = net_udp_opt_parse(pkt, &info);
	zassert_ok(ret, "extended-length option not handled (ret %d)", ret);
	zassert_true((info.present & NET_UDP_OPT_F_MDS) != 0U,
		     "option after extended-length option not parsed");
	zassert_equal(info.mds, 0x04b0U, "MDS value mismatch (%u)", info.mds);

	net_pkt_unref(pkt);
}

ZTEST(udp_fn_tests, test_udp_opt_apc_passthrough)
{
	/* APC (RFC 9868) is an application-managed checksum: the CRC32c value
	 * is carried verbatim to the application, which is responsible for
	 * validating it. Verify the parsed value is delivered unchanged.
	 */
	static const uint8_t apc_tlv[6] = {
		NET_UDP_OPT_KIND_APC, 0x06U, 0xdeU, 0xadU, 0xbeU, 0xefU,
	};
	struct net_udp_opt_info info;
	struct net_pkt *pkt;
	int ret;

	pkt = build_v4_udp_opt_pkt(apc_tlv, sizeof(apc_tlv), true, 0U);
	ret = net_udp_opt_parse(pkt, &info);
	zassert_ok(ret, "APC option not parsed (ret %d)", ret);
	zassert_true((info.present & NET_UDP_OPT_F_APC) != 0U, "APC not present");
	zassert_equal(info.apc_crc, 0xdeadbeefU, "APC CRC mismatch (0x%08x)", info.apc_crc);

	net_pkt_unref(pkt);
}
#endif /* CONFIG_NET_UDP_OPTIONS */

ZTEST_SUITE(udp_fn_tests, NULL, NULL, NULL, NULL, NULL);
