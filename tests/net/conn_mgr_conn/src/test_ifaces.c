/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_if.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include "test_conn_impl.h"
#include "test_ifaces.h"

/* Create test ifaces */

/* Generic iface initializer, shared by all three ifaces */
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

static struct dummy_api test_iface_api = {
	.iface_api.init = test_iface_init,
};

/* Create three ifaces, a1, a2, b such that:
 * iface a1 and a2 share L2 connectivity implementation a
 * iface b uses connectivity implementation b
 */
NET_DEVICE_INIT(test_iface_a1,
		"test_iface_a1",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_a2,
		"test_iface_a2",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_b,
		"test_iface_b",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

/* Create an ifaces with NULL implementation, NULL init, and no connectivity at all */
NET_DEVICE_INIT(test_iface_null,
		"test_iface_null",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_ni,
		"test_iface_ni",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);
NET_DEVICE_INIT(test_iface_none,
		"test_iface_none",
		test_iface_netdev_init,
		NULL,
		NULL,
		NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&test_iface_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		127);

/* Bind L2 connectivity implementations to ifaces */
CONN_MGR_BIND_CONN(test_iface_a1,	TEST_L2_CONN_IMPL_A);
CONN_MGR_BIND_CONN(test_iface_a2,	TEST_L2_CONN_IMPL_A);
CONN_MGR_BIND_CONN(test_iface_b,	TEST_L2_CONN_IMPL_B);

/* Bind edge-case L2 connectivity implementations to ifaces */
CONN_MGR_BIND_CONN(test_iface_null,	TEST_L2_CONN_IMPL_N);
CONN_MGR_BIND_CONN(test_iface_ni,	TEST_L2_CONN_IMPL_NI);

/* Public accessors for static iface structs */
struct net_if *ifa1  = NET_IF_GET(test_iface_a1, 0);
struct net_if *ifa2  = NET_IF_GET(test_iface_a2, 0);
struct net_if *ifb   = NET_IF_GET(test_iface_b,  0);
struct net_if *ifni  = NET_IF_GET(test_iface_ni,  0);
struct net_if *ifnull = NET_IF_GET(test_iface_null, 0);
struct net_if *ifnone = NET_IF_GET(test_iface_none, 0);
