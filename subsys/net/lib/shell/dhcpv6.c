/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdint.h>
#include <stdlib.h>
#include <zephyr/net/dhcpv6.h>
#include <zephyr/net/dhcpv6_server.h>
#include <zephyr/net/socket.h>

#include "net_shell_private.h"

static int cmd_net_dhcpv6_server_start(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV6_SERVER)
	struct net_dhcpv6_server_params params = { 0 };
	struct net_if *iface = NULL;
	char *endptr;
	long prefix_len;
	int idx, ret;

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	if (net_addr_pton(NET_AF_INET6, argv[2], &params.prefix)) {
		PR_ERROR("Invalid prefix: %s\n", argv[2]);
		return -EINVAL;
	}

	prefix_len = strtol(argv[3], &endptr, 10);
	if (*endptr != '\0' || prefix_len < 1 || prefix_len > 128) {
		PR_ERROR("Invalid prefix length: %s\n", argv[3]);
		return -EINVAL;
	}

	params.prefix_len = (uint8_t)prefix_len;
	params.offer_prefix = true;

	if (argc > 4) {
		if (net_addr_pton(NET_AF_INET6, argv[4], &params.addr)) {
			PR_ERROR("Invalid address: %s\n", argv[4]);
			return -EINVAL;
		}

		params.offer_addr = true;
	}

	ret = net_dhcpv6_server_start(iface, &params);
	if (ret == -EALREADY) {
		PR_WARNING("DHCPv6 server already running on interface %d\n", idx);
	} else if (ret < 0) {
		PR_ERROR("DHCPv6 server failed to start on interface %d, error %d\n",
			 idx, -ret);
	} else {
		PR("DHCPv6 server started on interface %d\n", idx);
	}
#else /* CONFIG_NET_DHCPV6_SERVER */
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DHCPV6_SERVER", "DHCPv6 server");
#endif /* CONFIG_NET_DHCPV6_SERVER */
	return 0;
}

static int cmd_net_dhcpv6_server_stop(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV6_SERVER)
	struct net_if *iface = NULL;
	int idx, ret;

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	ret = net_dhcpv6_server_stop(iface);
	if (ret == -ENOENT) {
		PR_WARNING("DHCPv6 server is not running on interface %d\n", idx);
	} else if (ret < 0) {
		PR_ERROR("DHCPv6 server failed to stop on interface %d, error %d\n",
			 idx, -ret);
	} else {
		PR("DHCPv6 server stopped on interface %d\n", idx);
	}
#else /* CONFIG_NET_DHCPV6_SERVER */
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DHCPV6_SERVER", "DHCPv6 server");
#endif /* CONFIG_NET_DHCPV6_SERVER */
	return 0;
}

#if defined(CONFIG_NET_DHCPV6_SERVER)
static void dhcpv6_lease_cb(struct net_if *iface,
			    struct net_dhcpv6_server_lease *lease,
			    void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char iface_name[NET_IFNAMSIZ] = "";
	char addr_str[INET6_ADDRSTRLEN] = "-";
	char prefix_str[INET6_ADDRSTRLEN + sizeof("/128")] = "-";
	uint32_t expiry_s;

	if (*count == 0) {
		PR("     Iface  IA_NA address                   "
		   "IA_PD prefix              Expiry (sec)\n");
	}

	(*count)++;

	(void)net_if_get_name(iface, iface_name, sizeof(iface_name));

	if (lease->has_addr) {
		(void)net_addr_ntop(NET_AF_INET6, &lease->addr, addr_str,
				    sizeof(addr_str));
	}

	if (lease->has_prefix) {
		char pfx[INET6_ADDRSTRLEN];

		(void)net_addr_ntop(NET_AF_INET6, &lease->prefix, pfx,
				    sizeof(pfx));
		snprintk(prefix_str, sizeof(prefix_str), "%s/%u", pfx,
			 lease->prefix_len);
	}

	if (lease->expiry == 0) {
		expiry_s = 0;
	} else {
		int64_t remaining = (lease->expiry - k_uptime_get()) /
				    MSEC_PER_SEC;

		expiry_s = (remaining <= 0) ? 0U : (uint32_t)remaining;
	}

	PR("%2d. %6s  %-31s %-18s %12u\n",
	   *count, iface_name, addr_str, prefix_str, expiry_s);
}
#endif /* CONFIG_NET_DHCPV6_SERVER */

