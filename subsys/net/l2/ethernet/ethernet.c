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
#include <net/net_mgmt.h>
#include <net/ethernet.h>
#include <net/ethernet_mgmt.h>
#include <net/gptp.h>
#include <net/lldp.h>

#include "arp.h"
#include "net_private.h"
#include "ipv6.h"
#include "ipv4_autoconf_internal.h"

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
#define print_ll_addrs(pkt, type, len, src, dst)			   \
	do {								   \
		char out[sizeof("xx:xx:xx:xx:xx:xx")];			   \
									   \
		snprintk(out, sizeof(out), "%s",			   \
			 net_sprint_ll_addr((src)->addr,		   \
					    sizeof(struct net_eth_addr))); \
									   \
		NET_DBG("iface %p src %s dst %s type 0x%x len %zu",	   \
			net_pkt_iface(pkt), out,			   \
			net_sprint_ll_addr((dst)->addr,			   \
					   sizeof(struct net_eth_addr)),   \
			type, (size_t)len);				   \
	} while (0)

#define print_vlan_ll_addrs(pkt, type, tci, len, src, dst)		   \
	do {								   \
		char out[sizeof("xx:xx:xx:xx:xx:xx")];			   \
									   \
		snprintk(out, sizeof(out), "%s",			   \
			 net_sprint_ll_addr((src)->addr,		   \
					    sizeof(struct net_eth_addr))); \
									   \
		NET_DBG("iface %p src %s dst %s type 0x%x tag %d pri %d "  \
			"len %zu",					   \
			net_pkt_iface(pkt), out,			   \
			net_sprint_ll_addr((dst)->addr,			   \
					   sizeof(struct net_eth_addr)),   \
			type, net_eth_vlan_get_vid(tci),		   \
			net_eth_vlan_get_pcp(tci), (size_t)len);	   \
	} while (0)
#else
#define print_ll_addrs(...)
#define print_vlan_ll_addrs(...)
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
		len = ntohs(NET_IPV4_HDR(pkt)->len);
	} else {
		len = ntohs(NET_IPV6_HDR(pkt)->len) + NET_IPV6H_LEN;
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
#if defined(CONFIG_NET_VLAN)
	struct net_eth_vlan_hdr *hdr_vlan =
		(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);
	struct ethernet_context *ctx = net_if_l2_data(iface);
	bool vlan_enabled = false;
#endif
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	struct net_linkaddr *lladdr;
	sa_family_t family;
	u16_t type = ntohs(hdr->type);
	u8_t hdr_len = sizeof(struct net_eth_hdr);

#if defined(CONFIG_NET_VLAN)
	if (net_eth_is_vlan_enabled(ctx, iface)) {
		if (type == NET_ETH_PTYPE_VLAN) {
			net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
			type = ntohs(hdr_vlan->type);
			hdr_len = sizeof(struct net_eth_vlan_hdr);
		}

		vlan_enabled = true;
	}
#endif

	switch (type) {
	case NET_ETH_PTYPE_IP:
	case NET_ETH_PTYPE_ARP:
		net_pkt_set_family(pkt, AF_INET);
		family = AF_INET;
		break;
	case NET_ETH_PTYPE_IPV6:
		net_pkt_set_family(pkt, AF_INET6);
		family = AF_INET6;
		break;
#if defined(CONFIG_NET_GPTP)
	case NET_ETH_PTYPE_PTP:
		family = AF_UNSPEC;
		break;
#endif
	case NET_ETH_PTYPE_LLDP:
#if defined(CONFIG_NET_LLDP)
		net_pkt_set_ll_reserve(pkt, hdr_len);
		net_buf_pull(pkt->frags, net_pkt_ll_reserve(pkt));
		return net_lldp_recv(iface, pkt);
#else
		NET_DBG("LLDP Rx agent not enabled");
		return NET_DROP;
#endif
	default:
		NET_DBG("Unknown hdr type 0x%04x iface %p", type, iface);
		return NET_DROP;
	}

	/* Set the pointers to ll src and dst addresses */
	lladdr = net_pkt_lladdr_src(pkt);
	lladdr->addr = ((struct net_eth_hdr *)net_pkt_ll(pkt))->src.addr;
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

	lladdr = net_pkt_lladdr_dst(pkt);
	lladdr->addr = ((struct net_eth_hdr *)net_pkt_ll(pkt))->dst.addr;
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

#if defined(CONFIG_NET_VLAN)
	if (vlan_enabled) {
		print_vlan_ll_addrs(pkt, type, ntohs(hdr_vlan->vlan.tci),
				    net_pkt_get_len(pkt),
				    net_pkt_lladdr_src(pkt),
				    net_pkt_lladdr_dst(pkt));
	} else
#endif
	{
		print_ll_addrs(pkt, type, net_pkt_get_len(pkt),
			       net_pkt_lladdr_src(pkt),
			       net_pkt_lladdr_dst(pkt));
	}

	if (!net_eth_is_addr_broadcast((struct net_eth_addr *)lladdr->addr) &&
	    !net_eth_is_addr_multicast((struct net_eth_addr *)lladdr->addr) &&
	    !net_eth_is_addr_lldp_multicast(
		    (struct net_eth_addr *)lladdr->addr) &&
	    !net_linkaddr_cmp(net_if_get_link_addr(iface), lladdr)) {
		/* The ethernet frame is not for me as the link addresses
		 * are different.
		 */
		NET_DBG("Dropping frame, not for me [%s]",
			net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
					   sizeof(struct net_eth_addr)));

		return NET_DROP;
	}

	net_pkt_set_ll_reserve(pkt, hdr_len);
	net_buf_pull(pkt->frags, net_pkt_ll_reserve(pkt));

