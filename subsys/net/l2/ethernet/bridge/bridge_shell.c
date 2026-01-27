/*
 * SPDX-FileCopyrightText: Copyright (c) 2021 BayLibre SAS
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet_bridge.h>
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
#include <zephyr/net/ethernet_bridge_fdb.h>
#endif
#include <zephyr/sys/slist.h>

static int get_idx(const struct shell *sh, char *index_str)
{
	char *endptr;
	int idx;

	idx = strtol(index_str, &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "Invalid index %s\n", index_str);
		return -ENOENT;
	}

	return idx;
}

static int cmd_bridge_addif(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DSA) && !defined(CONFIG_NET_DSA_DEPRECATED)
	struct ethernet_context *eth_ctx;
#endif
	int ret = 0, br_idx, if_idx;
	struct net_if *iface;
	struct net_if *br;

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}

	for (int i = 2; i < argc; i++) {
		if_idx = get_idx(sh, argv[i]);
		if (if_idx < 0) {
			continue;
		}

		iface = net_if_get_by_index(if_idx);
		if (iface == NULL) {
			shell_warn(sh, "Interface %d not found\n", if_idx);
			continue;
		}

		if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
			shell_warn(sh, "Interface %d is not Ethernet\n", if_idx);
			continue;
		}

#if defined(CONFIG_NET_DSA) && !defined(CONFIG_NET_DSA_DEPRECATED)
		eth_ctx = net_if_l2_data(iface);
		if (eth_ctx->dsa_port != DSA_USER_PORT &&
		    !(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
			shell_warn(sh, "Interface %d cannot do promiscuous mode\n", if_idx);
			continue;
		}
#else
		if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
			shell_warn(sh, "Interface %d cannot do promiscuous mode\n", if_idx);
			continue;
		}
#endif

		ret = eth_bridge_iface_add(br, iface);
		if (ret < 0) {
			shell_error(sh, "error: bridge iface add (%d)\n", ret);
		}
	}

	return ret;
}

static int cmd_bridge_delif(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0, br_idx, if_idx;
	struct net_if *iface;
	struct net_if *br;

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}

	for (int i = 2; i < argc; i++) {
		if_idx = get_idx(sh, argv[i]);
		if (if_idx < 0) {
			continue;
		}

		iface = net_if_get_by_index(if_idx);
		if (iface == NULL) {
			shell_warn(sh, "Interface %d not found\n", if_idx);
			continue;
		}

		ret = eth_bridge_iface_remove(br, iface);
		if (ret < 0) {
			shell_error(sh, "error: bridge iface remove (%d)\n", ret);
		}
	}

	return ret;
}

static void bridge_show(struct eth_bridge_iface_context *ctx, void *data)
{
	int br_idx = eth_bridge_get_index(ctx->iface);
	const struct shell *sh = data;

	shell_fprintf(sh, SHELL_NORMAL, "%-7d", br_idx);

	if (net_if_is_up(ctx->iface)) {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "up");
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "down");
	}

	if (ctx->is_setup) {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "ok");
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "no");
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ARRAY_FOR_EACH(ctx->eth_iface, i) {
		int if_idx;

		if (ctx->eth_iface[i] == NULL) {
			continue;
		}

		if_idx = net_if_get_by_iface(ctx->eth_iface[i]);

		shell_fprintf(sh, SHELL_NORMAL, "%-2d", if_idx);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");

	k_mutex_unlock(&ctx->lock);
}

static int cmd_bridge_show(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *br = NULL;
	int br_idx;

	if (argc == 2) {
		br_idx = get_idx(sh, argv[1]);
		if (br_idx < 0) {
			return br_idx;
		}

		br = eth_bridge_get_by_index(br_idx);
		if (br == NULL) {
			shell_warn(sh, "Bridge %d not found\n", br_idx);
			return -ENOENT;
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "Bridge %-9s%-9sInterfaces\n",
		      "Status", "Config");

	if (br != NULL) {
		bridge_show(net_if_get_device(br)->data, (void *)sh);
	} else {
		net_eth_bridge_foreach(bridge_show, (void *)sh);
	}

	return 0;
}

static int cmd_bridge_fdb_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	struct net_eth_addr mac;
	struct net_if *iface;
	int iface_idx;
	int ret;

	/* Parse MAC address */
	ret = net_bytes_from_str(mac.addr, sizeof(mac.addr), argv[1]);
	if (ret != 0) {
		shell_error(sh, "Wrong mac address format");
		return -EINVAL;
	}

	/* Parse interface */
	iface_idx = shell_strtol(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	iface = net_if_get_by_index(iface_idx);
	if (iface == NULL) {
		shell_error(sh, "Invalid interface: %d", iface_idx);
		return -EINVAL;
	}

	ret = eth_bridge_fdb_add(&mac, iface);
	if (ret != 0) {
		return ret;
	}

	shell_print(sh, "Add FDB entry successfully");
	return 0;
#else
	shell_warn(sh, "Set CONFIG_NET_ETHERNET_BRIDGE_FDB to enable Ethernet bridge FDB support.");
	return -ENOTSUP;
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB */
}

