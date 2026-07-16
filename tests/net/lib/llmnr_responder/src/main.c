/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(llmnr_resp_test);

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/ztest.h>

static struct net_if *iface1;

static struct net_if_test {
	uint8_t idx; /* not used for anything, just a dummy value */
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
} net_iface1_data;

static struct net_in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
					   0x04 } } };

static struct net_in_addr my_addr4 = { { { 192, 0, 2, 1 } } };

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
		data->mac_addr[5] = 0x01;
	}

	memcpy(data->ll_addr.addr, data->mac_addr, sizeof(data->mac_addr));
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

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

static void *test_setup(void)
{
	struct net_if_addr *ifaddr;

	iface1 = net_if_get_by_index(1);
	zassert_not_null(iface1, "Iface1 is NULL");

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Failed to add IPv6 LL-addr");
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv4_addr_add(iface1, &my_addr4, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Failed to add IPv4 addr");
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_if_up(iface1);

	return NULL;
}

/* Whether the interface is currently a member of the LLMNR multicast groups.
 * A query is only delivered to the responder's socket while the group is
 * joined, so these mirror what the responder needs to keep working.
 */
static bool ipv4_group_joined(struct net_if *iface)
{
	const struct net_in_addr mcast = { { { 224, 0, 0, 252 } } };
	struct net_if_mcast_addr *maddr;

	maddr = net_if_ipv4_maddr_lookup(&mcast, &iface);

	return maddr != NULL && net_if_ipv4_maddr_is_joined(maddr);
}

static bool ipv6_group_joined(struct net_if *iface)
{
	struct net_in6_addr mcast;
	struct net_if_mcast_addr *maddr;

	net_ipv6_addr_create(&mcast, 0xff02, 0, 0, 0, 0, 0, 0x01, 0x03);
	maddr = net_if_ipv6_maddr_lookup(&mcast, &iface);

	return maddr != NULL && net_if_ipv6_maddr_is_joined(maddr);
}

/* Regression test: the LLMNR responder must keep its multicast group
 * membership after the network interface goes down and comes back up
 * (e.g. an Ethernet cable removed and reattached). Bringing the interface
 * fully down makes the stack leave the LLMNR groups; if the responder does
 * not rejoin them when the interface comes back up, queries are dropped at
 * the IP input as being destined to an unjoined multicast address.
 */
ZTEST(llmnr_responder, test_group_recovery_after_iface_down_up)
{
	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 LLMNR group not joined before the link went down");
	zassert_true(ipv6_group_joined(iface1),
		     "IPv6 LLMNR group not joined before the link went down");

	/* Simulate the Ethernet cable being removed and reattached. */
	zassert_ok(net_if_down(iface1), "Cannot bring the interface down");
	zassert_ok(net_if_up(iface1), "Cannot bring the interface back up");

	/* Let the NET_EVENT_IF_UP handler run so it can rejoin the groups. */
	k_sleep(K_MSEC(100));

	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 LLMNR group not rejoined after the interface came back up");
	zassert_true(ipv6_group_joined(iface1),
		     "IPv6 LLMNR group not rejoined after the interface came back up");
}

ZTEST_SUITE(llmnr_responder, NULL, test_setup, NULL, NULL, NULL);