#ifdef CONFIG_NET_ARP
	if (family == AF_INET && type == NET_ETH_PTYPE_ARP) {
		NET_DBG("ARP packet from %s received",
			net_sprint_ll_addr((u8_t *)hdr->src.addr,
					   sizeof(struct net_eth_addr)));
#ifdef CONFIG_NET_IPV4_AUTO
		if (net_ipv4_autoconf_input(iface, pkt) == NET_DROP) {
			return NET_DROP;
		}
#endif
		return net_arp_input(pkt);
	}
#endif

#if defined(CONFIG_NET_GPTP)
	if (type == NET_ETH_PTYPE_PTP) {
		return net_gptp_recv(iface, pkt);
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
		net_pkt_lladdr_dst(pkt)->addr = (u8_t *)broadcast_eth_addr.addr;
		net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_lladdr_src(pkt)->addr =
			net_if_get_link_addr(iface)->addr;
		net_pkt_lladdr_src(pkt)->len = sizeof(struct net_eth_addr);

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

		net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_lladdr_src(pkt)->addr =
			net_if_get_link_addr(iface)->addr;
		net_pkt_lladdr_src(pkt)->len = sizeof(struct net_eth_addr);

		return true;
	}

	return false;
}

#if defined(CONFIG_NET_VLAN)
static enum net_verdict set_vlan_tag(struct ethernet_context *ctx,
				     struct net_if *iface,
				     struct net_pkt *pkt)
{
	int i;

	if (net_pkt_vlan_tag(pkt) != NET_VLAN_TAG_UNSPEC) {
		return NET_OK;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		struct net_if *target;

		if (net_if_ipv6_addr_lookup(&NET_IPV6_HDR(pkt)->src,
					    &target)) {
			if (target != iface) {
				NET_DBG("Iface %p should be %p", iface,
					target);

				iface = target;
			}
		}
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		struct net_if *target;

		if (net_if_ipv4_addr_lookup(&NET_IPV4_HDR(pkt)->src,
					    &target)) {
			if (target != iface) {
				NET_DBG("Iface %p should be %p", iface,
					target);
				iface = target;
			}
		}
	}
#endif

	for (i = 0; i < CONFIG_NET_VLAN_COUNT; i++) {
		if (ctx->vlan[i].tag == NET_VLAN_TAG_UNSPEC ||
		    ctx->vlan[i].iface != iface) {
			continue;
		}

		/* Depending on source address, use the proper network
		 * interface when sending.
		 */
		net_pkt_set_vlan_tag(pkt, ctx->vlan[i].tag);

		return NET_OK;
	}

	return NET_DROP;
}

static void set_vlan_priority(struct ethernet_context *ctx,
			      struct net_pkt *pkt)
{
	u8_t vlan_priority;

	vlan_priority = net_priority2vlan(net_pkt_priority(pkt));
	net_pkt_set_vlan_priority(pkt, vlan_priority);
}
#endif /* CONFIG_NET_VLAN */

