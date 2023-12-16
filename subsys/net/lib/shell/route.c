/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#include "../ip/route.h"

#if defined(CONFIG_NET_ROUTE) && defined(CONFIG_NET_NATIVE)
static void route_cb(struct net_route_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if *iface = data->user_data;
	struct net_route_nexthop *nexthop_route;
	int count;
	uint32_t now = k_uptime_get_32();

	if (entry->iface != iface) {
		return;
	}

	PR("IPv6 prefix : %s/%d\n", net_sprint_ipv6_addr(&entry->addr),
	   entry->prefix_len);

	count = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&entry->nexthop, nexthop_route, node) {
		struct net_linkaddr_storage *lladdr;
		char remaining_str[sizeof("01234567890 sec")];
		uint32_t remaining;

		if (!nexthop_route->nbr) {
			continue;
		}

		PR("\tneighbor : %p\t", nexthop_route->nbr);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			PR("addr : <unknown>\t");
		} else {
			lladdr = net_nbr_get_lladdr(nexthop_route->nbr->idx);

			PR("addr : %s\t", net_sprint_ll_addr(lladdr->addr,
							     lladdr->len));
		}

		if (entry->is_infinite) {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "infinite");
		} else {
			remaining = net_timeout_remaining(&entry->lifetime, now);
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "%u sec", remaining);
		}

		PR("lifetime : %s\n", remaining_str);

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}
}

static void iface_per_route_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	const char *extra;

	PR("\nIPv6 routes for interface %d (%p) (%s)\n",
	   net_if_get_by_iface(iface), iface,
	   iface2str(iface, &extra));
	PR("=========================================%s\n", extra);

	data->user_data = iface;

	net_route_foreach(route_cb, data);
}
#endif /* CONFIG_NET_ROUTE */

#if defined(CONFIG_NET_ROUTE_MCAST) && defined(CONFIG_NET_NATIVE)
static void route_mcast_cb(struct net_route_entry_mcast *entry,
			   void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if *iface = data->user_data;
	const char *extra;

	if (entry->iface != iface) {
		return;
	}

	PR("IPv6 multicast route %p for interface %d (%p) (%s)\n", entry,
	   net_if_get_by_iface(iface), iface, iface2str(iface, &extra));
	PR("==========================================================="
	   "%s\n", extra);

	PR("IPv6 group     : %s\n", net_sprint_ipv6_addr(&entry->group));
	PR("IPv6 group len : %d\n", entry->prefix_len);
	PR("Lifetime       : %u\n", entry->lifetime);
}

static void iface_per_mcast_route_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;

	data->user_data = iface;

	net_route_mcast_foreach(route_mcast_cb, NULL, data);
}
#endif /* CONFIG_NET_ROUTE_MCAST */

static int cmd_net_ip6_route_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6) && (CONFIG_NET_ROUTE)
	struct net_if *iface = NULL;
	int idx;
	struct net_route_entry *route;
	struct in6_addr gw = {0};
	struct in6_addr prefix = {0};

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

	if (net_addr_pton(AF_INET6, argv[2], &prefix)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET6, argv[3], &gw)) {
		PR_ERROR("Invalid gateway: %s\n", argv[3]);
		return -EINVAL;
	}

	route = net_route_add(iface, &prefix, NET_IPV6_DEFAULT_PREFIX_LEN,
				&gw, NET_IPV6_ND_INFINITE_LIFETIME,
				NET_ROUTE_PREFERENCE_MEDIUM);
	if (route == NULL) {
		PR_ERROR("Failed to add route\n");
		return -ENOEXEC;
	}

#else /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	PR_INFO("Set %s and %s to enable native %s support."
			" And enable CONFIG_NET_ROUTE.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	return 0;
}

static int cmd_net_ip6_route_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6) && (CONFIG_NET_ROUTE)
	struct net_if *iface = NULL;
	int idx;
	struct net_route_entry *route;
	struct in6_addr prefix = { 0 };

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

	if (net_addr_pton(AF_INET6, argv[2], &prefix)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	route = net_route_lookup(iface, &prefix);
	if (route) {
		net_route_del(route);
	}
#else /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	PR_INFO("Set %s and %s to enable native %s support."
			" And enable CONFIG_NET_ROUTE\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_ROUTE */
	return 0;
}

static int cmd_net_route(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_NATIVE)
#if defined(CONFIG_NET_ROUTE) || defined(CONFIG_NET_ROUTE_MCAST)
	struct net_shell_user_data user_data;
#endif

#if defined(CONFIG_NET_ROUTE) || defined(CONFIG_NET_ROUTE_MCAST)
	user_data.sh = sh;
#endif

#if defined(CONFIG_NET_ROUTE)
	net_if_foreach(iface_per_route_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_ROUTE",
		"network route");
#endif

#if defined(CONFIG_NET_ROUTE_MCAST)
	net_if_foreach(iface_per_mcast_route_cb, &user_data);
#endif
#endif
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_route,
	SHELL_CMD(add, NULL,
		  "'net route add <index> <destination> <gateway>'"
		  " adds the route to the destination.",
		  cmd_net_ip6_route_add),
	SHELL_CMD(del, NULL,
		  "'net route del <index> <destination>'"
		  " deletes the route to the destination.",
		  cmd_net_ip6_route_del),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), route, &net_cmd_route,
		 "Show network route.",
		 cmd_net_route, 1, 0);
