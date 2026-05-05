/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#include "../ip/route_ipv6.h"
#include "../ip/route_ipv4.h"

#if (defined(CONFIG_NET_NATIVE_IPV6) && defined(CONFIG_NET_IPV6_ROUTE)) || \
	(defined(CONFIG_NET_NATIVE_IPV4) && defined(CONFIG_NET_IPV4_ROUTE))
#define NATIVE_ROUTE
#else
#if defined(NATIVE_ROUTE)
#undef NATIVE_ROUTE
#endif
#endif

#if defined(CONFIG_NET_NATIVE) && defined(NATIVE_ROUTE)

struct user_data_lladdr {
	const struct net_in_addr *nexthop;
	uint8_t *lladdr;
	size_t lladdr_len;
	bool found;
};

#if defined(CONFIG_NET_ARP)
#include "ethernet/arp.h"

static void arp_cb(struct arp_entry *entry, void *user_data)
{
	struct user_data_lladdr *data = user_data;

	if (!data->found && net_ipv4_addr_cmp(&entry->ip, data->nexthop)) {
		memcpy(data->lladdr, entry->eth.addr,
		       MIN(sizeof(entry->eth.addr), data->lladdr_len));
		data->found = true;
	}
}
#endif

static int get_ipv4_lladdr(const struct net_in_addr *nexthop,
			   uint8_t *lladdr, size_t lladdr_len)
{
#if defined(CONFIG_NET_ARP)
	if (nexthop == NULL || lladdr == NULL || lladdr_len == 0U) {
		return -ENOENT;
	}

	struct user_data_lladdr user_data = {
		.nexthop = nexthop,
		.lladdr = lladdr,
		.lladdr_len = lladdr_len,
		.found = false,
	};

	net_arp_foreach(arp_cb, &user_data);

	if (!user_data.found) {
		return -ENOENT;
	}

	return MIN(NET_ETH_ADDR_LEN, user_data.lladdr_len);
#else
	return -ENOTSUP;
#endif /* CONFIG_NET_ARP */
}

static void route_cb(struct net_route_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if *iface = data->user_data;
	struct net_nbr *nbr;
	uint32_t now = k_uptime_get_32();
	char remaining_str[sizeof("01234567890 sec")];
	uint32_t remaining;

	if (entry->iface != iface) {
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE) && entry->addr.family == NET_AF_INET6) {
		struct net_in6_addr *nexthop;

		PR("IPv6 prefix : %s/%d\n", net_sprint_ipv6_addr(&entry->addr.in6_addr),
		   entry->prefix_len);

		nexthop = net_route_ipv6_get_nexthop(entry);
		nbr = net_route_ipv6_get_nbr(entry);

		if (entry->is_infinite) {
			snprintk(remaining_str, sizeof(remaining_str) - 1, "infinite");
		} else {
			remaining = net_timeout_remaining(&entry->lifetime, now);
			snprintk(remaining_str, sizeof(remaining_str) - 1, "%u sec", remaining);
		}

		PR("\tnexthop  : %s\t",
		   nexthop ? net_sprint_ipv6_addr(nexthop) : "<unknown>");

		if (nbr != NULL && nbr->idx != NET_NBR_LLADDR_UNKNOWN) {
			struct net_linkaddr *lladdr = net_nbr_get_lladdr(nbr->idx);

			PR("ll addr : %s\t", net_sprint_ll_addr(lladdr->addr, lladdr->len));
		} else {
			PR("ll addr : <unknown>\t");
		}

	} else if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE) && entry->addr.family == NET_AF_INET) {
		struct net_in_addr *nexthop;
		uint8_t lladdr[NET_LINK_ADDR_MAX_LENGTH];
		int ret;

		PR("IPv4 prefix : %s/%d\n", net_sprint_ipv4_addr(&entry->addr.in_addr),
		   entry->prefix_len);

		nexthop = net_route_ipv4_get_nexthop(entry);
		ret = nexthop != NULL ? get_ipv4_lladdr(nexthop, lladdr, sizeof(lladdr)) :
		      -ENOENT;

		if (entry->is_infinite) {
			snprintk(remaining_str, sizeof(remaining_str) - 1, "infinite");
		} else {
			remaining = net_timeout_remaining(&entry->lifetime, now);
			snprintk(remaining_str, sizeof(remaining_str) - 1, "%u sec", remaining);
		}

		PR("\tnexthop  : %s\t",
		   nexthop ? net_sprint_ipv4_addr(nexthop) : "<on-link>");

		if (ret > 0 && ret <= sizeof(lladdr)) {
			PR("ll addr : %s\t", net_sprint_ll_addr(lladdr, ret));
		} else {
			PR("ll addr : <unknown>\t");
		}
	} else {
		PR("Unknown route family %d\n", entry->addr.family);
		return;
	}

	PR("lifetime : %s\n", remaining_str);
}