struct net_eth_hdr *net_eth_fill_header(struct ethernet_context *ctx,
					struct net_pkt *pkt,
					u32_t ptype,
					u8_t *src,
					u8_t *dst)
{
	struct net_eth_hdr *hdr;
	struct net_buf *frag = pkt->frags;

#if defined(CONFIG_NET_VLAN)
	if (net_eth_is_vlan_enabled(ctx, net_pkt_iface(pkt))) {
		struct net_eth_vlan_hdr *hdr_vlan;

		NET_ASSERT(net_buf_headroom(frag) >=
			   sizeof(struct net_eth_vlan_hdr));

		hdr_vlan = (struct net_eth_vlan_hdr *)(frag->data -
						       net_pkt_ll_reserve(pkt));

		if (dst && ((u8_t *)&hdr_vlan->dst != dst)) {
			memcpy(&hdr_vlan->dst, dst,
			       sizeof(struct net_eth_addr));
		}

		if (src && ((u8_t *)&hdr_vlan->src != src)) {
			memcpy(&hdr_vlan->src, src,
			       sizeof(struct net_eth_addr));
		}

		hdr_vlan->type = ptype;
		hdr_vlan->vlan.tpid = htons(NET_ETH_PTYPE_VLAN);
		hdr_vlan->vlan.tci = htons(net_pkt_vlan_tci(pkt));

		print_vlan_ll_addrs(pkt, ntohs(hdr_vlan->type),
				    net_pkt_vlan_tci(pkt),
				    frag->len,
				    &hdr_vlan->src, &hdr_vlan->dst);

		return (struct net_eth_hdr *)hdr_vlan;
	}
#endif

	NET_ASSERT(net_buf_headroom(frag) >= sizeof(struct net_eth_hdr));

	hdr = (struct net_eth_hdr *)(frag->data - net_pkt_ll_reserve(pkt));

	if (dst && ((u8_t *)&hdr->dst != dst)) {
		memcpy(&hdr->dst, dst, sizeof(struct net_eth_addr));
	}

	if (src && ((u8_t *)&hdr->src != src)) {
		memcpy(&hdr->src, src, sizeof(struct net_eth_addr));
	}

	hdr->type = ptype;

	print_ll_addrs(pkt, ntohs(hdr->type), frag->len, &hdr->src, &hdr->dst);

	return hdr;
}

#if defined(CONFIG_NET_IPV4_AUTO)
static inline bool is_ipv4_auto_arp_msg(struct net_pkt *pkt)
{
	return net_pkt_ipv4_auto(pkt);
}
#else
#define is_ipv4_auto_arp_msg(...) false
#endif

static enum net_verdict ethernet_send(struct net_if *iface,
				      struct net_pkt *pkt)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	u16_t ptype;

	/* If this is a LLDP packet, we just send it */
	if (IS_ENABLED(CONFIG_NET_LLDP) &&
	    ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_LLDP) {
		ptype = htons(NET_ETH_PTYPE_LLDP);
		goto send_frame;
	}

