/*
 * Copyright (c) 2016-2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ethernet, CONFIG_NET_L2_ETHERNET_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/gptp.h>
#include <zephyr/random/random.h>

#if defined(CONFIG_NET_LLDP)
#include <zephyr/net/lldp.h>
#endif

#include <zephyr/internal/syscall_handler.h>

#include "arp.h"
#include "eth_stats.h"
#include "net_private.h"
#include "ipv6.h"
#include "ipv4.h"
#include "bridge.h"

#define NET_BUF_TIMEOUT K_MSEC(100)

static const struct net_eth_addr multicast_eth_addr __unused = {
	{ 0x33, 0x33, 0x00, 0x00, 0x00, 0x00 } };

static const struct net_eth_addr broadcast_eth_addr = {
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

#if defined(CONFIG_NET_NATIVE_IP) && !defined(CONFIG_NET_RAW_MODE)
static struct net_if_mcast_monitor mcast_monitor;
#endif

const struct net_eth_addr *net_eth_broadcast_addr(void)
{
	return &broadcast_eth_addr;
}

void net_eth_ipv4_mcast_to_mac_addr(const struct in_addr *ipv4_addr,
				    struct net_eth_addr *mac_addr)
{
	/* RFC 1112 6.4. Extensions to an Ethernet Local Network Module
	 * "An IP host group address is mapped to an Ethernet multicast
	 * address by placing the low-order 23-bits of the IP address into
	 * the low-order 23 bits of the Ethernet multicast address
	 * 01-00-5E-00-00-00 (hex)."
	 */
	mac_addr->addr[0] = 0x01;
	mac_addr->addr[1] = 0x00;
	mac_addr->addr[2] = 0x5e;
	mac_addr->addr[3] = ipv4_addr->s4_addr[1];
	mac_addr->addr[4] = ipv4_addr->s4_addr[2];
	mac_addr->addr[5] = ipv4_addr->s4_addr[3];

	mac_addr->addr[3] &= 0x7f;
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

#define print_ll_addrs(pkt, type, len, src, dst)			   \
	if (CONFIG_NET_L2_ETHERNET_LOG_LEVEL >= LOG_LEVEL_DBG) {	   \
		char out[sizeof("xx:xx:xx:xx:xx:xx")];			   \
									   \
		snprintk(out, sizeof(out), "%s",			   \
			 net_sprint_ll_addr((src)->addr,		   \
					    sizeof(struct net_eth_addr))); \
									   \
		NET_DBG("iface %d (%p) src %s dst %s type 0x%x len %zu",   \
			net_if_get_by_iface(net_pkt_iface(pkt)),	   \
			net_pkt_iface(pkt), out,			   \
			net_sprint_ll_addr((dst)->addr,			   \
					    sizeof(struct net_eth_addr)),  \
			type, (size_t)len);				   \
	}

#ifdef CONFIG_NET_VLAN
#define print_vlan_ll_addrs(pkt, type, tci, len, src, dst, tagstrip)       \
	if (CONFIG_NET_L2_ETHERNET_LOG_LEVEL >= LOG_LEVEL_DBG) {	   \
		char out[sizeof("xx:xx:xx:xx:xx:xx")];			   \
									   \
		snprintk(out, sizeof(out), "%s",			   \
			 net_sprint_ll_addr((src)->addr,		   \
					    sizeof(struct net_eth_addr))); \
									   \
		NET_DBG("iface %d (%p) src %s dst %s type 0x%x "	   \
			"tag %d %spri %d len %zu",			   \
			net_if_get_by_iface(net_pkt_iface(pkt)),	   \
			net_pkt_iface(pkt), out,			   \
			net_sprint_ll_addr((dst)->addr,			   \
				   sizeof(struct net_eth_addr)),	   \
			type, net_eth_vlan_get_vid(tci),		   \
			tagstrip ? "(stripped) " : "",			   \
			net_eth_vlan_get_pcp(tci), (size_t)len);	   \
	}
#else
#define print_vlan_ll_addrs(...)
#endif /* CONFIG_NET_VLAN */

