/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_TEST_APP)
#define SYS_LOG_DOMAIN "net-test/main"
#define NET_SYS_LOG_LEVEL CONFIG_SYS_LOG_NET_LEVEL
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>

#include "common.h"

static struct k_sem quit_lock;

static struct interfaces network_interfaces;

void panic(const char *msg)
{
	if (msg) {
		NET_ERR("%s", msg);
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

void quit(void)
{
	k_sem_give(&quit_lock);
}

void iface_cb(struct net_if *iface, void *user_data)
{
	struct interfaces *interfaces = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (interfaces->non_vlan == iface) {
		NET_DBG("1st interface %p", iface);
		return;
	}

	if (!interfaces->first_vlan) {
		NET_DBG("2nd interface %p", iface);
		interfaces->first_vlan = iface;
		return;
	}

	if (!interfaces->second_vlan) {
		NET_DBG("3rd interface %p", iface);
		interfaces->second_vlan = iface;
		return;
	}
}

static inline int init_app(struct interfaces *interfaces)
{
	struct net_if *iface;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (!iface) {
		NET_ERR("No ethernet interfaces found.");
		return -ENOENT;
	}

	interfaces->non_vlan = iface;
	interfaces->first_vlan = NULL;
	interfaces->second_vlan = NULL;

	net_if_foreach(iface_cb, interfaces);

	k_sem_init(&quit_lock, 0, UINT_MAX);

	return 0;
}

static int setup_vlan_iface(struct net_if *iface, char *ipv6_addr,
			    char *ipv4_addr, char *ipv4_gw, char *ipv4_netmask)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;

	if (net_addr_pton(AF_INET6, ipv6_addr, &addr6)) {
		NET_ERR("Invalid address: %s", ipv6_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s to interface %p", ipv6_addr, iface);
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
		NET_ERR("Invalid address: %s", ipv4_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s to interface %p", ipv4_addr, iface);
		return -EINVAL;
	}

	if (ipv4_gw[0]) {
		/* If not empty */
		if (net_addr_pton(AF_INET, ipv4_gw, &addr4)) {
			NET_ERR("Invalid gateway: %s", ipv4_gw);
		} else {
			net_if_ipv4_set_gw(iface, &addr4);
		}
	}

	if (ipv4_netmask[0]) {
		if (net_addr_pton(AF_INET, ipv4_netmask, &addr4)) {
			NET_ERR("Invalid netmask: %s", ipv4_netmask);
		} else {
			net_if_ipv4_set_netmask(iface, &addr4);
		}
	}

	return 0;
}

static int setup_vlan_iface_1(struct net_if *iface)
{
	return setup_vlan_iface(iface,
				CONFIG_SAMPLE_IPV6_ADDR_1,
				CONFIG_SAMPLE_IPV4_ADDR_1,
				CONFIG_SAMPLE_IPV4_GW_1,
				CONFIG_SAMPLE_IPV4_NETMASK_1);
}

static int setup_vlan_iface_2(struct net_if *iface)
{
	return setup_vlan_iface(iface,
				CONFIG_SAMPLE_IPV6_ADDR_2,
				CONFIG_SAMPLE_IPV4_ADDR_2,
				CONFIG_SAMPLE_IPV4_GW_2,
				CONFIG_SAMPLE_IPV4_NETMASK_2);
}

static int setup_ip_addresses(struct interfaces *interfaces)
{
	if (interfaces->first_vlan) {
		setup_vlan_iface_1(interfaces->first_vlan);
	}

	if (interfaces->second_vlan) {
		setup_vlan_iface_2(interfaces->second_vlan);
	}

	return 0;
}

void main(void)
{
	int ret;

	ret = init_app(&network_interfaces);
	if (ret < 0) {
		NET_ERR("Cannot initialize application (%d)", ret);
		return;
	}

#if defined(CONFIG_NET_VLAN)
	setup_vlan(&network_interfaces);
#endif

	ret = setup_ip_addresses(&network_interfaces);
	if (ret < 0) {
		NET_ERR("Cannot set IP addresses (%d)", ret);
		return;
	}

	setup_echo_server();

	k_sem_take(&quit_lock, K_FOREVER);

	NET_INFO("Stopping...");

	cleanup_echo_server();
}
