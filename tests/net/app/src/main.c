/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include <ztest.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>

#include <net/net_app.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_DEBUG_IF)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#if defined(CONFIG_NET_IPV6)
/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
#endif

#if defined(CONFIG_NET_IPV4)
/* Interface 1 addresses IPv4 */
static struct in_addr my_addr4 = { { { 192, 0, 1, 1 } } };
#endif

static struct net_if *iface1;

static bool test_failed;
static bool test_started;
static struct k_sem wait_data;

#define WAIT_TIME 250

struct net_if_test {
	u8_t idx;
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_iface_dev_init(struct device *dev)
{
	return 0;
}

static u8_t *net_iface_get_mac(struct device *dev)
{
	struct net_if_test *data = dev->driver_data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand32_get();
	}

	data->ll_addr.addr = data->mac_addr;
	data->ll_addr.len = 6;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	u8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int sender_iface(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		struct net_if_test *data = iface->dev->driver_data;

		DBG("Sending at iface %d %p\n", net_if_get_by_iface(iface),
		    iface);

		if (net_pkt_iface(pkt) != iface) {
			DBG("Invalid interface %p, expecting %p\n",
				 net_pkt_iface(pkt), iface);
			test_failed = true;
		}

		if (net_if_get_by_iface(iface) != data->idx) {
			DBG("Invalid interface %d index, expecting %d\n",
				 data->idx, net_if_get_by_iface(iface));
			test_failed = true;
		}
	}

	net_pkt_unref(pkt);

	k_sem_give(&wait_data);

	return 0;
}

struct net_if_test net_iface1_data;

static struct net_if_api net_iface_api = {
	.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 net_iface_dev_init,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

static void iface_setup(void)
{
#if defined(CONFIG_NET_IPV6)
	struct net_if_mcast_addr *maddr;
#endif
	struct net_if_addr *ifaddr;
	int idx;

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	iface1 = net_if_get_by_index(0);

	((struct net_if_test *)iface1->dev->driver_data)->idx = 0;

	idx = net_if_get_by_iface(iface1);
	zassert_equal(idx, 0, "Invalid index iface1");

	DBG("Interfaces: [%d] iface1 %p\n",
	    net_if_get_by_iface(iface1), iface1);

	zassert_not_null(iface1, "Interface 1");

#if defined(CONFIG_NET_IPV6)
	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&ll_addr));
		zassert_not_null(ifaddr, "ll_addr");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface1, &in6addr_mcast);
	if (!maddr) {
		DBG("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		zassert_not_null(maddr, "mcast");
	}
#endif /* IPv6 */

#if defined(CONFIG_NET_IPV4)
	ifaddr = net_if_ipv4_addr_add(iface1, &my_addr4,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_addr4));
		zassert_not_null(ifaddr, "addr4");
	}
#endif

	net_if_up(iface1);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;

	test_started = true;
}

static void app_init(void)
{
	int ret;

	ret = net_app_init("Test app", 0, 1);
	zassert_equal(ret, 0, "app init");
}

static struct net_app_ctx udp_server_ctx;
static struct net_app_ctx tcp_server_ctx;

static void app_udp_server_init(void)
{
	int ret;

	ret = net_app_init_udp_server(&udp_server_ctx, NULL, 42421, NULL);
	zassert_equal(ret, 0, "UDP server init");
}

static void app_tcp_server_init(void)
{
	int ret;

	ret = net_app_init_tcp_server(&tcp_server_ctx, NULL, 42422, NULL);
	zassert_equal(ret, 0, "TCP server init");
}

static void app_udp_server_listen(void)
{
	int ret;

	net_app_server_enable(&udp_server_ctx);

	ret = net_app_listen(&udp_server_ctx);
	zassert_equal(ret, 0, "UDP listen failed");
}

static void app_tcp_server_listen(void)
{
	int ret;

	net_app_server_enable(&tcp_server_ctx);

	ret = net_app_listen(&tcp_server_ctx);
	zassert_equal(ret, 0, "TCP listen failed");
}

