/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#if defined(CONFIG_NET_L2_ETHERNET)
#include <zephyr/net/ethernet.h>
#endif
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
#include <zephyr/net/ethernet_mgmt.h>
#endif
#if defined(CONFIG_NET_L2_VIRTUAL)
#include <zephyr/net/virtual.h>
#endif

#include "net_shell_private.h"

#if defined(CONFIG_NET_L2_ETHERNET) && defined(CONFIG_NET_NATIVE)
struct ethernet_capabilities {
	enum ethernet_hw_caps capability;
	const char * const description;
};

#define EC(cap, desc) { .capability = cap, .description = desc }

static struct ethernet_capabilities eth_hw_caps[] = {
	EC(ETHERNET_HW_TX_CHKSUM_OFFLOAD, "TX checksum offload"),
	EC(ETHERNET_HW_RX_CHKSUM_OFFLOAD, "RX checksum offload"),
	EC(ETHERNET_HW_VLAN,              "Virtual LAN"),
	EC(ETHERNET_HW_VLAN_TAG_STRIP,    "VLAN Tag stripping"),
	EC(ETHERNET_AUTO_NEGOTIATION_SET, "Auto negotiation"),
	EC(ETHERNET_LINK_10BASE_T,        "10 Mbits"),
	EC(ETHERNET_LINK_100BASE_T,       "100 Mbits"),
	EC(ETHERNET_LINK_1000BASE_T,      "1 Gbits"),
	EC(ETHERNET_DUPLEX_SET,           "Half/full duplex"),
	EC(ETHERNET_PTP,                  "IEEE 802.1AS gPTP clock"),
	EC(ETHERNET_QAV,                  "IEEE 802.1Qav (credit shaping)"),
	EC(ETHERNET_QBV,                  "IEEE 802.1Qbv (scheduled traffic)"),
	EC(ETHERNET_QBU,                  "IEEE 802.1Qbu (frame preemption)"),
	EC(ETHERNET_TXTIME,               "TXTIME"),
	EC(ETHERNET_PROMISC_MODE,         "Promiscuous mode"),
	EC(ETHERNET_PRIORITY_QUEUES,      "Priority queues"),
	EC(ETHERNET_HW_FILTERING,         "MAC address filtering"),
	EC(ETHERNET_DSA_SLAVE_PORT,       "DSA slave port"),
	EC(ETHERNET_DSA_MASTER_PORT,      "DSA master port"),
};

static void print_supported_ethernet_capabilities(
	const struct shell *sh, struct net_if *iface)
{
	enum ethernet_hw_caps caps = net_eth_get_hw_capabilities(iface);

	ARRAY_FOR_EACH(eth_hw_caps, i) {
		if (caps & eth_hw_caps[i].capability) {
			PR("\t%s\n", eth_hw_caps[i].description);
		}
	}
}
#endif /* CONFIG_NET_L2_ETHERNET */

#if defined(CONFIG_NET_NATIVE)
static const char *iface_flags2str(struct net_if *iface)
{
	static char str[sizeof("POINTOPOINT") + sizeof("PROMISC") +
			sizeof("NO_AUTO_START") + sizeof("SUSPENDED") +
			sizeof("MCAST_FORWARD") + sizeof("IPv4") +
			sizeof("IPv6") + sizeof("NO_ND") + sizeof("NO_MLD")];
	int pos = 0;

	if (net_if_flag_is_set(iface, NET_IF_POINTOPOINT)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"POINTOPOINT,");
	}

	if (net_if_flag_is_set(iface, NET_IF_PROMISC)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"PROMISC,");
	}

	if (net_if_flag_is_set(iface, NET_IF_NO_AUTO_START)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"NO_AUTO_START,");
	} else {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"AUTO_START,");
	}

	if (net_if_flag_is_set(iface, NET_IF_FORWARD_MULTICASTS)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"MCAST_FORWARD,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV4)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"IPv4,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"IPv6,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"NO_ND,");
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		pos += snprintk(str + pos, sizeof(str) - pos,
				"NO_MLD,");
	}

	/* get rid of last ',' character */
	str[pos - 1] = '\0';

	return str;
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
#if defined(CONFIG_NET_NATIVE)
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;

