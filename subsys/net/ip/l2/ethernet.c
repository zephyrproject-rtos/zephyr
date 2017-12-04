/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_ETHERNET)
#define SYS_LOG_DOMAIN "net/ethernet"
#define NET_LOG_ENABLED 1
#endif

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <net/arp.h>

#include "net_private.h"
#include "ipv6.h"

#if defined(CONFIG_NET_IPV6)
static const struct net_eth_addr multicast_eth_addr = {
	{ 0x33, 0x33, 0x00, 0x00, 0x00, 0x00 } };
#endif

static const struct net_eth_addr broadcast_eth_addr = {
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

const struct net_eth_addr *net_eth_broadcast_addr(void)
{
	return &broadcast_eth_addr;
}

void net_eth_ipv6_mcast_to_mac_addr(const struct in6_addr *ipv6_addr,
				    struct net_eth_addr *mac_addr)
{
	/* RFC 2464 7. Address Mapping -- Multicast
	 * "An IPv6 packet with a multicast destination address DST,
	 * consisting of the sixteen octets DST[1] through DST[16],
	 * is transmitted to the Ethernet multicast address whose
	 * first two octets are the value 3333 hexadecimal and whose
	 * last four octets are the last four octets of DST."
	 */
	mac_addr->addr[0] = mac_addr->addr[1] = 0x33;
	memcpy(mac_addr->addr + 2, &ipv6_addr->s6_addr[12], 4);
}

#if defined(CONFIG_NET_DEBUG_L2_ETHERNET)
#define print_ll_addrs(pkt, type, len)					   \
	do {								   \
		char out[sizeof("xx:xx:xx:xx:xx:xx")];			   \
									   \
		snprintk(out, sizeof(out), "%s",			   \
			 net_sprint_ll_addr(net_pkt_ll_src(pkt)->addr,    \
					    sizeof(struct net_eth_addr))); \
									   \
		NET_DBG("src %s dst %s type 0x%x len %zu", out,		   \
			net_sprint_ll_addr(net_pkt_ll_dst(pkt)->addr,	   \
					   sizeof(struct net_eth_addr)),   \
			type, (size_t)len);				   \
	} while (0)
#else
#define print_ll_addrs(...)
#endif /* CONFIG_NET_DEBUG_L2_ETHERNET */

static inline void ethernet_update_length(struct net_if *iface,
					  struct net_pkt *pkt)
{
	u16_t len;

	/* Let's check IP payload's length. If it's smaller than 46 bytes,
	 * i.e. smaller than minimal Ethernet frame size minus ethernet
	 * header size,then Ethernet has padded so it fits in the minimal
	 * frame size of 60 bytes. In that case, we need to get rid of it.
	 */

	if (net_pkt_family(pkt) == AF_INET) {
		len = ((NET_IPV4_HDR(pkt)->len[0] << 8) +
		       NET_IPV4_HDR(pkt)->len[1]);
	} else {
		len = ((NET_IPV6_HDR(pkt)->len[0] << 8) +
		       NET_IPV6_HDR(pkt)->len[1]) +
			NET_IPV6H_LEN;
	}

	if (len < NET_ETH_MINIMAL_FRAME_SIZE - sizeof(struct net_eth_hdr)) {
		struct net_buf *frag;

		for (frag = pkt->frags; frag; frag = frag->frags) {
			if (frag->len < len) {
				len -= frag->len;
			} else {
				frag->len = len;
				len = 0;
			}
		}
	}
}

static enum net_verdict ethernet_recv(struct net_if *iface,
				      struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	struct net_linkaddr *lladdr;
	sa_family_t family;

	switch (ntohs(hdr->type)) {
	case NET_ETH_PTYPE_IP:
	case NET_ETH_PTYPE_ARP:
		net_pkt_set_family(pkt, AF_INET);
		family = AF_INET;
		break;
	case NET_ETH_PTYPE_IPV6:
		net_pkt_set_family(pkt, AF_INET6);
		family = AF_INET6;
		break;
	default:
		NET_DBG("Unknown hdr type 0x%04x", hdr->type);
		return NET_DROP;
	}

	/* Set the pointers to ll src and dst addresses */
	lladdr = net_pkt_ll_src(pkt);
	lladdr->addr = ((struct net_eth_hdr *)net_pkt_ll(pkt))->src.addr;
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

	lladdr = net_pkt_ll_dst(pkt);
	lladdr->addr = ((struct net_eth_hdr *)net_pkt_ll(pkt))->dst.addr;
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

	print_ll_addrs(pkt, ntohs(hdr->type), net_pkt_get_len(pkt));

	if (!net_eth_is_addr_broadcast((struct net_eth_addr *)lladdr->addr) &&
	    !net_eth_is_addr_multicast((struct net_eth_addr *)lladdr->addr) &&
	    !net_linkaddr_cmp(net_if_get_link_addr(iface), lladdr)) {
		/* The ethernet frame is not for me as the link addresses
		 * are different.
		 */
		NET_DBG("Dropping frame, not for me [%s]",
			net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
					   sizeof(struct net_eth_addr)));

		return NET_DROP;
	}

	net_pkt_set_ll_reserve(pkt, sizeof(struct net_eth_hdr));
	net_buf_pull(pkt->frags, net_pkt_ll_reserve(pkt));

#ifdef CONFIG_NET_ARP
	if (family == AF_INET && hdr->type == htons(NET_ETH_PTYPE_ARP)) {
		NET_DBG("ARP packet from %s received",
			net_sprint_ll_addr((u8_t *)hdr->src.addr,
					   sizeof(struct net_eth_addr)));
		return net_arp_input(pkt);
	}
#endif
	ethernet_update_length(iface, pkt);

	return NET_CONTINUE;
}

