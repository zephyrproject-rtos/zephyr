/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_TCP_LOG_LEVEL);

#include <zephyr.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <sys/printk.h>
#include <net/buf.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/dummy.h>

#include <tc_util.h>

#if defined(CONFIG_NET_TCP_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define NET_LOG_ENABLED 1
#else
#define DBG(fmt, ...)
#endif

#include "tcp_internal.h"
#include "net_private.h"
#include "ipv6.h"
#include "ipv4.h"

static bool test_failed;
static bool fail = true;
static struct k_sem recv_lock;
static struct net_context *v6_ctx;
static struct net_context *reply_v6_ctx;
static struct net_context *v4_ctx;
static struct net_context *reply_v4_ctx;

static struct sockaddr_in6 any_addr6;
static const struct in6_addr sin6_addr_any = IN6ADDR_ANY_INIT;

static struct sockaddr_in any_addr4;
static const struct in_addr sin_addr_any = INADDR_ANY_INIT;

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

static struct net_if *my_iface;
static struct net_if *peer_iface;

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
};

int net_tcp_dev_init(const struct device *dev)
{
	return 0;
}

static void net_tcp_iface_init(struct net_if *iface)
{
	static uint8_t mac_addr_1[6];
	static uint8_t mac_addr_2[6];

	if (mac_addr_1[0] == 0U) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		mac_addr_1[0] = 0x00;
		mac_addr_1[1] = 0x00;
		mac_addr_1[2] = 0x5E;
		mac_addr_1[3] = 0x00;
		mac_addr_1[4] = 0x53;
		mac_addr_1[5] = 0x01;

		net_if_set_link_addr(iface, mac_addr_1, 6,
				     NET_LINK_ETHERNET);
	}

	if (mac_addr_2[0] == 0U) {
		mac_addr_2[0] = 0x00;
		mac_addr_2[1] = 0x00;
		mac_addr_2[2] = 0x5E;
		mac_addr_2[3] = 0x00;
		mac_addr_2[4] = 0x53;
		mac_addr_2[5] = 0x02;

		net_if_set_link_addr(iface, mac_addr_2, 6,
				     NET_LINK_ETHERNET);
	}

	return;
}

struct net_tcp_hdr *net_tcp_get_hdr(struct net_pkt *pkt,
				    struct net_tcp_hdr *hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(tcp_access, struct net_tcp_hdr);
	struct net_pkt_cursor backup;
	struct net_tcp_hdr *tcp_hdr;
	bool overwrite;

	tcp_access.data = hdr;

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ipv6_ext_len(pkt))) {
		tcp_hdr = NULL;
		goto out;
	}

	tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);

out:
	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, overwrite);

	return tcp_hdr;
}

struct net_tcp_hdr *net_tcp_set_hdr(struct net_pkt *pkt,
				    struct net_tcp_hdr *hdr)
{
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
	struct net_pkt_cursor backup;
	struct net_tcp_hdr *tcp_hdr;
	bool overwrite;

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ipv6_ext_len(pkt))) {
		tcp_hdr = NULL;
		goto out;
	}

	tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);
	if (!tcp_hdr) {
		goto out;
	}

	memcpy(tcp_hdr, hdr, sizeof(struct net_tcp_hdr));

	net_pkt_set_data(pkt, &tcp_access);
out:
	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, overwrite);

	return tcp_hdr == NULL ? NULL : hdr;
}