#if defined(CONFIG_NET_IPV6)
	struct net_if_ipv6_prefix *prefix;
	struct net_if_router *router;
	struct net_if_ipv6 *ipv6;
#endif
#if defined(CONFIG_NET_IPV4)
	struct net_if_ipv4 *ipv4;
#endif
#if defined(CONFIG_NET_IP)
	struct net_if_addr *unicast;
	struct net_if_mcast_addr *mcast;
#endif
#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	struct ethernet_req_params params;
	int ret;
#endif
	const char *extra;
#if defined(CONFIG_NET_IP) || defined(CONFIG_NET_L2_ETHERNET_MGMT)
	int count;
#endif

	if (data->user_data && data->user_data != iface) {
		return;
	}

#if defined(CONFIG_NET_INTERFACE_NAME)
	char ifname[CONFIG_NET_INTERFACE_NAME_LEN + 1] = { 0 };
	int ret_name;

	ret_name = net_if_get_name(iface, ifname, sizeof(ifname) - 1);
	if (ret_name < 1 || ifname[0] == '\0') {
		snprintk(ifname, sizeof(ifname), "?");
	}

	PR("\nInterface %s (%p) (%s) [%d]\n", ifname, iface, iface2str(iface, &extra),
	   net_if_get_by_iface(iface));
#else
	PR("\nInterface %p (%s) [%d]\n", iface, iface2str(iface, &extra),
	   net_if_get_by_iface(iface));
#endif
	PR("===========================%s\n", extra);

	if (!net_if_is_up(iface)) {
		PR_INFO("Interface is down.\n");

		/* Show detailed information only when user asks information
		 * about one specific network interface.
		 */
		if (data->user_data == NULL) {
			return;
		}
	}

#ifdef CONFIG_NET_POWER_MANAGEMENT
	if (net_if_is_suspended(iface)) {
		PR_INFO("Interface is suspended, thus not able to tx/rx.\n");
	}
#endif

#if defined(CONFIG_NET_L2_VIRTUAL)
	if (!sys_slist_is_empty(&iface->config.virtual_interfaces)) {
		struct virtual_interface_context *ctx, *tmp;

		PR("Virtual interfaces attached to this : ");
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(
					&iface->config.virtual_interfaces,
					ctx, tmp, node) {
			if (ctx->virtual_iface == iface) {
				continue;
			}

			PR("%d ", net_if_get_by_iface(ctx->virtual_iface));
		}

		PR("\n");
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		struct net_if *orig_iface;
		char *name, buf[CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN];

		name = net_virtual_get_name(iface, buf, sizeof(buf));
		if (!(name && name[0])) {
			name = "<unknown>";
		}

		PR("Virtual name : %s\n", name);

		orig_iface = net_virtual_get_iface(iface);
		if (orig_iface == NULL) {
			PR("No attached network interface.\n");
		} else {
			PR("Attached  : %d (%s / %p)\n",
			   net_if_get_by_iface(orig_iface),
			   iface2str(orig_iface, NULL),
			   orig_iface);
		}
	}
#endif /* CONFIG_NET_L2_VIRTUAL */

	net_if_lock(iface);
	if (net_if_get_link_addr(iface) &&
	    net_if_get_link_addr(iface)->addr) {
		PR("Link addr : %s\n",
		   net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
				      net_if_get_link_addr(iface)->len));
	}
	net_if_unlock(iface);

	PR("MTU       : %d\n", net_if_get_mtu(iface));
	PR("Flags     : %s\n", iface_flags2str(iface));
	PR("Device    : %s (%p)\n",
	   net_if_get_device(iface) ? net_if_get_device(iface)->name : "<?>",
	   net_if_get_device(iface));

