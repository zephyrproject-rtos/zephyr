/*
 * Copyright (c) 2016 Intel Corporation.
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

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_L2)
#define SYS_LOG_DOMAIN "net/l2"
#define NET_DEBUG 1
#endif

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <net/arp.h>

#include "net_private.h"
#include "ipv6.h"

static const struct net_eth_addr multicast_eth_addr = {
	{ 0x33, 0x33, 0x00, 0x00, 0x00, 0x00 } };

static const struct net_eth_addr broadcast_eth_addr = {
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

const struct net_eth_addr *net_eth_broadcast_addr(void)
{
	return &broadcast_eth_addr;
}

#if NET_DEBUG
#define print_ll_addrs(buf, type, len)					   \
	do {								   \
		char out[sizeof("xx:xx:xx:xx:xx:xx")];			   \
									   \
		snprintf(out, sizeof(out),				   \
			 net_sprint_ll_addr(net_nbuf_ll_src(buf)->addr,    \
					    sizeof(struct net_eth_addr))); \
									   \
		NET_DBG("src %s dst %s type 0x%x len %u", out,		   \
			net_sprint_ll_addr(net_nbuf_ll_dst(buf)->addr,	   \
					   sizeof(struct net_eth_addr)),   \
			type, len);					   \
	} while (0)
#else
#define print_ll_addrs(...)
#endif

static enum net_verdict ethernet_recv(struct net_if *iface,
				      struct net_buf *buf)
{
	struct net_eth_hdr *hdr = NET_ETH_BUF(buf);
	struct net_linkaddr *lladdr;

	switch (ntohs(hdr->type)) {
	case NET_ETH_PTYPE_IP:
	case NET_ETH_PTYPE_ARP:
		net_nbuf_set_family(buf, AF_INET);
		break;
	case NET_ETH_PTYPE_IPV6:
		net_nbuf_set_family(buf, AF_INET6);
		break;
	}

	net_nbuf_set_ll_reserve(buf, sizeof(struct net_eth_hdr));

	/* Set the pointers to ll src and dst addresses */
	lladdr = net_nbuf_ll_src(buf);
	lladdr->addr = ((struct net_eth_hdr *)net_nbuf_ll(buf))->src.addr;
	lladdr->len = sizeof(struct net_eth_hdr);

	lladdr = net_nbuf_ll_dst(buf);
	lladdr->addr = ((struct net_eth_hdr *)net_nbuf_ll(buf))->dst.addr;
	lladdr->len = sizeof(struct net_eth_hdr);

	print_ll_addrs(buf, ntohs(hdr->type), net_buf_frags_len(buf));

#ifdef CONFIG_NET_ARP
	if (net_nbuf_family(buf) == AF_INET &&
	    hdr->type == htons(NET_ETH_PTYPE_ARP)) {
		NET_DBG("ARP packet from %s received",
			net_sprint_ll_addr((uint8_t *)hdr->src.addr,
					   sizeof(struct net_eth_addr)));
		return net_arp_input(buf);
	}
#endif
	return NET_CONTINUE;
}

static enum net_verdict ethernet_send(struct net_if *iface,
				      struct net_buf *buf)
{
	struct net_eth_hdr *hdr = NET_ETH_BUF(buf);
	struct net_buf *frag;
	uint16_t ptype;

#ifdef CONFIG_NET_ARP
	if (net_nbuf_family(buf) == AF_INET) {
		struct net_buf *arp_buf = net_arp_prepare(buf);

		if (!arp_buf) {
			return NET_DROP;
		}

		NET_DBG("Sending arp buf %p (orig %p) to iface %p",
			arp_buf, buf, iface);

		buf = arp_buf;

		net_nbuf_ll_src(buf)->addr = (uint8_t *)&NET_ETH_BUF(buf)->src;
		net_nbuf_ll_src(buf)->len = sizeof(struct net_eth_addr);
		net_nbuf_ll_dst(buf)->addr = (uint8_t *)&NET_ETH_BUF(buf)->dst;
		net_nbuf_ll_dst(buf)->len = sizeof(struct net_eth_addr);

		/* For ARP message, we do not touch the packet further but will
		 * send it as it is because the arp.c has prepared the packet
		 * already.
		 */
		goto send;
	}
#else
	NET_DBG("Sending buf %p to iface %p", buf, iface);
#endif

