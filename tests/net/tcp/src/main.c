/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <net/buf.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/ethernet.h>

#include <tc_util.h>

#if defined(CONFIG_NET_DEBUG_TCP)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define NET_LOG_ENABLED 1
#else
#define DBG(fmt, ...)
#endif

#include "tcp.h"
#include "net_private.h"

static bool test_failed;
static bool fail = true;
static struct k_sem recv_lock;
static struct net_context *v6_ctx;
static struct net_context *reply_v6_ctx;
static struct net_context *v4_ctx;
static struct net_context *reply_v4_ctx;

static struct sockaddr_in6 any_addr6;
static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

static struct sockaddr_in any_addr4;
static const struct in_addr in4addr_any = { { { 0 } } };

static struct in6_addr my_v6_inaddr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x2a } } };
static struct in6_addr peer_v6_inaddr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0,
				      0, 0, 0, 0, 0x4e, 0x11, 0, 0, 0xa2 } } };
static struct sockaddr_in6 my_v6_addr;
static struct sockaddr_in6 peer_v6_addr;

static struct in_addr my_v4_inaddr = { { { 192, 0, 2, 150 } } };
static struct in_addr peer_v4_inaddr = { { { 192, 0, 2, 250 } } };
static struct sockaddr_in my_v4_addr;
static struct sockaddr_in peer_v4_addr;

#define MY_TCP_PORT 5545
#define PEER_TCP_PORT 9876

#define WAIT_TIME 250
#define WAIT_TIME_LONG MSEC_PER_SEC

static struct k_sem wait_connect;
#if 0
static struct k_sem wait_in_accept;
static bool connect_cb_called;
static int accept_cb_called;
#endif
static bool syn_v6_sent;

struct net_tcp_context {
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_tcp_dev_init(struct device *dev)
{
	struct net_tcp_context *net_tcp_context = dev->driver_data;

	net_tcp_context = net_tcp_context;

	return 0;
}

static u8_t *net_tcp_get_mac(struct device *dev)
{
	struct net_tcp_context *context = dev->driver_data;

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

static void net_tcp_iface_init(struct net_if *iface)
{
	u8_t *mac = net_tcp_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static void v6_send_syn_ack(struct net_if *iface, struct net_pkt *req)
{
	struct net_pkt *rsp = NULL;
	int ret;

	ret = net_tcp_prepare_segment(reply_v6_ctx->tcp,
				      NET_TCP_SYN | NET_TCP_ACK, NULL, 0,
				      NULL, (struct sockaddr *)&my_v6_addr,
				      &rsp);
	if (ret) {
		DBG("TCP packet creation failed\n");
		return;
	}

	DBG("1) rsp src %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->src),
	    ntohs(NET_TCP_HDR(rsp)->src_port));
	DBG("1) rsp dst %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->dst),
	    ntohs(NET_TCP_HDR(rsp)->dst_port));

	net_ipaddr_copy(&NET_IPV6_HDR(rsp)->src, &NET_IPV6_HDR(req)->dst);
	net_ipaddr_copy(&NET_IPV6_HDR(rsp)->dst, &NET_IPV6_HDR(req)->src);

	NET_TCP_HDR(rsp)->src_port = NET_TCP_HDR(req)->dst_port;
	NET_TCP_HDR(rsp)->dst_port = NET_TCP_HDR(req)->src_port;

	DBG("rsp src %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->src),
	    ntohs(NET_TCP_HDR(rsp)->src_port));
	DBG("rsp dst %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->dst),
	    ntohs(NET_TCP_HDR(rsp)->dst_port));

	net_hexdump_frags("request TCPv6", req);
	net_hexdump_frags("reply   TCPv6", rsp);

	ret =  net_recv_data(iface, rsp);
	if (!ret) {
		net_pkt_unref(rsp);
	}

	k_sem_give(&wait_connect);
}

static int send_status = -EINVAL;

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}
	if (syn_v6_sent && net_pkt_family(pkt) == AF_INET6) {
		DBG("v6 SYN was sent successfully\n");
		syn_v6_sent = false;
		v6_send_syn_ack(iface, pkt);
	} else {
		DBG("Data was sent successfully\n");
	}

	net_pkt_unref(pkt);

	send_status = 0;

	return 0;
}

static int tester_send_peer(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	DBG("Peer data was sent successfully\n");

	net_pkt_unref(pkt);

	return 0;
}

static inline struct in_addr *if_get_addr(struct net_if *iface)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->ipv4.unicast[i].is_used &&
		    iface->ipv4.unicast[i].address.family == AF_INET &&
		    iface->ipv4.unicast[i].addr_state == NET_ADDR_PREFERRED) {
			return &iface->ipv4.unicast[i].address.in_addr;
		}
	}

	return NULL;
}

