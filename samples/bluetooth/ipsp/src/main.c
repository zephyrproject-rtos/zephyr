/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>

/* admin-local, dynamically allocated multicast address */
#define MCAST_IP6ADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, \
			    0, 0, 0, 0, 0, 0, 0, 0x2 } } }

struct in6_addr in6addr_mcast = MCAST_IP6ADDR;

/* Define my IP address where to expect messages */
#define MY_IP6ADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_PREFIX_LEN 64

static struct in6_addr in6addr_my = MY_IP6ADDR;

#define MY_PORT 4242

#define STACKSIZE 2000
K_THREAD_STACK_DEFINE(thread_stack, STACKSIZE);
static struct k_thread thread_data;

#define MAX_DBG_PRINT 64

NET_PKT_TX_SLAB_DEFINE(echo_tx_tcp, 15);
NET_PKT_DATA_POOL_DEFINE(echo_data_tcp, 30);

static struct k_mem_slab *tx_tcp_pool(void)
{
	return &echo_tx_tcp;
}

static struct net_buf_pool *data_tcp_pool(void)
{
	return &echo_data_tcp;
}

static struct k_sem quit_lock;

static inline void quit(void)
{
	k_sem_give(&quit_lock);
}

static inline void init_app(void)
{
	printk("Run IPSP sample");

	k_sem_init(&quit_lock, 0, UINT_MAX);

	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_APP_MY_IPV6_ADDR,
			  &in6addr_my) < 0) {
		printk("Invalid IPv6 address %s",
			CONFIG_NET_APP_MY_IPV6_ADDR);
	}

	do {
		struct net_if_addr *ifaddr;

		ifaddr = net_if_ipv6_addr_add(net_if_get_default(),
					      &in6addr_my, NET_ADDR_MANUAL, 0);
	} while (0);

	net_if_ipv6_maddr_add(net_if_get_default(), &in6addr_mcast);
}

static inline bool get_context(struct net_context **udp_recv6,
			       struct net_context **tcp_recv6,
			       struct net_context **mcast_recv6)
{
	int ret;
	struct sockaddr_in6 mcast_addr6 = { 0 };
	struct sockaddr_in6 my_addr6 = { 0 };

	net_ipaddr_copy(&mcast_addr6.sin6_addr, &in6addr_mcast);
	mcast_addr6.sin6_family = AF_INET6;

	my_addr6.sin6_family = AF_INET6;
	my_addr6.sin6_port = htons(MY_PORT);

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, udp_recv6);
	if (ret < 0) {
		printk("Cannot get network context for IPv6 UDP (%d)",
			ret);
		return false;
	}

	ret = net_context_bind(*udp_recv6, (struct sockaddr *)&my_addr6,
			       sizeof(struct sockaddr_in6));
	if (ret < 0) {
		printk("Cannot bind IPv6 UDP port %d (%d)",
			ntohs(my_addr6.sin6_port), ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, mcast_recv6);
	if (ret < 0) {
		printk("Cannot get receiving IPv6 mcast "
			"network context (%d)", ret);
		return false;
	}

	ret = net_context_bind(*mcast_recv6, (struct sockaddr *)&mcast_addr6,
			       sizeof(struct sockaddr_in6));
	if (ret < 0) {
		printk("Cannot bind IPv6 mcast (%d)", ret);
		return false;
	}

	ret = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, tcp_recv6);
	if (ret < 0) {
		printk("Cannot get network context for IPv6 TCP (%d)", ret);
		return false;
	}

	net_context_setup_pools(*tcp_recv6, tx_tcp_pool, data_tcp_pool);

	ret = net_context_bind(*tcp_recv6, (struct sockaddr *)&my_addr6,
			       sizeof(struct sockaddr_in6));
	if (ret < 0) {
		printk("Cannot bind IPv6 TCP port %d (%d)",
			ntohs(my_addr6.sin6_port), ret);
		return false;
	}

	ret = net_context_listen(*tcp_recv6, 0);
	if (ret < 0) {
		printk("Cannot listen IPv6 TCP (%d)", ret);
		return false;
	}

	return true;
}

static struct net_pkt *build_reply_pkt(const char *name,
				       struct net_context *context,
				       struct net_pkt *pkt)
{
	struct net_pkt *reply_pkt;
	struct net_buf *tmp;
	int header_len, recv_len, reply_len;

	printk("%s received %d bytes", name,
	      net_pkt_appdatalen(pkt));

	reply_pkt = net_pkt_get_tx(context, K_FOREVER);

	recv_len = net_pkt_get_len(pkt);

	tmp = pkt->frags;
	/* Remove frag link so original pkt can be unrefed */
	pkt->frags = NULL;

