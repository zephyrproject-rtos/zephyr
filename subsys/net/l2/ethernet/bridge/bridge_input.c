/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_eth_bridge_input, CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL);

#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/net_log.h>

#include "bridge_internal.h"

#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
#include <zephyr/net/ethernet_bridge_fdb.h>

/* Send pkt out of the ports the FDB has for its destination. Returns false
 * when the FDB has no entry, so that the caller floods the frame.
 */
static bool eth_bridge_fdb_forward(struct net_if *bridge, struct net_if *orig_iface,
				   struct net_pkt *pkt)
{
	struct net_eth_addr *dst_addr = (struct net_eth_addr *)(net_pkt_lladdr_dst(pkt)->addr);
	struct net_if *ports[NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT];
	struct net_pkt *out_pkt;
	size_t count;

	count = eth_bridge_fdb_lookup(bridge, dst_addr, ports, ARRAY_SIZE(ports));
	if (count == 0) {
		return false;
	}

	/* A bridge has at most one entry per port and MAC address */
	count = MIN(count, ARRAY_SIZE(ports));

	for (size_t i = 0; i < count; i++) {
		/* The destination is on the segment the frame came from: filter it */
		if (ports[i] == orig_iface) {
			continue;
		}

		if (!net_if_flag_is_set(ports[i], NET_IF_UP) ||
		    !eth_bridge_port_is_forwarding(ports[i])) {
			continue;
		}

		out_pkt = net_pkt_clone(pkt, K_NO_WAIT);
		if (out_pkt == NULL) {
			NET_ERR("No enough memory for pkt clone");
			continue;
		}

		NET_DBG("FDB forwarding pkt %p (orig %p): iface %d -> iface %d", out_pkt, pkt,
			net_if_get_by_iface(orig_iface), net_if_get_by_iface(ports[i]));

		net_pkt_set_l2_bridged(out_pkt, true);
		net_pkt_set_iface(out_pkt, ports[i]);
		net_pkt_set_orig_iface(out_pkt, orig_iface);
		net_if_queue_tx(ports[i], out_pkt);
	}

	return true;
}
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB */

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

	NET_DBG("Passing rx pkt %p (orig %p) to bridge %d tx path from %d",
		out_pkt, pkt, net_if_get_by_iface(bridge),
		net_if_get_by_iface(orig_iface));

	net_if_queue_tx(bridge, out_pkt);

	return 0;
}

static enum net_verdict eth_bridge_handle_locally(struct net_if *bridge, struct net_if *orig_iface,
						  struct net_pkt *pkt)
{
	const struct net_l2 *l2 = net_if_l2(bridge);

	NET_ASSERT(l2 != NULL);

	net_pkt_set_iface(pkt, bridge);
	net_pkt_set_orig_iface(pkt, orig_iface);

	NET_DBG("Passing rx pkt %p to bridge %d rx path from %d",
		pkt, net_if_get_by_iface(bridge),
		net_if_get_by_iface(orig_iface));

	NET_ASSERT(l2->recv != NULL);

	return l2->recv(bridge, pkt);
}

/* The reserved addresses 01-80-C2-00-00-00 to 01-80-C2-00-00-0F, which a
 * MAC Bridge never forwards (IEEE Std 802.1Q-2014 8.6.3, Table 8-1)
 */
static inline bool is_link_local_addr(struct net_eth_addr *addr)
{
	if (addr->addr[0] == 0x01 &&
	    addr->addr[1] == 0x80 &&
	    addr->addr[2] == 0xc2 &&
	    addr->addr[3] == 0x00 &&
	    addr->addr[4] == 0x00 &&
	    (addr->addr[5] & 0xf0) == 0x00) {
		return true;
	}

	return false;
}

