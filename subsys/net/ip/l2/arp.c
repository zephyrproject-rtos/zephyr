/** @file
 * @brief ARP related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_ARP)
#define SYS_LOG_DOMAIN "net/arp"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_stats.h>
#include <net/arp.h>
#include "net_private.h"

static struct arp_entry arp_table[CONFIG_NET_ARP_TABLE_SIZE];

static inline struct arp_entry *find_entry(struct net_if *iface,
					   struct in_addr *dst,
					   struct arp_entry **free_entry,
					   struct arp_entry **non_pending)
{
	int i;

	NET_DBG("dst %s", net_sprint_ipv4_addr(dst));

	for (i = 0; i < CONFIG_NET_ARP_TABLE_SIZE; i++) {

		NET_DBG("[%d] iface %p dst %s ll %s pending %p", i, iface,
			net_sprint_ipv4_addr(&arp_table[i].ip),
			net_sprint_ll_addr((u8_t *)&arp_table[i].eth.addr,
					   sizeof(struct net_eth_addr)),
			arp_table[i].pending);

		if (arp_table[i].iface == iface &&
		    net_ipv4_addr_cmp(&arp_table[i].ip, dst)) {
			/* Is there already pending operation for this
			 * IP address.
			 */
			if (arp_table[i].pending) {
				NET_DBG("ARP already pending to %s ll %s",
					net_sprint_ipv4_addr(dst),
					net_sprint_ll_addr((u8_t *)
						&arp_table[i].eth.addr,
						sizeof(struct net_eth_addr)));
				*free_entry = NULL;
				*non_pending = NULL;
				return NULL;
			}

			return &arp_table[i];
		}

		/* We return also the first free entry */
		if (!*free_entry && !arp_table[i].pending &&
		    !arp_table[i].iface) {
			*free_entry = &arp_table[i];
		}

		/* And also first non pending entry */
		if (!*non_pending && !arp_table[i].pending) {
			*non_pending = &arp_table[i];
		}
	}

	return NULL;
}

static inline struct in_addr *if_get_addr(struct net_if *iface)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->ipv4.unicast[i].is_used &&
		    iface->ipv4.unicast[i].address.family == AF_INET &&
		    iface->ipv4.unicast[i].addr_state == NET_ADDR_PREFERRED) {
			return &iface->ipv4.unicast[i].address.in_addr;
		}
	}

	return NULL;
}

static inline struct net_pkt *prepare_arp(struct net_if *iface,
					  struct in_addr *next_addr,
					  struct arp_entry *entry,
					  struct net_pkt *pending)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_arp_hdr *hdr;
	struct net_eth_hdr *eth;
	struct in_addr *my_addr;

	pkt = net_pkt_get_reserve_tx(sizeof(struct net_eth_hdr), K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		net_pkt_unref(pkt);
		return NULL;
	}

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET);

	hdr = NET_ARP_HDR(pkt);
	eth = NET_ETH_HDR(pkt);

	/* If entry is not set, then we are just about to send
	 * an ARP request using the data in pending net_pkt.
	 * This can happen if there is already a pending ARP
	 * request and we want to send it again.
	 */
	if (entry) {
		entry->pending = net_pkt_ref(pending);
		entry->iface = net_pkt_iface(pkt);

		net_ipaddr_copy(&entry->ip, next_addr);

		memcpy(&eth->src.addr,
		       net_if_get_link_addr(entry->iface)->addr,
		       sizeof(struct net_eth_addr));
	} else {
		memcpy(&eth->src.addr,
		       net_if_get_link_addr(iface)->addr,
		       sizeof(struct net_eth_addr));
	}

	eth->type = htons(NET_ETH_PTYPE_ARP);
	memset(&eth->dst.addr, 0xff, sizeof(struct net_eth_addr));

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REQUEST);

	memset(&hdr->dst_hwaddr.addr, 0x00, sizeof(struct net_eth_addr));

	net_ipaddr_copy(&hdr->dst_ipaddr, next_addr);

	memcpy(hdr->src_hwaddr.addr, eth->src.addr,
	       sizeof(struct net_eth_addr));

	if (entry) {
		my_addr = if_get_addr(entry->iface);
	} else {
		my_addr = &NET_IPV4_HDR(pending)->src;
	}

	if (my_addr) {
		net_ipaddr_copy(&hdr->src_ipaddr, my_addr);
	} else {
		memset(&hdr->src_ipaddr, 0, sizeof(struct in_addr));
	}

	net_buf_add(frag, sizeof(struct net_arp_hdr));

	return pkt;
}

struct net_pkt *net_arp_prepare(struct net_pkt *pkt)
{
	struct net_buf *frag;
	struct arp_entry *entry, *free_entry = NULL, *non_pending = NULL;
	struct net_linkaddr *ll;
	struct net_eth_hdr *hdr;
	struct in_addr *addr;

