/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

static int cmd_net_nbr_rm(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	struct in6_addr addr;
	int ret;

	if (!argv[1]) {
		PR_WARNING("Neighbor IPv6 address missing.\n");
		return -ENOEXEC;
	}

	ret = net_addr_pton(AF_INET6, argv[1], &addr);
	if (ret < 0) {
		PR_WARNING("Cannot parse '%s'\n", argv[1]);
		return -ENOEXEC;
	}

	if (!net_ipv6_nbr_rm(NULL, &addr)) {
		PR_WARNING("Cannot remove neighbor %s\n",
			   net_sprint_ipv6_addr(&addr));
		return -ENOEXEC;
	}

	PR("Neighbor %s removed.\n", net_sprint_ipv6_addr(&addr));
#else
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Native IPv6 not enabled.\n");
#endif

	return 0;
}

#if defined(CONFIG_NET_NATIVE_IPV6)
static void nbr_cb(struct net_nbr *nbr, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char *padding = "";
	char *state_pad = "";
	const char *state_str;
#if defined(CONFIG_NET_IPV6_ND)
	int64_t remaining;
#endif

#if defined(CONFIG_NET_L2_IEEE802154)
	padding = "      ";
#endif

	if (*count == 0) {
		PR("     Neighbor  Interface  Flags    State     "
		   "Remain  Link              %sAddress\n", padding);
	}

	(*count)++;

	state_str = net_ipv6_nbr_state2str(net_ipv6_nbr_data(nbr)->state);

	/* This is not a proper way but the minimal libc does not honor
	 * string lengths in %s modifier so in order the output to look
	 * nice, do it like this.
	 */
	if (strlen(state_str) == 5) {
		state_pad = "    ";
	}

#if defined(CONFIG_NET_IPV6_ND)
	remaining = net_ipv6_nbr_data(nbr)->reachable +
		    net_ipv6_nbr_data(nbr)->reachable_timeout -
		    k_uptime_get();
#endif

	PR("[%2d] %p  %d      %5d/%d/%d/%d  %s%s %6d  %17s%s %s\n",
	   *count, nbr, net_if_get_by_iface(nbr->iface),
	   net_ipv6_nbr_data(nbr)->link_metric,
	   nbr->ref,
	   net_ipv6_nbr_data(nbr)->ns_count,
	   net_ipv6_nbr_data(nbr)->is_router,
	   state_str,
	   state_pad,
#if defined(CONFIG_NET_IPV6_ND)
	   (int)(remaining > 0 ? remaining : 0),
#else
	   0,
#endif
	   nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
	   net_sprint_ll_addr(
		   net_nbr_get_lladdr(nbr->idx)->addr,
		   net_nbr_get_lladdr(nbr->idx)->len),
	   nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "" :
		(net_nbr_get_lladdr(nbr->idx)->len == 8U ? "" : padding),
	   net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));
}
#endif

static int cmd_net_nbr(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	int count = 0;
	struct net_shell_user_data user_data;

	user_data.sh = sh;
	user_data.user_data = &count;

	net_ipv6_nbr_foreach(nbr_cb, &user_data);

	if (count == 0) {
		PR("No neighbors.\n");
	}
#else
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Native IPv6 not enabled.\n");
#endif /* CONFIG_NET_NATIVE_IPV6 */

	return 0;
}

#if defined(CONFIG_NET_NATIVE_IPV6) && defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)
static char nbr_address_buffer[CONFIG_NET_IPV6_MAX_NEIGHBORS][NET_IPV6_ADDR_LEN];

static void nbr_address_cb(struct net_nbr *nbr, void *user_data)
{
	int *count = user_data;

	if (*count >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
		return;
	}

	snprintk(nbr_address_buffer[*count], NET_IPV6_ADDR_LEN,
		 "%s", net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));

	(*count)++;
}

static void nbr_populate_addresses(void)
{
	int count = 0;

	net_ipv6_nbr_foreach(nbr_address_cb, &count);
}

static char *set_nbr_address(size_t idx)
{
	if (idx == 0) {
		memset(nbr_address_buffer, 0, sizeof(nbr_address_buffer));
		nbr_populate_addresses();
	}

	if (idx >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
		return NULL;
	}

	if (!nbr_address_buffer[idx][0]) {
		return NULL;
	}

	return nbr_address_buffer[idx];
}

static void nbr_address_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(nbr_address, nbr_address_get);

#define NBR_ADDRESS_CMD &nbr_address

static void nbr_address_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &nbr_address;
	entry->syntax = set_nbr_address(idx);
}

#else
#define NBR_ADDRESS_CMD NULL
#endif /* CONFIG_NET_NATIVE_IPV6 && CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_nbr,
	SHELL_CMD(rm, NBR_ADDRESS_CMD,
		  "'net nbr rm <address>' removes neighbor from cache.",
		  cmd_net_nbr_rm),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), nbr, &net_cmd_nbr,
		 "Print neighbor information.",
		 cmd_net_nbr, 1, 0);
