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

#if defined(CONFIG_WIREGUARD)
#include <zephyr/net/virtual.h>
#include <zephyr/net/wireguard.h>
#include "wg_internal.h"
#endif

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

#if defined(CONFIG_WIREGUARD)
struct wg_route_peer_snapshot {
	int id;
	uint32_t last_tx;
	uint64_t keypair_sending_counter;
	k_timepoint_t keypair_expires;
	uint32_t keypair_last_rx;
	bool send_handshake;
	bool keypair_is_valid;
	bool keypair_is_initiator;
};

struct user_data_wg_route {
	struct net_if *iface;
	net_sa_family_t family;
	const uint8_t *dst;
	struct wg_route_peer_snapshot peer;
	bool has_peer;
	bool allowed;
};

static bool is_wireguard_iface(struct net_if *iface)
{
	if (iface == NULL || net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return false;
	}

	return (net_virtual_get_iface_capabilities(iface) & VIRTUAL_INTERFACE_VPN) != 0U;
}

static const struct wg_keypair *wg_route_select_keypair(const struct wg_peer *peer)
{
	const struct wg_keypair *keypair = &peer->session.keypair.current;

	if (keypair->is_valid && !keypair->is_initiator && keypair->last_rx == 0U) {
		keypair = &peer->session.keypair.prev;
	}

	return keypair;
}

static void wg_route_copy_peer_snapshot(struct wg_route_peer_snapshot *snapshot,
					const struct wg_peer *peer)
{
	const struct wg_keypair *keypair = wg_route_select_keypair(peer);

	snapshot->id = peer->id;
	snapshot->last_tx = peer->last_tx;
	snapshot->send_handshake = peer->send_handshake;
	snapshot->keypair_is_valid = keypair->is_valid;
	snapshot->keypair_is_initiator = keypair->is_initiator;
	snapshot->keypair_last_rx = keypair->last_rx;
	snapshot->keypair_expires = keypair->expires;
	snapshot->keypair_sending_counter = keypair->sending_counter;
}

static void wg_route_cb(struct wg_peer *peer, void *user_data)
{
	struct user_data_wg_route *data = user_data;
	bool allowed;

	if (peer->iface != data->iface) {
		return;
	}

	allowed = wg_peer_is_allowed_ip(peer, data->family, data->dst);

	if (!data->has_peer || (!data->allowed && allowed)) {
		wg_route_copy_peer_snapshot(&data->peer, peer);
		data->has_peer = true;
		data->allowed = allowed;
	}
}

