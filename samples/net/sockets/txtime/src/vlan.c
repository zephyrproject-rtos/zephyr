/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_txtime_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/net/ethernet.h>

/* User data for the interface callback */
struct ud {
	struct net_if *first;
	struct net_if *second;
	struct net_if *third;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (!ud->first) {
		ud->first = iface;
		return;
	}

	if (!ud->second) {
		ud->second = iface;
		return;
	}

	if (!ud->third) {
		ud->third = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface, const char *ipv6_addr,
		       const char *ipv4_addr, uint16_t vlan_tag)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	int ret;

	if (!iface) {
		LOG_DBG("VLAN interface not set");
		return -ENOENT;
	}

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
			LOG_ERR("Invalid address: %s", ipv6_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv4_addr_add(iface, &addr4,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p",
				ipv4_addr, iface);
			return -EINVAL;
		}
	}

	LOG_DBG("Interface %p VLAN tag %d setup done.", iface, vlan_tag);

	return 0;
}

int init_vlan(void)
{
	enum ethernet_hw_caps caps;
	struct ud ud;
	int ret;

	memset(&ud, 0, sizeof(ud));

	net_if_foreach(iface_cb, &ud);

	caps = net_eth_get_hw_capabilities(ud.first);
	if (!(caps & ETHERNET_HW_VLAN)) {
		LOG_DBG("Interface %p does not support %s", ud.first, "VLAN");
		return -ENOENT;
	}

	/* This sample has two VLANs. For the second one we need to manually
	 * create IP address for this test. But first the VLAN needs to be
	 * added to the interface so that IPv6 DAD can work properly.
	 */
	ret = setup_iface(ud.second,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	ret = setup_iface(ud.third,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
