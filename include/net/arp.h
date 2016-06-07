/** @file
 @brief ARP handler

 This is not to be included by the application.
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

#ifndef __ARP_H
#define __ARP_H

#if defined(CONFIG_NET_ARP)

#include <net/ethernet.h>

#define NET_ARP_BUF(buf) ((struct net_arp_hdr *)net_nbuf_ll(buf))

struct net_arp_hdr {
	struct net_eth_hdr eth_hdr;
	uint16_t hwtype;		/* HTYPE */
	uint16_t protocol;		/* PTYPE */
	uint8_t hwlen;			/* HLEN */
	uint8_t protolen;		/* PLEN */
	uint16_t opcode;
	struct net_eth_addr src_hwaddr;	/* SHA */
	struct in_addr src_ipaddr;	/* SPA */
	struct net_eth_addr dst_hwaddr;	/* THA */
	struct in_addr dst_ipaddr;	/* TPA */
}  __packed;

#define NET_ARP_HTYPE_ETH 1

#define NET_ARP_REQUEST 1
#define NET_ARP_REPLY   2

struct net_buf *net_arp_prepare(struct net_buf *buf);
enum net_verdict net_arp_input(struct net_buf *buf);

void net_arp_init(void);

#else /* CONFIG_NET_ARP */

#define net_arp_init(...)

#endif /* CONFIG_NET_ARP */

#endif /* __ARP_H */
