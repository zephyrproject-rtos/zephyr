/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/mld.h>

#include "net_shell_private.h"
#include "../ip/ipv6.h"

#if defined(CONFIG_NET_IPV6_FRAGMENT)
void ipv6_frag_cb(struct net_ipv6_reassembly *reass, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char src[ADDR_LEN];
	int i;

	if (!*count) {
		PR("\nIPv6 reassembly Id         Remain "
		   "Src             \tDst\n");
	}

	snprintk(src, ADDR_LEN, "%s", net_sprint_ipv6_addr(&reass->src));

	PR("%p      0x%08x  %5d %16s\t%16s\n", reass, reass->id,
	   k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(&reass->timer)),
	   src, net_sprint_ipv6_addr(&reass->dst));

	for (i = 0; i < CONFIG_NET_IPV6_FRAGMENT_MAX_PKT; i++) {
		if (reass->pkt[i]) {
			struct net_buf *frag = reass->pkt[i]->frags;

			PR("[%d] pkt %p->", i, reass->pkt[i]);

			while (frag) {
				PR("%p", frag);

				frag = frag->frags;
				if (frag) {
					PR("->");
				}
			}

			PR("\n");
		}
	}

	(*count)++;
}
#endif /* CONFIG_NET_IPV6_FRAGMENT */

#if defined(CONFIG_NET_IPV6_PE)
static void ipv6_pe_filter_cb(struct in6_addr *prefix, bool is_denylist,
			      void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char ipaddr[INET6_ADDRSTRLEN + 1];

	net_addr_ntop(AF_INET6, prefix, ipaddr, sizeof(ipaddr) - 1);

	if (*count == 0) {
		PR("IPv6 privacy extension %s list filters :\n",
		   is_denylist ? "deny" : "allow");
	}

	PR("[%d] %s/64\n", *count, ipaddr);

	(*count)++;
}
#endif /* CONFIG_NET_IPV6_PE */

#if defined(CONFIG_NET_IPV6)
static void address_lifetime_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	const char *extra;

	ARG_UNUSED(user_data);

	PR("\nIPv6 addresses for interface %d (%p) (%s)\n",
	   net_if_get_by_iface(iface), iface, iface2str(iface, &extra));
	PR("============================================%s\n", extra);

	if (!ipv6) {
		PR("No IPv6 config found for this interface.\n");
		return;
	}

	PR("Type      \tState    \tLifetime (sec)\tRef\tAddress\n");

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		char remaining_str[sizeof("01234567890")];
		uint8_t prefix_len = 128U;

		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

#if defined(CONFIG_NET_NATIVE_IPV6)
		struct net_if_ipv6_prefix *prefix;
		uint32_t remaining;

		remaining = net_timeout_remaining(&ipv6->unicast[i].lifetime,
						  k_uptime_get_32());

		prefix = net_if_ipv6_prefix_get(iface,
					   &ipv6->unicast[i].address.in6_addr);
		if (prefix) {
			prefix_len = prefix->len;
		}

		if (ipv6->unicast[i].is_infinite) {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "infinite");
		} else {
			snprintk(remaining_str, sizeof(remaining_str) - 1,
				 "%u", remaining);
		}
#else
	snprintk(remaining_str, sizeof(remaining_str) - 1, "infinite");
#endif /* CONFIG_NET_NATIVE_IPV6 */

		PR("%s  \t%s\t%14s\t%ld\t%s/%d%s\n",
		   addrtype2str(ipv6->unicast[i].addr_type),
		   addrstate2str(ipv6->unicast[i].addr_state),
		   remaining_str, atomic_get(&ipv6->unicast[i].atomic_ref),
		   net_sprint_ipv6_addr(&ipv6->unicast[i].address.in6_addr),
		   prefix_len,
		   ipv6->unicast[i].is_temporary ? " (temporary)" : "");
	}
}
#endif /* CONFIG_NET_IPV6 */