static inline void ethernet_update_length(struct net_if *iface,
					  struct net_pkt *pkt)
{
	uint16_t len;

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
				len = 0U;
			}
		}
	}
}

static void ethernet_update_rx_stats(struct net_if *iface,
				     struct net_eth_hdr *hdr, size_t length)
{
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	eth_stats_update_bytes_rx(iface, length);
	eth_stats_update_pkts_rx(iface);

	if (net_eth_is_addr_broadcast(&hdr->dst)) {
		eth_stats_update_broadcast_rx(iface);
	} else if (net_eth_is_addr_multicast(&hdr->dst)) {
		eth_stats_update_multicast_rx(iface);
	}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
}

static inline bool eth_is_vlan_tag_stripped(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct ethernet_api *api = dev->api;

	return (api->get_capabilities(dev) & ETHERNET_HW_VLAN_TAG_STRIP);
}

/* Drop packet if it has broadcast destination MAC address but the IP
 * address is not multicast or broadcast address. See RFC 1122 ch 3.3.6
 */
static inline
enum net_verdict ethernet_check_ipv4_bcast_addr(struct net_pkt *pkt,
						struct net_eth_hdr *hdr)
{
	if (net_eth_is_addr_broadcast(&hdr->dst) &&
	    !(net_ipv4_is_addr_mcast((struct in_addr *)NET_IPV4_HDR(pkt)->dst) ||
	      net_ipv4_is_addr_bcast(net_pkt_iface(pkt),
				     (struct in_addr *)NET_IPV4_HDR(pkt)->dst))) {
		return NET_DROP;
	}

	return NET_OK;
}

#if defined(CONFIG_NET_NATIVE_IP) && !defined(CONFIG_NET_RAW_MODE)
static void ethernet_mcast_monitor_cb(struct net_if *iface, const struct net_addr *addr,
				      bool is_joined)
{
	struct ethernet_config cfg = {
		.filter = {
			.set = is_joined,
			.type = ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS,
		},
	};
	const struct device *dev;
	const struct ethernet_api *api;

	/* Make sure we're an ethernet device */
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	dev = net_if_get_device(iface);
	api = dev->api;

	if (!(api->get_capabilities(dev) & ETHERNET_HW_FILTERING) || api->set_config == NULL) {
		return;
	}

	switch (addr->family) {
#if defined(CONFIG_NET_IPV4)
	case AF_INET:
		net_eth_ipv4_mcast_to_mac_addr(&addr->in_addr, &cfg.filter.mac_address);
		break;
#endif /* CONFIG_NET_IPV4 */
#if defined(CONFIG_NET_IPV6)
	case AF_INET6:
		net_eth_ipv6_mcast_to_mac_addr(&addr->in6_addr, &cfg.filter.mac_address);
		break;
#endif /* CONFIG_NET_IPV6 */
	default:
		return;
	}

	api->set_config(dev, ETHERNET_CONFIG_TYPE_FILTER, &cfg);
}
#endif

