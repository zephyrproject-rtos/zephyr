/** @file
 * @brief ARP related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_IPV4_ARP
#define SYS_LOG_DOMAIN "net/arp"
#define NET_DEBUG 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_stats.h>
#include <net/arp.h>
#include "net_private.h"

struct arp_entry {
	uint32_t time;	/* FIXME - implement timeout functionality */
	struct net_if *iface;
	struct net_buf *pending;
	struct in_addr ip;
	struct net_eth_addr eth;
};

static struct arp_entry arp_table[CONFIG_NET_ARP_TABLE_SIZE];
static const struct net_eth_addr broadcast_eth_addr = {
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

static inline struct arp_entry *find_entry(struct net_if *iface,
					   struct in_addr *dst,
					   struct arp_entry **free_entry,
					   struct arp_entry **non_pending)
{
	int i;

	NET_DBG("dst %s", net_sprint_ipv4_addr(dst));

	for (i = 0; i < CONFIG_NET_ARP_TABLE_SIZE; i++) {

		NET_DBG("[%d] iface %p dst %s pending %p", i, iface,
			net_sprint_ipv4_addr(&arp_table[i].ip),
			arp_table[i].pending);

		if (arp_table[i].iface == iface &&
		    net_ipv4_addr_cmp(&arp_table[i].ip, dst)) {
			/* Is there already pending operation for this
			 * IP address.
			 */
			if (arp_table[i].pending) {
				NET_DBG("ARP already pending to %s",
					net_sprint_ipv4_addr(dst));
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

#define NET_ARP_BUF(buf) ((struct net_arp_hdr *)net_nbuf_ll(buf))

static inline struct net_buf *prepare_arp(struct net_if *iface,
					  struct arp_entry *entry,
					  struct net_buf *pending)
{
	struct net_buf *buf, *frag;
	struct net_arp_hdr *hdr;
	struct in_addr *my_addr;

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		goto fail;
	}

	frag = net_nbuf_get_reserve_data(sizeof(struct net_eth_hdr));
	if (!frag) {
		goto fail;
	}

	net_buf_frag_add(buf, frag);
	net_nbuf_iface(buf) = iface;
	net_nbuf_ll_reserve(buf) = sizeof(struct net_eth_hdr);

	hdr = NET_ARP_BUF(buf);

	entry->pending = net_buf_ref(pending);
	entry->iface = net_nbuf_iface(buf);

	net_ipaddr_copy(&entry->ip, &NET_IPV4_BUF(pending)->dst);

	hdr->eth_hdr.type = htons(NET_ETH_PTYPE_ARP);
	memset(&hdr->eth_hdr.dst.addr, 0xff, sizeof(struct net_eth_addr));
	memcpy(&hdr->eth_hdr.src.addr,
	       net_if_get_link_addr(entry->iface)->addr,
	       sizeof(struct net_eth_addr));

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REQUEST);

	memset(&hdr->dst_hwaddr.addr, 0x00, sizeof(struct net_eth_addr));

	/* Is the destination in local network */
	if (!net_if_ipv4_addr_mask_cmp(iface, &NET_IPV4_BUF(pending)->dst)) {
		net_ipaddr_copy(&hdr->dst_ipaddr, &iface->ipv4.gw);
	} else {
		net_ipaddr_copy(&hdr->dst_ipaddr, &NET_IPV4_BUF(pending)->dst);
	}

	memcpy(hdr->src_hwaddr.addr, hdr->eth_hdr.src.addr,
	       sizeof(struct net_eth_addr));
	my_addr = if_get_addr(entry->iface);
	if (my_addr) {
		net_ipaddr_copy(&hdr->src_ipaddr, my_addr);
	} else {
		memset(&hdr->src_ipaddr, 0, sizeof(struct in_addr));
	}

	net_buf_add(frag, sizeof(struct net_arp_hdr));

	return buf;

fail:
	net_nbuf_unref(buf);
	net_nbuf_unref(pending);
	return NULL;
}

struct net_buf *net_arp_prepare(struct net_buf *buf)
{
	struct net_buf *frag;
	struct arp_entry *entry, *free_entry = NULL, *non_pending = NULL;
	struct net_linkaddr *ll;
	struct net_eth_hdr *hdr;

	if (!buf || !buf->frags) {
		return NULL;
	}

	/* If the destination address is already known, we do not need
	 * to send any ARP packet.
	 */
	if (net_nbuf_ll_reserve(buf) != sizeof(struct net_eth_hdr)) {
		NET_DBG("Ethernet header missing from buf %p, len %d min %d",
			buf, net_nbuf_ll_reserve(buf),
			sizeof(struct net_eth_hdr));
		return NULL;
	}

	hdr = (struct net_eth_hdr *)net_nbuf_ll(buf);

	if (ntohs(hdr->type) == NET_ETH_PTYPE_ARP) {
		NET_DBG("Buf %p is already an ARP msg", buf);
		return buf;
	}

	if (net_ipv4_addr_cmp(&NET_IPV4_BUF(buf)->dst,
			      net_ipv4_broadcast_address())) {
		/* Broadcast address */
		memcpy(&hdr->dst.addr,
		       &broadcast_eth_addr.addr, sizeof(struct net_eth_addr));

		return buf;

	} else if (NET_IPV4_BUF(buf)->dst.s4_addr[0] == 224) {
		/* Multicast address */
		hdr->dst.addr[0] = 0x01;
		hdr->dst.addr[1] = 0x00;
		hdr->dst.addr[2] = 0x5e;
		hdr->dst.addr[3] = NET_IPV4_BUF(buf)->dst.s4_addr[1];
		hdr->dst.addr[4] = NET_IPV4_BUF(buf)->dst.s4_addr[2];
		hdr->dst.addr[5] = NET_IPV4_BUF(buf)->dst.s4_addr[3];

		return buf;

	}

	entry = find_entry(net_nbuf_iface(buf),
			   &NET_IPV4_BUF(buf)->dst,
			   &free_entry, &non_pending);
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
				net_nbuf_unref(buf);
				return NULL;
			}

			free_entry = non_pending;
		}

		return prepare_arp(net_nbuf_iface(buf), free_entry, buf);
	}

	ll = net_if_get_link_addr(entry->iface);

	NET_DBG("ARP using ll %s for IP %s",
		net_sprint_ll_addr(ll->addr, sizeof(struct net_eth_addr)),
		net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src));

	frag = buf->frags;
	while (frag) {
		/* If there is no room for link layer header, then
		 * just send the packet as is.
		 */
		if (!net_buf_headroom(frag)) {
			frag = frag->frags;
			continue;
		}

		hdr = (struct net_eth_hdr *)(frag->data -
						 net_nbuf_ll_reserve(buf));
		hdr->type = htons(NET_ETH_PTYPE_IP);

		memcpy(&hdr->src.addr, ll->addr,
		       sizeof(struct net_eth_addr));
		memcpy(&hdr->dst.addr, &entry->eth.addr,
		       sizeof(struct net_eth_addr));

		frag = frag->frags;
	}

	return buf;
}