	if (!pkt || !pkt->frags) {
		return NULL;
	}

	if (net_pkt_ll_reserve(pkt) != sizeof(struct net_eth_hdr)) {
		/* Add the ethernet header if it is missing. */
		struct net_buf *header;
		struct net_linkaddr *ll;

		net_pkt_set_ll_reserve(pkt, sizeof(struct net_eth_hdr));

		header = net_pkt_get_frag(pkt, K_FOREVER);

		hdr = (struct net_eth_hdr *)(header->data -
					     net_pkt_ll_reserve(pkt));

		hdr->type = htons(NET_ETH_PTYPE_IP);

		ll = net_pkt_ll_dst(pkt);
		if (ll->addr) {
			memcpy(&hdr->dst.addr, ll->addr,
			       sizeof(struct net_eth_addr));
		}

		ll = net_pkt_ll_src(pkt);
		if (ll->addr) {
			memcpy(&hdr->src.addr, ll->addr,
			       sizeof(struct net_eth_addr));
		}

		net_pkt_frag_insert(pkt, header);

		net_pkt_compact(pkt);
	}

	hdr = (struct net_eth_hdr *)net_pkt_ll(pkt);

	/* Is the destination in the local network, if not route via
	 * the gateway address.
	 */
	if (!net_if_ipv4_addr_mask_cmp(net_pkt_iface(pkt),
				       &NET_IPV4_HDR(pkt)->dst)) {
		addr = &net_pkt_iface(pkt)->ipv4.gw;
		if (net_is_ipv4_addr_unspecified(addr)) {
			NET_ERR("Gateway not set for iface %p",
				net_pkt_iface(pkt));

			return NULL;
		}
	} else {
		addr = &NET_IPV4_HDR(pkt)->dst;
	}

	/* If the destination address is already known, we do not need
	 * to send any ARP packet.
	 */
	entry = find_entry(net_pkt_iface(pkt),
			   addr, &free_entry, &non_pending);
	if (!entry) {
		if (!free_entry) {
			/* So all the slots are occupied, use the first
			 * that can be taken.
			 */
			if (!non_pending) {
				/* We cannot send the packet, the ARP
				 * cache is full or there is already a
				 * pending query to this IP address,
				 * so this packet must be discarded.
				 */
				struct net_pkt *req;

				req = prepare_arp(net_pkt_iface(pkt),
						  addr, NULL, pkt);
				NET_DBG("Resending ARP %p", req);

				return req;
			}

			free_entry = non_pending;
		}

		return prepare_arp(net_pkt_iface(pkt), addr, free_entry, pkt);
	}

	ll = net_if_get_link_addr(entry->iface);

	NET_DBG("ARP using ll %s for IP %s",
		net_sprint_ll_addr(ll->addr, sizeof(struct net_eth_addr)),
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src));

	frag = pkt->frags;
	while (frag) {
		/* If there is no room for link layer header, then
		 * just send the packet as is.
		 */
		if (!net_buf_headroom(frag)) {
			frag = frag->frags;
			continue;
		}

		hdr = (struct net_eth_hdr *)(frag->data -
						 net_pkt_ll_reserve(pkt));
		hdr->type = htons(NET_ETH_PTYPE_IP);

		memcpy(&hdr->src.addr, ll->addr,
		       sizeof(struct net_eth_addr));
		memcpy(&hdr->dst.addr, &entry->eth.addr,
		       sizeof(struct net_eth_addr));

		frag = frag->frags;
	}

	return pkt;
}

static inline void send_pending(struct net_if *iface, struct net_pkt **pkt)
{
	struct net_pkt *pending = *pkt;

	NET_DBG("dst %s pending %p frag %p",
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pending)->dst), pending,
		pending->frags);

	*pkt = NULL;

	if (net_if_send_data(iface, pending) == NET_DROP) {
		net_pkt_unref(pending);
	}
}

static inline void arp_update(struct net_if *iface,
			      struct in_addr *src,
			      struct net_eth_addr *hwaddr)
{
	int i;

	NET_DBG("src %s", net_sprint_ipv4_addr(src));