struct ud {
	const struct sockaddr *remote_addr;
	const struct sockaddr *local_addr;
	u16_t remote_port;
	u16_t local_port;
	char *test;
	struct net_conn_handle *handle;
};

static struct ud *returned_ud;

static enum net_verdict test_ok(struct net_conn *conn,
				struct net_pkt *pkt,
				void *user_data)
{
	struct ud *ud = (struct ud *)user_data;

	k_sem_give(&recv_lock);

	if (!ud) {
		fail = true;

		DBG("Test failed.\n");

		return NET_DROP;
	}

	fail = false;

	returned_ud = user_data;

	net_pkt_unref(pkt);

	return NET_OK;
}

static enum net_verdict test_fail(struct net_conn *conn,
				  struct net_pkt *pkt,
				  void *user_data)
{
	/* This function should never be called as there should not
	 * be a matching TCP connection.
	 */

	fail = true;
	return NET_DROP;
}

static void setup_ipv6_tcp(struct net_pkt *pkt,
			   struct in6_addr *remote_addr,
			   struct in6_addr *local_addr,
			   u16_t remote_port,
			   u16_t local_port)
{
	NET_IPV6_HDR(pkt)->vtc = 0x60;
	NET_IPV6_HDR(pkt)->tcflow = 0;
	NET_IPV6_HDR(pkt)->flow = 0;
	NET_IPV6_HDR(pkt)->len[0] = 0;
	NET_IPV6_HDR(pkt)->len[1] = NET_TCPH_LEN;

	NET_IPV6_HDR(pkt)->nexthdr = IPPROTO_TCP;
	NET_IPV6_HDR(pkt)->hop_limit = 255;

	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, remote_addr);
	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, local_addr);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	NET_TCP_HDR(pkt)->src_port = htons(remote_port);
	NET_TCP_HDR(pkt)->dst_port = htons(local_port);

	net_pkt_set_ipv6_ext_len(pkt, 0);

	net_buf_add(pkt->frags, net_pkt_ip_hdr_len(pkt) +
				sizeof(struct net_tcp_hdr));
}

static void setup_ipv4_tcp(struct net_pkt *pkt,
			   struct in_addr *remote_addr,
			   struct in_addr *local_addr,
			   u16_t remote_port,
			   u16_t local_port)
{
	NET_IPV4_HDR(pkt)->vhl = 0x45;
	NET_IPV4_HDR(pkt)->tos = 0;
	NET_IPV4_HDR(pkt)->len[0] = 0;
	NET_IPV4_HDR(pkt)->len[1] = NET_TCPH_LEN +
		sizeof(struct net_ipv4_hdr);

	NET_IPV4_HDR(pkt)->proto = IPPROTO_TCP;

	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src, remote_addr);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, local_addr);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	NET_TCP_HDR(pkt)->src_port = htons(remote_port);
	NET_TCP_HDR(pkt)->dst_port = htons(local_port);

	net_pkt_set_ipv6_ext_len(pkt, 0);

	net_buf_add(pkt->frags, net_pkt_ip_hdr_len(pkt) +
				sizeof(struct net_tcp_hdr));
}

#define TIMEOUT 200

static bool send_ipv6_tcp_msg(struct net_if *iface,
			      struct in6_addr *src,
			      struct in6_addr *dst,
			      u16_t src_port,
			      u16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	int ret;

	pkt = net_pkt_get_reserve_tx(0, K_FOREVER);

	net_pkt_set_ll_reserve(pkt, 0);

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);

	setup_ipv6_tcp(pkt, src, dst, src_port, dst_port);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		printk("Cannot recv pkt %p, ret %d\n", pkt, ret);
		return false;
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {
		printk("Timeout, packet not received\n");
		if (expect_failure) {
			return false;
		} else {
			return true;
		}
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		printk("IPv6 wrong user data %p returned, expected %p\n",
		       returned_ud, ud);
		return false;
	}

	return !fail;
}