static void v6_send_syn_ack(struct net_pkt *req)
{
	struct net_if *iface = net_pkt_iface(req);
	struct net_pkt *rsp = NULL;
	struct net_tcp_hdr *tcp_rsp, *tcp_req;
	int ret;

	ret = net_tcp_prepare_segment(reply_v6_ctx->tcp,
				      NET_TCP_SYN | NET_TCP_ACK, NULL, 0,
				      NULL, (struct sockaddr *)&my_v6_addr,
				      &rsp);
	if (ret) {
		DBG("TCP packet creation failed\n");
		return;
	}

	tcp_rsp = net_tcp_get_hdr(rsp, NULL);
	tcp_req = net_tcp_get_hdr(req, NULL);

	DBG("1) rsp src %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->src),
	    ntohs(tcp_rsp->src_port));
	DBG("1) rsp dst %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->dst),
	    ntohs(tcp_rsp->dst_port));

	net_ipaddr_copy(&NET_IPV6_HDR(rsp)->src, &NET_IPV6_HDR(req)->dst);
	net_ipaddr_copy(&NET_IPV6_HDR(rsp)->dst, &NET_IPV6_HDR(req)->src);

	tcp_rsp->src_port = tcp_req->dst_port;
	tcp_rsp->dst_port = tcp_req->src_port;

	DBG("rsp src %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->src),
	    ntohs(tcp_rsp->src_port));
	DBG("rsp dst %s/%d\n", net_sprint_ipv6_addr(&NET_IPV6_HDR(rsp)->dst),
	    ntohs(tcp_rsp->dst_port));

	net_pkt_hexdump(req, "request TCPv6");
	net_pkt_hexdump(rsp, "reply   TCPv6");

	ret =  net_recv_data(iface, rsp);
	if (!ret) {
		net_pkt_unref(rsp);
	}

	k_sem_give(&wait_connect);
}

static int send_status = -EINVAL;

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}
	if (syn_v6_sent && net_pkt_family(pkt) == AF_INET6) {
		DBG("v6 SYN was sent successfully\n");
		syn_v6_sent = false;
		v6_send_syn_ack(pkt);
	}

	send_status = 0;

	return 0;
}

static int tester_send_peer(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	DBG("Peer data was sent successfully\n");

	return 0;
}

static inline struct in_addr *if_get_addr(struct net_if *iface)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->config.ip.ipv4->unicast[i].is_used &&
		    iface->config.ip.ipv4->unicast[i].address.family ==
								AF_INET &&
		    iface->config.ip.ipv4->unicast[i].addr_state ==
							NET_ADDR_PREFERRED) {
			return
			    &iface->config.ip.ipv4->unicast[i].address.in_addr;
		}
	}

	return NULL;
}

struct ud {
	const struct sockaddr *remote_addr;
	const struct sockaddr *local_addr;
	uint16_t remote_port;
	uint16_t local_port;
	char *test;
	struct net_conn_handle *handle;
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
				  union net_ip_header *ip_hdr,
				  union net_proto_header *proto_hdr,
				  void *user_data)
{
	/* This function should never be called as there should not
	 * be a matching TCP connection.
	 */

	fail = true;
	return NET_DROP;
}

static uint8_t data[] = { 'f', 'o', 'o', 'b', 'a', 'r' };

static struct net_pkt *setup_ipv6_tcp(struct net_if *iface,
				      struct in6_addr *remote_addr,
				      struct in6_addr *local_addr,
				      uint16_t remote_port,
				      uint16_t local_port)
{
	struct net_pkt *pkt;
	struct net_tcp_hdr tcp_hdr = { 0 };

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(data),
					AF_INET6, IPPROTO_TCP, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	if (net_ipv6_create(pkt, remote_addr, local_addr)) {
		net_pkt_unref(pkt);
		return NULL;
	}

	tcp_hdr.src_port = htons(remote_port);
	tcp_hdr.dst_port = htons(local_port);

	net_pkt_write(pkt, &tcp_hdr, sizeof(struct net_tcp_hdr));
	net_pkt_write(pkt, data, sizeof(data));

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_TCP);

	return pkt;
}

static struct net_pkt *setup_ipv4_tcp(struct net_if *iface,
				      struct in_addr *remote_addr,
				      struct in_addr *local_addr,
				      uint16_t remote_port,
				      uint16_t local_port)
{
	struct net_pkt *pkt;
	struct net_tcp_hdr tcp_hdr = { 0 };

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(data),
					AF_INET, IPPROTO_TCP, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	if (net_ipv4_create(pkt, remote_addr, local_addr)) {
		net_pkt_unref(pkt);
		return NULL;
	}

	tcp_hdr.src_port = htons(remote_port);
	tcp_hdr.dst_port = htons(local_port);

	net_pkt_write(pkt, &tcp_hdr, sizeof(struct net_tcp_hdr));
	net_pkt_write(pkt, data, sizeof(data));

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, IPPROTO_TCP);

	return pkt;
}

uint8_t ipv6_hop_by_hop_ext_hdr[] = {
/* Next header TCP */
0x06,
/* Length (multiple of 8 octets) */
0x08,
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
/* Padding */
0x00,
/* Padding */
0x00,
};

