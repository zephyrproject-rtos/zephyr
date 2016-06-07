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

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/arp.h>

static enum net_verdict ethernet_recv(struct net_if *iface,
				      struct net_buf *buf)
{
	struct net_eth_hdr *hdr = NET_ETH_BUF(buf);
	struct net_linkaddr *lladdr;

	switch (ntohs(hdr->type)) {
	case NET_ETH_PTYPE_IP:
	case NET_ETH_PTYPE_ARP:
		net_nbuf_family(buf) = AF_INET;
		break;
	case NET_ETH_PTYPE_IPV6:
		net_nbuf_family(buf) = AF_INET6;
		break;
	}

	/* Set the pointers to ll src and dst addresses */
	lladdr = net_nbuf_ll_src(buf);
	lladdr->addr = ((struct net_eth_hdr *)net_nbuf_ll(buf))->src.addr;
	lladdr->len = sizeof(struct net_eth_hdr);

	lladdr = net_nbuf_ll_dst(buf);
	lladdr->addr = ((struct net_eth_hdr *)net_nbuf_ll(buf))->dst.addr;
	lladdr->len = sizeof(struct net_eth_hdr);

#ifdef CONFIG_NET_ARP
	if (net_nbuf_family(buf) == AF_INET &&
	    (net_nbuf_ll_reserve(buf) == sizeof(struct net_eth_hdr))) {
		struct net_eth_hdr *eth_hdr =
			(struct net_eth_hdr *) net_nbuf_ll(buf);

		if (ntohs(eth_hdr->type) == NET_ETH_PTYPE_ARP) {
			NET_DBG("ARP packet from %s received",
				net_sprint_ll_addr(
					(uint8_t *)eth_hdr->src.addr,
					sizeof(struct net_eth_addr)));
			return net_arp_input(buf);
		}
	}
#endif
	return NET_CONTINUE;
}

static enum net_verdict ethernet_send(struct net_if *iface,
				      struct net_buf *buf)
{
#ifdef CONFIG_NET_ARP
	if (net_nbuf_family(buf) == AF_INET) {
		struct net_buf *arp_buf = net_arp_prepare(buf);

		if (!arp_buf) {
			return NET_DROP;
		}

		buf = arp_buf;
	}
#endif

	net_if_queue_tx(iface, buf);

	return NET_OK;
}

NET_L2_INIT(ETHERNET_L2, ethernet_recv, ethernet_send);
