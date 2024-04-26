/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief IPv6 address autoconfiguration
 */

#include "ipv6.h"

LOG_MODULE_DECLARE(net_ipv6, CONFIG_NET_IPV6_LOG_LEVEL);

#include "net_private.h"

#define IPV6_SUBNET_BYTES 8
#define TWO_HOURS (2 * 60 * 60)

static inline uint32_t remaining_lifetime(struct net_if_addr *ifaddr)
{
	return net_timeout_remaining(&ifaddr->lifetime, k_uptime_get_32());
}

struct net_if_addr *net_ipv6_autoconf_addr_add(struct net_if *iface, const struct in6_addr *prefix,
					       uint32_t valid_lifetime)
{
	struct in6_addr addr = { };
	struct net_if_addr *ifaddr;

	/* Create IPv6 address using the given prefix and iid. We first
	 * setup link local address, and then copy prefix over first 8
	 * bytes of that address.
	 */
	net_ipv6_addr_create_iid(&addr, net_if_get_link_addr(iface));
	memcpy(&addr, prefix, IPV6_SUBNET_BYTES);

	ifaddr = net_if_ipv6_addr_lookup(&addr, NULL);
	if (ifaddr && ifaddr->addr_type == NET_ADDR_AUTOCONF) {
		if (valid_lifetime == NET_IPV6_ND_INFINITE_LIFETIME) {
			net_if_addr_set_lf(ifaddr, true);
			return ifaddr;
		}

		/* RFC 4862 ch 5.5.3 */
		if ((valid_lifetime > TWO_HOURS) || (valid_lifetime > remaining_lifetime(ifaddr))) {
			NET_DBG("Timer updating for address %s long lifetime %u secs",
				net_sprint_ipv6_addr(&addr), valid_lifetime);

			net_if_ipv6_addr_update_lifetime(ifaddr, valid_lifetime);
		} else {
			NET_DBG("Timer updating for address %s lifetime %u secs",
				net_sprint_ipv6_addr(&addr), TWO_HOURS);

			net_if_ipv6_addr_update_lifetime(ifaddr, TWO_HOURS);
		}

		net_if_addr_set_lf(ifaddr, false);
	} else {
		if (valid_lifetime == NET_IPV6_ND_INFINITE_LIFETIME) {
			ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF, 0);
		} else {
			ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF,
				 valid_lifetime);
		}
	}

	return ifaddr;
}