static struct net_pkt *setup_ipv6_tcp_long(struct net_if *iface,
					   struct in6_addr *remote_addr,
					   struct in6_addr *local_addr,
					   uint16_t remote_port,
					   uint16_t local_port)
{
	struct net_pkt *pkt;
	struct net_tcp_hdr tcp_hdr = { 0 };

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(ipv6_hop_by_hop_ext_hdr) +
					sizeof(data),
					AF_INET6, IPPROTO_TCP, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_ipv6_hop_limit(pkt, 255);
	net_ipv6_create(pkt, remote_addr, local_addr);

	net_pkt_write(pkt, ipv6_hop_by_hop_ext_hdr,
		      sizeof(ipv6_hop_by_hop_ext_hdr));

	net_pkt_set_ipv6_ext_len(pkt, sizeof(ipv6_hop_by_hop_ext_hdr));

	tcp_hdr.src_port = htons(remote_port);
	tcp_hdr.dst_port = htons(local_port);

	net_pkt_write(pkt, &tcp_hdr, sizeof(struct net_tcp_hdr));
	net_pkt_write(pkt, data, sizeof(data));

	net_pkt_set_ipv6_next_hdr(pkt, 0); /* hop-by-hop option */

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_TCP);

	return pkt;
}

#define TIMEOUT K_MSEC(200)

static bool send_ipv6_tcp_msg(struct net_if *iface,
			      struct in6_addr *src,
			      struct in6_addr *dst,
			      uint16_t src_port,
			      uint16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = setup_ipv6_tcp(iface, src, dst, src_port, dst_port);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		DBG("Cannot recv pkt %p, ret %d\n", pkt, ret);
		return false;
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {
		if (expect_failure) {
			return true;
		} else {
			DBG("Timeout, packet not received\n");
			return false;
		}
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		DBG("IPv6 wrong user data %p returned, expected %p\n",
		    returned_ud, ud);
		return false;
	}

	return !fail;
}

static bool send_ipv4_tcp_msg(struct net_if *iface,
			      struct in_addr *src,
			      struct in_addr *dst,
			      uint16_t src_port,
			      uint16_t dst_port,
			      struct ud *ud,
			      bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = setup_ipv4_tcp(iface, src, dst, src_port, dst_port);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		DBG("Cannot recv pkt %p, ret %d\n", pkt, ret);
		return false;
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {
		if (expect_failure) {
			return true;
		} else {
			DBG("Timeout, packet not received\n");
			return false;
		}
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		DBG("IPv4 wrong user data %p returned, expected %p\n",
		    returned_ud, ud);
		return false;
	}

	return !fail;
}