static bool send_ipv4_tcp_msg(struct net_if *iface,
			      struct in_addr *src,
			      struct in_addr *dst,
			      u16_t src_port,
			      u16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	int ret;

	pkt = net_pkt_get_reserve_tx(0, K_FOREVER);

	net_pkt_set_ll_reserve(pkt, 0);

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);

	setup_ipv4_tcp(pkt, src, dst, src_port, dst_port);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		printk("Cannot recv pkt %p, ret %d\n", pkt, ret);
		return false;
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {
		printk("Timeout, packet not received\n");
		if (expect_failure) {
			return false;
		} else {
			return true;
		}
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		printk("IPv4 wrong user data %p returned, expected %p\n",
		       returned_ud, ud);
		return false;
	}

	return !fail;
}

static void set_port(sa_family_t family, struct sockaddr *raddr,
		     struct sockaddr *laddr, u16_t rport,
		     u16_t lport)
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

static bool test_register(void)
{
	struct net_conn_handle *handlers[CONFIG_NET_MAX_CONN];
	struct net_if *iface = net_if_get_default();
	struct net_if_addr *ifaddr;
	struct ud *ud;
	int ret, i = 0;
	bool st;

	struct sockaddr_in6 my_addr6;
	struct in6_addr in6addr_my = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x1 } } };

	struct sockaddr_in6 peer_addr6;
	struct in6_addr in6addr_peer = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0x4e, 0x11, 0, 0, 0x2 } } };

	struct sockaddr_in my_addr4;
	struct in_addr in4addr_my = { { { 192, 0, 2, 1 } } };

	struct sockaddr_in peer_addr4;
	struct in_addr in4addr_peer = { { { 192, 0, 2, 9 } } };

	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
	my_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&peer_addr6.sin6_addr, &in6addr_peer);
	peer_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&my_addr4.sin_addr, &in4addr_my);
	my_addr4.sin_family = AF_INET;

	net_ipaddr_copy(&peer_addr4.sin_addr, &in4addr_peer);
	peer_addr4.sin_family = AF_INET;

	k_sem_init(&recv_lock, 0, UINT_MAX);

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv6_addr(&in6addr_my), iface);
		return false;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv4_addr(&in4addr_my), iface);
		return false;
	}

	/*
	 * First test the TCP port handling logic. This just simulates
	 * received packet when TCP connection has been already connected.
	 * These tests are similar as when testing UDP.
	 */

#define REGISTER(family, raddr, laddr, rport, lport)			\
	({								\
		static struct ud user_data;				\
									\
		user_data.remote_addr = (struct sockaddr *)raddr;	\
		user_data.local_addr =  (struct sockaddr *)laddr;	\
		user_data.remote_port = rport;				\
		user_data.local_port = lport;				\
		user_data.test = #raddr"-"#laddr"-"#rport"-"#lport;	\
									\
		set_port(family, (struct sockaddr *)raddr,		\
			 (struct sockaddr *)laddr, rport, lport);	\
									\
		ret = net_tcp_register((struct sockaddr *)raddr,	\
				       (struct sockaddr *)laddr,	\
				       rport, lport,			\
				       test_ok, &user_data,		\
				       &handlers[i]);			\
		if (ret) {						\
			printk("TCP register %s failed (%d)\n",		\
			       user_data.test, ret);			\
			return false;					\
		}							\
		user_data.handle = handlers[i++];			\
		&user_data;						\
	})

#define REGISTER_FAIL(raddr, laddr, rport, lport)			\
	ret = net_tcp_register((struct sockaddr *)raddr,		\
			       (struct sockaddr *)laddr,		\
			       rport, lport,				\
			       test_fail, INT_TO_POINTER(0), NULL);	\
	if (!ret) {							\
		printk("TCP register invalid match %s failed\n",	\
		       #raddr"-"#laddr"-"#rport"-"#lport);		\
		return false;						\
	}

#define UNREGISTER(ud)							\
	ret = net_tcp_unregister(ud->handle);				\
	if (ret) {							\
		printk("TCP unregister %p failed (%d)\n", ud->handle,	\
		       ret);						\
		return false;						\
	}

#define TEST_IPV6_OK(ud, raddr, laddr, rport, lport)			\
	st = send_ipv6_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       false);					\
	if (!st) {							\
		printk("%d: TCP test \"%s\" fail\n", __LINE__,		\
		       ud->test);					\
		return false;						\
	}