static int print_wireguard_route_status(const struct shell *sh,
					struct net_if *iface,
					net_sa_family_t family,
					const uint8_t *dst)
{
	struct user_data_wg_route data = {
		.iface = iface,
		.family = family,
		.dst = dst,
	};
	const char *reason;
	bool can_send;
	int status;

	if (!is_wireguard_iface(iface)) {
		return 0;
	}

	wireguard_peer_foreach(wg_route_cb, &data);

	if (!data.has_peer) {
		PR_SHELL(sh, "\tWireGuard: no peer configured for interface %d\n",
			 net_if_get_by_iface(iface));
		return -ENOENT;
	}

	if (!data.allowed) {
		can_send = false;
		status = -ENETUNREACH;
		reason = "destination not in AllowedIP";
	} else if (!net_if_is_up(iface)) {
		can_send = false;
		status = -ENETDOWN;
		reason = "interface down";
	} else if (data.peer.last_tx == 0U) {
		can_send = false;
		status = -EAGAIN;
		reason = "handshake required";
	} else {
		if (!(data.peer.keypair_is_valid &&
		      (data.peer.keypair_is_initiator ||
		       data.peer.keypair_last_rx != 0U))) {
			can_send = false;
			status = -ENOTCONN;
			reason = "no valid session key";
		} else if (sys_timepoint_expired(data.peer.keypair_expires) ||
			   data.peer.keypair_sending_counter >= REJECT_AFTER_MESSAGES) {
			can_send = false;
			status = -EKEYEXPIRED;
			reason = "session key expired";
		} else {
			can_send = true;
			status = 0;
			reason = data.peer.send_handshake ?
				"current session usable, rekey pending" :
				"current session usable";
		}
	}

	PR_SHELL(sh, "\tWireGuard peer %d: AllowedIP %s, can send %s (%s)\n",
		 data.peer.id, data.allowed ? "yes" : "no",
		 can_send ? "yes" : "no", reason);

	return status;
}
#endif /* CONFIG_WIREGUARD */

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

	if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE) && net_if_flag_is_set(iface, NET_IF_IPV6)) {
		PR("\nIPv6 routes for interface %d (%p) (%s)\n",
		   net_if_get_by_iface(iface), iface,
		   iface2str(iface, &extra));
		PR("=========================================%s\n", extra);

		data->user_data = iface;

		net_route_ipv6_foreach(route_cb, data);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE) && net_if_flag_is_set(iface, NET_IF_IPV4)) {
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

static int cmd_net_route_check(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(NATIVE_ROUTE)
	struct net_sockaddr_storage addr = { 0 };
	struct net_if *iface = NULL;
	struct net_route_entry *route;
	net_sa_family_t family;
	const uint8_t *dst = NULL;
	int ret = 0;

	if (argc != 2) {
		PR_ERROR("Correct usage: net route check <destination>\n");
		return -EINVAL;
	}

	if (!net_ipaddr_parse(argv[1], strlen(argv[1]), net_sad(&addr))) {
		PR_ERROR("Invalid destination: %s\n", argv[1]);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE) && addr.ss_family == NET_AF_INET) {
		struct net_in_addr *addr4 = &net_sin(net_sad(&addr))->sin_addr;

		family = NET_AF_INET;
		dst = addr4->s4_addr;
		iface = net_if_ipv4_select_src_iface(addr4);
		if (iface == NULL) {
			PR_ERROR("No source interface for destination %s\n", argv[1]);
			return -ENOEXEC;
		}

		route = net_route_ipv4_lookup(iface, addr4);

	} else if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE) && addr.ss_family == NET_AF_INET6) {
		struct net_in6_addr *addr6 = &net_sin6(net_sad(&addr))->sin6_addr;

		family = NET_AF_INET6;
		dst = addr6->s6_addr;
		iface = net_if_ipv6_select_src_iface(addr6);
		if (iface == NULL) {
			PR_ERROR("No source interface for destination %s\n", argv[1]);
			return -ENOEXEC;
		}

		route = net_route_ipv6_lookup(iface, addr6);

	} else {
		PR("Unknown route family %d\n", addr.ss_family);
		return -EINVAL;
	}

	if (route == NULL) {
		/* There was no specific route to the destination, use the default
		 * route then.
		 */
		if (IS_ENABLED(CONFIG_NET_IPV4) && addr.ss_family == NET_AF_INET) {
			struct net_if_addr *ifaddr;

			ifaddr = net_if_ipv4_addr_lookup(&net_sin(net_sad(&addr))->sin_addr,
							 NULL);
			if (ifaddr == NULL) {
				struct net_in_addr gw;

				gw = net_if_ipv4_get_gw(iface);

				if (!net_ipv4_addr_cmp(&gw, net_ipv4_unspecified_address())) {
					PR("%s route to %s is via %s (interface %d)\n",
					   "IPv4", argv[1], net_sprint_ipv4_addr(&gw),
					   net_if_get_by_iface(iface));
				} else {
					PR("%s route to %s is via interface %d\n",
					   "IPv4", argv[1], net_if_get_by_iface(iface));
				}
			} else {
				PR("%s address %s is local address in interface %d\n",
				   "IPv4", argv[1], net_if_get_by_iface(iface));
			}

#if defined(CONFIG_WIREGUARD)
			if (ifaddr == NULL) {
				ret = print_wireguard_route_status(sh, iface, family, dst);
			}
#endif

			return ret;
		}

		if (IS_ENABLED(CONFIG_NET_IPV6) && addr.ss_family == NET_AF_INET6) {
			struct net_if_addr *ifaddr;
			struct net_in6_addr *addr6 = &net_sin6(net_sad(&addr))->sin6_addr;

			ifaddr = net_if_ipv6_addr_lookup(addr6, NULL);
			if (ifaddr == NULL) {
				struct net_if_router *router;

				router = net_if_ipv6_router_find_default(NULL, addr6);
				if (router != NULL) {
					PR("%s route to %s is via %s (interface %d)\n",
					   "IPv6", argv[1],
					   net_sprint_ipv6_addr(&router->address.in6_addr),
					   net_if_get_by_iface(iface));
				} else {
					PR("%s route to %s is via interface %d\n",
					   "IPv6", argv[1], net_if_get_by_iface(iface));
				}
			} else {
				PR("%s address %s is local address in interface %d\n",
				   "IPv6", argv[1], net_if_get_by_iface(iface));
			}

#if defined(CONFIG_WIREGUARD)
			if (ifaddr == NULL) {
				ret = print_wireguard_route_status(sh, iface, family, dst);
			}
#endif

			return ret;
		}

		PR_ERROR("Failed to get route\n");
		return -ENOEXEC;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_ROUTE) && route->addr.family == NET_AF_INET) {
		struct net_in_addr *nexthop = net_route_ipv4_get_nexthop(route);

		PR("%s route to %s is via %s (interface %d)\n",
		   "IPv4", argv[1], nexthop ? net_sprint_ipv4_addr(nexthop) : "<on-link>",
		   net_if_get_by_iface(route->iface));
	} else if (IS_ENABLED(CONFIG_NET_IPV6_ROUTE) && route->addr.family == NET_AF_INET6) {
		struct net_in6_addr *nexthop = net_route_ipv6_get_nexthop(route);

		PR("%s route to %s is via %s (interface %d)\n",
		   "IPv6", argv[1], nexthop ? net_sprint_ipv6_addr(nexthop) : "<on-link>",
		   net_if_get_by_iface(route->iface));
	} else {
		PR("Unknown route family %d\n", route->addr.family);
		return -EINVAL;
	}

#if defined(CONFIG_WIREGUARD)
	ret = print_wireguard_route_status(sh, route->iface, family, dst);
#endif

	return ret;

#else /* NATIVE_ROUTE */

	PR_INFO("Set %s and %s to enable native %s support."
		" And enable CONFIG_NET_IPV6_ROUTE or CONFIG_NET_IPV4_ROUTE.\n",
		"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6 or CONFIG_NET_IPV4", "IPv6 or IPv4");

	return 0;
#endif /* NATIVE_ROUTE */
}

static int cmd_net_route_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(NATIVE_ROUTE)
	struct net_if *iface = NULL;
	int idx;
	struct net_route_entry *route = NULL;
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
	SHELL_CMD(check, NULL,
		  SHELL_HELP("Show how the packet will be routed to the destination",
			     "<destination>"),
		  cmd_net_route_check),
	SHELL_CMD(del, NULL,
		  SHELL_HELP("Deletes the route to the destination",
			     "<index> <destination>"),
		  cmd_net_route_del),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), route, &net_cmd_route,
		 "Show network route.",
		 cmd_net_route, 1, 0);