static enum net_verdict ethernet_recv(struct net_if *iface,
				      struct net_pkt *pkt)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	uint8_t hdr_len = sizeof(struct net_eth_hdr);
	uint16_t type;
	struct net_linkaddr *lladdr;
	sa_family_t family = AF_UNSPEC;
	bool is_vlan_pkt = false;

	/* This expects that the Ethernet header is in the first net_buf
	 * fragment. This is a safe expectation here as it would not make
	 * any sense to split the Ethernet header to two net_buf's by the
	 * Ethernet driver.
	 */
	if (hdr == NULL || pkt->buffer->len < hdr_len) {
		goto drop;
	}

	if (IS_ENABLED(CONFIG_NET_ETHERNET_BRIDGE) &&
	    net_eth_iface_is_bridged(ctx)) {
		net_pkt_set_l2_bridged(pkt, true);
		net_pkt_lladdr_src(pkt)->addr = hdr->src.addr;
		net_pkt_lladdr_src(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_lladdr_src(pkt)->type = NET_LINK_ETHERNET;
		net_pkt_lladdr_dst(pkt)->addr = hdr->dst.addr;
		net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);
		net_pkt_lladdr_dst(pkt)->type = NET_LINK_ETHERNET;
		ethernet_update_rx_stats(iface, hdr, net_pkt_get_len(pkt));
		return net_eth_bridge_input(ctx, pkt);
	}

	type = ntohs(hdr->type);

	if (IS_ENABLED(CONFIG_NET_VLAN) && type == NET_ETH_PTYPE_VLAN) {
		if (net_eth_is_vlan_enabled(ctx, iface) &&
		    !eth_is_vlan_tag_stripped(iface)) {
			struct net_eth_vlan_hdr *hdr_vlan =
				(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);
			enum net_verdict verdict;

			net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
			type = ntohs(hdr_vlan->type);
			hdr_len = sizeof(struct net_eth_vlan_hdr);
			is_vlan_pkt = true;

			net_pkt_set_iface(pkt,
					  net_eth_get_vlan_iface(iface,
						       net_pkt_vlan_tag(pkt)));

			/* If we receive a packet with a VLAN tag, for that we don't
			 * have a VLAN interface, drop the packet.
			 */
			if (net_if_l2(net_pkt_iface(pkt)) == NULL) {
				goto drop;
			}

			/* We could call VLAN interface directly but then the
			 * interface statistics would not get updated so route
			 * the call via Virtual L2 layer.
			 */
			if (net_if_l2(net_pkt_iface(pkt))->recv != NULL) {
				verdict = net_if_l2(net_pkt_iface(pkt))->recv(iface, pkt);
				if (verdict == NET_DROP) {
					goto drop;
				}
			}
		}
	}

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
	case NET_ETH_PTYPE_EAPOL:
		family = AF_UNSPEC;
		break;
#if defined(CONFIG_NET_L2_PTP)
	case NET_ETH_PTYPE_PTP:
		family = AF_UNSPEC;
		break;
#endif
	case NET_ETH_PTYPE_LLDP:
#if defined(CONFIG_NET_LLDP)
		net_buf_pull(pkt->frags, hdr_len);
		return net_lldp_recv(iface, pkt);
#else
		NET_DBG("LLDP Rx agent not enabled");
		goto drop;
#endif
	default:
		if (IS_ENABLED(CONFIG_NET_ETHERNET_FORWARD_UNRECOGNISED_ETHERTYPE)) {
			family = AF_UNSPEC;
			break;
		}

		NET_DBG("Unknown hdr type 0x%04x iface %d (%p)", type,
			net_if_get_by_iface(iface), iface);
		eth_stats_update_unknown_protocol(iface);
		return NET_DROP;
	}

	/* Set the pointers to ll src and dst addresses */
	lladdr = net_pkt_lladdr_src(pkt);
	lladdr->addr = hdr->src.addr;
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

	lladdr = net_pkt_lladdr_dst(pkt);
	lladdr->addr = hdr->dst.addr;
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

	net_pkt_set_ll_proto_type(pkt, type);

	if (is_vlan_pkt) {
		print_vlan_ll_addrs(pkt, type, net_pkt_vlan_tci(pkt),
				    net_pkt_get_len(pkt),
				    net_pkt_lladdr_src(pkt),
				    net_pkt_lladdr_dst(pkt),
				    eth_is_vlan_tag_stripped(iface));
	} else {
		print_ll_addrs(pkt, type, net_pkt_get_len(pkt),
			       net_pkt_lladdr_src(pkt),
			       net_pkt_lladdr_dst(pkt));
	}

	if (!net_eth_is_addr_broadcast((struct net_eth_addr *)lladdr->addr) &&
	    !net_eth_is_addr_multicast((struct net_eth_addr *)lladdr->addr) &&
	    !net_eth_is_addr_lldp_multicast(
		    (struct net_eth_addr *)lladdr->addr) &&
	    !net_eth_is_addr_ptp_multicast((struct net_eth_addr *)lladdr->addr) &&
	    !net_linkaddr_cmp(net_if_get_link_addr(iface), lladdr)) {
		/* The ethernet frame is not for me as the link addresses
		 * are different.
		 */
		NET_DBG("Dropping frame, not for me [%s]",
			net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
					   sizeof(struct net_eth_addr)));
		goto drop;
	}

	net_buf_pull(pkt->frags, hdr_len);

	if (IS_ENABLED(CONFIG_NET_IPV4) && type == NET_ETH_PTYPE_IP &&
	    ethernet_check_ipv4_bcast_addr(pkt, hdr) == NET_DROP) {
		goto drop;
	}

	ethernet_update_rx_stats(iface, hdr, net_pkt_get_len(pkt) + hdr_len);

	if (IS_ENABLED(CONFIG_NET_ARP) &&
	    family == AF_INET && type == NET_ETH_PTYPE_ARP) {
		NET_DBG("ARP packet from %s received",
			net_sprint_ll_addr((uint8_t *)hdr->src.addr,
					   sizeof(struct net_eth_addr)));

		if (IS_ENABLED(CONFIG_NET_IPV4_ACD) &&
		    net_ipv4_acd_input(iface, pkt) == NET_DROP) {
			return NET_DROP;
		}

		return net_arp_input(pkt, hdr);
	}

	if (IS_ENABLED(CONFIG_NET_GPTP) && type == NET_ETH_PTYPE_PTP) {
		return net_gptp_recv(iface, pkt);
	}

	if (type != NET_ETH_PTYPE_EAPOL) {
		ethernet_update_length(iface, pkt);
	}

	return NET_CONTINUE;