static int cmd_net_ipv6(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	struct net_shell_user_data user_data;
#endif /* CONFIG_NET_IPV6 */

	PR("IPv6 support                              : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6) ?
	   "enabled" : "disabled");
	if (!IS_ENABLED(CONFIG_NET_IPV6)) {
		return -ENOEXEC;
	}

#if defined(CONFIG_NET_NATIVE_IPV6)
	PR("IPv6 fragmentation support                : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT) ? "enabled" :
	   "disabled");
	PR("Multicast Listener Discovery support      : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_MLD) ? "enabled" :
	   "disabled");
	PR("Neighbor cache support                    : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_NBR_CACHE) ? "enabled" :
	   "disabled");
	PR("Neighbor discovery support                : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_ND) ? "enabled" :
	   "disabled");
	PR("Duplicate address detection (DAD) support : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_DAD) ? "enabled" :
	   "disabled");
	PR("Router advertisement RDNSS option support : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_RA_RDNSS) ? "enabled" :
	   "disabled");
	PR("6lo header compression support            : %s\n",
	   IS_ENABLED(CONFIG_NET_6LO) ? "enabled" :
	   "disabled");

	if (IS_ENABLED(CONFIG_NET_6LO_CONTEXT)) {
		PR("6lo context based compression "
		   "support     : %s\n",
		   IS_ENABLED(CONFIG_NET_6LO_CONTEXT) ? "enabled" :
		   "disabled");
	}

	PR("Privacy extension support                 : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_PE) ? "enabled" : "disabled");
	PR("SLAAC IID generation method               : %s\n",
	   IS_ENABLED(CONFIG_NET_IPV6_IID_STABLE) ?
	   "stable (RFC 7217)" : "EUI-64 (RFC 4862)");

#if defined(CONFIG_NET_IPV6_PE)
	PR("Max number of IPv6 privacy extension filters "
	   "                : %d\n",
	   CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT);
#endif /* CONFIG_NET_IPV6_PE */
#endif /* CONFIG_NET_NATIVE_IPV6 */

#if defined(CONFIG_NET_IPV6)
	PR("Max number of IPv6 network interfaces "
	   "in the system          : %d\n",
	   CONFIG_NET_IF_MAX_IPV6_COUNT);
	PR("Max number of unicast IPv6 addresses "
	   "per network interface   : %d\n",
	   CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT);
	PR("Max number of multicast IPv6 addresses "
	   "per network interface : %d\n",
	   CONFIG_NET_IF_MCAST_IPV6_ADDR_COUNT);
	PR("Max number of IPv6 prefixes per network "
	   "interface            : %d\n",
	   CONFIG_NET_IF_IPV6_PREFIX_COUNT);

	user_data.sh = sh;
	user_data.user_data = NULL;

	/* Print information about address lifetime */
	net_if_foreach(address_lifetime_cb, &user_data);
#endif /* CONFIG_NET_IPV6 */

	return 0;
}

static int cmd_net_ip6_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	struct net_if *iface = NULL;
	int idx;
	struct in6_addr addr;

	if (argc != 3) {
		PR_ERROR("Correct usage: net ipv6 add <index> <address>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOENT;
	}

	if (net_addr_pton(AF_INET6, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (net_ipv6_is_addr_mcast(&addr)) {
		int ret;

		ret = net_ipv6_mld_join(iface, &addr);
		if (ret < 0) {
			PR_ERROR("Cannot %s multicast group %s for interface %d (%d)\n",
				 "join", net_sprint_ipv6_addr(&addr), idx, ret);
			if (ret == -ENOTSUP) {
				PR_INFO("Enable CONFIG_NET_IPV6_MLD for %s multicast "
					"group\n", "joining");
			}
			return ret;
		}
	} else {
		if (!net_if_ipv6_addr_add(iface, &addr, NET_ADDR_MANUAL, 0)) {
			PR_ERROR("Failed to add %s address to interface %p\n", argv[2], iface);
		}
	}

#else /* CONFIG_NET_IPV6 */
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_IPV6 */
	return 0;
}

static int cmd_net_ip6_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_IPV6)
	struct net_if *iface = NULL;
	int idx;
	struct in6_addr addr;

	if (argc != 3) {
		PR_ERROR("Correct usage: net ipv6 del <index> <address>\n");
		return -EINVAL;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		return -ENOEXEC;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		return -ENOENT;
	}

	if (net_addr_pton(AF_INET6, argv[2], &addr)) {
		PR_ERROR("Invalid address: %s\n", argv[2]);
		return -EINVAL;
	}

	if (net_ipv6_is_addr_mcast(&addr)) {
		int ret;

		ret = net_ipv6_mld_leave(iface, &addr);
		if (ret < 0) {
			PR_ERROR("Cannot %s multicast group %s for interface %d (%d)\n",
				 "leave", net_sprint_ipv6_addr(&addr), idx, ret);
			if (ret == -ENOTSUP) {
				PR_INFO("Enable CONFIG_NET_IPV6_MLD for %s multicast "
					"group\n", "leaving");
			}
			return ret;
		}
	} else {
		if (!net_if_ipv6_addr_rm(iface, &addr)) {
			PR_ERROR("Failed to delete %s\n", argv[2]);
			return -1;
		}
	}

#else /* CONFIG_NET_IPV6 */
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_IPV6 */
	return 0;
}

static int cmd_net_ip6_pe(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_NATIVE_IPV6)
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	bool do_allowlisting = true;
	struct in6_addr prefix;
	bool do_add;
	int arg = 1;
	int ret;

	if (argc == 0) {
		PR_ERROR("Correct usage: net ipv6 pe [add | del] [allow | deny] [<prefix>]\n");
		return -EINVAL;
	}

	if (argc == 1) {
		struct net_shell_user_data user_data;
		int count = 0;

		user_data.sh = sh;
		user_data.user_data = &count;

		count = net_ipv6_pe_filter_foreach(ipv6_pe_filter_cb, &user_data);
		if (count == 0) {
			PR("No privacy extension filters found.");
		}

		return 0;
	}

	if (strcmp(argv[arg], "add") == 0) {
		arg++;
		do_add = true;
	} else if (strcmp(argv[arg], "del") == 0) {
		arg++;
		do_add = false;
	} else {
		PR("Unknown sub-option \"%s\"\n", argv[arg]);
		return 0;
	}

	if (!argv[arg]) {
		PR("No sub-options given. See \"help net ipv6\" "
		   "command for details.\n");
		return 0;
	}

	if (strcmp(argv[arg], "allow") == 0) {
		arg++;
	} else if (strcmp(argv[arg], "deny") == 0) {
		arg++;
		do_allowlisting = false;
	}

	if (!argv[arg]) {
		PR("No sub-options given. See \"help net ipv6\" "
		   "command for details.\n");
		return 0;
	}

	ret = net_addr_pton(AF_INET6, argv[arg], &prefix);
	if (ret < 0) {
		PR("Invalid prefix \"%s\"\n", argv[arg]);
		if (strstr(argv[arg], "/")) {
			PR("Do not add the prefix length.\n");
		}

		return 0;
	}

	if (do_add) {
		ret = net_ipv6_pe_add_filter(&prefix, !do_allowlisting);
	} else {
		ret = net_ipv6_pe_del_filter(&prefix);
	}

	if (ret < 0) {
		if (ret == -EALREADY) {
			PR("Filter %s already in %s list\n",
			   net_sprint_ipv6_addr(&prefix),
			   do_allowlisting ? "allow" : "deny");
		} else if (ret == -ENOENT) {
			PR("No such filter %s found\n",
			   net_sprint_ipv6_addr(&prefix));
		} else {
			PR("Cannot %s %s %sfilter (%d)\n",
			   do_add ? "add" : "delete",
			   argv[arg],
			   do_add ?
			   (do_allowlisting ? "allowlist " :
			    "denylist ") : "",
			   ret);
		}

		return 0;
	}

	PR("%s %s%sfilter for %s\n",
	   do_add ? "Added" : "Deleted",
	   do_add ? (do_allowlisting ? "allow" : "deny") : "",
	   do_add ? " list " : "",
	   argv[arg]);
#else
	PR("IPv6 privacy extension filter support is disabled.\n");
	PR("Set CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0 to enable it.\n");
#endif
#else /* CONFIG_NET_NATIVE_IPV6 */
	PR_INFO("Set %s and %s to enable native %s support.\n",
			"CONFIG_NET_NATIVE", "CONFIG_NET_IPV6", "IPv6");
#endif /* CONFIG_NET_NATIVE_IPV6 */
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ip6,
	SHELL_CMD(add, NULL,
		  "'net ipv6 add <index> <address>' adds the address to the interface.",
		  cmd_net_ip6_add),
	SHELL_CMD(del, NULL,
		  "'net ipv6 del <index> <address>' deletes the address from the interface.",
		  cmd_net_ip6_del),
	SHELL_CMD(pe, NULL,
		  "net ipv6 pe add [allow|deny] <IPv6 prefix>\n"
		  "Add IPv6 address to filter list. The allow/deny "
		  "parameter tells if this is allow listed (accepted) or "
		  "deny listed (declined) prefix. Default is to allow list "
		  "the prefix.\n"
		  "ipv6 pe del <IPv6 prefix>\n"
		  "Delete IPv6 address from filter list.",
		  cmd_net_ip6_pe),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), ipv6, &net_cmd_ip6,
		 "Print information about IPv6 specific information and "
		 "configuration.",
		 cmd_net_ipv6, 1, 0);