static int cmd_bridge_fdb_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	struct net_eth_addr mac;
	struct net_if *iface;
	int iface_idx;
	int ret;

	/* Parse MAC address */
	ret = net_bytes_from_str(mac.addr, sizeof(mac.addr), argv[1]);
	if (ret != 0) {
		shell_error(sh, "Wrong mac address format");
		return -EINVAL;
	}

	/* Parse interface */
	iface_idx = shell_strtol(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	iface = net_if_get_by_index(iface_idx);
	if (iface == NULL) {
		shell_error(sh, "Invalid interface: %d", iface_idx);
		return -EINVAL;
	}

	ret = eth_bridge_fdb_del(&mac, iface);
	if (ret != 0) {
		return ret;
	}

	shell_print(sh, "Delete FDB entry successfully");
	return 0;
#else
	shell_warn(sh, "Set CONFIG_NET_ETHERNET_BRIDGE_FDB to enable Ethernet bridge FDB support.");
	return -ENOTSUP;
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB */
}

#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
static void cmd_bridge_fdb_handler(struct eth_bridge_fdb_entry *entry, void *user_data)
{
	const struct shell *sh = (const struct shell *)user_data;

	shell_print(sh, "%02x:%02x:%02x:%02x:%02x:%02x  %-8d  %s",
		 entry->mac.addr[0], entry->mac.addr[1],
		 entry->mac.addr[2], entry->mac.addr[3],
		 entry->mac.addr[4], entry->mac.addr[5],
		 net_if_get_by_iface(entry->iface),
		 (entry->flags & ETHERNET_BRIDGE_FDB_FLAG_STATIC) ? "static" : "dynamic");
}
#endif

static int cmd_bridge_fdb_show(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	shell_print(sh, "FDB Table:");
	shell_print(sh, "MAC Address        Interface  Flags");
	shell_print(sh, "-------------------------------------------");

	eth_bridge_fdb_foreach(&cmd_bridge_fdb_handler, (void *)sh);
	return 0;
#else
	shell_warn(sh, "Set CONFIG_NET_ETHERNET_BRIDGE_FDB to enable Ethernet bridge FDB support.");
	return -ENOTSUP;
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB */
}

SHELL_STATIC_SUBCMD_SET_CREATE(bridge_fdb_commands,
	SHELL_CMD_ARG(add, NULL,
		SHELL_HELP("Add fdb table entry", "<mac address> <interface index>"),
		cmd_bridge_fdb_add, 3, 0),
	SHELL_CMD_ARG(del, NULL,
		SHELL_HELP("Delete fdb table entry", "<mac address> <interface index>"),
		cmd_bridge_fdb_del, 3, 0),
	SHELL_CMD_ARG(show, NULL,
		SHELL_HELP("Show fdb table", ""),
		cmd_bridge_fdb_show, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(bridge_commands,
	SHELL_CMD_ARG(addif, NULL,
		SHELL_HELP("Add a network interface to a bridge.",
			"<bridge_index> <one or more interface index>"),
		cmd_bridge_addif, 3, 5),
	SHELL_CMD_ARG(delif, NULL,
		SHELL_HELP("Delete a network interface from a bridge.",
			"<bridge_index> <one or more interface index>"),
		cmd_bridge_delif, 3, 5),
	SHELL_CMD_ARG(show, NULL,
		SHELL_HELP("Show bridge information.", "<bridge_index>"),
		cmd_bridge_show, 1, 1),
	SHELL_CMD_ARG(fdb, &bridge_fdb_commands,
		SHELL_HELP("Manage FDB table. Check usage with -h.", NULL),
		NULL, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), bridge, &bridge_commands,
		 "Ethernet bridge commands.",
		 cmd_bridge_show, 1, 1);
