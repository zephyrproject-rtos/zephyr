/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_stats_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt_filter.h>

#include "net_sample_common.h"

#define MAX_INTERFACES 3

static struct net_if *interfaces[MAX_INTERFACES];

/* Interface match rule. The interface value is set at runtime in init_app() */
static NPF_IFACE_MATCH(match_iface_vlan1, NULL);
static NPF_IFACE_MATCH(match_iface_vlan2, NULL);
static NPF_IFACE_MATCH(match_iface_eth, NULL);

/* Match ethernet packets for the precision time protocol (PTP) */
static NPF_ETH_TYPE_MATCH(match_ptype_ptp, NET_ETH_PTYPE_PTP);
/* Match ethernet packets for the virtual local area network (VLAN) */
static NPF_ETH_TYPE_MATCH(match_ptype_vlan, NET_ETH_PTYPE_VLAN);
/* Match max size rule */
static NPF_SIZE_MAX(match_smaller_200, 200);
/* Match min size rule */
static NPF_SIZE_MIN(match_bigger_100, 100);
/* Match virtual internet protocol traffic */
static NPF_ETH_VLAN_TYPE_MATCH(match_ipv4_vlan, NET_ETH_PTYPE_IP);
/* Match virtual address resolution protocol (ARP) traffic */
static NPF_ETH_VLAN_TYPE_MATCH(match_arp_vlan, NET_ETH_PTYPE_ARP);

/* Allow all traffic to Ethernet interface */
static NPF_RULE(eth_iface_rule, NET_OK, match_iface_eth);
/* Maximal priority for ptp traffic */
static NPF_PRIORITY(eth_priority_ptp, NET_PRIORITY_NC, match_iface_eth, match_ptype_ptp);
/* Prioritize VLAN traffic on Ethernet interface */
static NPF_PRIORITY(eth_priority_vlan, NET_PRIORITY_EE, match_iface_eth, match_ptype_vlan);
/* Deprioritize all other traffic */
static NPF_PRIORITY(eth_priority_default, NET_PRIORITY_BK, match_iface_eth);

/* Allow only small ipv4 packets to the first VLAN interface */
static NPF_RULE(small_ipv4_pkt, NET_OK, match_iface_vlan1, match_ipv4_vlan, match_smaller_200);
/* Allow only ipv4 packets of minimum size to the second VLAN interface */
static NPF_RULE(large_ipv4_pkt, NET_OK, match_iface_vlan2, match_ipv4_vlan, match_bigger_100);
/* Allow ARP for both VLAN interfaces */
static NPF_RULE(arp_pkt_vlan1, NET_OK, match_iface_vlan1, match_arp_vlan);
static NPF_RULE(arp_pkt_vlan2, NET_OK, match_iface_vlan2, match_arp_vlan);
/* But give ARP the lowest priority */
static NPF_PRIORITY(arp_priority_vlan1, NET_PRIORITY_BK, match_iface_vlan1, match_arp_vlan);
static NPF_PRIORITY(arp_priority_vlan2, NET_PRIORITY_BK, match_iface_vlan2, match_arp_vlan);

/* Block IPv4 or IPv6 packets from only these addresses */
#define PEER1_IPV4_ADDR_INIT {{{ 192, 0, 2, 2 }}}
#define PEER2_IPV4_ADDR_INIT {{{ 198, 51, 100, 2 }}}
#define PEER1_IPV6_ADDR_INIT \
	{{{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0x02 }}}
#define PEER2_IPV6_ADDR_INIT \
	{{{ 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,	0, 0, 0, 0, 0x1, 0, 0, 0x02 }}}

static struct net_in_addr peer_ipv4_addr[] = {
	[0] = PEER1_IPV4_ADDR_INIT,
	[1] = PEER2_IPV4_ADDR_INIT,
};