drop:
	eth_stats_update_errors_rx(iface);
	return NET_DROP;
}

#ifdef CONFIG_NET_IPV4
static inline bool ethernet_ipv4_dst_is_broadcast_or_mcast(struct net_pkt *pkt)
{
	if (net_ipv4_is_addr_bcast(net_pkt_iface(pkt),
				   (struct in_addr *)NET_IPV4_HDR(pkt)->dst) ||
	    net_ipv4_is_addr_mcast((struct in_addr *)NET_IPV4_HDR(pkt)->dst)) {
		return true;
	}

	return false;
}

static bool ethernet_fill_in_dst_on_ipv4_mcast(struct net_pkt *pkt,
					       struct net_eth_addr *dst)
{
	if (net_pkt_family(pkt) == AF_INET &&
	    net_ipv4_is_addr_mcast((struct in_addr *)NET_IPV4_HDR(pkt)->dst)) {
		/* Multicast address */
		net_eth_ipv4_mcast_to_mac_addr(
			(struct in_addr *)NET_IPV4_HDR(pkt)->dst, dst);

		return true;
	}

	return false;
}

static struct net_pkt *ethernet_ll_prepare_on_ipv4(struct net_if *iface,
						   struct net_pkt *pkt)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (IS_ENABLED(CONFIG_NET_VLAN) &&
	    net_pkt_vlan_tag(pkt) != NET_VLAN_TAG_UNSPEC &&
	    net_eth_is_vlan_enabled(ctx, net_pkt_iface(pkt))) {
		iface = net_eth_get_vlan_iface(iface,
					       net_pkt_vlan_tag(pkt));
		net_pkt_set_iface(pkt, iface);
	}

	if (ethernet_ipv4_dst_is_broadcast_or_mcast(pkt)) {
		return pkt;
	}

	if (IS_ENABLED(CONFIG_NET_ARP)) {
		struct net_pkt *arp_pkt;

		arp_pkt = net_arp_prepare(pkt, (struct in_addr *)NET_IPV4_HDR(pkt)->dst, NULL);
		if (!arp_pkt) {
			return NULL;
		}

		if (pkt != arp_pkt) {
			NET_DBG("Sending arp pkt %p (orig %p) to iface %d (%p)",
				arp_pkt, pkt, net_if_get_by_iface(iface), iface);
			net_pkt_unref(pkt);
			return arp_pkt;
		}

		NET_DBG("Found ARP entry, sending pkt %p to iface %d (%p)",
			pkt, net_if_get_by_iface(iface), iface);
	}

	return pkt;
}
#else
#define ethernet_ipv4_dst_is_broadcast_or_mcast(...) false
#define ethernet_fill_in_dst_on_ipv4_mcast(...) false
#define ethernet_ll_prepare_on_ipv4(...) NULL
#endif /* CONFIG_NET_IPV4 */