	/* First fragment will contain IP header so move the data
	 * down in order to get rid of it.
	 */
	header_len = net_pkt_appdata(pkt) - tmp->data;

	/* After this pull, the tmp->data points directly to application
	 * data.
	 */
	net_buf_pull(tmp, header_len);

	/* Add the entire chain into reply */
	net_pkt_frag_add(reply_pkt, tmp);

	reply_len = net_pkt_get_len(reply_pkt);

	printk("Received %d bytes, sending %d bytes", recv_len - header_len,
	       reply_len);

	return reply_pkt;
}

static inline void pkt_sent(struct net_context *context,
			    int status,
			    void *token,
			    void *user_data)
{
	if (!status) {
		printk("Sent %d bytes", POINTER_TO_UINT(token));
	}
}

static inline void set_dst_addr(sa_family_t family,
				struct net_pkt *pkt,
				struct sockaddr *dst_addr)
{
	net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
			&NET_IPV6_HDR(pkt)->src);
	net_sin6(dst_addr)->sin6_family = AF_INET6;
	net_sin6(dst_addr)->sin6_port = NET_UDP_HDR(pkt)->src_port;
}

static void udp_received(struct net_context *context,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	struct net_pkt *reply_pkt;
	struct sockaddr dst_addr;
	sa_family_t family = net_pkt_family(pkt);
	static char dbg[MAX_DBG_PRINT + 1];
	int ret;

	snprintf(dbg, MAX_DBG_PRINT, "UDP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	set_dst_addr(family, pkt, &dst_addr);

	reply_pkt = build_reply_pkt(dbg, context, pkt);

	net_pkt_unref(pkt);

	ret = net_context_sendto(reply_pkt, &dst_addr,
				 family == AF_INET6 ?
				 sizeof(struct sockaddr_in6) :
				 sizeof(struct sockaddr_in),
				 pkt_sent, 0,
				 UINT_TO_POINTER(net_pkt_get_len(reply_pkt)),
				 user_data);
	if (ret < 0) {
		printk("Cannot send data to peer (%d)", ret);
		net_pkt_unref(reply_pkt);
	}
}

static void setup_udp_recv(struct net_context *udp_recv6)
{
	int ret;

	ret = net_context_recv(udp_recv6, udp_received, 0, NULL);
	if (ret < 0) {
		printk("Cannot receive IPv6 UDP packets");
	}
}

static void tcp_received(struct net_context *context,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	static char dbg[MAX_DBG_PRINT + 1];
	sa_family_t family = net_pkt_family(pkt);
	struct net_pkt *reply_pkt;
	int ret;

	snprintf(dbg, MAX_DBG_PRINT, "TCP IPv%c",
		 family == AF_INET6 ? '6' : '4');

	reply_pkt = build_reply_pkt(dbg, context, pkt);

	net_pkt_unref(pkt);

	ret = net_context_send(reply_pkt, pkt_sent, K_NO_WAIT,
			       UINT_TO_POINTER(net_pkt_get_len(reply_pkt)),
			       NULL);
	if (ret < 0) {
		printk("Cannot send data to peer (%d)", ret);
		net_pkt_unref(reply_pkt);

		quit();
	}
}

static void tcp_accepted(struct net_context *context,
			 struct sockaddr *addr,
			 socklen_t addrlen,
			 int error,
			 void *user_data)
{
	int ret;

	NET_DBG("Accept called, context %p error %d", context, error);

	ret = net_context_recv(context, tcp_received, 0, NULL);
	if (ret < 0) {
		printk("Cannot receive TCP packet (family %d)",
			net_context_get_family(context));
	}
}

static void setup_tcp_accept(struct net_context *tcp_recv6)
{
	int ret;

	ret = net_context_accept(tcp_recv6, tcp_accepted, 0, NULL);
	if (ret < 0) {
		printk("Cannot receive IPv6 TCP packets (%d)", ret);
	}
}

static void listen(void)
{
	struct net_context *udp_recv6 = { 0 };
	struct net_context *tcp_recv6 = { 0 };
	struct net_context *mcast_recv6 = { 0 };

	if (!get_context(&udp_recv6, &tcp_recv6, &mcast_recv6)) {
		printk("Cannot get network contexts");
		return;
	}

	printk("Starting to wait");

	setup_tcp_accept(tcp_recv6);
	setup_udp_recv(udp_recv6);

	k_sem_take(&quit_lock, K_FOREVER);

	printk("Stopping...");

	net_context_put(udp_recv6);
	net_context_put(mcast_recv6);
	net_context_put(tcp_recv6);
}

void main(void)
{
	int err;

	init_app();

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	ipss_init();

	err = ipss_advertise();
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	k_thread_create(&thread_data, thread_stack, STACKSIZE,
			(k_thread_entry_t)listen,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