	/* If the ll address is not set at all, then we must set
	 * it here.
	 */
	if (!net_nbuf_ll_src(buf)->addr) {
		net_nbuf_ll_src(buf)->addr = net_nbuf_ll_if(buf)->addr;
		net_nbuf_ll_src(buf)->len = net_nbuf_ll_if(buf)->len;
	} else {
		/* If the src ll address is multicast or broadcast, then
		 * what probably happened is that the RX buffer is used
		 * for sending data back to recipient. We must
		 * substitute the src address using the real ll address.
		 */
		if (net_eth_is_addr_broadcast((struct net_eth_addr *)
					      net_nbuf_ll_src(buf)->addr) ||
		    net_eth_is_addr_multicast((struct net_eth_addr *)
					      net_nbuf_ll_src(buf)->addr)) {
			net_nbuf_ll_src(buf)->addr = net_nbuf_ll_if(buf)->addr;
			net_nbuf_ll_src(buf)->len = net_nbuf_ll_if(buf)->len;
		}
	}

	/* If the destination address is not set, then use broadcast
	 * or multicast address.
	 */
	if (!net_nbuf_ll_dst(buf)->addr) {
#if defined(CONFIG_NET_IPV6)
		if (net_nbuf_family(buf) == AF_INET6) {
			if (net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
				struct net_eth_addr *dst =
					&NET_ETH_BUF(buf)->dst;

				memcpy(dst, (uint8_t *)multicast_eth_addr.addr,
				       sizeof(struct net_eth_addr) - 4);
				memcpy((uint8_t *)dst + 2,
				       (uint8_t *)(&NET_IPV6_BUF(buf)->dst) +
									12,
				       sizeof(struct net_eth_addr) - 2);

				net_nbuf_ll_dst(buf)->addr =
					(uint8_t *)dst->addr;
			} else {
				/* Check neighbor cache if it has the
				 * destination address.
				 */
				net_nbuf_set_ll_reserve(buf,
						sizeof(struct net_eth_hdr));

				buf = net_ipv6_prepare_for_send(buf);
				if (!buf) {
					/* The actual packet will be send later
					 */
					NET_DBG("Buf will be sent later");
					return NET_CONTINUE;
				}
			}
		} else
#endif
		{
			net_nbuf_ll_dst(buf)->addr =
				(uint8_t *)broadcast_eth_addr.addr;
		}

		net_nbuf_ll_dst(buf)->len = sizeof(struct net_eth_addr);

		NET_DBG("Destination address was not set, using %s",
			net_sprint_ll_addr(net_nbuf_ll_dst(buf)->addr,
					   net_nbuf_ll_dst(buf)->len));
	}

	if (net_nbuf_family(buf) == AF_INET) {
		ptype = htons(NET_ETH_PTYPE_IP);
	} else {
		ptype = htons(NET_ETH_PTYPE_IPV6);
	}

	/* Then go through the fragments and set the ethernet header.
	 */
	frag = buf->frags;

	NET_ASSERT_INFO(frag, "No data!");

	while (frag) {
		hdr = (struct net_eth_hdr *)(frag->data -
					     net_nbuf_ll_reserve(buf));
		memcpy(&hdr->dst, net_nbuf_ll_dst(buf)->addr,
		       sizeof(struct net_eth_addr));
		memcpy(&hdr->src, net_nbuf_ll_src(buf)->addr,
		       sizeof(struct net_eth_addr));
		hdr->type = ptype;
		print_ll_addrs(buf, ntohs(hdr->type), frag->len);

		frag = frag->frags;
	}

#ifdef CONFIG_NET_ARP
send:
#endif /* CONFIG_NET_ARP */

	net_if_queue_tx(iface, buf);

	return NET_OK;
}

static inline uint16_t ethernet_reserve(struct net_if *iface, void *unused)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(unused);

	return sizeof(struct net_eth_hdr);
}

NET_L2_INIT(ETHERNET_L2, ethernet_recv, ethernet_send, ethernet_reserve);