static bool send_ipv6_tcp_long_msg(struct net_if *iface,
				   struct in6_addr *src,
				   struct in6_addr *dst,
				   uint16_t src_port,
				   uint16_t dst_port,
				   struct ud *ud,
				   bool expect_failure)
{
	struct net_pkt *pkt;
	int ret;

	pkt = setup_ipv6_tcp_long(iface, src, dst, src_port, dst_port);

	ret = net_recv_data(iface, pkt);
	if (ret < 0) {
		DBG("Cannot recv pkt %p, ret %d\n", pkt, ret);
		return false;
	}

	if (k_sem_take(&recv_lock, TIMEOUT)) {
		if (expect_failure) {
			return true;
		} else {
			DBG("Timeout, packet not received\n");
			return false;
		}
	}

	/* Check that the returned user data is the same as what was given
	 * as a parameter.
	 */
	if (ud != returned_ud && !expect_failure) {
		DBG("IPv6 wrong user data %p returned, expected %p\n",
		    returned_ud, ud);
		return false;
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

static bool test_register(void)
{
	struct net_conn_handle *handlers[CONFIG_NET_MAX_CONN];
	struct net_if *iface = my_iface;
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
		DBG("Cannot add %s to interface %p\n",
		    net_sprint_ipv6_addr(&in6addr_my), iface);
		return false;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add %s to interface %p\n",
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
		ret = net_tcp_register(family,				\
				       (struct sockaddr *)raddr,	\
				       (struct sockaddr *)laddr,	\
				       rport, lport,			\
				       test_ok, &user_data,		\
				       &handlers[i]);			\
		if (ret) {						\
			DBG("TCP register %s failed (%d)\n",		\
			    user_data.test, ret);			\
			return false;					\
		}							\
		user_data.handle = handlers[i++];			\
		&user_data;						\
	})

#define REGISTER_FAIL(raddr, laddr, rport, lport)			\
	ret = net_tcp_register(AF_INET,					\
			       (struct sockaddr *)raddr,		\
			       (struct sockaddr *)laddr,		\
			       rport, lport,				\
			       test_fail, INT_TO_POINTER(0), NULL);	\
	if (!ret) {							\
		DBG("TCP register invalid match %s failed\n",		\
		    #raddr"-"#laddr"-"#rport"-"#lport);			\
		return false;						\
	}

#define UNREGISTER(ud)							\
	ret = net_tcp_unregister(ud->handle);				\
	if (ret) {							\
		DBG("TCP unregister %p failed (%d)\n", ud->handle,	\
		    ret);						\
		return false;						\
	}

#define TEST_IPV6_OK(ud, raddr, laddr, rport, lport)			\
	st = send_ipv6_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       false);					\
	if (!st) {							\
		DBG("%d: TCP test \"%s\" fail\n", __LINE__,		\
		    ud->test);						\
		return false;						\
	}

#define TEST_IPV4_OK(ud, raddr, laddr, rport, lport)			\
	st = send_ipv4_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       false);					\
	if (!st) {							\
		DBG("%d: TCP test \"%s\" fail\n", __LINE__,		\
		    ud->test);						\
		return false;						\
	}

#define TEST_IPV6_LONG_OK(ud, raddr, laddr, rport, lport)		\
	st = send_ipv6_tcp_long_msg(iface, raddr, laddr, rport, lport, ud, \
			       false);					\
	if (!st) {							\
		DBG("%d: TCP long test \"%s\" fail\n", __LINE__,	\
		    ud->test);						\
		return false;						\
	}

#define TEST_IPV4_LONG_OK(ud, raddr, laddr, rport, lport)		\
	st = send_ipv4_tcp_long_msg(iface, raddr, laddr, rport, lport, ud, \
			       false);					\
	if (!st) {							\
		DBG("%d: TCP long_test \"%s\" fail\n", __LINE__,	\
		    ud->test);						\
		return false;						\
	}

#define TEST_IPV6_FAIL(ud, raddr, laddr, rport, lport)			\
	st = send_ipv6_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       true);					\
	if (!st) {							\
		DBG("%d: TCP neg test \"%s\" fail\n", __LINE__,		\
		    ud->test);						\
		return false;						\
	}

#define TEST_IPV4_FAIL(ud, raddr, laddr, rport, lport)			\
	st = send_ipv4_tcp_msg(iface, raddr, laddr, rport, lport, ud,	\
			       true);					\
	if (!st) {							\
		DBG("%d: TCP neg test \"%s\" fail\n", __LINE__,		\
		    ud->test);						\
		return false;						\
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
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
	TEST_IPV6_LONG_OK(ud, &in6addr_peer, &in6addr_my, 1234, 4242);
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

	if (fail) {
		DBG("Tests failed\n");
		return false;
	}

	i--;
	while (i) {
		ret = net_tcp_unregister(handlers[i]);
		if (ret < 0 && ret != -ENOENT) {
			DBG("Cannot unregister tcp %d\n", i);
			return false;
		}

		i--;
	}

	if (!(net_tcp_unregister(NULL) < 0)) {
		DBG("Unregister tcp failed\n");
		return false;
	}

	st = net_if_ipv6_addr_rm(iface, &in6addr_my);
	if (!st) {
		DBG("Cannot remove %s from interface %p\n",
		    net_sprint_ipv6_addr(&in6addr_my), iface);
		return false;
	}

	st = net_if_ipv4_addr_rm(iface, &in4addr_my);
	if (!st) {
		DBG("Cannot rm %s from interface %p\n",
		    net_sprint_ipv4_addr(&in4addr_my), iface);
		return false;
	}

	return true;
}