#ifdef CONFIG_NET_IPV6
static bool ethernet_fill_in_dst_on_ipv6_mcast(struct net_pkt *pkt,
					       struct net_eth_addr *dst)
{
	if (net_pkt_family(pkt) == AF_INET6 &&
	    net_ipv6_is_addr_mcast((struct in6_addr *)NET_IPV6_HDR(pkt)->dst)) {
		memcpy(dst, (uint8_t *)multicast_eth_addr.addr,
		       sizeof(struct net_eth_addr) - 4);
		memcpy((uint8_t *)dst + 2,
		       NET_IPV6_HDR(pkt)->dst + 12,
		       sizeof(struct net_eth_addr) - 2);

		return true;
	}

	return false;
}
#else
#define ethernet_fill_in_dst_on_ipv6_mcast(...) false
#endif /* CONFIG_NET_IPV6 */

static struct net_buf *ethernet_fill_header(struct ethernet_context *ctx,
					    struct net_if *iface,
					    struct net_pkt *pkt,
					    uint32_t ptype)
{
	struct net_buf *hdr_frag;
	struct net_eth_hdr *hdr;
	size_t hdr_len = IS_ENABLED(CONFIG_NET_VLAN) ?
			 sizeof(struct net_eth_vlan_hdr) :
			 sizeof(struct net_eth_hdr);

	hdr_frag = net_pkt_get_frag(pkt, hdr_len, NET_BUF_TIMEOUT);
	if (!hdr_frag) {
		return NULL;
	}

	if (IS_ENABLED(CONFIG_NET_VLAN) &&
	    net_eth_is_vlan_enabled(ctx, iface) &&
	    net_pkt_vlan_tag(pkt) != NET_VLAN_TAG_UNSPEC) {
		struct net_eth_vlan_hdr *hdr_vlan;

		hdr_vlan = (struct net_eth_vlan_hdr *)(hdr_frag->data);

		if (ptype == htons(NET_ETH_PTYPE_ARP) ||
		    (!ethernet_fill_in_dst_on_ipv4_mcast(pkt, &hdr_vlan->dst) &&
		     !ethernet_fill_in_dst_on_ipv6_mcast(pkt, &hdr_vlan->dst))) {
			memcpy(&hdr_vlan->dst, net_pkt_lladdr_dst(pkt)->addr,
			       sizeof(struct net_eth_addr));
		}

		memcpy(&hdr_vlan->src, net_pkt_lladdr_src(pkt)->addr,
		       sizeof(struct net_eth_addr));

		hdr_vlan->type = ptype;
		hdr_vlan->vlan.tpid = htons(NET_ETH_PTYPE_VLAN);
		hdr_vlan->vlan.tci = htons(net_pkt_vlan_tci(pkt));
		net_buf_add(hdr_frag, sizeof(struct net_eth_vlan_hdr));

		print_vlan_ll_addrs(pkt, ntohs(hdr_vlan->type),
				    net_pkt_vlan_tci(pkt),
				    hdr_frag->len,
				    &hdr_vlan->src, &hdr_vlan->dst, false);
	} else {
		hdr = (struct net_eth_hdr *)(hdr_frag->data);

		if (ptype == htons(NET_ETH_PTYPE_ARP) ||
		    (!ethernet_fill_in_dst_on_ipv4_mcast(pkt, &hdr->dst) &&
		     !ethernet_fill_in_dst_on_ipv6_mcast(pkt, &hdr->dst))) {
			memcpy(&hdr->dst, net_pkt_lladdr_dst(pkt)->addr,
			       sizeof(struct net_eth_addr));
		}

		memcpy(&hdr->src, net_pkt_lladdr_src(pkt)->addr,
		       sizeof(struct net_eth_addr));

		hdr->type = ptype;
		net_buf_add(hdr_frag, sizeof(struct net_eth_hdr));

		print_ll_addrs(pkt, ntohs(hdr->type),
			       hdr_frag->len, &hdr->src, &hdr->dst);
	}

	net_pkt_frag_insert(pkt, hdr_frag);

	return hdr_frag;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static void ethernet_update_tx_stats(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

	eth_stats_update_bytes_tx(iface, net_pkt_get_len(pkt));
	eth_stats_update_pkts_tx(iface);

	if (net_eth_is_addr_multicast(&hdr->dst)) {
		eth_stats_update_multicast_tx(iface);
	} else if (net_eth_is_addr_broadcast(&hdr->dst)) {
		eth_stats_update_broadcast_tx(iface);
	}
}
#else
#define ethernet_update_tx_stats(...)
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static void ethernet_remove_l2_header(struct net_pkt *pkt)
{
	struct net_buf *buf;

	/* Remove the buffer added in ethernet_fill_header() */
	buf = pkt->buffer;
	pkt->buffer = buf->frags;
	buf->frags = NULL;

	net_pkt_frag_unref(buf);
}

static int ethernet_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct ethernet_api *api = net_if_get_device(iface)->api;
	struct ethernet_context *ctx = net_if_l2_data(iface);
	uint16_t ptype = 0;
	int ret;
	struct net_pkt *orig_pkt = pkt;

	if (!api) {
		ret = -ENOENT;
		goto error;
	}

	if (IS_ENABLED(CONFIG_NET_ETHERNET_BRIDGE) &&
	    net_pkt_is_l2_bridged(pkt)) {
		net_pkt_cursor_init(pkt);
		ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
		if (ret != 0) {
			eth_stats_update_errors_tx(iface);
			goto error;
		}
		ethernet_update_tx_stats(iface, pkt);
		ret = net_pkt_get_len(pkt);
		net_pkt_unref(pkt);
		return ret;
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_pkt_family(pkt) == AF_INET) {
		struct net_pkt *tmp;

		if (net_pkt_ipv4_acd(pkt)) {
			ptype = htons(NET_ETH_PTYPE_ARP);
		} else {
			tmp = ethernet_ll_prepare_on_ipv4(iface, pkt);
			if (!tmp) {
				ret = -ENOMEM;
				goto error;
			} else if (IS_ENABLED(CONFIG_NET_ARP) && tmp != pkt) {
				/* Original pkt got queued and is replaced
				 * by an ARP request packet.
				 */
				pkt = tmp;
				ptype = htons(NET_ETH_PTYPE_ARP);
				net_pkt_set_family(pkt, AF_INET);
			} else {
				ptype = htons(NET_ETH_PTYPE_IP);
			}
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		ptype = htons(NET_ETH_PTYPE_IPV6);
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
		   net_pkt_family(pkt) == AF_PACKET) {
		struct net_context *context = net_pkt_context(pkt);

		if (context && net_context_get_type(context) == SOCK_DGRAM) {
			struct sockaddr_ll *dst_addr;
			struct sockaddr_ll_ptr *src_addr;

			/* The destination address is set in remote for this
			 * socket type.
			 */
			dst_addr = (struct sockaddr_ll *)&context->remote;
			src_addr = (struct sockaddr_ll_ptr *)&context->local;

			net_pkt_lladdr_dst(pkt)->addr = dst_addr->sll_addr;
			net_pkt_lladdr_dst(pkt)->len =
						sizeof(struct net_eth_addr);
			net_pkt_lladdr_src(pkt)->addr = src_addr->sll_addr;
			net_pkt_lladdr_src(pkt)->len =
						sizeof(struct net_eth_addr);
			ptype = dst_addr->sll_protocol;
		} else {
			goto send;
		}
	} else if (IS_ENABLED(CONFIG_NET_L2_PTP) && net_pkt_is_ptp(pkt)) {
		ptype = htons(NET_ETH_PTYPE_PTP);
	} else if (IS_ENABLED(CONFIG_NET_LLDP) && net_pkt_is_lldp(pkt)) {
		ptype = htons(NET_ETH_PTYPE_LLDP);
	} else if (IS_ENABLED(CONFIG_NET_ARP)) {
		/* Unknown type: Unqueued pkt is an ARP reply.
		 */
		ptype = htons(NET_ETH_PTYPE_ARP);
		net_pkt_set_family(pkt, AF_INET);
	} else {
		ret = -ENOTSUP;
		goto error;
	}

	/* If the ll dst addr has not been set before, let's assume
	 * temporarily it's a broadcast one. When filling the header,
	 * it might detect this should be multicast and act accordingly.
	 */
	if (!net_pkt_lladdr_dst(pkt)->addr) {
		net_pkt_lladdr_dst(pkt)->addr = (uint8_t *)broadcast_eth_addr.addr;
		net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);
	}

	/* Then set the ethernet header. Note that the iface parameter tells
	 * where we are actually sending the packet. The interface in net_pkt
	 * is used to determine if the VLAN header is added to Ethernet frame.
	 */
	if (!ethernet_fill_header(ctx, iface, pkt, ptype)) {
		ret = -ENOMEM;
		goto arp_error;
	}

	net_pkt_cursor_init(pkt);

send:
	ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
	if (ret != 0) {
		eth_stats_update_errors_tx(iface);
		ethernet_remove_l2_header(pkt);
		goto arp_error;
	}

	ethernet_update_tx_stats(iface, pkt);

	ret = net_pkt_get_len(pkt);
	ethernet_remove_l2_header(pkt);

	net_pkt_unref(pkt);
error:
	return ret;

arp_error:
	if (IS_ENABLED(CONFIG_NET_ARP) && ptype == htons(NET_ETH_PTYPE_ARP)) {
		/* Original packet was added to ARP's pending Q, so, to avoid it
		 * being freed, take a reference, the reference is dropped when we
		 * clear the pending Q in ARP and then it will be freed by net_if.
		 */
		net_pkt_ref(orig_pkt);
		if (net_arp_clear_pending(
			    iface, (struct in_addr *)NET_IPV4_HDR(pkt)->dst)) {
			NET_DBG("Could not find pending ARP entry");
		}
		/* Free the ARP request */
		net_pkt_unref(pkt);
	}

	return ret;
}