static void app_tcp6_client_peer(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "2001:db8:200::1",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv6 client init");

	port = ntohs(net_sin6(&ctx.ipv6.remote)->sin6_port);
	zassert_equal(port, 42422, "TCP port invalid");

	ret = net_ipv6_addr_cmp(&net_sin6(&ctx.ipv6.remote)->sin6_addr,
				&my_addr2);
	zassert_equal(ret, true, "IPv6 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "TCP IPv6 client close");
#endif
}

static void app_tcp4_client_peer(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "192.0.1.1",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv4 client init");

	port = ntohs(net_sin(&ctx.ipv4.remote)->sin_port);
	zassert_equal(port, 42422, "TCP port invalid");

	ret = net_ipv4_addr_cmp(&net_sin(&ctx.ipv4.remote)->sin_addr,
				&my_addr4);
	zassert_equal(ret, true, "IPv4 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "TCP IPv4 client close");
#endif
}

static void app_udp6_client_peer(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "2001:db8:200::1",
				      42421,
				      0,
				      NULL);
	zassert_equal(ret, 0, "UDP IPv6 client init");

	port = ntohs(net_sin6(&ctx.ipv6.remote)->sin6_port);
	zassert_equal(port, 42421, "UDP port invalid");

	ret = net_ipv6_addr_cmp(&net_sin6(&ctx.ipv6.remote)->sin6_addr,
				&my_addr2);
	zassert_equal(ret, true, "IPv6 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "UDP IPv6 client close");
#endif
}

static void app_udp4_client_peer(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "192.0.1.1",
				      42421,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv4 client init");

	port = ntohs(net_sin(&ctx.ipv4.remote)->sin_port);
	zassert_equal(port, 42421, "UDP port invalid");

	ret = net_ipv4_addr_cmp(&net_sin(&ctx.ipv4.remote)->sin_addr,
				&my_addr4);
	zassert_equal(ret, true, "IPv4 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "UDP IPv4 client close");
#endif
}

static void app_tcp6_client_peer_with_port(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "[2001:db8:200::1]:1234",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv6 client init");

	port = ntohs(net_sin6(&ctx.ipv6.remote)->sin6_port);
	zassert_equal(port, 1234, "TCP port invalid");

	ret = net_ipv6_addr_cmp(&net_sin6(&ctx.ipv6.remote)->sin6_addr,
				&my_addr2);
	zassert_equal(ret, true, "IPv6 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "TCP IPv6 client close");
#endif
}

static void app_tcp4_client_peer_with_port(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "192.0.1.1:1234",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv4 client init");

	port = ntohs(net_sin(&ctx.ipv4.remote)->sin_port);
	zassert_equal(port, 1234, "TCP port invalid");

	ret = net_ipv4_addr_cmp(&net_sin(&ctx.ipv4.remote)->sin_addr,
				&my_addr4);
	zassert_equal(ret, true, "IPv4 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "TCP IPv4 client close");
#endif
}

static void app_udp6_client_peer_with_port(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "[2001:db8:200::1]:9999",
				      42421,
				      0,
				      NULL);
	zassert_equal(ret, 0, "UDP IPv6 client init");

	port = ntohs(net_sin6(&ctx.ipv6.remote)->sin6_port);
	zassert_equal(port, 9999, "UDP port invalid");

	ret = net_ipv6_addr_cmp(&net_sin6(&ctx.ipv6.remote)->sin6_addr,
				&my_addr2);
	zassert_equal(ret, true, "IPv6 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "UDP IPv6 client close");
#endif
}

static void app_udp4_client_peer_with_port(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	int port;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "192.0.1.1:9999",
				      42421,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv4 client init");

	port = ntohs(net_sin(&ctx.ipv4.remote)->sin_port);
	zassert_equal(port, 9999, "UDP port invalid");

	ret = net_ipv4_addr_cmp(&net_sin(&ctx.ipv4.remote)->sin_addr,
				&my_addr4);
	zassert_equal(ret, true, "IPv4 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "UDP IPv4 client close");
#endif
}

static void app_tcp6_client_peer_addr(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	struct sockaddr_in6 peer;
	int port;
	int ret;

	net_ipaddr_copy(&peer.sin6_addr, &my_addr2);
	peer.sin6_port = htons(8765);
	peer.sin6_family = AF_INET6;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      (struct sockaddr *)&peer,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv6 client init");

	port = ntohs(net_sin6(&ctx.ipv6.remote)->sin6_port);
	zassert_equal(port, 8765, "TCP port invalid");

	ret = net_ipv6_addr_cmp(&net_sin6(&ctx.ipv6.remote)->sin6_addr,
				&my_addr2);
	zassert_equal(ret, true, "IPv6 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "TCP IPv6 client close");
#endif
}

static void app_tcp4_client_peer_addr(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	struct sockaddr_in peer;
	int port;
	int ret;

	net_ipaddr_copy(&peer.sin_addr, &my_addr4);
	peer.sin_port = htons(8765);
	peer.sin_family = AF_INET;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      (struct sockaddr *)&peer,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "TCP IPv4 client init");

	port = ntohs(net_sin(&ctx.ipv4.remote)->sin_port);
	zassert_equal(port, 8765, "TCP port invalid");

	ret = net_ipv4_addr_cmp(&net_sin(&ctx.ipv4.remote)->sin_addr,
				&my_addr4);
	zassert_equal(ret, true, "IPv4 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "TCP IPv4 client close");
#endif
}

static void app_udp6_client_peer_addr(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	struct sockaddr_in6 peer;
	int port;
	int ret;

	net_ipaddr_copy(&peer.sin6_addr, &my_addr2);
	peer.sin6_port = htons(8765);
	peer.sin6_family = AF_INET6;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      (struct sockaddr *)&peer,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "UDP IPv6 client init");

	port = ntohs(net_sin6(&ctx.ipv6.remote)->sin6_port);
	zassert_equal(port, 8765, "UDP port invalid");

	ret = net_ipv6_addr_cmp(&net_sin6(&ctx.ipv6.remote)->sin6_addr,
				&my_addr2);
	zassert_equal(ret, true, "IPv6 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "UDP IPv6 client close");
#endif
}

static void app_udp4_client_peer_addr(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	struct sockaddr_in peer;
	int port;
	int ret;

	net_ipaddr_copy(&peer.sin_addr, &my_addr4);
	peer.sin_port = htons(8765);
	peer.sin_family = AF_INET;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      (struct sockaddr *)&peer,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, 0, "UDP IPv4 client init");

	port = ntohs(net_sin(&ctx.ipv4.remote)->sin_port);
	zassert_equal(port, 8765, "UDP port invalid");

	ret = net_ipv4_addr_cmp(&net_sin(&ctx.ipv4.remote)->sin_addr,
				&my_addr4);
	zassert_equal(ret, true, "IPv4 address mismatch");

	ret = net_app_close(&ctx);
	zassert_equal(ret, 0, "UDP IPv4 client close");
#endif
}

static void app_tcp6_client_hostname_fail(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, -EINVAL, "TCP IPv6 client init");
#endif
}

static void app_tcp4_client_hostname_fail(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, -EINVAL, "TCP IPv4 client init");
#endif
}

static void app_udp6_client_hostname_fail(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, -EINVAL, "UDP IPv6 client init");
#endif
}

static void app_udp4_client_hostname_fail(void)
{
#if defined(CONFIG_NET_IPV4)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      0,
				      NULL);
	zassert_equal(ret, -EINVAL, "UDP IPv4 client init");
#endif
}

static void app_tcp6_client_hostname(void)
{
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_DNS_RESOLVER)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      MSEC(100),
				      NULL);
	zassert_equal(ret, -ETIMEDOUT, "TCP IPv6 client init");
#endif
}