#if defined(CONFIG_NET_L2_ETHERNET_MGMT)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		count = 0;
		ret = net_mgmt(NET_REQUEST_ETHERNET_GET_PRIORITY_QUEUES_NUM,
				iface, &params,
				sizeof(struct ethernet_req_params));

		if (!ret && params.priority_queues_num) {
			count = params.priority_queues_num;
			PR("Priority queues:\n");
			for (int i = 0; i < count; ++i) {
				params.qav_param.queue_id = i;
				params.qav_param.type =
					ETHERNET_QAV_PARAM_TYPE_STATUS;
				ret = net_mgmt(
					NET_REQUEST_ETHERNET_GET_QAV_PARAM,
					iface, &params,
					sizeof(struct ethernet_req_params));

				PR("\t%d: Qav ", i);
				if (ret) {
					PR("not supported\n");
				} else {
					PR("%s\n",
						params.qav_param.enabled ?
						"enabled" :
						"disabled");
				}
			}
		}
	}
#endif

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	PR("Promiscuous mode : %s\n",
	   net_if_is_promisc(iface) ? "enabled" : "disabled");
#endif

#if defined(CONFIG_NET_VLAN)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		if (net_virtual_get_iface_capabilities(iface) & VIRTUAL_INTERFACE_VLAN) {
			uint16_t tag;

			tag = net_eth_get_vlan_tag(iface);
			if (tag == NET_VLAN_TAG_UNSPEC) {
				PR("VLAN not configured\n");
			} else {
				PR("VLAN tag  : %d (0x%03x)\n", tag, tag);
			}
		}
	}
#endif

#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		PR("Ethernet capabilities supported:\n");
		print_supported_ethernet_capabilities(sh, iface);
	}
#endif /* CONFIG_NET_L2_ETHERNET */