#define TEST_IPV4_OK(ud, raddr, laddr, rport, lport)			\
	st = send_ipv4_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       false);					\
	if (!st) {							\
		printk("%d: TCP test \"%s\" fail\n", __LINE__,		\
		       ud->test);					\
		return false;						\
	}

#define TEST_IPV6_FAIL(ud, raddr, laddr, rport, lport)			\
	st = send_ipv6_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       true);					\
	if (st) {							\
		printk("%d: TCP neg test \"%s\" fail\n", __LINE__,	\
		       ud->test);					\
		return false;						\
	}

#define TEST_IPV4_FAIL(ud, raddr, laddr, rport, lport)			\
	st = send_ipv4_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       true);					\
	if (st) {							\
		printk("%d: TCP neg test \"%s\" fail\n", __LINE__,	\
		       ud->test);					\
		return false;						\
	}

	ud = REGISTER(AF_INET6, &any_addr6, &any_addr6, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
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

	/* Remote addr same as local addr, these two will never match */
	REGISTER(AF_INET6, &my_addr6, NULL, 1234, 4242);
	REGISTER(AF_INET, &my_addr4, NULL, 1234, 4242);

	/* IPv4 remote addr and IPv6 remote addr, impossible combination */
	REGISTER_FAIL(&my_addr4, &my_addr6, 1234, 4242);

	if (fail) {
		printk("Tests failed\n");
		return false;
	}

	i--;
	while (i) {
		ret = net_tcp_unregister(handlers[i]);
		if (ret < 0 && ret != -ENOENT) {
			printk("Cannot unregister tcp %d\n", i);
			return false;
		}

		i--;
	}

	if (!(net_tcp_unregister(NULL) < 0)) {
		printk("Unregister tcp failed\n");
		return false;
	}

	st = net_if_ipv6_addr_rm(iface, &in6addr_my);
	if (!st) {
		printk("Cannot remove %s from interface %p\n",
		       net_sprint_ipv6_addr(&in6addr_my), iface);
		return false;
	}

	st = net_if_ipv4_addr_rm(iface, &in4addr_my);
	if (!st) {
		printk("Cannot rm %s from interface %p\n",
		       net_sprint_ipv4_addr(&in4addr_my), iface);
		return false;
	}

	return true;
}

static bool v6_check_port_and_address(char *test_str, struct net_pkt *pkt,
				      const struct in6_addr *expected_dst_addr,
				      u16_t expected_dst_port)
{
	if (!net_ipv6_addr_cmp(&NET_IPV6_HDR(pkt)->src,
			       &my_v6_addr.sin6_addr)) {
		printk("%s: IPv6 source address mismatch, should be %s ",
		       test_str,
		       net_sprint_ipv6_addr(&my_v6_addr.sin6_addr));
		printk("was %s\n",
		       net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src));
		return false;
	}

	if (NET_TCP_HDR(pkt)->src_port != my_v6_addr.sin6_port) {
		printk("%s: IPv6 source port mismatch, %d vs %d\n",
		       test_str, ntohs(NET_TCP_HDR(pkt)->src_port),
		       ntohs(my_v6_addr.sin6_port));
		return false;
	}

	if (!net_ipv6_addr_cmp(expected_dst_addr, &NET_IPV6_HDR(pkt)->dst)) {
		printk("%s: IPv6 destination address mismatch, should be %s ",
		       test_str,
		       net_sprint_ipv6_addr(expected_dst_addr));
		printk("was %s\n",
		       net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
		return false;
	}

	if (NET_TCP_HDR(pkt)->dst_port != htons(expected_dst_port)) {
		printk("%s: IPv6 destination port mismatch, %d vs %d\n",
		       test_str, ntohs(NET_TCP_HDR(pkt)->dst_port),
		       expected_dst_port);
		return false;
	}

	return true;
}