static bool v6_check_port_and_address(char *test_str, struct net_pkt *pkt,
				      const struct in6_addr *expected_dst_addr,
				      uint16_t expected_dst_port)
{
	struct net_tcp_hdr *tcp_hdr;

	tcp_hdr = net_tcp_get_hdr(pkt, NULL);

	if (!net_ipv6_addr_cmp(&NET_IPV6_HDR(pkt)->src,
			       &my_v6_addr.sin6_addr)) {
		DBG("%s: IPv6 source address mismatch, should be %s ",
		    test_str, net_sprint_ipv6_addr(&my_v6_addr.sin6_addr));
		DBG("was %s\n",
		    net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src));
		return false;
	}

	if (tcp_hdr->src_port != my_v6_addr.sin6_port) {
		DBG("%s: IPv6 source port mismatch, %d vs %d\n",
		    test_str, ntohs(tcp_hdr->src_port),
		    ntohs(my_v6_addr.sin6_port));
		return false;
	}

	if (!net_ipv6_addr_cmp(expected_dst_addr, &NET_IPV6_HDR(pkt)->dst)) {
		DBG("%s: IPv6 destination address mismatch, should be %s ",
		    test_str, net_sprint_ipv6_addr(expected_dst_addr));
		DBG("was %s\n",
		    net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
		return false;
	}

	if (tcp_hdr->dst_port != htons(expected_dst_port)) {
		DBG("%s: IPv6 destination port mismatch, %d vs %d\n",
		    test_str, ntohs(tcp_hdr->dst_port),
		    expected_dst_port);
		return false;
	}

	return true;
}

static bool v4_check_port_and_address(char *test_str, struct net_pkt *pkt,
				      const struct in_addr *expected_dst_addr,
				      uint16_t expected_dst_port)
{
	struct net_tcp_hdr *tcp_hdr;

	tcp_hdr = net_tcp_get_hdr(pkt, NULL);

	if (!net_ipv4_addr_cmp(&NET_IPV4_HDR(pkt)->src,
			       &my_v4_addr.sin_addr)) {
		DBG("%s: IPv4 source address mismatch, should be %s ",
		    test_str, net_sprint_ipv4_addr(&my_v4_addr.sin_addr));
		DBG("was %s\n",
		    net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src));
		return false;
	}

	if (tcp_hdr->src_port != my_v4_addr.sin_port) {
		DBG("%s: IPv4 source port mismatch, %d vs %d\n",
		    test_str, ntohs(tcp_hdr->src_port),
		    ntohs(my_v4_addr.sin_port));
		return false;
	}

	if (!net_ipv4_addr_cmp(expected_dst_addr, &NET_IPV4_HDR(pkt)->dst)) {
		DBG("%s: IPv4 destination address mismatch, should be %s ",
		    test_str, net_sprint_ipv4_addr(expected_dst_addr));
		DBG("was %s\n",
		    net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));
		return false;
	}

	if (tcp_hdr->dst_port != htons(expected_dst_port)) {
		DBG("%s: IPv4 destination port mismatch, %d vs %d\n",
		    test_str, ntohs(tcp_hdr->dst_port),
		    expected_dst_port);
		return false;
	}

	return true;
}

