/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdint.h>
#include <zephyr/net/socket.h>

#include "net_shell_private.h"

static int cmd_net_dhcpv6_client_start(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV6)
	struct net_if *iface = NULL;
	int idx;

	if (argc < 1) {
		PR_ERROR("Correct usage: net dhcpv6 client %s <index>\n", "start");
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

	net_dhcpv6_restart(iface);

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

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dhcpv6_client,
	SHELL_CMD_ARG(start, NULL, "Start the Dhcpv6 client operation on the interface.\n"
		      "'net dhcpv6 client start <index>'\n"
		      "<index> is the network interface index.",
		      cmd_net_dhcpv6_client_start, 2, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop the Dhcpv6 client operation on the interface.\n"
		      "'net dhcpv6 client stop <index>'\n"
		      "<index> is the network interface index.",
		      cmd_net_dhcpv6_client_stop, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dhcpv6,
	SHELL_CMD(client, &net_cmd_dhcpv6_client,
		  "Dhcpv6 client management.",
		  NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), dhcpv6, &net_cmd_dhcpv6, "Manage DHPCv6 services.",
		 NULL, 1, 0);