static bool v4_check_port_and_address(char *test_str, struct net_pkt *pkt,
				      const struct in_addr *expected_dst_addr,
				      u16_t expected_dst_port)
{
	if (!net_ipv4_addr_cmp(&NET_IPV4_HDR(pkt)->src,
			       &my_v4_addr.sin_addr)) {
		printk("%s: IPv4 source address mismatch, should be %s ",
		       test_str,
		       net_sprint_ipv4_addr(&my_v4_addr.sin_addr));
		printk("was %s\n",
		       net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src));
		return false;
	}

	if (NET_TCP_HDR(pkt)->src_port != my_v4_addr.sin_port) {
		printk("%s: IPv4 source port mismatch, %d vs %d\n",
		       test_str, ntohs(NET_TCP_HDR(pkt)->src_port),
		       ntohs(my_v4_addr.sin_port));
		return false;
	}

	if (!net_ipv4_addr_cmp(expected_dst_addr, &NET_IPV4_HDR(pkt)->dst)) {
		printk("%s: IPv4 destination address mismatch, should be %s ",
		       test_str,
		       net_sprint_ipv4_addr(expected_dst_addr));
		printk("was %s\n",
		       net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));
		return false;
	}

	if (NET_TCP_HDR(pkt)->dst_port != htons(expected_dst_port)) {
		printk("%s: IPv4 destination port mismatch, %d vs %d\n",
		       test_str, ntohs(NET_TCP_HDR(pkt)->dst_port),
		       expected_dst_port);
		return false;
	}

	return true;
}