#ifdef CONFIG_NET_ARP
	if (net_pkt_family(pkt) == AF_INET) {
		struct net_pkt *arp_pkt;

		if (check_if_dst_is_broadcast_or_mcast(iface, pkt)) {
			if (!net_pkt_lladdr_dst(pkt)->addr) {
				struct net_eth_addr *dst;

				dst = &NET_ETH_HDR(pkt)->dst;
				net_pkt_lladdr_dst(pkt)->addr =
					(u8_t *)dst->addr;
			}

			goto setup_hdr;
		}

		/* Trying to send ARP message so no need to setup it twice */
		if (!is_ipv4_auto_arp_msg(pkt)) {
			arp_pkt = net_arp_prepare(pkt, &NET_IPV4_HDR(pkt)->dst,
						  NULL);
			if (!arp_pkt) {
				return NET_DROP;
			}

			if (pkt != arp_pkt) {
				NET_DBG("Sending arp pkt %p (orig %p) to "
					"iface %p",
					arp_pkt, pkt, iface);

				/* Either pkt went to ARP pending queue or
				 * there was no space in the queue anymore.
				 */
				net_pkt_unref(pkt);

				pkt = arp_pkt;
			} else {
				NET_DBG("Found ARP entry, sending pkt %p to "
					"iface %p",
					pkt, iface);
			}
		}

		net_pkt_lladdr_src(pkt)->addr = (u8_t *)&NET_ETH_HDR(pkt)->src;
		net_pkt_lladdr_src(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_lladdr_dst(pkt)->addr = (u8_t *)&NET_ETH_HDR(pkt)->dst;
		net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);

		/* For ARP message, we do not touch the packet further but will
		 * send it as it is because the arp.c has prepared the packet
		 * already.
		 */
		ptype = htons(NET_ETH_PTYPE_ARP);

		goto send_frame;
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
					net_pkt_lladdr_src(pkt)->addr) ||
	    net_eth_is_addr_multicast((struct net_eth_addr *)
					net_pkt_lladdr_src(pkt)->addr)) {
		net_pkt_lladdr_src(pkt)->addr = net_pkt_lladdr_if(pkt)->addr;
		net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;
	}

	/* If the destination address is not set, then use broadcast
	 * or multicast address.
	 */
	if (!net_pkt_lladdr_dst(pkt)->addr) {
#if defined(CONFIG_NET_IPV6)
		if (net_pkt_family(pkt) == AF_INET6 &&
		    net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
			struct net_eth_addr *dst = &NET_ETH_HDR(pkt)->dst;

			memcpy(dst, (u8_t *)multicast_eth_addr.addr,
			       sizeof(struct net_eth_addr) - 4);
			memcpy((u8_t *)dst + 2,
			       (u8_t *)(&NET_IPV6_HDR(pkt)->dst) + 12,
				sizeof(struct net_eth_addr) - 2);

			net_pkt_lladdr_dst(pkt)->addr = (u8_t *)dst->addr;
		} else
#endif
		{
			net_pkt_lladdr_dst(pkt)->addr =
				(u8_t *)broadcast_eth_addr.addr;
		}

		net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);

		NET_DBG("Destination address was not set, using %s",
			net_sprint_ll_addr(net_pkt_lladdr_dst(pkt)->addr,
					   net_pkt_lladdr_dst(pkt)->len));
	}

setup_hdr:
	__unused;

	if (net_pkt_family(pkt) == AF_INET) {
		ptype = htons(NET_ETH_PTYPE_IP);
	} else {
		ptype = htons(NET_ETH_PTYPE_IPV6);
	}

send_frame:

#if defined(CONFIG_NET_VLAN)
	if (net_eth_is_vlan_enabled(ctx, iface)) {
		if (set_vlan_tag(ctx, iface, pkt) == NET_DROP) {
			return NET_DROP;
		}

		set_vlan_priority(ctx, pkt);
	}
#endif /* CONFIG_NET_VLAN */

	/* Then set the ethernet header. This is not done for ARP as arp.c
	 * has already prepared the message to be sent.
	 */
	if (ptype != htons(NET_ETH_PTYPE_ARP)) {
		net_eth_fill_header(ctx, pkt, ptype,
				    net_pkt_lladdr_src(pkt)->addr,
				    net_pkt_lladdr_dst(pkt)->addr);
	}

	net_if_queue_tx(iface, pkt);

	return NET_OK;
}

static inline u16_t ethernet_reserve(struct net_if *iface, void *unused)
{
#if defined(CONFIG_NET_VLAN)
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (net_eth_is_vlan_enabled(ctx, iface)) {
		return sizeof(struct net_eth_vlan_hdr);
	}
#endif

	ARG_UNUSED(unused);

	return sizeof(struct net_eth_hdr);
}

static inline int ethernet_enable(struct net_if *iface, bool state)
{
	const struct ethernet_api *eth =
		net_if_get_device(iface)->driver_api;

	if (!state) {
		net_arp_clear_cache(iface);

		if (eth->stop) {
			eth->stop(net_if_get_device(iface));
		}
	} else {
		if (eth->start) {
			eth->start(net_if_get_device(iface));
		}
	}

	return 0;
}

enum net_l2_flags ethernet_flags(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	return ctx->ethernet_l2_flags;
}

#if defined(CONFIG_NET_VLAN)
struct net_if *net_eth_get_vlan_iface(struct net_if *iface, u16_t tag)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_if *first_non_vlan_iface = NULL;
	int i;

	for (i = 0; i < CONFIG_NET_VLAN_COUNT; i++) {
		if (ctx->vlan[i].tag == NET_VLAN_TAG_UNSPEC) {
			if (!first_non_vlan_iface) {
				first_non_vlan_iface = ctx->vlan[i].iface;
			}

			continue;
		}

		if (ctx->vlan[i].tag != tag) {
			continue;
		}

		NET_DBG("[%d] vlan tag %d -> iface %p", i, tag,
			ctx->vlan[i].iface);

		return ctx->vlan[i].iface;
	}

	return first_non_vlan_iface;
}