static inline int ethernet_enable(struct net_if *iface, bool state)
{
	int ret = 0;
	const struct ethernet_api *eth =
		net_if_get_device(iface)->api;

	if (!eth) {
		return -ENOENT;
	}

	if (!state) {
		net_arp_clear_cache(iface);

		if (eth->stop) {
			ret = eth->stop(net_if_get_device(iface));
		}
	} else {
		if (eth->start) {
			ret = eth->start(net_if_get_device(iface));
		}
	}

	return ret;
}

enum net_l2_flags ethernet_flags(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	return ctx->ethernet_l2_flags;
}

NET_L2_INIT(ETHERNET_L2, ethernet_recv, ethernet_send, ethernet_enable,
	    ethernet_flags);

static void carrier_on_off(struct k_work *work)
{
	struct ethernet_context *ctx = CONTAINER_OF(work, struct ethernet_context,
						    carrier_work);
	bool eth_carrier_up;

	if (ctx->iface == NULL) {
		return;
	}

	eth_carrier_up = atomic_test_bit(&ctx->flags, ETH_CARRIER_UP);

	if (eth_carrier_up == ctx->is_net_carrier_up) {
		return;
	}

	ctx->is_net_carrier_up = eth_carrier_up;

	NET_DBG("Carrier %s for interface %p", eth_carrier_up ? "ON" : "OFF",
		ctx->iface);

	if (eth_carrier_up) {
		ethernet_mgmt_raise_carrier_on_event(ctx->iface);
		net_if_carrier_on(ctx->iface);
	} else {
		ethernet_mgmt_raise_carrier_off_event(ctx->iface);
		net_if_carrier_off(ctx->iface);
	}
}