static bool test_create_v6_reset_packet(void)
{
	struct net_tcp *tcp = v6_ctx->tcp;
	u8_t flags = NET_TCP_RST;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv6", pkt);

	if (!(NET_TCP_FLAGS(pkt) & NET_TCP_RST)) {
		printk("Reset flag not set\n");
		return false;
	}

	if (!v6_check_port_and_address("TCP reset", pkt,
				       &peer_v6_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_create_v4_reset_packet(void)
{
	struct net_tcp *tcp = v4_ctx->tcp;
	u8_t flags = NET_TCP_RST;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv4", pkt);

	if (!(NET_TCP_FLAGS(pkt) & NET_TCP_RST)) {
		printk("Reset flag not set\n");
		return false;
	}

	if (!v4_check_port_and_address("TCP reset", pkt,
				       &peer_v4_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_create_v6_syn_packet(void)
{
	struct net_tcp *tcp = v6_ctx->tcp;
	u8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv6", pkt);

	if (!(NET_TCP_FLAGS(pkt) & NET_TCP_SYN)) {
		printk("SYN flag not set\n");
		return false;
	}

	if (!v6_check_port_and_address("TCP syn", pkt,
				       &peer_v6_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_create_v4_syn_packet(void)
{
	struct net_tcp *tcp = v4_ctx->tcp;
	u8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv4", pkt);

	if (!(NET_TCP_FLAGS(pkt) & NET_TCP_SYN)) {
		printk("Reset flag not set\n");
		return false;
	}

	if (!v4_check_port_and_address("TCP syn", pkt,
				       &peer_v4_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_create_v6_synack_packet(void)
{
	struct net_tcp *tcp = v6_ctx->tcp;
	u8_t flags = NET_TCP_SYN | NET_TCP_ACK;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv6", pkt);

	if (!((NET_TCP_FLAGS(pkt) & NET_TCP_SYN) &&
	      (NET_TCP_FLAGS(pkt) & NET_TCP_ACK))) {
		printk("SYN|ACK flag not set\n");
		return false;
	}

	if (!v6_check_port_and_address("TCP synack", pkt,
				       &peer_v6_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_create_v4_synack_packet(void)
{
	struct net_tcp *tcp = v4_ctx->tcp;
	u8_t flags = NET_TCP_SYN | NET_TCP_ACK;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv4", pkt);

	if (!((NET_TCP_FLAGS(pkt) & NET_TCP_SYN) &&
	      (NET_TCP_FLAGS(pkt) & NET_TCP_ACK))) {
		printk("SYN|ACK flag not set\n");
		return false;
	}

	if (!v4_check_port_and_address("TCP synack", pkt,
				       &peer_v4_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_create_v6_fin_packet(void)
{
	struct net_tcp *tcp = v6_ctx->tcp;
	u8_t flags = NET_TCP_FIN;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv6", pkt);

	if (!(NET_TCP_FLAGS(pkt) & NET_TCP_FIN)) {
		printk("FIN flag not set\n");
		return false;
	}

	if (!v6_check_port_and_address("TCP fin", pkt,
				       &peer_v6_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_create_v4_fin_packet(void)
{
	struct net_tcp *tcp = v4_ctx->tcp;
	u8_t flags = NET_TCP_FIN;
	struct net_pkt *pkt = NULL;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv4", pkt);

	if (!(NET_TCP_FLAGS(pkt) & NET_TCP_FIN)) {
		printk("FIN flag not set\n");
		return false;
	}

	if (!v4_check_port_and_address("TCP fin", pkt,
				       &peer_v4_inaddr, PEER_TCP_PORT)) {
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_v6_seq_check(void)
{
	struct net_tcp *tcp = v6_ctx->tcp;
	u8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	u32_t seq;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv6", pkt);

	seq = NET_TCP_HDR(pkt)->seq[0] << 24 |
		NET_TCP_HDR(pkt)->seq[1] << 16 |
		NET_TCP_HDR(pkt)->seq[2] << 8 |
		NET_TCP_HDR(pkt)->seq[3];
	if (seq != (tcp->send_seq - 1)) {
		printk("Seq does not match (%u vs %u)\n",
		       seq + 1, tcp->send_seq);
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_v4_seq_check(void)
{
	struct net_tcp *tcp = v4_ctx->tcp;
	u8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	u32_t seq;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		printk("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_hexdump_frags("TCPv4", pkt);

	seq = NET_TCP_HDR(pkt)->seq[0] << 24 |
		NET_TCP_HDR(pkt)->seq[1] << 16 |
		NET_TCP_HDR(pkt)->seq[2] << 8 |
		NET_TCP_HDR(pkt)->seq[3];
	if (seq != (tcp->send_seq - 1)) {
		printk("Seq does not match (%u vs %u)\n",
		       seq + 1, tcp->send_seq);
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

#if 0
static void connect_v6_cb(struct net_context *context, void *user_data)
{
	if (POINTER_TO_INT(user_data) != AF_INET6) {
		fail = true;
	}

	fail = false;

	connect_cb_called = true;

	DBG("IPv6 connect cb called\n");
}

static void connect_v4_cb(struct net_context *context, void *user_data)
{
	if (POINTER_TO_INT(user_data) != AF_INET) {
		fail = true;
	}

	fail = false;

	connect_cb_called = true;
	k_sem_give(&wait_connect);

	DBG("IPv4 connect cb called\n");
}
#endif

#if 0
static bool test_create_v6_data_packet(void)
{

	return true;
}
#endif

struct net_tcp_context net_tcp_context_data;
struct net_tcp_context net_tcp_context_data_peer;

static struct net_if_api net_tcp_if_api = {
	.init = net_tcp_iface_init,
	.send = tester_send,
};

static struct net_if_api net_tcp_if_api_peer = {
	.init = net_tcp_iface_init,
	.send = tester_send_peer,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_tcp_test, "net_tcp_test", host,
		net_tcp_dev_init, &net_tcp_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_tcp_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(net_tcp_test_peer, "net_tcp_test_peer", peer,
		 net_tcp_dev_init, &net_tcp_context_data_peer, NULL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		 &net_tcp_if_api_peer, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

static bool test_init_tcp_context(void)
{
	struct net_if *iface = net_if_get_default();
	struct net_if_addr *ifaddr;
	int ret;

	if (!iface) {
		TC_ERROR("Interface is NULL\n");
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &my_v6_inaddr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv6_addr(&my_v6_inaddr), iface);
		return false;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &my_v4_inaddr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv4_addr(&my_v4_inaddr), iface);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context get v6 test failed.\n");
		return false;
	}

	if (!v6_ctx) {
		TC_ERROR("Got NULL v6 context\n");
		return false;
	}

	net_ipaddr_copy(&my_v6_addr.sin6_addr, &my_v6_inaddr);
	my_v6_addr.sin6_family = AF_INET6;
	my_v6_addr.sin6_port = htons(MY_TCP_PORT);

	net_ipaddr_copy(&peer_v6_addr.sin6_addr, &peer_v6_inaddr);
	peer_v6_addr.sin6_family = AF_INET6;
	peer_v6_addr.sin6_port = htons(PEER_TCP_PORT);

	ret = net_context_bind(v6_ctx, (struct sockaddr *)&my_v6_addr,
			       sizeof(my_v6_addr));
	if (ret) {
		TC_ERROR("Context bind v6 test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context get v4 test failed.\n");
		return false;
	}

	if (!v4_ctx) {
		TC_ERROR("Got NULL v4 context\n");
		return false;
	}

	net_ipaddr_copy(&my_v4_addr.sin_addr, &my_v4_inaddr);
	my_v4_addr.sin_family = AF_INET;
	my_v4_addr.sin_port = htons(MY_TCP_PORT);

	net_ipaddr_copy(&peer_v4_addr.sin_addr, &peer_v4_inaddr);
	peer_v4_addr.sin_family = AF_INET;
	peer_v4_addr.sin_port = htons(PEER_TCP_PORT);

	ret = net_context_bind(v4_ctx, (struct sockaddr *)&my_v4_addr,
			       sizeof(my_v4_addr));
	if (ret) {
		TC_ERROR("Context bind v4 test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static bool test_init_tcp_reply_context(void)
{
	struct net_if *iface = net_if_get_default() + 1;
	struct net_if_addr *ifaddr;
	int ret;

	ifaddr = net_if_ipv6_addr_add(iface, &peer_v6_inaddr, NET_ADDR_MANUAL,
				      0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv6_addr(&peer_v6_inaddr), iface);
		return false;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &peer_v4_inaddr, NET_ADDR_MANUAL,
				      0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv4_addr(&peer_v4_inaddr), iface);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
			      &reply_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context get reply v6 test failed.\n");
		return false;
	}

	if (!reply_v6_ctx) {
		TC_ERROR("Got NULL reply v6 context\n");
		return false;
	}

	ret = net_context_bind(reply_v6_ctx, (struct sockaddr *)&peer_v6_addr,
			       sizeof(peer_v6_addr));
	if (ret) {
		TC_ERROR("Context bind reply v6 test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			      &reply_v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context get reply v4 test failed.\n");
		return false;
	}

	if (!reply_v4_ctx) {
		TC_ERROR("Got NULL reply v4 context\n");
		return false;
	}

	ret = net_context_bind(reply_v4_ctx, (struct sockaddr *)&peer_v4_addr,
			       sizeof(peer_v4_addr));
	if (ret) {
		TC_ERROR("Context bind reply v4 test failed (%d)\n", ret);
		return false;
	}

	return true;
}

static void accept_v6_cb(struct net_context *new_context,
			 struct sockaddr *addr,
			 socklen_t addrlen,
			 int error,
			 void *user_data)
{
	DBG("error %d\n", error);
}

static void accept_v4_cb(struct net_context *new_context,
			 struct sockaddr *addr,
			 socklen_t addrlen,
			 int error,
			 void *user_data)
{
	DBG("error %d\n", error);
}

static bool test_init_tcp_accept(void)
{
	int ret;

	ret = net_context_listen(reply_v6_ctx, 0);
	if (ret) {
		TC_ERROR("Context listen v6 test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_accept(reply_v6_ctx, accept_v6_cb, 0,
				 INT_TO_POINTER(AF_INET6));
	if (ret) {
		TC_ERROR("Context accept v6 test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_listen(reply_v4_ctx, 0);
	if (ret) {
		TC_ERROR("Context listen v4 test failed (%d)\n", ret);
		return false;
	}

	ret = net_context_accept(reply_v4_ctx, accept_v4_cb, 0,
				 INT_TO_POINTER(AF_INET));
	if (ret) {
		TC_ERROR("Context accept v4 test failed (%d)\n", ret);
		return false;
	}

	DBG("Waiting a connection...\n");

	return true;
}

#if 0
static bool test_init_tcp_connect(void)
{
	int ret;

	fail = false;
	syn_v6_sent = true;

	ret = net_context_connect(v6_ctx, (struct sockaddr *)&peer_v6_addr,
				  sizeof(peer_v6_addr), connect_v6_cb,
				  0, INT_TO_POINTER(AF_INET6));
	if (ret) {
		TC_ERROR("Context connect v6 test failed (%d)\n", ret);
		return false;
	}

	if (k_sem_take(&wait_in_accept, WAIT_TIME_LONG)) {
		TC_ERROR("Timeout while waiting data back\n");
		return false;
	}

	if (!accept_cb_called) {
		TC_ERROR("No IPv6 accept cb called on time, "
			 "TCP accept test failed\n");
		return false;
	}
	accept_cb_called = false;

	ret = net_context_connect(v6_ctx, (struct sockaddr *)&my_v6_addr,
				  sizeof(my_v6_addr), connect_v6_cb,
				  0, INT_TO_POINTER(AF_INET6));
	if (ret || fail) {
		TC_ERROR("Context connect v6 test failed (%d)\n", ret);
		return false;
	}

	DBG("Waiting v6 connection\n");

	if (k_sem_take(&wait_connect, WAIT_TIME_LONG)) {
		TC_ERROR("Timeout while waiting data back\n");
		return false;
	}

	if (!connect_cb_called) {
		TC_ERROR("No IPv6 connect cb called on time, "
			 "TCP connect test failed\n");
		return false;
	}
	connect_cb_called = false;

	ret = net_context_connect(v4_ctx, (struct sockaddr *)&my_v4_addr,
				  sizeof(my_v4_addr), connect_v4_cb,
				  0, INT_TO_POINTER(AF_INET));
	if (ret || fail) {
		TC_ERROR("Context connect v4 test failed (%d)\n", ret);
		return false;
	}

	k_sem_take(&wait_connect, WAIT_TIME);
	if (!connect_cb_called) {
		TC_ERROR("No IPv4 connect cb called on time, "
			 "TCP connect test failed\n");
		return false;
	}
	connect_cb_called = false;

	return true;
}
#endif

static bool test_init(void)
{
	net_ipaddr_copy(&any_addr6.sin6_addr, &in6addr_any);
	any_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&any_addr4.sin_addr, &in4addr_any);
	any_addr4.sin_family = AF_INET;

	k_sem_init(&wait_connect, 0, UINT_MAX);

	return true;
}

static bool test_cleanup(void)
{
	int ret;

	ret = net_context_put(v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context free v6 failed.\n");
		return false;
	}

	ret = net_context_put(v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context free v4 failed.\n");
		return false;
	}

	ret = net_context_put(reply_v6_ctx);
	if (ret != 0) {
		TC_ERROR("Context free reply v6 failed.\n");
		return false;
	}

	ret = net_context_put(reply_v4_ctx);
	if (ret != 0) {
		TC_ERROR("Context free reply v4 failed.\n");
		return false;
	}

	return true;
}

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "test TCP init", test_init },
	{ "test TCP register/unregister port cb", test_register },
	{ "test TCP context init", test_init_tcp_context },
	{ "test IPv6 TCP reset packet creation", test_create_v6_reset_packet },
	{ "test IPv4 TCP reset packet creation", test_create_v4_reset_packet },
	{ "test IPv6 TCP syn packet creation", test_create_v6_syn_packet },
	{ "test IPv4 TCP syn packet creation", test_create_v4_syn_packet },
	{ "test IPv6 TCP synack packet create", test_create_v6_synack_packet },
	{ "test IPv4 TCP synack packet create", test_create_v4_synack_packet },
	{ "test IPv6 TCP fin packet creation", test_create_v6_fin_packet },
	{ "test IPv4 TCP fin packet creation", test_create_v4_fin_packet },
	{ "test IPv6 TCP seq check", test_v6_seq_check },
	{ "test IPv4 TCP seq check", test_v4_seq_check },
	{ "test TCP reply context init", test_init_tcp_reply_context },
	{ "test TCP accept init", test_init_tcp_accept },
#if 0
	{ "test TCP connect init", test_init_tcp_connect },
	{ "test IPv6 TCP data packet creation", test_create_v6_data_packet },
	{ "test IPv4 TCP data packet creation", test_create_v4_data_packet },
#endif
	{ "test cleanup", test_cleanup },
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