#if defined(CONFIG_NET_IPV6)
	count = 0;
	ipv6 = iface->config.ip.ipv6;

	if (!net_if_flag_is_set(iface, NET_IF_IPV6) || ipv6 == NULL) {
		PR("%s not %s for this interface.\n", "IPv6", "enabled");
		ipv6 = NULL;
		goto skip_ipv6;
	}

	PR("IPv6 unicast addresses (max %d):\n", NET_IF_MAX_IPV6_ADDR);
	ARRAY_FOR_EACH(ipv6->unicast, i) {
		unicast = &ipv6->unicast[i];

		if (!unicast->is_used) {
			continue;
		}

		PR("\t%s %s %s%s%s%s\n",
		   net_sprint_ipv6_addr(&unicast->address.in6_addr),
		   addrtype2str(unicast->addr_type),
		   addrstate2str(unicast->addr_state),
		   unicast->is_infinite ? " infinite" : "",
		   unicast->is_mesh_local ? " meshlocal" : "",
		   unicast->is_temporary ? " temporary" : "");
		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	count = 0;

	PR("IPv6 multicast addresses (max %d):\n", NET_IF_MAX_IPV6_MADDR);
	ARRAY_FOR_EACH(ipv6->mcast, i) {
		mcast = &ipv6->mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		PR("\t%s%s\n", net_sprint_ipv6_addr(&mcast->address.in6_addr),
		   net_if_ipv6_maddr_is_joined(mcast) ? "" : "  <not joined>");

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	count = 0;

	PR("IPv6 prefixes (max %d):\n", NET_IF_MAX_IPV6_PREFIX);
	ARRAY_FOR_EACH(ipv6->prefix, i) {
		prefix = &ipv6->prefix[i];

		if (!prefix->is_used) {
			continue;
		}

		PR("\t%s/%d%s\n",
		   net_sprint_ipv6_addr(&prefix->prefix),
		   prefix->len, prefix->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	router = net_if_ipv6_router_find_default(iface, NULL);
	if (router) {
		PR("IPv6 default router :\n");
		PR("\t%s%s\n",
		   net_sprint_ipv6_addr(&router->address.in6_addr),
		   router->is_infinite ? " infinite" : "");
	}

skip_ipv6:

#if defined(CONFIG_NET_IPV6_PE)
	PR("IPv6 privacy extension   : %s (preferring %s addresses)\n",
	   iface->pe_enabled ? "enabled" : "disabled",
	   iface->pe_prefer_public ? "public" : "temporary");
#endif

	if (ipv6) {
		PR("IPv6 hop limit           : %d\n",
		   ipv6->hop_limit);
		PR("IPv6 base reachable time : %d\n",
		   ipv6->base_reachable_time);
		PR("IPv6 reachable time      : %d\n",
		   ipv6->reachable_time);
		PR("IPv6 retransmit timer    : %d\n",
		   ipv6->retrans_timer);
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	/* No need to print IPv4 information for interface that does not
	 * support that protocol.
	 */
	if (
#if defined(CONFIG_NET_L2_IEEE802154)
		(net_if_l2(iface) == &NET_L2_GET_NAME(IEEE802154)) ||
#endif
		 0) {
		PR_WARNING("%s not %s for this interface.\n", "IPv4",
			   "supported");
		return;
	}

	count = 0;
	ipv4 = iface->config.ip.ipv4;

	if (!net_if_flag_is_set(iface, NET_IF_IPV4) || ipv4 == NULL) {
		PR("%s not %s for this interface.\n", "IPv4", "enabled");
		ipv4 = NULL;
		goto skip_ipv4;
	}

	PR("IPv4 unicast addresses (max %d):\n", NET_IF_MAX_IPV4_ADDR);
	ARRAY_FOR_EACH(ipv4->unicast, i) {
		unicast = &ipv4->unicast[i].ipv4;

		if (!unicast->is_used) {
			continue;
		}

		PR("\t%s/%s %s %s%s\n",
		   net_sprint_ipv4_addr(&unicast->address.in_addr),
		   net_sprint_ipv4_addr(&ipv4->unicast[i].netmask),

		   addrtype2str(unicast->addr_type),
		   addrstate2str(unicast->addr_state),
		   unicast->is_infinite ? " infinite" : "");

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

	count = 0;

	PR("IPv4 multicast addresses (max %d):\n", NET_IF_MAX_IPV4_MADDR);
	ARRAY_FOR_EACH(ipv4->mcast, i) {
		mcast = &ipv4->mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		PR("\t%s%s\n", net_sprint_ipv4_addr(&mcast->address.in_addr),
		   net_if_ipv4_maddr_is_joined(mcast) ? "" : "  <not joined>");

		count++;
	}

	if (count == 0) {
		PR("\t<none>\n");
	}

skip_ipv4:

	if (ipv4) {
		PR("IPv4 gateway : %s\n",
		   net_sprint_ipv4_addr(&ipv4->gw));
	}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_DHCPV4)
	if (net_if_flag_is_set(iface, NET_IF_IPV4)) {
		PR("DHCPv4 lease time : %u\n",
		   iface->config.dhcpv4.lease_time);
		PR("DHCPv4 renew time : %u\n",
		   iface->config.dhcpv4.renewal_time);
		PR("DHCPv4 server     : %s\n",
		   net_sprint_ipv4_addr(&iface->config.dhcpv4.server_id));
		PR("DHCPv4 requested  : %s\n",
		   net_sprint_ipv4_addr(&iface->config.dhcpv4.requested_ip));
		PR("DHCPv4 state      : %s\n",
		   net_dhcpv4_state_name(iface->config.dhcpv4.state));
		PR("DHCPv4 attempts   : %d\n",
		   iface->config.dhcpv4.attempts);
	}
#endif /* CONFIG_NET_DHCPV4 */

#else
	ARG_UNUSED(iface);
	ARG_UNUSED(user_data);

#endif /* CONFIG_NET_NATIVE */
}

static int cmd_net_set_mac(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_L2_ETHERNET) || !defined(CONFIG_NET_L2_ETHERNET_MGMT)
	PR_WARNING("Unsupported command, please enable CONFIG_NET_L2_ETHERNET "
		"and CONFIG_NET_L2_ETHERNET_MGMT\n");
	return -ENOEXEC;
#else
	struct net_if *iface;
	struct ethernet_req_params params;
	char *mac_addr = params.mac_address.addr;
	int idx;
	int ret;

	if (argc < 3) {
		PR_WARNING("Missing interface index and/or MAC address\n");
		goto err;
	}

	idx = get_iface_idx(sh, argv[1]);
	if (idx < 0) {
		goto err;
	}

	iface = net_if_get_by_index(idx);
	if (!iface) {
		PR_WARNING("No such interface in index %d\n", idx);
		goto err;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		PR_WARNING("MAC address can be set only for Ethernet\n");
		goto err;
	}

	if ((net_bytes_from_str(mac_addr, sizeof(params.mac_address), argv[2]) < 0) ||
	    !net_eth_is_addr_valid(&params.mac_address)) {
		PR_WARNING("Invalid MAC address: %s\n", argv[2]);
		goto err;
	}

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface, &params, sizeof(params));
	if (ret < 0) {
		if (ret == -EACCES) {
			PR_WARNING("MAC address cannot be set when interface is operational\n");
			goto err;
		}
		PR_WARNING("Failed to set MAC address (%d)\n", ret);
		goto err;
	}

	PR_INFO("MAC address set to %s\n",
		net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
		net_if_get_link_addr(iface)->len));

	return 0;
err:
	return -ENOEXEC;
#endif /* CONFIG_NET_L2_ETHERNET */
}

static int cmd_net_iface_up(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface;
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

	if (net_if_is_up(iface)) {
		PR_WARNING("Interface %d is already up.\n", idx);
		return -ENOEXEC;
	}

	ret = net_if_up(iface);
	if (ret) {
		PR_WARNING("Cannot take interface %d up (%d)\n", idx, ret);
		return -ENOEXEC;
	}

	PR("Interface %d is up\n", idx);

	return 0;
}

static int cmd_net_iface_down(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface;
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

	ret = net_if_down(iface);
	if (ret) {
		PR_WARNING("Cannot take interface %d down (%d)\n", idx, ret);
		return -ENOEXEC;
	}

	PR("Interface %d is down\n", idx);

	return 0;
}

static int cmd_net_iface(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = NULL;
	struct net_shell_user_data user_data;
	int idx;

	if (argv[1]) {
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

#if defined(CONFIG_NET_HOSTNAME_ENABLE)
	PR("Hostname: %s\n\n", net_hostname_get());
#endif

	user_data.sh = sh;
	user_data.user_data = iface;

	net_if_foreach(iface_cb, &user_data);

	return 0;
}

#if defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)

#include "iface_dynamic.h"

#endif /* CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_iface,
	SHELL_CMD(up, IFACE_DYN_CMD,
		  "'net iface up <index>' takes network interface up.",
		  cmd_net_iface_up),
	SHELL_CMD(down, IFACE_DYN_CMD,
		  "'net iface down <index>' takes network interface "
		  "down.",
		  cmd_net_iface_down),
	SHELL_CMD(show, IFACE_DYN_CMD,
		  "'net iface <index>' shows network interface "
		  "information.",
		  cmd_net_iface),
	SHELL_CMD(set_mac, IFACE_DYN_CMD,
		  "'net iface set_mac <index> <MAC>' sets MAC address for the network interface.",
		  cmd_net_set_mac),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), iface, &net_cmd_iface,
		 "Print information about network interfaces.",
		 cmd_net_iface, 1, 1);