void net_eth_carrier_on(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (!atomic_test_and_set_bit(&ctx->flags, ETH_CARRIER_UP)) {
		k_work_submit(&ctx->carrier_work);
	}
}

void net_eth_carrier_off(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	if (atomic_test_and_clear_bit(&ctx->flags, ETH_CARRIER_UP)) {
		k_work_submit(&ctx->carrier_work);
	}
}

#if defined(CONFIG_PTP_CLOCK)
const struct device *net_eth_get_ptp_clock(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct ethernet_api *api = dev->api;

	if (!api) {
		return NULL;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return NULL;
	}

	if (!(api->get_capabilities(dev) & ETHERNET_PTP)) {
		return NULL;
	}

	if (!api->get_ptp_clock) {
		return NULL;
	}

	return api->get_ptp_clock(net_if_get_device(iface));
}
#endif /* CONFIG_PTP_CLOCK */

#if defined(CONFIG_PTP_CLOCK)
const struct device *z_impl_net_eth_get_ptp_clock_by_index(int index)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return NULL;
	}

	return net_eth_get_ptp_clock(iface);
}

#ifdef CONFIG_USERSPACE
static inline const struct device *z_vrfy_net_eth_get_ptp_clock_by_index(int index)
{
	return z_impl_net_eth_get_ptp_clock_by_index(index);
}
#include <zephyr/syscalls/net_eth_get_ptp_clock_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */
#else /* CONFIG_PTP_CLOCK */
const struct device *z_impl_net_eth_get_ptp_clock_by_index(int index)
{
	ARG_UNUSED(index);

	return NULL;
}
#endif /* CONFIG_PTP_CLOCK */

