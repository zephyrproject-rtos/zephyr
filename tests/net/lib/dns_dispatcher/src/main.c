/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_DNS_RESOLVER_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_DNS_RESOLVER_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

extern void dns_dispatcher_svc_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(test_pair_svc, dns_dispatcher_svc_handler, 1);

static int test_dispatch_cb(struct dns_socket_dispatcher *ctx, int sock,
			    struct net_sockaddr *addr, size_t addrlen,
			    struct net_buf *buf, size_t data_len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(sock);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);
	ARG_UNUSED(buf);
	ARG_UNUSED(data_len);

	return 0;
}

#define NAME4 "4.zephyr.test"
#define NAME6 "6.zephyr.test"
#define NAME_IPV4 "192.0.2.1"
#define NAME_IPV6 "2001:db8::1"

#define DNS_NAME_IPV4 "192.0.2.4"
#define DNS2_NAME_IPV4 "192.0.2.5"
#define DNS_NAME_IPV6 "2001:db8::4"

#define DNS_TIMEOUT 500 /* ms */

#if defined(CONFIG_NET_IPV6)
/* Interface 1 addresses */
static struct net_in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0x1 } } };
#endif

#if defined(CONFIG_NET_IPV4)
/* Interface 1 addresses */
static struct net_in_addr my_addr2 = { { { 192, 0, 2, 1 } } };
#endif

static struct net_if *iface1;

/* this must be higher that the DNS_TIMEOUT */
#define WAIT_TIME K_MSEC((DNS_TIMEOUT + 300) * 3)

struct net_if_test {
	uint8_t idx;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
};

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand8_get();
	}

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	return 0;
}

struct net_if_test net_iface1_data;

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

static void *test_init(void)
{
	struct net_if_addr *ifaddr;

	iface1 = net_if_get_by_index(0);
	zassert_is_null(iface1, "iface1");

	iface1 = net_if_get_by_index(1);

	((struct net_if_test *) net_if_get_device(iface1)->data)->idx =
		net_if_get_by_iface(iface1);

#if defined(CONFIG_NET_IPV6)
	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");

		return NULL;
	}

	/* For testing purposes we need to set the addresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;
#endif

#if defined(CONFIG_NET_IPV4)
	ifaddr = net_if_ipv4_addr_add(iface1, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");

		return NULL;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;
#endif

	net_if_up(iface1);

	return NULL;
}

ZTEST(dns_dispatcher, test_dns_dispatcher)
{
	struct dns_resolve_context *ctx;
	int ret, sock1, sock2 = -1;

	ctx = dns_resolve_get_default();

	dns_resolve_close(ctx);

	ret = dns_resolve_init_default(ctx);
	zassert_equal(ret, 0, "Cannot initialize DNS resolver (%d)", ret);

	sock1 = ctx->servers[0].sock;

	for (int i = 0; i < ctx->servers[0].dispatcher.fds_len; i++) {
		if (ctx->servers[0].dispatcher.fds[i].fd == sock1) {
			sock2 = i;
			break;
		}
	}

	zassert_not_equal(sock2, -1, "Cannot find socket");

	k_sleep(K_MSEC(10));

	dns_resolve_close(ctx);

	zassert_equal(ctx->servers[0].dispatcher.fds[sock2].fd, -1, "Socket not closed");
	zassert_equal(ctx->servers[0].dispatcher.sock, -1, "Dispatcher still registered");
}

/* Register a responder and a resolver on the same family/port so the
 * dispatcher pairs them (responder->pair points at the resolver). Closing the
 * resolver must clear that back-reference, otherwise the surviving responder
 * would later delegate traffic to an unregistered context.
 */
ZTEST(dns_dispatcher, test_dispatcher_pair_cleanup)
{
	static struct dns_socket_dispatcher responder;
	static struct dns_socket_dispatcher resolver;
	static struct zsock_pollfd responder_fds[1];
	static struct zsock_pollfd resolver_fds[1];
	struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(65123),
	};
	int responder_sock, resolver_sock;

	responder_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(responder_sock >= 0, "Cannot create responder socket");

	resolver_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(resolver_sock >= 0, "Cannot create resolver socket");

	responder_fds[0].fd = responder_sock;
	resolver_fds[0].fd = resolver_sock;

	responder.type = DNS_SOCKET_RESPONDER;
	responder.cb = test_dispatch_cb;
	responder.fds = responder_fds;
	responder.fds_len = 1;
	responder.sock = responder_sock;
	responder.svc = &test_pair_svc;
	memcpy(&responder.local_addr, &local, sizeof(local));

	resolver.type = DNS_SOCKET_RESOLVER;
	resolver.cb = test_dispatch_cb;
	resolver.fds = resolver_fds;
	resolver.fds_len = 1;
	resolver.sock = resolver_sock;
	resolver.svc = &test_pair_svc;
	memcpy(&resolver.local_addr, &local, sizeof(local));

	zassert_ok(dns_dispatcher_register(&responder), "Cannot register responder");
	zassert_ok(dns_dispatcher_register(&resolver), "Cannot register resolver");

	zassert_equal(responder.pair, &resolver, "Dispatchers were not paired");

	zassert_ok(dns_dispatcher_unregister(&resolver), "Cannot unregister resolver");
	zassert_is_null(responder.pair, "Pair back-reference was not cleared");

	zassert_ok(dns_dispatcher_unregister(&responder), "Cannot unregister responder");

	(void)zsock_close(responder_sock);
	(void)zsock_close(resolver_sock);
}

/* A poll event may still be in flight for a socket whose dispatch slot was
 * cleared concurrently (e.g. the server was just closed). Emulate that by
 * handing the handler a dispatch table whose slot is NULL and verify the event
 * is dropped instead of dereferencing a NULL dispatcher.
 */
ZTEST(dns_dispatcher, test_dispatcher_null_slot_dropped)
{
	/* dispatch_table entries start with the dispatcher pointer, so a plain
	 * NULL pointer slot is layout-compatible for index 0.
	 */
	struct dns_socket_dispatcher *table[1] = { NULL };
	struct net_socket_service_event pev = {
		.event = {
			.fd = 0,
			.revents = ZSOCK_POLLIN,
		},
		.user_data = table,
	};

	/* Prior to the NULL guard this dereferenced a NULL dispatcher and
	 * crashed; reaching the next statement proves the event was dropped.
	 */
	dns_dispatcher_svc_handler(&pev);
}

ZTEST_SUITE(dns_dispatcher, NULL, test_init, NULL, NULL, NULL);