static bool test_create_v6_reset_packet(void)
{
	struct net_tcp *tcp = v6_ctx->tcp;
	uint8_t flags = NET_TCP_RST;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv6");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!(NET_TCP_FLAGS(tcp_hdr) & NET_TCP_RST)) {
		DBG("Reset flag not set\n");
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
	uint8_t flags = NET_TCP_RST;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv4");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!(NET_TCP_FLAGS(tcp_hdr) & NET_TCP_RST)) {
		DBG("Reset flag not set\n");
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
	uint8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv6");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!(NET_TCP_FLAGS(tcp_hdr) & NET_TCP_SYN)) {
		DBG("SYN flag not set\n");
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
	uint8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv4");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!(NET_TCP_FLAGS(tcp_hdr) & NET_TCP_SYN)) {
		DBG("Reset flag not set\n");
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
	uint8_t flags = NET_TCP_SYN | NET_TCP_ACK;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv6");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!((NET_TCP_FLAGS(tcp_hdr) & NET_TCP_SYN) &&
	      (NET_TCP_FLAGS(tcp_hdr) & NET_TCP_ACK))) {
		DBG("SYN|ACK flag not set\n");
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
	uint8_t flags = NET_TCP_SYN | NET_TCP_ACK;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv4");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!((NET_TCP_FLAGS(tcp_hdr) & NET_TCP_SYN) &&
	      (NET_TCP_FLAGS(tcp_hdr) & NET_TCP_ACK))) {
		DBG("SYN|ACK flag not set\n");
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
	uint8_t flags = NET_TCP_FIN;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv6");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!(NET_TCP_FLAGS(tcp_hdr) & NET_TCP_FIN)) {
		DBG("FIN flag not set\n");
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
	uint8_t flags = NET_TCP_FIN;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	net_pkt_hexdump(pkt, "TCPv4");

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return false;
	}

	if (!(NET_TCP_FLAGS(tcp_hdr) & NET_TCP_FIN)) {
		DBG("FIN flag not set\n");
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
	uint8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr *tcp_hdr;
	uint32_t seq;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	tcp_hdr = net_tcp_get_hdr(pkt, NULL);

	net_pkt_hexdump(pkt, "TCPv6");

	seq = tcp_hdr->seq[0] << 24 |
		tcp_hdr->seq[1] << 16 |
		tcp_hdr->seq[2] << 8 |
		tcp_hdr->seq[3];
	if (seq != tcp->send_seq) {
		DBG("Seq does not match (%u vs %u)\n",
		    seq, tcp->send_seq);
		return false;
	}

	net_pkt_unref(pkt);

	return true;
}