static bool enable_vlan_iface(struct ethernet_context *ctx,
			      struct net_if *iface)
{
	int iface_idx = net_if_get_by_iface(iface);

	if (iface_idx < 0) {
		return false;
	}

	atomic_set_bit(ctx->interfaces, iface_idx);

	return true;
}

static bool disable_vlan_iface(struct ethernet_context *ctx,
			       struct net_if *iface)
{
	int iface_idx = net_if_get_by_iface(iface);

	if (iface_idx < 0) {
		return false;
	}

	atomic_clear_bit(ctx->interfaces, iface_idx);

	return true;
}

static bool is_vlan_enabled_for_iface(struct ethernet_context *ctx,
				      struct net_if *iface)
{
	int iface_idx = net_if_get_by_iface(iface);

	if (iface_idx < 0) {
		return false;
	}

	return !!atomic_test_bit(ctx->interfaces, iface_idx);
}

bool net_eth_is_vlan_enabled(struct ethernet_context *ctx,
			     struct net_if *iface)
{
	if (ctx->vlan_enabled) {
		if (ctx->vlan_enabled == NET_VLAN_MAX_COUNT) {
			/* All network interface are using VLAN, no need
			 * to check further.
			 */
			return true;
		}

		if (is_vlan_enabled_for_iface(ctx, iface)) {
			return true;
		}
	}

	return false;
}

u16_t net_eth_get_vlan_tag(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	int i;

	for (i = 0; i < CONFIG_NET_VLAN_COUNT; i++) {
		if (ctx->vlan[i].iface == iface) {
			return ctx->vlan[i].tag;
		}
	}

	return NET_VLAN_TAG_UNSPEC;
}

bool net_eth_get_vlan_status(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (ctx->vlan_enabled &&
	    net_eth_get_vlan_tag(iface) != NET_VLAN_TAG_UNSPEC) {
		return true;
	}

	return false;
}

static struct ethernet_vlan *get_vlan(struct ethernet_context *ctx,
				      struct net_if *iface,
				      u16_t vlan_tag)
{
	int i;

	for (i = 0; i < CONFIG_NET_VLAN_COUNT; i++) {
		if (ctx->vlan[i].iface == iface &&
		    ctx->vlan[i].tag == vlan_tag) {
			return &ctx->vlan[i];
		}
	}

	return NULL;
}

int net_eth_vlan_enable(struct net_if *iface, u16_t tag)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	const struct ethernet_api *eth =
		net_if_get_device(iface)->driver_api;
	struct ethernet_vlan *vlan;
	int i;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -EPERM;
	}

	if (tag == NET_VLAN_TAG_UNSPEC) {
		return -EBADF;
	}

	vlan = get_vlan(ctx, iface, tag);
	if (vlan) {
		return -EALREADY;
	}

	for (i = 0; i < CONFIG_NET_VLAN_COUNT; i++) {
		if (ctx->vlan[i].iface != iface) {
			continue;
		}

		if (ctx->vlan[i].tag != NET_VLAN_TAG_UNSPEC) {
			continue;
		}

		NET_DBG("[%d] Adding vlan tag %d to iface %p", i, tag, iface);

		ctx->vlan[i].tag = tag;

		enable_vlan_iface(ctx, iface);

		if (eth->vlan_setup) {
			eth->vlan_setup(net_if_get_device(iface),
					iface, tag, true);
		}

		ctx->vlan_enabled++;
		if (ctx->vlan_enabled > NET_VLAN_MAX_COUNT) {
			ctx->vlan_enabled = NET_VLAN_MAX_COUNT;
		}

		ethernet_mgmt_raise_vlan_enabled_event(iface, tag);

		return 0;
	}

	return -ENOSPC;
}

int net_eth_vlan_disable(struct net_if *iface, u16_t tag)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	const struct ethernet_api *eth =
		net_if_get_device(iface)->driver_api;
	struct ethernet_vlan *vlan;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -EINVAL;
	}

	if (tag == NET_VLAN_TAG_UNSPEC) {
		return -EBADF;
	}

	vlan = get_vlan(ctx, iface, tag);
	if (!vlan) {
		return -ESRCH;
	}

	NET_DBG("Removing vlan tag %d from iface %p", vlan->tag, vlan->iface);

	vlan->tag = NET_VLAN_TAG_UNSPEC;

	disable_vlan_iface(ctx, iface);

	if (eth->vlan_setup) {
		eth->vlan_setup(net_if_get_device(iface), iface, tag, false);
	}

	ethernet_mgmt_raise_vlan_disabled_event(iface, tag);

	ctx->vlan_enabled--;
	if (ctx->vlan_enabled < 0) {
		ctx->vlan_enabled = 0;
	}

	return 0;
}
#endif

