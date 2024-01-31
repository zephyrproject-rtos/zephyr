/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/igmp.h>

#include "net_shell_private.h"
#include "../ip/ipv4.h"

#if defined(CONFIG_NET_NATIVE_IPV4)
static void ip_address_lifetime_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	const char *extra;
	int i;

	ARG_UNUSED(user_data);

	PR("\nIPv4 addresses for interface %d (%p) (%s)\n",
	   net_if_get_by_iface(iface), iface, iface2str(iface, &extra));
	PR("============================================%s\n", extra);

	if (!ipv4) {
		PR("No IPv4 config found for this interface.\n");
		return;
	}

	PR("Type      \tState    \tLifetime (sec)\tAddress\n");

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		PR("%s  \t%s    \t%12s/%12s\n",
		       addrtype2str(ipv4->unicast[i].ipv4.addr_type),
		       addrstate2str(ipv4->unicast[i].ipv4.addr_state),
		       net_sprint_ipv4_addr(
			       &ipv4->unicast[i].ipv4.address.in_addr),
		       net_sprint_ipv4_addr(
			       &ipv4->unicast[i].netmask));
	}
}
#endif /* CONFIG_NET_NATIVE_IPV4 */

static int cmd_net_ipv4(const struct shell *sh, size_t argc, char *argv[])
{
	PR("IPv4 support                              : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV4) ?
	   "enabled" : "disabled");
	if (!IS_ENABLED(CONFIG_NET_IPV4)) {
		return -ENOEXEC;
	}

#if defined(CONFIG_NET_NATIVE_IPV4)
	struct net_shell_user_data user_data;

	PR("IPv4 fragmentation support                : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV4_FRAGMENT) ? "enabled" :
	   "disabled");
	PR("Max number of IPv4 network interfaces "
	   "in the system          : %d\n",
	   CONFIG_NET_IF_MAX_IPV4_COUNT);
	PR("Max number of unicast IPv4 addresses "
	   "per network interface   : %d\n",
	   CONFIG_NET_IF_UNICAST_IPV4_ADDR_COUNT);
	PR("Max number of multicast IPv4 addresses "
	   "per network interface : %d\n",
	   CONFIG_NET_IF_MCAST_IPV4_ADDR_COUNT);

	user_data.sh = sh;
	user_data.user_data = NULL;

	/* Print information about address lifetime */
	net_if_foreach(ip_address_lifetime_cb, &user_data);
#endif

	return 0;
}

static int cmd_net_ip_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV4)
	struct net_if *iface = NULL;
	int idx;
	struct in_addr addr;

	if (argc < 3) {
		PR_ERROR("Correct usage: net ipv4 add <index> <address> [<netmask>]\n");
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

	if (net_addr_pton(AF_INET, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (net_ipv4_is_addr_mcast(&addr)) {
		int ret;

		ret = net_ipv4_igmp_join(iface, &addr, NULL);
		if (ret < 0) {
			PR_ERROR("Cannot %s multicast group %s for interface %d (%d)\n",
				 "join", net_sprint_ipv4_addr(&addr), idx, ret);
			return ret;
		}
	} else {
		struct net_if_addr *ifaddr;
		struct in_addr netmask;

		if (argc < 4) {
			PR_ERROR("Netmask is missing.\n");
			return -EINVAL;
		}

		ifaddr = net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
		if (ifaddr == NULL) {
			PR_ERROR("Cannot add address %s to interface %d\n",
				 net_sprint_ipv4_addr(&addr), idx);
			return -ENOMEM;
		}

		if (net_addr_pton(AF_INET, argv[3], &netmask)) {
			PR_ERROR("Invalid netmask: %s", argv[3]);
			return -EINVAL;
		}

		net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask);
	}

#else /* CONFIG_NET_NATIVE_IPV4 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV4", "IPv4");
#endif /* CONFIG_NET_NATIVE_IPV4 */
	return 0;
}

static int cmd_net_ip_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV4)
	struct net_if *iface = NULL;
	int idx;
	struct in_addr addr;

	if (argc != 3) {
		PR_ERROR("Correct usage: net ipv4 del <index> <address>");
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

	if (net_addr_pton(AF_INET, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (net_ipv4_is_addr_mcast(&addr)) {
		int ret;

		ret = net_ipv4_igmp_leave(iface, &addr);
		if (ret < 0) {
			PR_ERROR("Cannot %s multicast group %s for interface %d (%d)\n",
				 "leave", net_sprint_ipv4_addr(&addr), idx, ret);
			return ret;
		}
	} else {
		if (!net_if_ipv4_addr_rm(iface, &addr)) {
			PR_ERROR("Failed to delete %s\n", argv[2]);
			return -ENOEXEC;
		}
	}
#else /* CONFIG_NET_NATIVE_IPV4 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV4", "IPv4");
#endif /* CONFIG_NET_NATIVE_IPV4 */
	return 0;
}

static int cmd_net_ip_gateway(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV4)
	struct net_if *iface;
	int idx;
	struct in_addr addr;

	if (argc != 3) {
		PR_ERROR("Correct usage: net ipv4 gateway <index> <gateway_ip>\n");
		return -ENOEXEC;
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

	if (net_addr_pton(AF_INET, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	net_if_ipv4_set_gw(iface, &addr);

#else /* CONFIG_NET_NATIVE_IPV4 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV4", "IPv4");
#endif /* CONFIG_NET_NATIVE_IPV4 */
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ip,
	SHELL_CMD(add, NULL,
		  "'net ipv4 add <index> <address> [<netmask>]' adds the address to the interface.",
		  cmd_net_ip_add),
	SHELL_CMD(del, NULL,
		  "'net ipv4 del <index> <address>' deletes the address from the interface.",
		  cmd_net_ip_del),
	SHELL_CMD(gateway, NULL,
		  "'net ipv4 gateway <index> <gateway_ip>' sets IPv4 gateway for the interface.",
		  cmd_net_ip_gateway),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), ipv4, &net_cmd_ip,
		 "Print information about IPv4 specific information and "
		 "configuration.",
		 cmd_net_ipv4, 1, 0);
