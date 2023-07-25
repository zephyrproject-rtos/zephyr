/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_vlan_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>

#if CONFIG_NET_VLAN_COUNT > 1
#define CREATE_MULTIPLE_TAGS
#endif

struct ud {
	struct net_if *first;
	struct net_if *second;
};

#if defined(CREATE_MULTIPLE_TAGS)
static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (iface == ud->first) {
		return;
	}

	ud->second = iface;
}
#endif

static int init_app(void)
{
	struct net_if *iface;
	int ret;

#if defined(CREATE_MULTIPLE_TAGS)
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	struct ud ud;
#endif

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (!iface) {
		LOG_ERR("No ethernet interfaces found.");
		return -ENOENT;
	}

	ret = net_eth_vlan_enable(iface, CONFIG_SAMPLE_VLAN_TAG);
	if (ret < 0) {
		LOG_ERR("Cannot enable VLAN for tag %d (%d)",
			CONFIG_SAMPLE_VLAN_TAG, ret);
	}

#if defined(CREATE_MULTIPLE_TAGS)
	ud.first = iface;
	ud.second = NULL;

	net_if_foreach(iface_cb, &ud);

	/* This sample has two VLANs. For the second one we need to manually
	 * create IP address for this test. But first the VLAN needs to be
	 * added to the interface so that IPv6 DAD can work properly.
	 */
	ret = net_eth_vlan_enable(ud.second, CONFIG_SAMPLE_VLAN_TAG_2);
	if (ret < 0) {
		LOG_ERR("Cannot enable VLAN for tag %d (%d)",
			CONFIG_SAMPLE_VLAN_TAG_2, ret);
	}

	if (net_addr_pton(AF_INET6, CONFIG_SAMPLE_IPV6_ADDR_2, &addr6)) {
		LOG_ERR("Invalid address: %s", CONFIG_SAMPLE_IPV6_ADDR_2);
		return -EINVAL;
	}

	ifaddr = net_if_ipv6_addr_add(ud.second, &addr6, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Cannot add %s to interface %p",
			CONFIG_SAMPLE_IPV6_ADDR_2, ud.second);
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET, CONFIG_SAMPLE_IPV4_ADDR_2, &addr4)) {
		LOG_ERR("Invalid address: %s", CONFIG_SAMPLE_IPV4_ADDR_2);
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(ud.second, &addr4, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Cannot add %s to interface %p",
			CONFIG_SAMPLE_IPV4_ADDR_2, ud.second);
		return -EINVAL;
	}
#endif

	return ret;
}

int main(void)
{
	init_app();
	return 0;
}
