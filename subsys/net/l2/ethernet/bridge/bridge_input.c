/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_eth_bridge_input, CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL);

#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/net_log.h>

#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
#include <zephyr/net/ethernet_bridge_fdb.h>

struct eth_bridge_fdb_forward_data {
	struct net_if *bridge;
	struct net_if *orig_iface;
	struct net_pkt *pkt;
	bool match;
};

static void eth_bridge_fdb_forward_handler(struct eth_bridge_fdb_entry *entry, void *user_data)
{
	struct eth_bridge_fdb_forward_data *data = (struct eth_bridge_fdb_forward_data *)user_data;
	struct net_if *bridge = data->bridge;
	struct net_if *orig_iface = data->orig_iface;
	struct net_pkt *pkt = data->pkt;
	struct net_eth_addr *dst_addr = (struct net_eth_addr *)(net_pkt_lladdr_dst(pkt)->addr);
	struct net_pkt *out_pkt;

	/* Check if the entry is matched */
	if (memcmp(&entry->mac, dst_addr, sizeof(struct net_eth_addr)) != 0) {
		return;
	}

	if (net_eth_get_bridge(net_if_l2_data(entry->iface)) != bridge) {
		return;
	}

	data->match = true;

	/* Forward per FDB entry */
	if (!net_if_flag_is_set(entry->iface, NET_IF_UP)) {
		return;
	}

	out_pkt = net_pkt_clone(pkt, K_NO_WAIT);
	if (out_pkt == NULL) {
		NET_ERR("No enough memory for pkt clone");
		return;
	}

	NET_DBG("FDB forwarding pkt %p (orig %p): iface %d -> iface %d", out_pkt, pkt,
		net_if_get_by_iface(orig_iface), net_if_get_by_iface(entry->iface));

	net_pkt_set_l2_bridged(out_pkt, true);
	net_pkt_set_iface(out_pkt, entry->iface);
	net_pkt_set_orig_iface(out_pkt, orig_iface);
	net_if_queue_tx(entry->iface, out_pkt);
}

static bool eth_bridge_fdb_forward(struct net_if *bridge, struct net_if *orig_iface,
				   struct net_pkt *pkt)
{
	struct eth_bridge_fdb_forward_data data = {0};

	data.bridge = bridge;
	data.orig_iface = orig_iface;
	data.pkt = pkt;

	eth_bridge_fdb_foreach(&eth_bridge_fdb_forward_handler, (void *)&data);

	return data.match;
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
	net_pkt_set_iface(pkt, bridge);
	net_pkt_set_orig_iface(pkt, orig_iface);

	NET_DBG("Passing rx pkt %p to bridge %d rx path from %d",
		pkt, net_if_get_by_iface(bridge),
		net_if_get_by_iface(orig_iface));

	NET_ASSERT(net_if_l2(bridge)->recv != NULL);

	return net_if_l2(bridge)->recv(bridge, pkt);
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

enum net_verdict eth_bridge_input_process(struct net_if *iface, struct net_pkt *pkt,
					  struct net_if **dst_iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_if *bridge = net_eth_get_bridge(ctx);
	struct net_eth_addr *dst_addr = (struct net_eth_addr *)(net_pkt_lladdr_dst(pkt)->addr);
	struct net_eth_addr *bridge_addr =
		(struct net_eth_addr *)(net_if_get_link_addr(bridge)->addr);
	enum net_verdict verdict = NET_DROP;

	/* Lookup FDB table to forward */
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	if (eth_bridge_fdb_forward(bridge, iface, pkt)) {
		LOG_DBG("FDB handled forwarding pkt %p from iface %p to bridge %p",
			pkt, iface, bridge);
			return NET_DROP;
	}
#endif

	/* Drop all link-local packets for now. */
	if (is_link_local_addr(dst_addr)) {
		NET_DBG("DROP: lladdr");
		return NET_DROP;
	}

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