static void iface_per_route_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	const char *extra;

	if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE)) {
		PR("\nIPv6 routes for interface %d (%p) (%s)\n",
		   net_if_get_by_iface(iface), iface,
		   iface2str(iface, &extra));
		PR("=========================================%s\n", extra);

		data->user_data = iface;

		net_route_ipv6_foreach(route_cb, data);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE)) {
		PR("\nIPv4 routes for interface %d (%p) (%s)\n",
		   net_if_get_by_iface(iface), iface,
		   iface2str(iface, &extra));
		PR("=========================================%s\n", extra);

		data->user_data = iface;

		net_route_ipv4_foreach(route_cb, data);
	}
}
#endif /* NATIVE_ROUTE */

#if defined(CONFIG_NET_IPV6_ROUTE_MCAST) && defined(CONFIG_NET_NATIVE)
static void route_mcast_cb(struct net_route_ipv6_entry_mcast *entry,
			   void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;

	PR("IPv6 multicast route (%p)\n", entry);
	PR("=================================\n");

	PR("IPv6 group     : %s\n", net_sprint_ipv6_addr(&entry->group));
	PR("IPv6 group len : %d\n", entry->prefix_len);
	PR("Lifetime       : %u\n", entry->lifetime);

	for (int i = 0; i < CONFIG_NET_IPV6_MCAST_ROUTE_MAX_IFACES; ++i) {
		if (entry->ifaces[i]) {
			PR("Interface      : %d (%p) %s\n", net_if_get_by_iface(entry->ifaces[i]),
			   entry->ifaces[i], iface2str(entry->ifaces[i], NULL));
		}
	}
}
#endif /* CONFIG_NET_IPV6_ROUTE_MCAST */

static int cmd_net_route_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(NATIVE_ROUTE)
	struct net_if *iface = NULL;
	int idx;
	struct net_route_entry *route;
	struct net_sockaddr_storage addr;
	const char *str;
	uint8_t mask_len;

	if (argc != 4) {
		PR_ERROR("Correct usage: net route add <index> "
				 "<destination> <gateway>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	str = net_ipaddr_parse_mask(argv[2], strlen(argv[2]), net_sad(&addr), &mask_len);
	if (str == NULL) {
		PR_ERROR("Invalid destination: %s\n", argv[2]);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE) && addr.ss_family == NET_AF_INET) {
		struct net_in_addr gw4 = {0};

		if (net_addr_pton(NET_AF_INET, argv[3], &gw4)) {
			PR_ERROR("Invalid gateway: %s\n", argv[3]);
			return -EINVAL;
		}

		route = net_route_ipv4_add(iface, &net_sin(net_sad(&addr))->sin_addr,
					   mask_len,
					   &gw4, NET_ROUTE_INFINITE_LIFETIME,
					   NET_ROUTE_PREFERENCE_MEDIUM);

	} else if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE) && addr.ss_family == NET_AF_INET6) {
		struct net_in6_addr gw6 = {0};

		if (net_addr_pton(NET_AF_INET6, argv[3], &gw6)) {
			PR_ERROR("Invalid gateway: %s\n", argv[3]);
			return -EINVAL;
		}

		route = net_route_ipv6_add(iface, &net_sin6(net_sad(&addr))->sin6_addr,
					   mask_len,
					   &gw6, NET_IPV6_ND_INFINITE_LIFETIME,
					   NET_ROUTE_PREFERENCE_MEDIUM);
	} else {
		PR("Unknown route family %d\n", addr.ss_family);
		return -EINVAL;
	}

	if (route == NULL) {
		PR_ERROR("Failed to add route\n");
		return -ENOEXEC;
	}