static void app_tcp4_client_hostname(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_DNS_RESOLVER)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_tcp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      MSEC(100),
				      NULL);
	zassert_equal(ret, -ETIMEDOUT, "TCP IPv4 client init");
#endif
}

static void app_udp6_client_hostname(void)
{
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_DNS_RESOLVER)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      MSEC(100),
				      NULL);
	zassert_equal(ret, -ETIMEDOUT, "UDP IPv6 client init");
#endif
}

static void app_udp4_client_hostname(void)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_DNS_RESOLVER)
	static struct net_app_ctx ctx;
	int ret;

	ret = net_app_init_udp_client(&ctx,
				      NULL,
				      NULL,
				      "foobar",
				      42422,
				      MSEC(100),
				      NULL);
	zassert_equal(ret, -ETIMEDOUT, "UDP IPv4 client init");
#endif
}

static void app_close_server(void)
{
	int ret;

	ret = net_app_close(&udp_server_ctx);
	zassert_equal(ret, 0, "UDP server close");

	ret = net_app_close(&tcp_server_ctx);
	zassert_equal(ret, 0, "TCP server close");
}

void test_main(void)
{
	ztest_test_suite(net_app_test,
			 ztest_unit_test(iface_setup),
			 ztest_unit_test(app_init),
			 ztest_unit_test(app_udp_server_init),
			 ztest_unit_test(app_tcp_server_init),
			 ztest_unit_test(app_udp_server_listen),
			 ztest_unit_test(app_tcp_server_listen),
			 ztest_unit_test(app_tcp6_client_peer),
			 ztest_unit_test(app_udp6_client_peer),
			 ztest_unit_test(app_tcp4_client_peer),
			 ztest_unit_test(app_udp4_client_peer),
			 ztest_unit_test(app_tcp6_client_peer_with_port),
			 ztest_unit_test(app_tcp4_client_peer_with_port),
			 ztest_unit_test(app_udp6_client_peer_with_port),
			 ztest_unit_test(app_udp4_client_peer_with_port),
			 ztest_unit_test(app_tcp6_client_peer_addr),
			 ztest_unit_test(app_tcp4_client_peer_addr),
			 ztest_unit_test(app_udp6_client_peer_addr),
			 ztest_unit_test(app_udp4_client_peer_addr),
			 ztest_unit_test(app_tcp6_client_hostname_fail),
			 ztest_unit_test(app_tcp4_client_hostname_fail),
			 ztest_unit_test(app_udp6_client_hostname_fail),
			 ztest_unit_test(app_udp4_client_hostname_fail),
			 ztest_unit_test(app_tcp6_client_hostname),
			 ztest_unit_test(app_tcp4_client_hostname),
			 ztest_unit_test(app_udp6_client_hostname),
			 ztest_unit_test(app_udp4_client_hostname),
			 ztest_unit_test(app_close_server)
			 );

	ztest_run_test_suite(net_app_test);
}