static inline void send_pending(struct net_buf **buf)
{
	struct net_buf *pending = *buf;

	NET_DBG("dst %s pending %p",
		net_sprint_ipv4_addr(&NET_IPV4_BUF(pending)->dst), pending);

	*buf = NULL;

	if (net_send_data(pending) < 0) {
		/* This is to unref the original ref */
		net_nbuf_unref(pending);
	}

	/* The pending buf was referenced when
	 * it was added to cache so we need to
	 * unref it now when it is removed from
	 * the cache.
	 */
	net_nbuf_unref(pending);
}

static inline void arp_update(struct net_if *iface,
			      struct in_addr *src,
			      struct net_eth_addr *hwaddr)
{
	int i;

	NET_DBG("src %s", net_sprint_ipv4_addr(src));

	for (i = 0; i < CONFIG_NET_ARP_TABLE_SIZE; i++) {

		NET_DBG("[%d] iface %p dst %s pending %p", i, iface,
			net_sprint_ipv4_addr(&arp_table[i].ip),
			arp_table[i].pending);

		if (arp_table[i].iface == iface &&
		    net_ipv4_addr_cmp(&arp_table[i].ip, src)) {

			if (arp_table[i].pending) {
				/* We only update the ARP cache if we were
				 * initiating a request.
				 */
				memcpy(&arp_table[i].eth, hwaddr,
				       sizeof(struct net_eth_addr));

				send_pending(&arp_table[i].pending);
			}

			return;
		}
	}
}

