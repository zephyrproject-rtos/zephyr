/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_coap_service_sample, LOG_LEVEL_DBG);

#include <zephyr/net/coap_service.h>
#include <zephyr/net/mld.h>

#ifdef CONFIG_NET_IPV6
#include "net_private.h"
#include "ipv6.h"
#endif

static const uint16_t coap_port = 5683;

#ifdef CONFIG_NET_IPV6

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#define MY_IP6ADDR \
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }

static int join_coap_multicast_group(void)
{
	static struct in6_addr my_addr = MY_IP6ADDR;
	static struct sockaddr_in6 mcast_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = ALL_NODES_LOCAL_COAP_MCAST,
		.sin6_port = htons(coap_port) };
	struct net_if_addr *ifaddr;
	struct net_if *iface;
	int ret;

	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("Could not get the default interface");
		return -ENOENT;
	}

#if defined(CONFIG_NET_CONFIG_SETTINGS)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_CONFIG_MY_IPV6_ADDR,
			  &my_addr) < 0) {
		LOG_ERR("Invalid IPv6 address %s",
			CONFIG_NET_CONFIG_MY_IPV6_ADDR);
	}
#endif

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Could not add unicast address to interface");
		return -EINVAL;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ret = net_ipv6_mld_join(iface, &mcast_addr.sin6_addr);
	if (ret < 0) {
		LOG_ERR("Cannot join %s IPv6 multicast group (%d)",
			net_sprint_ipv6_addr(&mcast_addr.sin6_addr), ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	return join_coap_multicast_group();
}

#else /* CONFIG_NET_IPV6 */

int main(void)
{
	return 0;
}

#endif /* CONFIG_NET_IPV6 */

COAP_SERVICE_DEFINE(coap_server, NULL, &coap_port, COAP_SERVICE_AUTOSTART);