enum net_verdict eth_bridge_input_process(struct net_if *iface, struct net_pkt *pkt,
					  struct net_if **dst_iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_if *bridge = net_eth_get_bridge(ctx);
	struct net_eth_addr *dst_addr = (struct net_eth_addr *)(net_pkt_lladdr_dst(pkt)->addr);
	struct net_linkaddr *bridge_lladdr = net_if_get_link_addr(bridge);
	struct net_eth_addr *bridge_addr;
	enum net_verdict verdict = NET_DROP;
	enum eth_bridge_port_state state;

	NET_ASSERT(bridge_lladdr != NULL);

	bridge_addr = (struct net_eth_addr *)bridge_lladdr->addr;

	/* A frame to a reserved address is for a protocol of the port it was
	 * received on, such as gPTP, LLDP or 802.1X, and is never forwarded,
	 * whatever the FDB has for the address.
	 */
	if (is_link_local_addr(dst_addr)) {
		NET_DBG("Link-local frame handled by iface %d", net_if_get_by_iface(iface));
		return NET_CONTINUE;
	}

	state = eth_bridge_port_get_state(iface);

	/* Learn where the sender is, on a learning or forwarding port */
	if (IS_ENABLED(CONFIG_NET_ETHERNET_BRIDGE_FDB_LEARNING) &&
	    state != ETH_BRIDGE_PORT_STATE_DISCARDING) {
		eth_bridge_fdb_learn(bridge, (struct net_eth_addr *)net_pkt_lladdr_src(pkt)->addr,
				     iface);
	}

	if (state != ETH_BRIDGE_PORT_STATE_FORWARDING) {
		/* gPTP runs on every port whatever its forwarding state */
		if (net_ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP) {
			return NET_CONTINUE;
		}

		NET_DBG("DROP: iface %d not forwarding", net_if_get_by_iface(iface));
		return NET_DROP;
	}

	/* Lookup FDB table to forward. A frame for the bridge itself is
	 * delivered locally whatever the FDB has for its address.
	 */
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	if (memcmp(bridge_addr, dst_addr, NET_ETH_ADDR_LEN) != 0 &&
	    eth_bridge_fdb_forward(bridge, iface, pkt)) {
		LOG_DBG("FDB handled forwarding pkt %p from iface %p to bridge %p",
			pkt, iface, bridge);
			return NET_DROP;
	}
#endif

	/* gPTP frame should be handled by original interface via gPTP bridge stack */
	if (net_ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP) {
		return NET_CONTINUE;
	}

	if (net_eth_is_addr_broadcast(dst_addr) || net_eth_is_addr_multicast(dst_addr)) {
		/* Handle broadcast and multicast */

		/* Forward broadcast/multicast packets */
		if (eth_bridge_forward(bridge, iface, pkt) < 0) {
			LOG_ERR("Failed to forward broadcast/multicast pkt %p from iface %p to "
				"bridge %p",
				pkt, iface, bridge);
		}

		/* Check if the packet should also be handled locally */
		verdict = eth_bridge_handle_locally(bridge, iface, pkt);
		if (verdict == NET_DROP) {
			NET_DBG("DROP: broadcast/multicast");
			return NET_DROP;
		}
	} else if (memcmp(bridge_addr, dst_addr, NET_ETH_ADDR_LEN) == 0) {
		/* Handle local address */

		/* Check if the packet should be handled locally (ie bridge turned off) */
		verdict = eth_bridge_handle_locally(bridge, iface, pkt);
		if (verdict == NET_DROP) {
			NET_DBG("DROP: unicast to bridge address");
			return NET_DROP;
		}
	} else {
		/* Packet not meant for us, but forward it to other interfaces */
		eth_bridge_forward(bridge, iface, pkt);
		/* Always drop forwarded pkt for original iface */
		NET_DBG("DROP: unicast to other address");
		return NET_DROP;
	}

	/* If the verdict is NET_CONTINUE, then the packet should be passed down the stack for
	 * further processing and we must override the destination iface to be the bridge iface.
	 */
	if (verdict == NET_CONTINUE) {
		LOG_DBG("Processing pkt %p locally on bridge %p from iface %p", pkt, bridge, iface);
		*dst_iface = bridge;
	}

	return verdict;
}
