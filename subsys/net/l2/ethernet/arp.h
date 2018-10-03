/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARP_H
#define __ARP_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_ARP)

#include <misc/slist.h>
#include <net/ethernet.h>

/**
 * @brief Address resolution (ARP) library
 * @defgroup arp ARP Library
 * @ingroup networking
 * @{
 */

#define NET_ARP_HDR(pkt) ((struct net_arp_hdr *)net_pkt_ip_data(pkt))

struct net_arp_hdr {
	u16_t hwtype;		/* HTYPE */
	u16_t protocol;		/* PTYPE */
	u8_t hwlen;			/* HLEN */
	u8_t protolen;		/* PLEN */
	u16_t opcode;
	struct net_eth_addr src_hwaddr;	/* SHA */
	struct in_addr src_ipaddr;	/* SPA */
	struct net_eth_addr dst_hwaddr;	/* THA */
	struct in_addr dst_ipaddr;	/* TPA */
}  __packed;

#define NET_ARP_HTYPE_ETH 1

#define NET_ARP_REQUEST 1
#define NET_ARP_REPLY   2

struct net_pkt *net_arp_prepare(struct net_pkt *pkt,
				struct in_addr *request_ip,
				struct in_addr *current_ip);
enum net_verdict net_arp_input(struct net_pkt *pkt);

struct arp_entry {
	sys_snode_t node;
	s64_t req_start;
	struct net_if *iface;
	struct in_addr ip;
	union {
		struct net_pkt *pending;
		struct net_eth_addr eth;
	};
};

typedef void (*net_arp_cb_t)(struct arp_entry *entry,
			     void *user_data);
int net_arp_foreach(net_arp_cb_t cb, void *user_data);

void net_arp_clear_cache(struct net_if *iface);
void net_arp_init(void);

/**
 * @}
 */

#else /* CONFIG_NET_ARP */

#define net_arp_clear_cache(...)
#define net_arp_init(...)

#endif /* CONFIG_NET_ARP */

#ifdef __cplusplus
}
#endif

#endif /* __ARP_H */