static bool test_v4_seq_check(void)
{
	struct net_tcp *tcp = v4_ctx->tcp;
	uint8_t flags = NET_TCP_SYN;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr *tcp_hdr;
	uint32_t seq;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v4_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	tcp_hdr = net_tcp_get_hdr(pkt, NULL);

	net_pkt_hexdump(pkt, "TCPv4");

	seq = tcp_hdr->seq[0] << 24 |
		tcp_hdr->seq[1] << 16 |
		tcp_hdr->seq[2] << 8 |
		tcp_hdr->seq[3];
	if (seq != tcp->send_seq) {
		DBG("Seq does not match (%u vs %u)\n",
		    seq, tcp->send_seq);
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

static struct dummy_api net_tcp_if_api = {
	.iface_api.init = net_tcp_iface_init,
	.send = tester_send,
};

static struct dummy_api net_tcp_if_api_peer = {
	.iface_api.init = net_tcp_iface_init,
	.send = tester_send_peer,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_tcp_test, "net_tcp_test", host,
			 net_tcp_dev_init, device_pm_control_nop,
			 &net_tcp_context_data, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_tcp_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(net_tcp_test_peer, "net_tcp_test_peer", peer,
			 net_tcp_dev_init, device_pm_control_nop,
			 &net_tcp_context_data_peer, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_tcp_if_api_peer,
			 _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

static bool test_init_tcp_context(void)
{
	struct net_if *iface = my_iface;
	struct net_if_addr *ifaddr;
	int ret;

	ifaddr = net_if_ipv6_addr_add(iface, &my_v6_inaddr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add %s to interface %p\n",
		       net_sprint_ipv6_addr(&my_v6_inaddr), iface);
		return false;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &my_v4_inaddr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add %s to interface %p\n",
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

/* Receive window helper function copied from tcp.c */
static inline uint32_t get_recv_wnd(struct net_tcp *tcp)
{
	ARG_UNUSED(tcp);

	/* We don't queue received data inside the stack, we hand off
	 * packets to synchronous callbacks (who can queue if they
	 * want, but it's not our business).  So the available window
	 * size is always the same.  There are two configurables to
	 * check though.
	 */
	return MIN(NET_TCP_MAX_WIN, NET_TCP_BUF_MAX_LEN);
}

static bool test_tcp_seq_validity(void)
{
	struct net_tcp *tcp = v6_ctx->tcp;
	uint8_t flags = NET_TCP_RST;
	struct net_pkt *pkt = NULL;
	struct net_tcp_hdr hdr, *tcp_hdr;
	int ret;

	ret = net_tcp_prepare_segment(tcp, flags, NULL, 0, NULL,
				      (struct sockaddr *)&peer_v6_addr, &pkt);
	if (ret) {
		DBG("Prepare segment failed (%d)\n", ret);
		return false;
	}

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		DBG("Grabbing TCP hdr failed\n");
		return false;
	}

	tcp->send_ack = sys_get_be32(tcp_hdr->seq) -
		get_recv_wnd(tcp) / 2U;
	if (!net_tcp_validate_seq(tcp, tcp_hdr)) {
		DBG("1) Sequence validation failed (send_ack %u vs seq %u)\n",
		    tcp->send_ack, sys_get_be32(tcp_hdr->seq));
		return false;
	}

	tcp->send_ack = sys_get_be32(tcp_hdr->seq);
	if (!net_tcp_validate_seq(tcp, tcp_hdr)) {
		DBG("2) Sequence validation failed (send_ack %u vs seq %u)\n",
		    tcp->send_ack, sys_get_be32(tcp_hdr->seq));
		return false;
	}

	tcp->send_ack = sys_get_be32(tcp_hdr->seq) +
		get_recv_wnd(tcp) * 2U;
	if (net_tcp_validate_seq(tcp, tcp_hdr)) {
		DBG("3) Sequence validation failed (send_ack %u vs seq %u)\n",
		    tcp->send_ack, sys_get_be32(tcp_hdr->seq));
		return false;
	}

	tcp->send_ack = sys_get_be32(tcp_hdr->seq) -
		get_recv_wnd(tcp) * 2U;
	if (net_tcp_validate_seq(tcp, tcp_hdr)) {
		DBG("4) Sequence validation failed (send_ack %u vs seq %u)\n",
		    tcp->send_ack, sys_get_be32(tcp_hdr->seq));
		return false;
	}

	return true;
}

static bool test_init_tcp_reply_context(void)
{
	struct net_if *iface = peer_iface;
	struct net_if_addr *ifaddr;
	int ret;

	ifaddr = net_if_ipv6_addr_add(iface, &peer_v6_inaddr, NET_ADDR_MANUAL,
				      0);
	if (!ifaddr) {
		DBG("Cannot add %s to interface %p\n",
		    net_sprint_ipv6_addr(&peer_v6_inaddr), iface);
		return false;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &peer_v4_inaddr, NET_ADDR_MANUAL,
				      0);
	if (!ifaddr) {
		DBG("Cannot add %s to interface %p\n",
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

	ret = net_context_accept(reply_v6_ctx, accept_v6_cb, K_NO_WAIT,
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

	ret = net_context_accept(reply_v4_ctx, accept_v4_cb, K_NO_WAIT,
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

	if (k_sem_take(&wait_in_accept, K_MSEC(WAIT_TIME_LONG))) {
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

	if (k_sem_take(&wait_connect, K_MSEC(WAIT_TIME_LONG))) {
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

	k_sem_take(&wait_connect, K_MSEC(WAIT_TIME));
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
	struct net_if *iface = net_if_get_default();
	struct net_if_addr *ifaddr;
	const struct dummy_api *api;

	if (!iface) {
		TC_ERROR("Interface is NULL\n");
		return false;
	}

	/* Make sure that we always use the correct network interface when
	 * simulating the local and peer devices. To do this, we check what
	 * device API corresponds to what network interface sending function.
	 * This way we can use the correct network interface to set the IP
	 * addresses etc.
	 * The network interfaces might be set in different order depending on
	 * used target board and linker. We cannot guarantee that network
	 * interfaces are always set the same way in the linker section.
	 */
	api = net_if_get_device(iface)->api;
	if (api->send != tester_send) {
		my_iface = iface + 1;
		peer_iface = iface;
	} else {
		my_iface = iface;
		peer_iface = iface + 1;
	}

	iface = my_iface;

	ifaddr = net_if_ipv6_addr_add(iface, &my_v6_inaddr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add address\n");
		return false;
	}

	net_ipaddr_copy(&any_addr6.sin6_addr, &sin6_addr_any);
	any_addr6.sin6_family = AF_INET6;

	net_ipaddr_copy(&any_addr4.sin_addr, &sin_addr_any);
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
	{ "test TCP seq validity", test_tcp_seq_validity },
	{ "test TCP reply context init", test_init_tcp_reply_context },
	{ "test TCP accept init", test_init_tcp_accept },
#if 0
	/* TBD: more tests are needed */
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