static inline struct net_buf *prepare_arp_reply(struct net_if *iface,
						struct net_buf *req)
{
	struct net_buf *buf, *frag;
	struct net_arp_hdr *hdr, *query;

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		goto fail;
	}

	frag = net_nbuf_get_reserve_data(sizeof(struct net_eth_hdr));
	if (!frag) {
		goto fail;
	}

	net_buf_frag_add(buf, frag);
	net_nbuf_iface(buf) = iface;
	net_nbuf_ll_reserve(buf) = sizeof(struct net_eth_hdr);

	hdr = NET_ARP_BUF(buf);
	query = NET_ARP_BUF(req);

	hdr->eth_hdr.type = htons(NET_ETH_PTYPE_ARP);

	memcpy(&hdr->eth_hdr.dst.addr, &query->eth_hdr.src.addr,
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->eth_hdr.src.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REPLY);

	memcpy(&hdr->dst_hwaddr.addr, &query->eth_hdr.src.addr,
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, &hdr->eth_hdr.src.addr,
	       sizeof(struct net_eth_addr));

	net_ipaddr_copy(&hdr->dst_ipaddr, &query->src_ipaddr);
	net_ipaddr_copy(&hdr->src_ipaddr, &query->dst_ipaddr);

	net_buf_add(frag, sizeof(struct net_arp_hdr));

	return buf;

fail:
	net_nbuf_unref(buf);
	return NULL;
}

enum net_verdict net_arp_input(struct net_buf *buf)
{
	struct net_arp_hdr *arp_hdr;
	struct net_buf *reply;

	if (net_buf_frags_len(buf) < (sizeof(struct net_arp_hdr) -
				       net_nbuf_ll_reserve(buf))) {
		NET_DBG("Invalid ARP header (len %d, min %d bytes)",
			net_buf_frags_len(buf),
			sizeof(struct net_arp_hdr) - net_nbuf_ll_reserve(buf));
		return NET_DROP;
	}

	arp_hdr = NET_ARP_BUF(buf);

	switch (ntohs(arp_hdr->opcode)) {
	case NET_ARP_REQUEST:
		/* Someone wants to know our ll address */
		if (!net_ipv4_addr_cmp(&arp_hdr->dst_ipaddr,
				       if_get_addr(net_nbuf_iface(buf)))) {
			/* Not for us so drop the packet silently */
			return NET_DROP;
		}

#if NET_DEBUG > 0
		do {
			char out[sizeof("xxx.xxx.xxx.xxx")];
			snprintf(out, sizeof(out),
				 net_sprint_ipv4_addr(&arp_hdr->src_ipaddr));
			NET_DBG("ARP request from %s [%s] for %s",
				out,
				net_sprint_ll_addr(
					(uint8_t *)&arp_hdr->src_hwaddr,
					arp_hdr->hwlen),
				net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr));
		} while (0);
#endif

		/* Send reply */
		reply = prepare_arp_reply(net_nbuf_iface(buf), buf);
		if (reply) {
			if (net_send_data(reply) < 0) {
				net_nbuf_unref(reply);
			}
		}
		break;

	case NET_ARP_REPLY:
		if (net_is_my_ipv4_addr(&arp_hdr->dst_ipaddr)) {
			arp_update(net_nbuf_iface(buf), &arp_hdr->src_ipaddr,
				   &arp_hdr->src_hwaddr);
		}
		break;
	}

	return NET_DROP;
}

void net_arp_init(void)
{
	static bool is_initialized;

	if (is_initialized) {
		return;
	}

	is_initialized = true;

	memset(&arp_table, 0, sizeof(arp_table));
}