#if defined(CONFIG_NET_L2_PTP)
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
#endif /* CONFIG_NET_L2_PTP */

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
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
#endif/* CONFIG_NET_PROMISCUOUS_MODE */

int net_eth_txinjection_mode(struct net_if *iface, bool enable)
{
	struct ethernet_req_params params;

	if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_TXINJECTION_MODE)) {
		return -ENOTSUP;
	}

	params.txinjection_mode = enable;

	return net_mgmt(NET_REQUEST_ETHERNET_SET_TXINJECTION_MODE, iface,
			&params, sizeof(struct ethernet_req_params));
}

int net_eth_mac_filter(struct net_if *iface, struct net_eth_addr *mac,
		       enum ethernet_filter_type type, bool enable)
{
#ifdef CONFIG_NET_L2_ETHERNET_MGMT
	struct ethernet_req_params params;

	if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_HW_FILTERING)) {
		return -ENOTSUP;
	}

	memcpy(&params.filter.mac_address, mac, sizeof(struct net_eth_addr));
	params.filter.type = type;
	params.filter.set = enable;

	return net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_FILTER, iface, &params,
			sizeof(struct ethernet_req_params));
#else
	ARG_UNUSED(iface);
	ARG_UNUSED(mac);
	ARG_UNUSED(type);
	ARG_UNUSED(enable);

	return -ENOTSUP;
#endif
}

void ethernet_init(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	NET_DBG("Initializing Ethernet L2 %p for iface %d (%p)", ctx,
		net_if_get_by_iface(iface), iface);

	ctx->ethernet_l2_flags = NET_L2_MULTICAST;
	ctx->iface = iface;
	k_work_init(&ctx->carrier_work, carrier_on_off);

	if (net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE) {
		ctx->ethernet_l2_flags |= NET_L2_PROMISC_MODE;
	}

#if defined(CONFIG_NET_NATIVE_IP) && !defined(CONFIG_NET_RAW_MODE)
	if (net_eth_get_hw_capabilities(iface) & ETHERNET_HW_FILTERING) {
		net_if_mcast_mon_register(&mcast_monitor, NULL, ethernet_mcast_monitor_cb);
	}
#endif

	net_arp_init();

	ctx->is_init = true;
}