static inline bool check_if_dst_is_broadcast_or_mcast(struct net_if *iface,
						      struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

	if (net_ipv4_addr_cmp(&NET_IPV4_HDR(pkt)->dst,
			      net_ipv4_broadcast_address())) {
		/* Broadcast address */
		net_pkt_ll_dst(pkt)->addr = (u8_t *)broadcast_eth_addr.addr;
		net_pkt_ll_dst(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_ll_src(pkt)->addr = net_if_get_link_addr(iface)->addr;
		net_pkt_ll_src(pkt)->len = sizeof(struct net_eth_addr);

		return true;
	} else if (NET_IPV4_HDR(pkt)->dst.s4_addr[0] == 224) {
		/* Multicast address */
		hdr->dst.addr[0] = 0x01;
		hdr->dst.addr[1] = 0x00;
		hdr->dst.addr[2] = 0x5e;
		hdr->dst.addr[3] = NET_IPV4_HDR(pkt)->dst.s4_addr[1];
		hdr->dst.addr[4] = NET_IPV4_HDR(pkt)->dst.s4_addr[2];
		hdr->dst.addr[5] = NET_IPV4_HDR(pkt)->dst.s4_addr[3];

		hdr->dst.addr[3] = hdr->dst.addr[3] & 0x7f;

		net_pkt_ll_dst(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_ll_src(pkt)->addr = net_if_get_link_addr(iface)->addr;
		net_pkt_ll_src(pkt)->len = sizeof(struct net_eth_addr);

		return true;
	}

	return false;
}

static enum net_verdict ethernet_send(struct net_if *iface,
				      struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	struct net_buf *frag;
	u16_t ptype;

#ifdef CONFIG_NET_ARP
	if (net_pkt_family(pkt) == AF_INET) {
		struct net_pkt *arp_pkt;

		if (check_if_dst_is_broadcast_or_mcast(iface, pkt)) {
			if (!net_pkt_ll_dst(pkt)->addr) {
				struct net_eth_addr *dst;

				dst = &NET_ETH_HDR(pkt)->dst;
				net_pkt_ll_dst(pkt)->addr = (u8_t *)dst->addr;
			}

			goto setup_hdr;
		}

		arp_pkt = net_arp_prepare(pkt);
		if (!arp_pkt) {
			return NET_DROP;
		}

		if (pkt != arp_pkt) {
			NET_DBG("Sending arp pkt %p (orig %p) to iface %p",
				arp_pkt, pkt, iface);

			/* Either pkt went to ARP pending queue
			 * or there was not space in the queue anymore
			 */
			net_pkt_unref(pkt);

			pkt = arp_pkt;
		} else {
			NET_DBG("Found ARP entry, sending pkt %p to iface %p",
				pkt, iface);
		}

		net_pkt_ll_src(pkt)->addr = (u8_t *)&NET_ETH_HDR(pkt)->src;
		net_pkt_ll_src(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_ll_dst(pkt)->addr = (u8_t *)&NET_ETH_HDR(pkt)->dst;
		net_pkt_ll_dst(pkt)->len = sizeof(struct net_eth_addr);

		/* For ARP message, we do not touch the packet further but will
		 * send it as it is because the arp.c has prepared the packet
		 * already.
		 */
		goto send;
	}
#else
	NET_DBG("Sending pkt %p to iface %p", pkt, iface);
#endif

	/* If the src ll address is multicast or broadcast, then
	 * what probably happened is that the RX buffer is used
	 * for sending data back to recipient. We must
	 * substitute the src address using the real ll address.
	 */
	if (net_eth_is_addr_broadcast((struct net_eth_addr *)
					net_pkt_ll_src(pkt)->addr) ||
	    net_eth_is_addr_multicast((struct net_eth_addr *)
					net_pkt_ll_src(pkt)->addr)) {
		net_pkt_ll_src(pkt)->addr = net_pkt_ll_if(pkt)->addr;
		net_pkt_ll_src(pkt)->len = net_pkt_ll_if(pkt)->len;
	}

	/* If the destination address is not set, then use broadcast
	 * or multicast address.
	 */
	if (!net_pkt_ll_dst(pkt)->addr) {
#if defined(CONFIG_NET_IPV6)
		if (net_pkt_family(pkt) == AF_INET6 &&
		    net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
			struct net_eth_addr *dst = &NET_ETH_HDR(pkt)->dst;

			memcpy(dst, (u8_t *)multicast_eth_addr.addr,
			       sizeof(struct net_eth_addr) - 4);
			memcpy((u8_t *)dst + 2,
			       (u8_t *)(&NET_IPV6_HDR(pkt)->dst) + 12,
				sizeof(struct net_eth_addr) - 2);

			net_pkt_ll_dst(pkt)->addr = (u8_t *)dst->addr;
		} else
#endif
		{
			net_pkt_ll_dst(pkt)->addr =
				(u8_t *)broadcast_eth_addr.addr;
		}

		net_pkt_ll_dst(pkt)->len = sizeof(struct net_eth_addr);

		NET_DBG("Destination address was not set, using %s",
			net_sprint_ll_addr(net_pkt_ll_dst(pkt)->addr,
					   net_pkt_ll_dst(pkt)->len));
	}

setup_hdr:
	__unused;

	if (net_pkt_family(pkt) == AF_INET) {
		ptype = htons(NET_ETH_PTYPE_IP);
	} else {
		ptype = htons(NET_ETH_PTYPE_IPV6);
	}

	/* Then go through the fragments and set the ethernet header.
	 */
	frag = pkt->frags;

	NET_ASSERT_INFO(frag, "No data!");

	while (frag) {
		NET_ASSERT(net_buf_headroom(frag) > sizeof(struct net_eth_addr));

		hdr = (struct net_eth_hdr *)(frag->data -
					     net_pkt_ll_reserve(pkt));
		memcpy(&hdr->dst, net_pkt_ll_dst(pkt)->addr,
		       sizeof(struct net_eth_addr));
		memcpy(&hdr->src, net_pkt_ll_src(pkt)->addr,
		       sizeof(struct net_eth_addr));
		hdr->type = ptype;
		print_ll_addrs(pkt, ntohs(hdr->type), frag->len);

		frag = frag->frags;
	}

#ifdef CONFIG_NET_ARP
send:
#endif /* CONFIG_NET_ARP */

	net_if_queue_tx(iface, pkt);

	return NET_OK;
}

static inline u16_t ethernet_reserve(struct net_if *iface, void *unused)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(unused);

	return sizeof(struct net_eth_hdr);
}

static inline int ethernet_enable(struct net_if *iface, bool state)
{
	ARG_UNUSED(iface);

	if (!state) {
		net_arp_clear_cache();
	}

	return 0;
}

NET_L2_INIT(ETHERNET_L2, ethernet_recv, ethernet_send, ethernet_reserve,
	    ethernet_enable);