#else /* NATIVE_ROUTE */

	PR_INFO("Set %s and %s to enable native %s support."
		" And enable CONFIG_NET_IPV6_ROUTE or CONFIG_NET_IPV4_ROUTE.\n",
		"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6 or CONFIG_NET_IPV4", "IPv6 or IPv4");

#endif /* NATIVE_ROUTE */

	return 0;
}

static int cmd_net_route_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(NATIVE_ROUTE)
	struct net_if *iface = NULL;
	int idx;
	struct net_route_entry *route;
	struct net_sockaddr_storage addr;
	const char *str;
	uint8_t mask_len;

	if (argc != 3) {
		PR_ERROR("Correct usage: net route del <index> <destination>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	str = net_ipaddr_parse_mask(argv[2], strlen(argv[2]), net_sad(&addr), &mask_len);
	if (str == NULL) {
		PR_ERROR("Invalid destination: %s\n", argv[2]);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE) && addr.ss_family == NET_AF_INET) {
		route = net_route_ipv4_lookup(iface, &net_sin(net_sad(&addr))->sin_addr);
		if (route != NULL) {
			net_route_ipv4_del(route);
		}

	} else if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE) && addr.ss_family == NET_AF_INET6) {
		route = net_route_ipv6_lookup(iface, &net_sin6(net_sad(&addr))->sin6_addr);
		if (route != NULL) {
			net_route_ipv6_del(route);
		}
	}

	if (route == NULL) {
		PR_ERROR("No such route \"%s\" found\n", argv[2]);
		return -ENOEXEC;
	}

#else /* NATIVE_ROUTE */

	PR_INFO("Set %s and %s to enable native %s support."
		" And enable CONFIG_NET_IPV6_ROUTE or CONFIG_NET_IPV4_ROUTE.\n",
		"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6 or CONFIG_NET_IPV4", "IPv6 or IPv4");

#endif /* NATIVE_ROUTE */

	return 0;
}

static int cmd_net_route(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(NATIVE_ROUTE) || defined(CONFIG_NET_IPV6_ROUTE_MCAST)
	struct net_shell_user_data user_data;

	user_data.sh = sh;
#endif

#if defined(NATIVE_ROUTE)
	net_if_foreach(iface_per_route_cb, &user_data);
#endif

#if defined(CONFIG_NET_IPV6_ROUTE_MCAST)
	net_route_ipv6_mcast_foreach(route_mcast_cb, NULL, &user_data);
#endif

#if !defined(CONFIG_NET_IPV6_ROUTE)
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_IPV6_ROUTE",
		"network route");
#endif
#if !defined(CONFIG_NET_IPV4_ROUTE)
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_IPV4_ROUTE",
		"network route");
#endif
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_route,
	SHELL_CMD(add, NULL,
		  SHELL_HELP("Adds the route to the destination",
			     "<index> <destination> <gateway>"),
		  cmd_net_route_add),
	SHELL_CMD(del, NULL,
		  SHELL_HELP("Deletes the route to the destination",
			     "<index> <destination>"),
		  cmd_net_route_del),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), route, &net_cmd_route,
		 "Show network route.",
		 cmd_net_route, 1, 0);
