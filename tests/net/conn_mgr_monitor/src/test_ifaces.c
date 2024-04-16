/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/net/net_if.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/net/ethernet.h>
#include "test_ifaces.h"

/* Create test ifaces */

/* Generic iface initializer, shared by all test ifaces */
static void test_iface_init(struct net_if *iface)
{
	/* Fake link layer address is needed to silence assertions inside the net core */
	static uint8_t fake_lladdr[] = { 0x01 };

	net_if_set_link_addr(iface, fake_lladdr, sizeof(fake_lladdr), NET_LINK_DUMMY);

	/* Do not automatically start the iface */
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
}

/* Mandatory stub for NET_DEVICE_INIT */
static int test_iface_netdev_init(const struct device *dev)
{
	return 0;
}

/* This is needed specifically for IPv6 DAD.
 * DAD tries to send a packet, and the test will hang if send is not implemented.
 */
static int test_iface_send(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

static struct dummy_api test_iface_api = {
	.iface_api.init = test_iface_init,
	.send = test_iface_send,
};

static struct ethernet_api dummy_eth_api = {
	.iface_api.init = test_iface_init,
};

NET_DEVICE_INIT(test_if_simple_a,
		"test_if_simple_a",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

NET_DEVICE_INIT(test_if_simple_b,
		"test_if_simple_b",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

NET_DEVICE_INIT(test_if_connected_a,
		"test_if_connected_a",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

NET_DEVICE_INIT(test_if_connected_b,
		"test_if_connected_b",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

/* A dummy ETHERNET_L2 iface so that we can test L2 ignore.
 * This iface is not properly defined, do not attempt to use it.
 */
NET_DEVICE_INIT(test_if_dummy_eth,
		"test_if_dummy_eth",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_ETH_INIT_PRIORITY,
		&dummy_eth_api,
		ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		127);


static void test_conn_api_init(struct conn_mgr_conn_binding *const binding)
{
	/* Mark the iface dormant (disconnected) on initialization */
	net_if_dormant_on(binding->iface);
}

static int test_conn_api_connect(struct conn_mgr_conn_binding *const binding)
{
	/* Mark iface as connected */
	net_if_dormant_off(binding->iface);
	return 0;
}

static int test_conn_api_disconnect(struct conn_mgr_conn_binding *const binding)
{
	/* Mark iface as dormant (disconnected) */
	net_if_dormant_on(binding->iface);
	return 0;
}

static struct conn_mgr_conn_api test_conn_api = {
	.init = test_conn_api_init,
	.connect = test_conn_api_connect,
	.disconnect = test_conn_api_disconnect,
};

/* Dummy struct */
struct test_conn_data {
};

#define TEST_CONN_IMPL_CTX_TYPE struct test_conn_data
CONN_MGR_CONN_DEFINE(TEST_CONN_IMPL, &test_conn_api);

/* Bind connectivity implementation to ifaces */
CONN_MGR_BIND_CONN(test_if_connected_a,	TEST_CONN_IMPL);
CONN_MGR_BIND_CONN(test_if_connected_b,	TEST_CONN_IMPL);


struct net_if *if_simp_a = NET_IF_GET(test_if_simple_a,    0);
struct net_if *if_simp_b = NET_IF_GET(test_if_simple_b,    0);
struct net_if *if_conn_a = NET_IF_GET(test_if_connected_a, 0);
struct net_if *if_conn_b = NET_IF_GET(test_if_connected_b, 0);
struct net_if *if_dummy_eth = NET_IF_GET(test_if_dummy_eth, 0);