static struct net_in6_addr peer_ipv6_addr[] = {
	[0] = PEER1_IPV6_ADDR_INIT,
	[1] = PEER2_IPV6_ADDR_INIT,
};

static NPF_IP_SRC_ADDR_BLOCKLIST(ipv4_src_block,
				 peer_ipv4_addr, ARRAY_SIZE(peer_ipv4_addr),
				 NET_AF_INET);
static NPF_IP_SRC_ADDR_BLOCKLIST(ipv6_src_block,
				 peer_ipv6_addr, ARRAY_SIZE(peer_ipv6_addr),
				 NET_AF_INET6);
static NPF_RULE(ipv4_addr_block, NET_OK, ipv4_src_block);
static NPF_RULE(ipv6_addr_block, NET_OK, ipv6_src_block);

static void iface_cb(struct net_if *iface, void *user_data)
{
	int count = 0;

	ARG_UNUSED(user_data);

	ARRAY_FOR_EACH(interfaces, i) {
		if (interfaces[i] == NULL) {
			interfaces[i] = iface;
			return;
		}

		count++;
	}

	LOG_ERR("Too many interfaces %d (max is %d)", count, MAX_INTERFACES);
}

static void init_app(void)
{
	struct net_if *iface, *eth = NULL, *vlan1 = NULL, *vlan2 = NULL;
	int found = 0;

	ARRAY_FOR_EACH(interfaces, i) {
		if (interfaces[i] == NULL) {
			continue;
		}

		iface = interfaces[i];

		if (net_eth_is_vlan_interface(iface)) {
			if (vlan1 == NULL) {
				vlan1 = iface;
			} else if (vlan2 == NULL) {
				vlan2 = iface;
			}
		} else if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
			eth = iface;
		} else {
			continue;
		}

		found++;
	}

	if (found == 0) {
		LOG_ERR("No interfaces found");
		return;
	}

	match_iface_vlan1.iface = vlan1;
	match_iface_vlan2.iface = vlan2;
	match_iface_eth.iface = eth;

	/* The sample will setup the Ethernet interface and two VLAN
	 * optional interfaces (if VLAN is enabled).
	 * We allow all traffic to the Ethernet interface, but have
	 * filters for the VLAN interfaces and check IPv4 and IPv6 source addresses.
	 *
	 * First append the priority rules, so that they get evaluated before
	 * deciding on the final verdict for the packet.
	 */
	if (IS_ENABLED(CONFIG_NET_SAMPLE_USE_PACKET_PRIORITIES)) {
		LOG_INF("Using packet priorities");

		npf_append_recv_rule(&eth_priority_default);
		npf_append_recv_rule(&eth_priority_ptp);
		npf_append_recv_rule(&eth_priority_vlan);
		npf_append_recv_rule(&arp_priority_vlan1);
		npf_append_recv_rule(&arp_priority_vlan2);
	} else {
		LOG_INF("Packet priorities are disabled");
	}

	/* We allow small IPv4 packets to the VLAN interface 1 */
	npf_append_recv_rule(&small_ipv4_pkt);

	/* We allow large IPv4 packets to the VLAN interface 2 */
	npf_append_recv_rule(&large_ipv4_pkt);

	/* We allow all traffic to the Ethernet interface */
	npf_append_recv_rule(&eth_iface_rule);

	/* We allow ARP traffic to both VLAN interfaces */
	npf_append_recv_rule(&arp_pkt_vlan1);
	npf_append_recv_rule(&arp_pkt_vlan2);

	/* The remaining packets that do not match are dropped */
	npf_append_recv_rule(&npf_default_drop);

	/* We block packets from specific IPv4 addresses */
	npf_append_ipv4_recv_rule(&ipv4_addr_block);

	/* We block packets from specific IPv6 addresses */
	npf_append_ipv6_recv_rule(&ipv6_addr_block);
}

int main(void)
{
	net_if_foreach(iface_cb, interfaces);

	init_vlan();
	init_app();

	return 0;
}
