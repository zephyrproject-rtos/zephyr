/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_mdns_responder_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/net/ethernet.h>

/* User data for the interface callback */
struct ud {
	struct net_if *first;
	struct net_if *second;
	struct net_if *third;
};

static void iface_cb(struct net_if *iface, void *user_data_param)
{
	struct ud *user_data = user_data_param;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (!user_data->first) {
		user_data->first = iface;
		return;
	}

	if (!user_data->second) {
		user_data->second = iface;
		return;
	}

	if (!user_data->third) {
		user_data->third = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface, const char *ipv6_addr,
		       const char *ipv4_addr, const char *netmask,
		       uint16_t vlan_tag)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	int ret;

	ret = net_eth_vlan_enable(iface, vlan_tag);
	if (ret < 0) {
		LOG_ERR("Cannot enable VLAN for tag %d (%d)", vlan_tag, ret);
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		if (net_addr_pton(AF_INET6, ipv6_addr, &addr6)) {
			LOG_ERR("Invalid address: %s", ipv6_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv6_addr_add(iface, &addr6,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p",
				ipv6_addr, iface);
			return -EINVAL;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
			LOG_ERR("Invalid address: %s", ipv4_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv4_addr_add(iface, &addr4,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p",
				ipv4_addr, iface);
			return -EINVAL;
		}

		if (netmask && netmask[0]) {
			struct in_addr nm;

			if (net_addr_pton(AF_INET, netmask, &nm)) {
				LOG_ERR("Invalid netmask: %s", ipv4_addr);
				return -EINVAL;
			}

			net_if_ipv4_set_netmask_by_addr(iface, &addr4, &nm);
		}
	}

	LOG_DBG("Interface %p VLAN tag %d setup done.", iface, vlan_tag);

	return 0;
}

int init_vlan(void)
{
	struct ud user_data;
	int ret;

	memset(&user_data, 0, sizeof(user_data));

	net_if_foreach(iface_cb, &user_data);

	/* This sample has two VLANs. For the second one we need to manually
	 * create IP address for this test. But first the VLAN needs to be
	 * added to the interface so that IPv6 DAD can work properly.
	 */
	ret = setup_iface(user_data.second,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_NETMASK,
			  CONFIG_NET_SAMPLE_IFACE2_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	ret = setup_iface(user_data.third,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV4_NETMASK,
			  CONFIG_NET_SAMPLE_IFACE3_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
