/** @file
 * @brief Network shell module
 *
 * Provide some networking shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_shell, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <stdlib.h>

#include <zephyr/net/ethernet.h>

#include "net_shell_private.h"
#include "net_shell.h"

int get_iface_idx(const struct shell *sh, char *index_str)
{
	char *endptr;
	int idx;

	if (!index_str) {
		PR_WARNING("Interface index is missing.\n");
		return -EINVAL;
	}

	idx = strtol(index_str, &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid index %s\n", index_str);
		return -ENOENT;
	}

	if (idx < 0 || idx > 255) {
		PR_WARNING("Invalid index %d\n", idx);
		return -ERANGE;
	}

	return idx;
}

const char *addrtype2str(enum net_addr_type addr_type)
{
	switch (addr_type) {
	case NET_ADDR_ANY:
		return "<unknown type>";
	case NET_ADDR_AUTOCONF:
		return "autoconf";
	case NET_ADDR_DHCP:
		return "DHCP";
	case NET_ADDR_MANUAL:
		return "manual";
	case NET_ADDR_OVERRIDABLE:
		return "overridable";
	}

	return "<invalid type>";
}

const char *addrstate2str(enum net_addr_state addr_state)
{
	switch (addr_state) {
	case NET_ADDR_ANY_STATE:
		return "<unknown state>";
	case NET_ADDR_TENTATIVE:
		return "tentative";
	case NET_ADDR_PREFERRED:
		return "preferred";
	case NET_ADDR_DEPRECATED:
		return "deprecated";
	}

	return "<invalid state>";
}

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
void get_addresses(struct net_context *context,
		   char addr_local[], int local_len,
		   char addr_remote[], int remote_len)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && context->local.family == AF_INET6) {
		snprintk(addr_local, local_len, "[%s]:%u",
			 net_sprint_ipv6_addr(
				 net_sin6_ptr(&context->local)->sin6_addr),
			 ntohs(net_sin6_ptr(&context->local)->sin6_port));
		snprintk(addr_remote, remote_len, "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(&context->remote)->sin6_addr),
			 ntohs(net_sin6(&context->remote)->sin6_port));

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && context->local.family == AF_INET) {
		snprintk(addr_local, local_len, "%s:%d",
			 net_sprint_ipv4_addr(
				 net_sin_ptr(&context->local)->sin_addr),
			 ntohs(net_sin_ptr(&context->local)->sin_port));

		/* Check if we need to print the v4-mapping-to-v6 address */
		if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6) &&
		    net_sin(&context->remote)->sin_family == AF_INET6 &&
		    net_ipv6_addr_is_v4_mapped(&net_sin6(&context->remote)->sin6_addr)) {
			snprintk(addr_remote, remote_len, "[%s]:%d",
				 net_sprint_ipv6_addr(
					 &net_sin6(&context->remote)->sin6_addr),
				 ntohs(net_sin6(&context->remote)->sin6_port));
		} else {
			snprintk(addr_remote, remote_len, "%s:%d",
				 net_sprint_ipv4_addr(
					 &net_sin(&context->remote)->sin_addr),
				 ntohs(net_sin(&context->remote)->sin_port));
		}

	} else if (context->local.family == AF_UNSPEC) {
		snprintk(addr_local, local_len, "AF_UNSPEC");
	} else if (context->local.family == AF_PACKET) {
		snprintk(addr_local, local_len, "AF_PACKET");
	} else if (context->local.family == AF_CAN) {
		snprintk(addr_local, local_len, "AF_CAN");
	} else {
		snprintk(addr_local, local_len, "AF_UNK(%d)",
			 context->local.family);
	}
}
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

const char *iface2str(struct net_if *iface, const char **extra)
{
#ifdef CONFIG_NET_L2_IEEE802154
	if (net_if_l2(iface) == &NET_L2_GET_NAME(IEEE802154)) {
		if (extra) {
			*extra = "=============";
		}

		return "IEEE 802.15.4";
	}
#endif

#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		struct ethernet_context *eth_ctx = net_if_l2_data(iface);

		if (eth_ctx->eth_if_type == L2_ETH_IF_TYPE_WIFI) {
			if (extra) {
				*extra = "====";
			}

			return "WiFi";
		}

		if (extra) {
			*extra = "========";
		}

		return "Ethernet";
	}
#endif

#ifdef CONFIG_NET_L2_VIRTUAL
	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		if (extra) {
			*extra = "=======";
		}

		return "Virtual";
	}
#endif

#ifdef CONFIG_NET_L2_PPP
	if (net_if_l2(iface) == &NET_L2_GET_NAME(PPP)) {
		if (extra) {
			*extra = "===";
		}

		return "PPP";
	}
#endif

#ifdef CONFIG_NET_L2_DUMMY
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		if (extra) {
			*extra = "=====";
		}

		return "Dummy";
	}
#endif

#ifdef CONFIG_NET_L2_OPENTHREAD
	if (net_if_l2(iface) == &NET_L2_GET_NAME(OPENTHREAD)) {
		if (extra) {
			*extra = "==========";
		}

		return "OpenThread";
	}
#endif

#ifdef CONFIG_NET_OFFLOAD
	if (net_if_is_ip_offloaded(iface)) {
		if (extra) {
			*extra = "==========";
		}

		return "IP Offload";
	}
#endif

#ifdef CONFIG_NET_L2_CANBUS_RAW
	if (net_if_l2(iface) == &NET_L2_GET_NAME(CANBUS_RAW)) {
		if (extra) {
			*extra = "==========";
		}

		return "CANBUS_RAW";
	}
#endif

	if (extra) {
		*extra = "==============";
	}

	return "<unknown type>";
}

/* Placeholder for net commands that are configured in the rest of the .c files */
SHELL_SUBCMD_SET_CREATE(net_cmds, (net));

SHELL_CMD_REGISTER(net, &net_cmds, "Networking commands", NULL);

int net_shell_init(void)
{
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_MONITOR_AUTO_START)) {
		events_enable();
	}

	return 0;
}
