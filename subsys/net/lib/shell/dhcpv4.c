/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdint.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/socket.h>

#include "net_shell_private.h"

static int cmd_net_dhcpv4_server_start(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV4_SERVER)
	struct net_if *iface = NULL;
	struct in_addr base_addr;
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

	if (net_addr_pton(AF_INET, argv[2], &base_addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	ret = net_dhcpv4_server_start(iface, &base_addr);
	if (ret == -EALREADY) {
		PR_WARNING("DHCPv4 server already running on interface %d\n", idx);
	} else if (ret < 0) {
		PR_ERROR("DHCPv4 server failed to start on interface %d, error %d\n",
			 idx, -ret);
	} else {
		PR("DHCPv4 server started on interface %d\n", idx);
	}
#else /* CONFIG_NET_DHCPV4_SERVER */
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DHCPV4_SERVER", "DHCPv4 server");
#endif /* CONFIG_NET_DHCPV4_SERVER */
	return 0;
}

static int cmd_net_dhcpv4_server_stop(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV4_SERVER)
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

	ret = net_dhcpv4_server_stop(iface);
	if (ret == -ENOENT) {
		PR_WARNING("DHCPv4 server is not running on interface %d\n", idx);
	} else if (ret < 0) {
		PR_ERROR("DHCPv4 server failed to stop on interface %d, error %d\n",
			 idx, -ret);
	} else {
		PR("DHCPv4 server stopped on interface %d\n", idx);
	}
#else /* CONFIG_NET_DHCPV4_SERVER */
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DHCPV4_SERVER", "DHCPv4 server");
#endif /* CONFIG_NET_DHCPV4_SERVER */
	return 0;
}

#if defined(CONFIG_NET_DHCPV4_SERVER)
static const char *dhcpv4_addr_state_to_str(enum dhcpv4_server_addr_state state)
{
	switch (state) {
	case DHCPV4_SERVER_ADDR_FREE:
		return "FREE";
	case DHCPV4_SERVER_ADDR_RESERVED:
		return "RESERVED";
	case DHCPV4_SERVER_ADDR_ALLOCATED:
		return "ALLOCATED";
	case DHCPV4_SERVER_ADDR_DECLINED:
		return "DECLINED";
	}

	return "<UNKNOWN>";
}

static uint32_t timepoint_to_s(k_timepoint_t timepoint)
{
	k_timeout_t timeout = sys_timepoint_timeout(timepoint);

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return 0;
	}

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return UINT32_MAX;
	}

	return k_ticks_to_ms_floor64(timeout.ticks) / 1000;
}

static void dhcpv4_lease_cb(struct net_if *iface,
			    struct dhcpv4_addr_slot *lease,
			    void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char expiry_str[] = "4294967295"; /* Lease time is uint32_t, so take
					   * theoretical max.
					   */
	char iface_name[IFNAMSIZ] = "";

	if (*count == 0) {
		PR("     Iface         Address\t    State\tExpiry (sec)\n");
	}

	(*count)++;

	(void)net_if_get_name(iface, iface_name, sizeof(iface_name));

	if (lease->state == DHCPV4_SERVER_ADDR_DECLINED) {
		snprintk(expiry_str, sizeof(expiry_str) - 1, "infinite");
	} else {
		snprintk(expiry_str, sizeof(expiry_str) - 1, "%u",
			 timepoint_to_s(lease->expiry));
	}

	PR("%2d. %6s %15s\t%9s\t%12s\n",
	   *count, iface_name, net_sprint_ipv4_addr(&lease->addr),
	   dhcpv4_addr_state_to_str(lease->state), expiry_str);
}
#endif /* CONFIG_NET_DHCPV4_SERVER */

static int cmd_net_dhcpv4_server_status(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DHCPV4_SERVER)
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

	ret = net_dhcpv4_server_foreach_lease(iface, dhcpv4_lease_cb, &user_data);
	if (ret == -ENOENT) {
		PR_WARNING("DHCPv4 server is not running on interface %d\n", idx);
	} else if (count == 0) {
		PR("DHCPv4 server - no addresses assigned\n");
	}
#else /* CONFIG_NET_DHCPV4_SERVER */
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DHCPV4_SERVER", "DHCPv4 server");
#endif /* CONFIG_NET_DHCPV4_SERVER */
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dhcpv4_server,
	SHELL_CMD_ARG(start, NULL, "Start the DHCPv4 server operation on the interface.\n"
		      "'net dhcpv4 server start <index> <base address>'\n"
		      "<index> is the network interface index.\n"
		      "<base address> is the first address for the address pool.",
		      cmd_net_dhcpv4_server_start, 3, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop the DHCPv4 server operation on the interface.\n"
		      "'net dhcpv4 server stop <index>'\n"
		      "<index> is the network interface index.",
		      cmd_net_dhcpv4_server_stop, 2, 0),
	SHELL_CMD_ARG(status, NULL, "Print the DHCPv4 server status on the interface.\n"
		      "'net dhcpv4 server status <index>'\n"
		      "<index> is the network interface index. Optional.",
		      cmd_net_dhcpv4_server_status, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dhcpv4,
	SHELL_CMD(server, &net_cmd_dhcpv4_server,
		  "DHCPv4 server service management.",
		  NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), dhcpv4, &net_cmd_dhcpv4, "Manage DHPCv4 services.",
		 NULL, 1, 0);
