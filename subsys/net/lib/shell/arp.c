/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#if defined(CONFIG_NET_ARP)
#include "ethernet/arp.h"
#endif

#if defined(CONFIG_NET_ARP) && defined(CONFIG_NET_NATIVE)
static void arp_cb(struct arp_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (*count == 0) {
		PR("     Interface  Link              Address\n");
	}

	PR("[%2d] %d          %s %s\n", *count,
	   net_if_get_by_iface(entry->iface),
	   net_sprint_ll_addr(entry->eth.addr, sizeof(struct net_eth_addr)),
	   net_sprint_ipv4_addr(&entry->ip));

	(*count)++;
}
#endif /* CONFIG_NET_ARP */

#if !defined(CONFIG_NET_ARP)
static void print_arp_error(const struct shell *sh)
{
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_NATIVE, CONFIG_NET_ARP, CONFIG_NET_IPV4 and"
		" CONFIG_NET_L2_ETHERNET", "ARP");
}
#endif

static int cmd_net_arp(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ARP)
	struct net_shell_user_data user_data;
	int arg = 1;
#endif

	ARG_UNUSED(argc);

#if defined(CONFIG_NET_ARP)
	if (!argv[arg]) {
		/* ARP cache content */
		int count = 0;

		user_data.sh = sh;
		user_data.user_data = &count;

		if (net_arp_foreach(arp_cb, &user_data) == 0) {
			PR("ARP cache is empty.\n");
		}
	}
#else
	print_arp_error(sh);
#endif

	return 0;
}

static int cmd_net_arp_flush(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_ARP)
	PR("Flushing ARP cache.\n");
	net_arp_clear_cache(NULL);
#else
	print_arp_error(sh);
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_arp,
	SHELL_CMD(flush, NULL, "Remove all entries from ARP cache.",
		  cmd_net_arp_flush),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), arp, &net_cmd_arp,
		 "Print information about IPv4 ARP cache.",
		 cmd_net_arp, 1, 0);
