/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#if defined(CONFIG_NET_L2_VIRTUAL)
#include <zephyr/net/virtual.h>
#endif
#include <zephyr/net/ethernet.h>

#include <zephyr/net/socket.h>
#include <stdlib.h>

#include "net_shell_private.h"

#if defined(CONFIG_NET_VLAN)
static void iface_vlan_del_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	uint16_t vlan_tag = POINTER_TO_UINT(data->user_data);
	int ret;

	ret = net_eth_vlan_disable(iface, vlan_tag);
	if (ret < 0) {
		if (ret != -ESRCH) {
			PR_WARNING("Cannot delete VLAN tag %d from "
				   "interface %d (%p)\n",
				   vlan_tag,
				   net_if_get_by_iface(iface),
				   iface);
		}

		return;
	}

	PR("VLAN tag %d removed from interface %d (%p)\n", vlan_tag,
	   net_if_get_by_iface(iface), iface);
}

static void iface_vlan_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char name[IFNAMSIZ];

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	if (!(net_virtual_get_iface_capabilities(iface) & VIRTUAL_INTERFACE_VLAN)) {
		return;
	}

	if (*count == 0) {
		PR("    Interface  Name        \tTag\tAttached\n");
	}

	(void)net_if_get_name(iface, name, sizeof(name));

	PR("[%d] %p  %-12s\t%d\t%d\n", net_if_get_by_iface(iface), iface,
	   name, net_eth_get_vlan_tag(iface),
	   net_if_get_by_iface(net_eth_get_vlan_main(iface)));

	(*count)++;
}
#endif /* CONFIG_NET_VLAN */

static int cmd_net_vlan(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	struct net_shell_user_data user_data;
	int count = 0;

	user_data.sh = sh;
	user_data.user_data = &count;

	net_if_foreach(iface_vlan_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static int cmd_net_vlan_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	int arg = 0;
	int ret;
	uint16_t tag;
	struct net_if *iface;
	char *endptr;
	uint32_t iface_idx;

	/* vlan add <tag> <interface index> */
	if (!argv[++arg]) {
		PR_WARNING("VLAN tag missing.\n");
		goto usage;
	}

	tag = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid tag %s\n", argv[arg]);
		return -ENOEXEC;
	}

	if (!argv[++arg]) {
		PR_WARNING("Network interface index missing.\n");
		goto usage;
	}

	iface_idx = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid index %s\n", argv[arg]);
		goto usage;
	}

	iface = net_if_get_by_index(iface_idx);
	if (!iface) {
		PR_WARNING("Network interface index %d is invalid.\n",
			   iface_idx);
		goto usage;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		PR_WARNING("Network interface %d (%p) is not ethernet interface\n",
			   net_if_get_by_iface(iface), iface);
		return -ENOEXEC;
	}

	ret = net_eth_vlan_enable(iface, tag);
	if (ret < 0) {
		if (ret == -ENOENT) {
			PR_WARNING("No IP address configured.\n");
		}

		PR_WARNING("Cannot set VLAN tag (%d)\n", ret);

		return -ENOEXEC;
	}

	iface = net_eth_get_vlan_iface(iface, tag);

	PR("VLAN tag %d set to interface %d (%p)\n", tag,
	   net_if_get_by_iface(iface), iface);

	return 0;

usage:
	PR("Usage:\n");
	PR("\tvlan add <tag> <interface index>\n");
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

static int cmd_net_vlan_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_VLAN)
	int arg = 0;
	struct net_shell_user_data user_data;
	char *endptr;
	uint16_t tag;

	/* vlan del <tag> */
	if (!argv[++arg]) {
		PR_WARNING("VLAN tag missing.\n");
		goto usage;
	}

	tag = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid tag %s\n", argv[arg]);
		return -ENOEXEC;
	}

	user_data.sh = sh;
	user_data.user_data = UINT_TO_POINTER((uint32_t)tag);

	net_if_foreach(iface_vlan_del_cb, &user_data);

	return 0;

usage:
	PR("Usage:\n");
	PR("\tvlan del <tag>\n");
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_VLAN", "VLAN");
#endif /* CONFIG_NET_VLAN */

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_vlan,
	SHELL_CMD_ARG(add, NULL,
		      "'net vlan add <tag> <index>' adds VLAN tag to the "
		      "network interface.",
		      cmd_net_vlan_add, 3, 0),
	SHELL_CMD_ARG(del, NULL,
		      "'net vlan del <tag>' deletes VLAN tag from the network "
		      "interface.",
		      cmd_net_vlan_del, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), vlan, &net_cmd_vlan,
		 "Show VLAN information.",
		 cmd_net_vlan, 1, 0);
