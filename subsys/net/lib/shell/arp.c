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
#include <zephyr/net/net_if.h>
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
#endif

	ARG_UNUSED(argc);

#if defined(CONFIG_NET_ARP)
	if (argv[1] == NULL) {
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

#if defined(CONFIG_NET_ARP) && defined(CONFIG_NET_NATIVE)
static int cmd_net_arp_add(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_in_addr ip;
	struct net_eth_addr eth;
	struct net_if *iface;
	int ret;
	int idx;
	const char *ip_str;
	const char *mac_str;

	if (argc < 3) {
		PR_WARNING("Usage: net arp add [<iface_index>] <IPv4> <MAC>\n");
		return -ENOEXEC;
	}

	if (argc >= 4) {
		idx = get_iface_idx(sh, argv[1]);
		if (idx <= 0) {
			PR_WARNING("Invalid interface index: %d\n", idx);
			return -ENOEXEC;
		}
		iface = net_if_get_by_index(idx);
		if (iface == NULL) {
			PR_WARNING("No such interface index: %d\n", idx);
			return -ENOEXEC;
		}
		ip_str = argv[2];
		mac_str = argv[3];
	} else {
		iface = net_if_get_default();
		if (iface == NULL) {
			PR_WARNING("No default interface; specify iface index.\n");
			return -ENOEXEC;
		}
		ip_str = argv[1];
		mac_str = argv[2];
	}

	ret = net_addr_pton(NET_AF_INET, ip_str, &ip);
	if (ret < 0) {
		PR_WARNING("Invalid IPv4 address: %s\n", ip_str);
		return -ENOEXEC;
	}

	if (net_bytes_from_str(eth.addr, sizeof(eth.addr), mac_str) < 0) {
		PR_WARNING("Invalid MAC address: %s\n", mac_str);
		return -ENOEXEC;
	}

	net_arp_update(iface, &ip, &eth, false, true);
	PR("Added static ARP entry %s -> %s on interface %d\n", net_sprint_ipv4_addr(&ip),
	   net_sprint_ll_addr(eth.addr, sizeof(eth.addr)), net_if_get_by_iface(iface));
	return 0;
}
#endif /* CONFIG_NET_ARP && CONFIG_NET_NATIVE */

SHELL_STATIC_SUBCMD_SET_CREATE(
	net_cmd_arp,
	SHELL_CMD(flush, NULL, SHELL_HELP("Remove all entries from ARP cache", ""),
		  cmd_net_arp_flush),
#if defined(CONFIG_NET_ARP) && defined(CONFIG_NET_NATIVE)
	SHELL_CMD(add, NULL, SHELL_HELP("Add a static ARP entry", "[<iface_index>] <IPv4> <MAC>"),
		  cmd_net_arp_add),
#endif
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((net), arp, &net_cmd_arp,
		 "Print information about IPv4 ARP cache.",
		 cmd_net_arp, 1, 0);
