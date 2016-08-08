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

#ifndef __NET_TESTING_H
#define __NET_TESTING_H

#include <contiki/ipv6/uip-ds6-route.h>
#include <contiki/ip/uip.h>
#include <contiki/ipv6/uip-ds6.h>

#if defined(CONFIG_NETWORKING_WITH_15_4)
#define net_testing_server_mac { 0x5e, 0x25, 0xe2, 0xff, 0xfe, 0x15, 0x01, 0x01 }
#define net_testing_client_mac { 0xc1, 0x1e, 0x47, 0xff, 0xfe, 0x15, 0x02, 0x02 }
#else
#define net_testing_server_mac { 0x5e, 0x25, 0xe2, 0x15, 0x01, 0x01 }
#define net_testing_client_mac { 0xc1, 0x1e, 0x47, 0x15, 0x02, 0x02 }
#endif


#if NET_TESTING_SERVER
/* The peer is the client in our case. Just invent a mac
 * address for it because lower parts of the stack cannot set it
 * in this test as we do not have any radios.
 */
static uint8_t net_testing_peer_mac[] = net_testing_client_mac;

/* This is my mac address
 */
static uint8_t net_testing_my_mac[] = net_testing_server_mac;

#else /* NET_TESTING_SERVER */

static uint8_t net_testing_peer_mac[] = net_testing_server_mac;

/* This is my mac address
 */
static uint8_t net_testing_my_mac[] = net_testing_client_mac;
#endif


#if defined(CONFIG_NETWORKING_WITH_IPV6)
#if NET_TESTING_USE_RFC3849_ADDRESSES
/* The 2001:db8::/32 is the private address space for documentation RFC 3849 */
#if defined(NET_TESTING_SERVER)
#define MY_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define PEER_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } }
#else /* NET_TESTING_SERVER */
#define MY_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } }
#define PEER_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#endif /* NET_TESTING_SERVER */
#else /* NET_TESTING_USE_RFC3849_ADDRESSES */
#if NET_TESTING_SERVER
#define MY_IPADDR { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x5c, 0x25, 0xe2, 0xff, 0xfe, 0x15, 0x01, 0x01 } } }
#define PEER_IPADDR { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0xc3, 0x1e, 0x47, 0xff, 0xfe, 0x15, 0x02, 0x02 } } }
#else /* NET_TESTING_SERVER */
#define PEER_IPADDR { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x5c, 0x25, 0xe2, 0xff, 0xfe, 0x15, 0x01, 0x01 } } }
#define MY_IPADDR { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0xc3, 0x1e, 0x47, 0xff, 0xfe, 0x15, 0x02, 0x02 } } }
#endif /* NET_TESTING_SERVER */
#endif /* NET_TESTING_USE_RFC3849_ADDRESSES */

static const struct in6_addr net_testing_in6addr_peer = PEER_IPADDR;
static struct in6_addr net_testing_in6addr_my = MY_IPADDR;

#else /* CONFIG_NETWORKING_WITH_IPV6 */

/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define UIP_IPADDR0 192
#define UIP_IPADDR1 0
#define UIP_IPADDR2 2

#if defined(NET_TESTING_SERVER)
#define UIP_IPADDR3 1
#else
#define UIP_IPADDR3 2
#endif

#define UIP_DRIPADDR0 UIP_IPADDR0
#define UIP_DRIPADDR1 UIP_IPADDR1
#define UIP_DRIPADDR2 UIP_IPADDR2
#define UIP_DRIPADDR3 99

#define MY_IPADDR { { { UIP_IPADDR0, UIP_IPADDR1, UIP_IPADDR2, UIP_IPADDR3 } } }

uip_ipaddr_t uip_hostaddr = { { UIP_IPADDR0, UIP_IPADDR1,
				UIP_IPADDR2, UIP_IPADDR3 } };
uip_ipaddr_t uip_draddr = { { UIP_DRIPADDR0, UIP_DRIPADDR1,
			      UIP_DRIPADDR2, UIP_DRIPADDR3 } };
uip_ipaddr_t uip_netmask = { { 255, 255, 255, 0 } };

#endif /* CONFIG_NETWORKING_WITH_IPV6 */


/* Generic routines for Qemu testing with slip */

static inline void net_testing_setup(void)
{
	net_set_mac(net_testing_my_mac, sizeof(net_testing_my_mac));

#if defined(CONFIG_NETWORKING_WITH_IPV4)
	{
		uip_ipaddr_t addr;
		uip_ipaddr(&addr, 192, 0, 2, 2);
		uip_sethostaddr(&addr);
	}
#endif

#if defined(CONFIG_NETWORKING_WITH_IPV6)
	{
		uip_ipaddr_t *addr;

		/* Set the routes and neighbor cache only if we do not have
		 * neighbor discovery enabled. This setting should only be
		 * used if running in qemu and using slip (tun device).
		 */
		const uip_lladdr_t *lladdr = (const uip_lladdr_t *)&net_testing_peer_mac;

		addr = (uip_ipaddr_t *)&net_testing_in6addr_peer;
		uip_ds6_defrt_add(addr, 0);

		/* We cannot send to peer unless it is in neighbor
		 * cache. Neighbor cache should be populated automatically
		 * but do it here so that test works from first packet.
		 */
		uip_ds6_nbr_add(addr, lladdr, 0, NBR_REACHABLE);

		addr = (uip_ipaddr_t *)&net_testing_in6addr_my;
		uip_ds6_addr_add(addr, 0, ADDR_MANUAL);

		uip_ds6_prefix_add(addr, 64, 0);
	}
#endif
}

static inline void net_testing_set_reply_address(struct net_buf *buf)
{
	/* Set the mac address of the peer in net_buf because
	 * there is no radio layer involved in this test app.
	 * Normally there is no need to do this.
	 */
	memcpy(&ip_buf_ll_src(buf), &net_testing_peer_mac,
	       sizeof(ip_buf_ll_src(buf)));
}

#endif /* __NET_TESTING_H */
