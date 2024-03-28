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

struct ud {
	struct net_if *first;
	struct net_if *second;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	if (ud->first == NULL) {
		ud->first = iface;
		return;
	}

	ud->second = iface;
}

static int init_app(void)
{
	struct net_if *iface;
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	struct ud ud;
	int ret;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (!iface) {
		LOG_ERR("No ethernet interfaces found.");
		return -ENOENT;
	}

	net_if_foreach(iface_cb, &ud);

	ret = net_eth_vlan_enable(iface, CONFIG_SAMPLE_VLAN_TAG);
	if (ret < 0) {
		LOG_ERR("Cannot enable VLAN for tag %d (%d)",
			CONFIG_SAMPLE_VLAN_TAG, ret);
	}

	ret = net_eth_vlan_enable(iface, CONFIG_SAMPLE_VLAN_TAG_2);
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

	return ret;
}

int main(void)
{
	init_app();
	return 0;
}
