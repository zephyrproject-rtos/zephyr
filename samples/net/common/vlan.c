/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_samples_common, LOG_LEVEL_DBG);

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/net/ethernet.h>

/* User data for the interface callback */
struct ud {
	struct net_if *first;
	struct net_if *second;
	struct net_if *eth;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) && ud->eth == NULL) {
		ud->eth = iface;
		return;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
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
}

static int setup_iface(struct net_if *eth_iface,
		       struct net_if *vlan_iface,
		       const char *option)
{
	struct sockaddr_storage addr = { 0 };
	struct sockaddr *paddr = (struct sockaddr *)&addr;
	const char *addr_str, *next;
	struct net_if_addr *ifaddr;
	uint8_t mask_len = 0;
	unsigned long value;
	uint16_t vlan_tag;
	char *endptr;
	bool status;
	int ret;

	if (option[0] == '\0') {
		return 0;
	}

	next = strstr(option, ";");
	if (next == NULL) {
		LOG_ERR("VLAN tag not found, invalid option \"%s\"", option);
		return -EINVAL;
	}

	value = strtoul(option, &endptr, 10);
	if (*endptr != '\0' && endptr != next) {
		LOG_ERR("Invalid VLAN tag \"%s\"", option);
		return -EINVAL;
	}

	vlan_tag = (uint16_t)value;
	addr_str = ++next;

	do {
		char my_addr[INET6_ADDRSTRLEN] = { 'N', 'o', ' ', 'I', 'P', '\0'};

		next = net_ipaddr_parse_mask(addr_str, strlen(addr_str),
					     paddr, &mask_len);
		if (next == NULL) {
			LOG_ERR("Cannot parse IP address \"%s\"", addr_str);
			return -EINVAL;
		}

		inet_ntop(paddr->sa_family, net_sin(paddr)->sin_addr.s4_addr,
			  my_addr, sizeof(my_addr));

		if (paddr->sa_family == AF_INET) {
			struct sockaddr_in *addr4 = (struct sockaddr_in *)paddr;
			struct sockaddr_in mask;

			ifaddr = net_if_ipv4_addr_add(vlan_iface, &addr4->sin_addr,
						      NET_ADDR_MANUAL, 0);

			ret = net_mask_len_to_netmask(AF_INET, mask_len,
						      (struct sockaddr *)&mask);
			if (ret < 0) {
				LOG_ERR("Invalid network mask length (%d)", ret);
				return ret;
			}

			status = net_if_ipv4_set_netmask_by_addr(vlan_iface,
								 &addr4->sin_addr,
								 &mask.sin_addr);

		} else if (paddr->sa_family == AF_INET6) {
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)paddr;
			struct in6_addr netaddr6;

			ifaddr = net_if_ipv6_addr_add(vlan_iface, &addr6->sin6_addr,
						      NET_ADDR_MANUAL, 0);

			net_ipv6_addr_prefix_mask((uint8_t *)&addr6->sin6_addr,
						  (uint8_t *)&netaddr6,
						  mask_len);

			if (!net_if_ipv6_prefix_add(vlan_iface, &netaddr6, mask_len,
						    (uint32_t)0xffffffff)) {
				LOG_ERR("Cannot add %s to interface %d", my_addr,
					net_if_get_by_iface(vlan_iface));
				return -EINVAL;
			}

		} else {
			LOG_ERR("Cannot parse IP address \"%s\"", my_addr);
			return -EAFNOSUPPORT;
		}

		if (ifaddr == NULL) {
			LOG_ERR("Cannot add IP address \"%s\" to interface %d",
				my_addr, net_if_get_by_iface(vlan_iface));
			return -ENOENT;
		}

		addr_str = next;
	} while (addr_str != NULL && *addr_str != '\0');

	ret = net_eth_vlan_enable(eth_iface, vlan_tag);
	if (ret < 0) {
		LOG_ERR("Cannot enable VLAN for tag %d (%d)", vlan_tag, ret);
	}

	LOG_DBG("Interface %d VLAN tag %d setup done.",
		net_if_get_by_iface(vlan_iface), vlan_tag);

	/* Take the interface up if the setup was ok */
	net_if_up(vlan_iface);

	return 0;
}

int init_vlan(void)
{
	struct ud user_data;
	int ret;

	if (CONFIG_NET_VLAN_COUNT == 0) {
		LOG_DBG("No VLAN interfaces defined.");
		return 0;
	}

	memset(&user_data, 0, sizeof(user_data));

	net_if_foreach(iface_cb, &user_data);

	/* This sample has two VLANs. For the second one we need to manually
	 * create IP address for this test. But first the VLAN needs to be
	 * added to the interface so that IPv6 DAD can work properly.
	 */
	ret = setup_iface(user_data.eth, user_data.first,
			  CONFIG_NET_SAMPLE_COMMON_VLAN_SETUP_1);
	if (ret < 0) {
		return ret;
	}

	ret = setup_iface(user_data.eth, user_data.second,
			  CONFIG_NET_SAMPLE_COMMON_VLAN_SETUP_2);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
