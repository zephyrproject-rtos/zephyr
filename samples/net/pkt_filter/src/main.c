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

/* Allow all traffic to Ethernet interface, drop VLAN traffic except
 * couple of exceptions for IPv4 and IPv6
 */
static NPF_RULE(eth_iface_rule, NET_OK, match_iface_eth);

/* Match max size rule */
static NPF_SIZE_MAX(maxsize_200, 200);
static NPF_ETH_VLAN_TYPE_MATCH(ipv4_packet1, NET_ETH_PTYPE_IP);
static NPF_RULE(small_ipv4_pkt, NET_OK, ipv4_packet1, maxsize_200,
		match_iface_vlan1);

/* Match min size rule */
static NPF_SIZE_MIN(minsize_100, 100);
static NPF_ETH_VLAN_TYPE_MATCH(ipv4_packet2, NET_ETH_PTYPE_IP);
static NPF_RULE(large_ipv4_pkt, NET_OK, ipv4_packet2, minsize_100,
		match_iface_vlan2);

/* Allow ARP for VLAN interface */
static NPF_ETH_VLAN_TYPE_MATCH(arp_packet, NET_ETH_PTYPE_ARP);
static NPF_RULE(arp_pkt_vlan1, NET_OK, arp_packet, match_iface_vlan1);
static NPF_RULE(arp_pkt_vlan2, NET_OK, arp_packet, match_iface_vlan2);

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
	 * filters for the VLAN interfaces.
	 */

	/* We allow small IPv4 packets to the VLAN interface 1 */
	npf_append_recv_rule(&small_ipv4_pkt);

	/* We allow large IPv4 packets to the VLAN interface 2 */
	npf_append_recv_rule(&large_ipv4_pkt);

	/* We allow all traffic to the Ethernet interface */
	npf_append_recv_rule(&eth_iface_rule);

	/* We allow ARP traffic to the VLAN 1 interface */
	npf_append_recv_rule(&arp_pkt_vlan1);

	/* We allow ARP traffic to the VLAN 2 interface */
	npf_append_recv_rule(&arp_pkt_vlan2);

	/* The remaining packets that do not match are dropped */
	npf_append_recv_rule(&npf_default_drop);
}

int main(void)
{
	net_if_foreach(iface_cb, interfaces);

	init_vlan();
	init_app();

	return 0;
}
