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

#if defined(CONFIG_NET_IPV4)

#ifndef __ARP_H
#define __ARP_H

#include <stdint.h>

#include <net/net_ip.h>
#include <net/nbuf.h>

struct net_eth_addr {
	uint8_t addr[6];
};

struct net_eth_hdr {
	struct net_eth_addr dst;
	struct net_eth_addr src;
	uint16_t type;
} __packed;

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

#define NET_ETH_PTYPE_ARP  0x0806
#define NET_ETH_PTYPE_IP   0x0800
#define NET_ETH_PTYPE_IPV6 0x86dd

#define NET_ARP_HTYPE_ETH 1

#define NET_ARP_REQUEST 1
#define NET_ARP_REPLY   2

struct net_buf *net_arp_prepare(struct net_buf *buf);
enum net_verdict net_arp_input(struct net_buf *buf);
void net_arp_init(void);

#endif /* __ICMPV4_H */

#endif /* CONFIG_NET_IPV4 */