static int cmd_net_dhcpv6_server_status(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV6_SERVER)
	struct net_shell_user_data user_data;
	struct net_if *iface = NULL;
	int idx = 0, ret;
	int count = 0;

	if (argc > 1) {
		idx = get_iface_idx(sh, argv[1]);
		if (idx < 0) {
			return -ENOEXEC;
		}

		iface = net_if_get_by_index(idx);
		if (!iface) {
			PR_WARNING("No such interface in index %d\n", idx);
			return -ENOEXEC;
		}
	}

	user_data.sh = sh;
	user_data.user_data = &count;

	ret = net_dhcpv6_server_foreach_lease(iface, dhcpv6_lease_cb, &user_data);
	if (ret == -ENOENT) {
		PR_WARNING("DHCPv6 server is not running on interface %d\n", idx);
	} else if (count == 0) {
		PR("DHCPv6 server - no leases assigned\n");
	}
#else /* CONFIG_NET_DHCPV6_SERVER */
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DHCPV6_SERVER", "DHCPv6 server");
#endif /* CONFIG_NET_DHCPV6_SERVER */
	return 0;
}

static int cmd_net_dhcpv6_client_start(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV6)
	struct net_if *iface = NULL;
	int idx;

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOEXEC;
	}

	/* Additional arguments are downstream interface indices onto which the
	 * delegated prefix is sub-delegated, turning this node into a
	 * requesting router (RFC 8415).
	 */
	if (argc > 2) {
		struct net_dhcpv6_params params = {
			.request_addr = true,
			.request_prefix = true,
		};

		for (size_t i = 2; i < argc; i++) {
			char *endptr;
			long down;

			if (params.downstream_count >= ARRAY_SIZE(params.downstream_ifaces)) {
				PR_WARNING("Too many downstream interfaces, "
					   "increase %s\n",
					   "CONFIG_NET_DHCPV6_MAX_DOWNSTREAM");
				break;
			}

			down = strtol(argv[i], &endptr, 10);
			if (*endptr != '\0' || down <= 0) {
				PR_ERROR("Invalid downstream interface: %s\n", argv[i]);
				return -EINVAL;
			}

			params.downstream_ifaces[params.downstream_count++] = (int)down;
		}

		net_dhcpv6_start(iface, &params);
	} else {
		net_dhcpv6_restart(iface);
	}

#else /* CONFIG_NET_DHCPV6 */
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_DHCPV6", "Dhcpv6");
#endif /* CONFIG_NET_DHCPV6 */
	return 0;
}

static int cmd_net_dhcpv6_client_stop(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV6)
	struct net_if *iface = NULL;
	int idx;

	if (argc < 1) {
		PR_ERROR("Correct usage: net dhcpv6 client %s <index>\n", "stop");
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

	net_dhcpv6_stop(iface);

#else /* CONFIG_NET_DHCPV6 */
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_DHCPV6", "Dhcpv6");
#endif /* CONFIG_NET_DHCPV6 */
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dhcpv6_server,
	SHELL_CMD_ARG(start, NULL,
		      SHELL_HELP("Start the DHCPv6 server operation on the interface",
				 "<index> <prefix> <prefix len> [<address>]\n"
				 "<prefix>/<prefix len> is the pool used for prefix "
				 "delegation (IA_PD)\n"
				 "<address> is the optional base address for the "
				 "address pool (IA_NA)"),
		      cmd_net_dhcpv6_server_start, 4, 1),
	SHELL_CMD_ARG(stop, NULL,
		      SHELL_HELP("Stop the DHCPv6 server operation on the interface",
				 "<index>"),
		      cmd_net_dhcpv6_server_stop, 2, 0),
	SHELL_CMD_ARG(status, NULL,
		      SHELL_HELP("Print the DHCPv6 server status on the interface",
				 "<index>"),
		      cmd_net_dhcpv6_server_status, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dhcpv6_client,
	SHELL_CMD_ARG(start, NULL,
		      SHELL_HELP("Start the Dhcpv6 client operation on the interface",
				 "<index> [<downstream index> ...]\n"
				 "When one or more downstream interface indices are "
				 "given, a delegated prefix (IA_PD) is requested and "
				 "each downstream interface is assigned a distinct /64 "
				 "advertised via Router Advertisements"),
		      cmd_net_dhcpv6_client_start, 2, NET_DHCPV6_MAX_DOWNSTREAM),
	SHELL_CMD_ARG(stop, NULL,
		      SHELL_HELP("Stop the Dhcpv6 client operation on the interface",
				 "<index>"),
		      cmd_net_dhcpv6_client_stop, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dhcpv6,
	SHELL_CMD(server, &net_cmd_dhcpv6_server,
		  SHELL_HELP("DHCPv6 server service management", ""),
		  NULL),
	SHELL_CMD(client, &net_cmd_dhcpv6_client,
		  SHELL_HELP("Dhcpv6 client management", ""),
		  NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), dhcpv6, &net_cmd_dhcpv6, "Manage DHPCv6 services.",
		 NULL, 1, 0);