	for (i = 0; i < CONFIG_NET_ARP_TABLE_SIZE; i++) {

		NET_DBG("[%d] iface %p dst %s ll %s pending %p", i, iface,
			net_sprint_ipv4_addr(&arp_table[i].ip),
			net_sprint_ll_addr((u8_t *)&arp_table[i].eth.addr,
					   sizeof(struct net_eth_addr)),
			arp_table[i].pending);

		if (arp_table[i].iface != iface ||
		    !net_ipv4_addr_cmp(&arp_table[i].ip, src)) {
			continue;
		}

		if (arp_table[i].pending) {
			/* We only update the ARP cache if we were
			 * initiating a request.
			 */
			memcpy(&arp_table[i].eth, hwaddr,
			       sizeof(struct net_eth_addr));

			/* Set the dst in the pending packet */
			net_pkt_ll_dst(arp_table[i].pending)->len =
				sizeof(struct net_eth_addr);
			net_pkt_ll_dst(arp_table[i].pending)->addr =
				(u8_t *)
				&NET_ETH_HDR(arp_table[i].pending)->dst.addr;

			send_pending(iface, &arp_table[i].pending);
		}

		return;
	}
}

static inline struct net_pkt *prepare_arp_reply(struct net_if *iface,
						struct net_pkt *req)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_arp_hdr *hdr, *query;
	struct net_eth_hdr *eth, *eth_query;

	pkt = net_pkt_get_reserve_tx(sizeof(struct net_eth_hdr), K_FOREVER);
	if (!pkt) {
		goto fail;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		goto fail;
	}

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_INET);

	hdr = NET_ARP_HDR(pkt);
	eth = NET_ETH_HDR(pkt);
	query = NET_ARP_HDR(req);
	eth_query = NET_ETH_HDR(req);

	eth->type = htons(NET_ETH_PTYPE_ARP);

	memcpy(&eth->dst.addr, &eth_query->src.addr,
	       sizeof(struct net_eth_addr));
	memcpy(&eth->src.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REPLY);

	memcpy(&hdr->dst_hwaddr.addr, &eth_query->src.addr,
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, &eth->src.addr,
	       sizeof(struct net_eth_addr));

	net_ipaddr_copy(&hdr->dst_ipaddr, &query->src_ipaddr);
	net_ipaddr_copy(&hdr->src_ipaddr, &query->dst_ipaddr);

	net_buf_add(frag, sizeof(struct net_arp_hdr));

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

enum net_verdict net_arp_input(struct net_pkt *pkt)
{
	struct net_arp_hdr *arp_hdr;
	struct net_pkt *reply;
	struct in_addr *addr;

	if (net_pkt_get_len(pkt) < (sizeof(struct net_arp_hdr) -
				    net_pkt_ll_reserve(pkt))) {
		NET_DBG("Invalid ARP header (len %zu, min %zu bytes)",
			net_pkt_get_len(pkt),
			sizeof(struct net_arp_hdr) -
			net_pkt_ll_reserve(pkt));
		return NET_DROP;
	}

	arp_hdr = NET_ARP_HDR(pkt);

	switch (ntohs(arp_hdr->opcode)) {
	case NET_ARP_REQUEST:
		/* Someone wants to know our ll address */
		addr = if_get_addr(net_pkt_iface(pkt));
		if (!addr) {
			return NET_DROP;
		}

		if (!net_ipv4_addr_cmp(&arp_hdr->dst_ipaddr, addr)) {
			/* Not for us so drop the packet silently */
			return NET_DROP;
		}

#if defined(CONFIG_NET_DEBUG_ARP)
		do {
			char out[sizeof("xxx.xxx.xxx.xxx")];
			snprintk(out, sizeof(out), "%s",
				 net_sprint_ipv4_addr(&arp_hdr->src_ipaddr));
			NET_DBG("ARP request from %s [%s] for %s",
				out,
				net_sprint_ll_addr(
					(u8_t *)&arp_hdr->src_hwaddr,
					arp_hdr->hwlen),
				net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr));
		} while (0);
#endif /* CONFIG_NET_DEBUG_ARP */

		/* Send reply */
		reply = prepare_arp_reply(net_pkt_iface(pkt), pkt);
		if (reply) {
			net_if_queue_tx(net_pkt_iface(reply), reply);
		}
		break;

	case NET_ARP_REPLY:
		if (net_is_my_ipv4_addr(&arp_hdr->dst_ipaddr)) {
			arp_update(net_pkt_iface(pkt), &arp_hdr->src_ipaddr,
				   &arp_hdr->src_hwaddr);
		}
		break;
	}

	net_pkt_unref(pkt);

	return NET_OK;
}

void net_arp_clear_cache(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_ARP_TABLE_SIZE; i++) {
		if (arp_table[i].pending) {
			net_pkt_unref(arp_table[i].pending);
		}
	}

	memset(&arp_table, 0, sizeof(arp_table));
}

int net_arp_foreach(net_arp_cb_t cb, void *user_data)
{
	int i, ret = 0;

	for (i = 0; i < CONFIG_NET_ARP_TABLE_SIZE; i++) {
		if (!arp_table[i].iface) {
			continue;
		}

		ret++;

		cb(&arp_table[i], user_data);
	}

	return ret;
}

void net_arp_init(void)
{
	net_arp_clear_cache();
}
