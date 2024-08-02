/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_bridge.h>
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
	int br_idx, if_idx;
	struct eth_bridge *br;
	struct net_if *iface;

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}
	if_idx = get_idx(sh, argv[2]);
	if (if_idx < 0) {
		return if_idx;
	}
	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}
	iface = net_if_get_by_index(if_idx);
	if (iface == NULL) {
		shell_warn(sh, "Interface %d not found\n", if_idx);
		return -ENOENT;
	}
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		shell_warn(sh, "Interface %d is not Ethernet\n", if_idx);
		return -EINVAL;
	}
	if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
		shell_warn(sh, "Interface %d cannot do promiscuous mode\n", if_idx);
		return -EINVAL;
	}

	int ret = eth_bridge_iface_add(br, iface);

	if (ret < 0) {
		shell_error(sh, "error: eth_bridge_iface_add() returned %d\n", ret);
	}
	return ret;
}

static int cmd_bridge_delif(const struct shell *sh, size_t argc, char *argv[])
{
	int br_idx, if_idx;
	struct eth_bridge *br;
	struct net_if *iface;

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}
	if_idx = get_idx(sh, argv[2]);
	if (if_idx < 0) {
		return if_idx;
	}
	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}
	iface = net_if_get_by_index(if_idx);
	if (iface == NULL) {
		shell_warn(sh, "Interface %d not found\n", if_idx);
		return -ENOENT;
	}

	int ret = eth_bridge_iface_remove(br, iface);

	if (ret < 0) {
		shell_error(sh, "error: eth_bridge_iface_remove() returned %d\n", ret);
	}
	return ret;
}

static int cmd_bridge_allow_tx(const struct shell *sh, size_t argc, char *argv[])
{
	int br_idx, if_idx;
	struct eth_bridge *br;
	struct net_if *iface;
	struct ethernet_context *ctx;

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}
	if_idx = get_idx(sh, argv[2]);
	if (if_idx < 0) {
		return if_idx;
	}
	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_error(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}
	iface = net_if_get_by_index(if_idx);
	if (iface == NULL) {
		shell_error(sh, "Interface %d not found", if_idx);
		return -ENOENT;
	}
	ctx = net_if_l2_data(iface);
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET) ||
	    ctx->bridge.instance != br) {
		shell_error(sh, "Interface %d is not tied to bridge %d",
			    if_idx, br_idx);
		return -ENOENT;
	}

	if (!strcmp(argv[2], "1") ||
	    !strcmp(argv[2], "yes") ||
	    !strcmp(argv[2], "on") ||
	    !strcmp(argv[2], "true")) {
		eth_bridge_iface_allow_tx(iface, true);
	} else {
		eth_bridge_iface_allow_tx(iface, false);
	}
	return 0;
}

static void bridge_show(struct eth_bridge *br, void *data)
{
	const struct shell *sh = data;
	int br_idx = eth_bridge_get_index(br);
	sys_snode_t *node;
	bool pad;

	shell_fprintf(sh, SHELL_NORMAL, "%-10d", br_idx);
	pad = false;

	k_mutex_lock(&br->lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE(&br->interfaces, node) {
		struct ethernet_context *ctx;
		int if_idx;

		ctx = CONTAINER_OF(node, struct ethernet_context, bridge.node);
		if_idx = net_if_get_by_iface(ctx->iface);

		if (pad) {
			shell_fprintf(sh, SHELL_NORMAL, "%-10s", "");
		}
		shell_fprintf(sh, SHELL_NORMAL, "%-10d%s", if_idx,
			      ctx->bridge.allow_tx ? "*" : "");
		pad = true;
	}
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	k_mutex_unlock(&br->lock);
}

static int cmd_bridge_show(const struct shell *sh, size_t argc, char *argv[])
{
	int br_idx;
	struct eth_bridge *br = NULL;

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

	shell_fprintf(sh, SHELL_NORMAL, "bridge    iface     tx_enabled\n");

	if (br != NULL) {
		bridge_show(br, (void *)sh);
	} else {
		net_eth_bridge_foreach(bridge_show, (void *)sh);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bridge_commands,
	SHELL_CMD_ARG(addif, NULL,
		  "Add a network interface to a bridge.\n"
		  "'bridge addif <bridge_index> <interface_index>'",
		  cmd_bridge_addif, 3, 0),
	SHELL_CMD_ARG(delif, NULL,
		  "Delete a network interface from a bridge.\n"
		  "'bridge delif <bridge_index> <interface_index>'",
		  cmd_bridge_delif, 3, 0),
	SHELL_CMD_ARG(tx, NULL,
		  "Enable/disable tx from given bridged interface.\n"
		  "'bridge tx <bridge_index> <interface_index> {on|off}'",
		  cmd_bridge_allow_tx, 4, 0),
	SHELL_CMD_ARG(show, NULL,
		  "Show bridge information.\n"
		  "'bridge show [<bridge_index>]'",
		  cmd_bridge_show, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bridge, &bridge_commands, "Ethernet Bridge commands", NULL);

#if defined(CONFIG_NET_ETHERNET_BRIDGE_DEFAULT)
static ETH_BRIDGE_INIT(shell_default_bridge);
#endif