NET_L2_INIT(ETHERNET_L2, ethernet_recv, ethernet_send, ethernet_reserve,
	    ethernet_enable, ethernet_flags);

static void carrier_on(struct k_work *work)
{
	struct ethernet_context *ctx = CONTAINER_OF(work,
						    struct ethernet_context,
						    carrier_mgmt.work);

	NET_DBG("Carrier ON for interface %p", ctx->carrier_mgmt.iface);

	ethernet_mgmt_raise_carrier_on_event(ctx->carrier_mgmt.iface);

	net_if_up(ctx->carrier_mgmt.iface);
}

static void carrier_off(struct k_work *work)
{
	struct ethernet_context *ctx = CONTAINER_OF(work,
						    struct ethernet_context,
						    carrier_mgmt.work);

	NET_DBG("Carrier OFF for interface %p", ctx->carrier_mgmt.iface);

	ethernet_mgmt_raise_carrier_off_event(ctx->carrier_mgmt.iface);

	net_if_carrier_down(ctx->carrier_mgmt.iface);
}

static void handle_carrier(struct ethernet_context *ctx,
			   struct net_if *iface,
			   k_work_handler_t handler)
{
	k_work_init(&ctx->carrier_mgmt.work, handler);

	ctx->carrier_mgmt.iface = iface;

	k_work_submit(&ctx->carrier_mgmt.work);
}

void net_eth_carrier_on(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	handle_carrier(ctx, iface, carrier_on);
}

void net_eth_carrier_off(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	handle_carrier(ctx, iface, carrier_off);
}

struct device *net_eth_get_ptp_clock(struct net_if *iface)
{
#if defined(CONFIG_PTP_CLOCK)
	struct device *dev = net_if_get_device(iface);
	const struct ethernet_api *api = dev->driver_api;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return NULL;
	}

	if (!(api->get_capabilities(dev) & ETHERNET_PTP)) {
		return NULL;
	}

	return api->get_ptp_clock(net_if_get_device(iface));
#else
	return NULL;
#endif
}

#if defined(CONFIG_NET_GPTP)
int net_eth_get_ptp_port(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	return ctx->port;
}

void net_eth_set_ptp_port(struct net_if *iface, int port)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	ctx->port = port;
}
#endif /* CONFIG_NET_GPTP */

int net_eth_promisc_mode(struct net_if *iface, bool enable)
{
	struct ethernet_req_params params;

	if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
		return -ENOTSUP;
	}

	params.promisc_mode = enable;

	return net_mgmt(NET_REQUEST_ETHERNET_SET_PROMISC_MODE, iface,
			&params, sizeof(struct ethernet_req_params));
}

#if defined(CONFIG_NET_LLDP)
int net_eth_set_lldpdu(struct net_if *iface, const struct net_lldpdu *lldpdu)
{
	return net_lldp_config(iface, lldpdu);
}

void net_eth_unset_lldpdu(struct net_if *iface)
{
	net_lldp_config(iface, NULL);
}
#endif

void ethernet_init(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

#if defined(CONFIG_NET_VLAN)
	int i;
#endif

	NET_DBG("Initializing Ethernet L2 %p for iface %p", ctx, iface);

	ctx->ethernet_l2_flags = NET_L2_MULTICAST;

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE) {
		ctx->ethernet_l2_flags |= NET_L2_PROMISC_MODE;
	}

#if defined(CONFIG_NET_VLAN)
	if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_HW_VLAN)) {
		return;
	}

	for (i = 0; i < CONFIG_NET_VLAN_COUNT; i++) {
		if (!ctx->vlan[i].iface) {
			NET_DBG("[%d] alloc ctx %p iface %p", i, ctx, iface);
			ctx->vlan[i].tag = NET_VLAN_TAG_UNSPEC;
			ctx->vlan[i].iface = iface;

			if (!ctx->is_init) {
				atomic_clear(ctx->interfaces);
			}

			break;
		}
	}
#endif

	net_arp_init();

	ctx->is_init = true;
}
