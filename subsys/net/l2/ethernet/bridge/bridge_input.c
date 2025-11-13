/*
 * SPDX-FileCopyrightText: Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_eth_bridge_input, CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL);

#include <zephyr/net/ethernet_bridge.h>

static int eth_bridge_forward(struct net_if *bridge, struct net_if *orig_iface, struct net_pkt *pkt)
{
	struct net_pkt *out_pkt;

	out_pkt = net_pkt_clone(pkt, K_NO_WAIT);
	if (out_pkt == NULL) {
		return -ENOMEM;
	}

	net_pkt_set_l2_bridged(out_pkt, true);
	net_pkt_set_iface(out_pkt, bridge);
	net_pkt_set_orig_iface(out_pkt, orig_iface);

	net_if_queue_tx(bridge, out_pkt);

	NET_DBG("Passing rx pkt %p (orig %p) to bridge %d tx path from %d",
		out_pkt, pkt, net_if_get_by_iface(bridge),
		net_if_get_by_iface(orig_iface));

	return 0;
}

static inline bool is_link_local_addr(struct net_eth_addr *addr)
{
	if (addr->addr[0] == 0x01 &&
	    addr->addr[1] == 0x80 &&
	    addr->addr[2] == 0xc2 &&
	    addr->addr[3] == 0x00 &&
	    addr->addr[4] == 0x00 &&
	    (addr->addr[5] & 0x0f) == 0x00) {
		return true;
	}

	return false;
}

enum net_verdict eth_bridge_input_process(struct net_if *iface, struct net_pkt *pkt)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_if *bridge = net_eth_get_bridge(ctx);
	struct net_eth_addr *dst_addr = (struct net_eth_addr *)(net_pkt_lladdr_dst(pkt)->addr);

	/* Drop all link-local packets for now. */
	if (is_link_local_addr(dst_addr)) {
		NET_DBG("DROP: lladdr");
		return NET_DROP;
	}

	if (eth_bridge_forward(bridge, iface, pkt) != 0) {
		return NET_DROP;
	}

	return NET_CONTINUE;
}
