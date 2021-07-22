/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_eth_bridge, CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <net/ethernet_bridge.h>

#include <sys/slist.h>

#include "bridge.h"

extern struct eth_bridge _eth_bridge_list_start[];
extern struct eth_bridge _eth_bridge_list_end[];

void net_eth_bridge_foreach(eth_bridge_cb_t cb, void *user_data)
{
	Z_STRUCT_SECTION_FOREACH(eth_bridge, br) {
		cb(br, user_data);
	}
}

int eth_bridge_get_index(struct eth_bridge *br)
{
	if (!(br >= _eth_bridge_list_start && br < _eth_bridge_list_end)) {
		return -1;
	}

	return (br - _eth_bridge_list_start) + 1;
}

struct eth_bridge *eth_bridge_get_by_index(int index)
{
	if (index <= 0) {
		return NULL;
	}

	if (&_eth_bridge_list_start[index - 1] >= _eth_bridge_list_end) {
		NET_DBG("Index %d is too large", index);
		return NULL;
	}

	return &_eth_bridge_list_start[index - 1];
}

int eth_bridge_iface_add(struct eth_bridge *br, struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET) ||
	    !(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
		return -EINVAL;
	}

	k_mutex_lock(&br->lock, K_FOREVER);

	if (ctx->bridge.instance != NULL) {
		k_mutex_unlock(&br->lock);
		return -EBUSY;
	}

	ctx->bridge.instance = br;
	ctx->bridge.allow_tx = false;
	sys_slist_append(&br->interfaces, &ctx->bridge.node);

	k_mutex_unlock(&br->lock);

	int ret = net_eth_promisc_mode(iface, true);

	if (ret != 0) {
		NET_DBG("iface %p promiscuous mode failed: %d", iface, ret);
		eth_bridge_iface_remove(br, iface);
		return ret;
	}

	NET_DBG("iface %p added to bridge %p", iface, br);
	return 0;
}

int eth_bridge_iface_remove(struct eth_bridge *br, struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -EINVAL;
	}

	k_mutex_lock(&br->lock, K_FOREVER);

	if (ctx->bridge.instance != br) {
		k_mutex_unlock(&br->lock);
		return -EINVAL;
	}

	sys_slist_find_and_remove(&br->interfaces, &ctx->bridge.node);
	ctx->bridge.instance = NULL;

	k_mutex_unlock(&br->lock);

	NET_DBG("iface %p removed from bridge %p", iface, br);
	return 0;
}

int eth_bridge_iface_allow_tx(struct net_if *iface, bool allow)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET) ||
	    ctx->bridge.instance == NULL) {
		return -EINVAL;
	}

	ctx->bridge.allow_tx = allow;
	return 0;
}

int eth_bridge_listener_add(struct eth_bridge *br, struct eth_bridge_listener *l)
{
	k_mutex_lock(&br->lock, K_FOREVER);
	sys_slist_append(&br->listeners, &l->node);
	k_mutex_unlock(&br->lock);
	return 0;
}

int eth_bridge_listener_remove(struct eth_bridge *br, struct eth_bridge_listener *l)
{
	k_mutex_lock(&br->lock, K_FOREVER);
	sys_slist_find_and_remove(&br->listeners, &l->node);
	k_mutex_unlock(&br->lock);
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

enum net_verdict net_eth_bridge_input(struct ethernet_context *ctx,
				      struct net_pkt *pkt)
{
	struct eth_bridge *br = ctx->bridge.instance;
	sys_snode_t *node;

	NET_DBG("new pkt %p", pkt);

	/* Drop all link-local packets for now. */
	if (is_link_local_addr((struct net_eth_addr *)net_pkt_lladdr_dst(pkt))) {
		return NET_DROP;
	}

	k_mutex_lock(&br->lock, K_FOREVER);

	/*
	 * Send packet to all registered interfaces for now.
	 * Eventually we could get smarter with a MAC address cache.
	 */
	SYS_SLIST_FOR_EACH_NODE(&br->interfaces, node) {
		struct ethernet_context *out_ctx;
		struct net_pkt *out_pkt;

		out_ctx = CONTAINER_OF(node, struct ethernet_context, bridge.node);

		/* Don't xmit on the same interface as the incoming packet's */
		if (ctx == out_ctx) {
			continue;
		}

		/* Skip it if not allowed to transmit */
		if (!out_ctx->bridge.allow_tx) {
			continue;
		}

		/* Skip it if not up */
		if (!net_if_flag_is_set(out_ctx->iface, NET_IF_UP)) {
			continue;
		}

		out_pkt = net_pkt_shallow_clone(pkt, K_NO_WAIT);
		if (out_pkt == NULL) {
			continue;
		}

		NET_DBG("sending pkt %p as %p on iface %p", pkt, out_pkt, out_ctx->iface);

		/*
		 * Use AF_UNSPEC to avoid interference, set the output
		 * interface and send the packet.
		 */
		net_pkt_set_family(out_pkt, AF_UNSPEC);
		net_pkt_set_orig_iface(out_pkt, net_pkt_iface(pkt));
		net_pkt_set_iface(out_pkt, out_ctx->iface);
		net_if_queue_tx(out_ctx->iface, out_pkt);
	}

	SYS_SLIST_FOR_EACH_NODE(&br->listeners, node) {
		struct eth_bridge_listener *l;
		struct net_pkt *out_pkt;

		l = CONTAINER_OF(node, struct eth_bridge_listener, node);

		out_pkt = net_pkt_shallow_clone(pkt, K_NO_WAIT);
		if (out_pkt == NULL) {
			continue;
		}

		k_fifo_put(&l->pkt_queue, out_pkt);
	}

	k_mutex_unlock(&br->lock);

	net_pkt_unref(pkt);
	return NET_OK;
}
