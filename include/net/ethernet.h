/** @file
 @brief Ethernet

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ETHERNET_H
#define __ETHERNET_H

#include <stdint.h>
#include <stdbool.h>

#include <net/net_ip.h>
#include <net/nbuf.h>
#include <misc/util.h>

#define NET_ETH_BUF(buf) ((struct net_eth_hdr *)net_nbuf_ll(buf))

#define NET_ETH_PTYPE_ARP		0x0806
#define NET_ETH_PTYPE_IP		0x0800
#define NET_ETH_PTYPE_IPV6		0x86dd

#define NET_ETH_MINIMAL_FRAME_SIZE	60

struct net_eth_addr {
	uint8_t addr[6];
};

struct net_eth_hdr {
	struct net_eth_addr dst;
	struct net_eth_addr src;
	uint16_t type;
} __packed;

static inline bool net_eth_is_addr_broadcast(struct net_eth_addr *addr)
{
	if (addr->addr[0] == 0xff &&
	    addr->addr[1] == 0xff &&
	    addr->addr[2] == 0xff &&
	    addr->addr[3] == 0xff &&
	    addr->addr[4] == 0xff &&
	    addr->addr[5] == 0xff) {
		return true;
	}

	return false;
}

static inline bool net_eth_is_addr_multicast(struct net_eth_addr *addr)
{
	if (addr->addr[0] == 0x33 &&
	    addr->addr[1] == 0x33) {
		return true;
	}

	return false;
}

const struct net_eth_addr *net_eth_broadcast_addr(void);

#endif /* __ETHERNET_H */
