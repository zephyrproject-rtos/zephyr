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

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_IF)
#define SYS_LOG_DOMAIN "net/if"
#define NET_DEBUG 1
#endif

#include <init.h>
#include <nanokernel.h>
#include <sections.h>
#include <string.h>
#include <misc/sys_log.h>
#include <net/net_if.h>
#include <net/net_core.h>

/* net_if dedicated section limiters */
extern struct net_if __net_if_start[];
extern struct net_if __net_if_end[];

/* Stack for the TX fiber */
#ifndef CONFIG_NET_TX_STACK_SIZE
#define CONFIG_NET_TX_STACK_SIZE 1024
#endif
static char __noinit __stack tx_fiber_stack[CONFIG_NET_TX_STACK_SIZE];

static void net_if_tx_fiber(struct net_if *iface)
{
	NET_DBG("Starting TX fiber (stack %d bytes)", sizeof(tx_fiber_stack));

	while (1) {
		struct net_buf *buf;

		/* Get next packet from application - wait if necessary */
		buf = nano_fifo_get(&iface->tx_queue, TICKS_UNLIMITED);

		NET_DBG("Processing (buf %p, len %u) network packet",
			buf, buf->len);

		/* FIXME - Do something with the packet */
	}
}

static inline void init_tx_queue(struct net_if *iface)
{
	nano_fifo_init(&iface->tx_queue);

	fiber_start(tx_fiber_stack, sizeof(tx_fiber_stack),
		    (nano_fiber_entry_t)net_if_tx_fiber, (int)iface, 0, 7, 0);
}

struct net_if *net_if_get_by_link_addr(struct net_linkaddr *ll_addr)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		if (!memcmp(iface->link_addr.addr, ll_addr->addr,
			    ll_addr->len)) {
			return iface;
		}
	}

	return NULL;
}

struct net_if_addr *net_if_ipv6_addr_lookup(struct in6_addr *addr)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		int i;

		for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			if (!iface->ipv6.unicast[i].is_used ||
			    iface->ipv6.unicast[i].address.family != AF_INET6) {
				continue;
			}

			if (net_is_ipv6_prefix(addr->s6_addr,
				iface->ipv6.unicast[i].address.in6_addr.s6_addr,
					       128)) {
				return &iface->ipv6.unicast[i];
			}
		}
	}

	return NULL;
}

struct net_if_addr *net_if_ipv6_addr_add(struct net_if *iface,
					 struct in6_addr *addr,
					 enum net_addr_type addr_type,
					 uint32_t vlifetime)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (iface->ipv6.unicast[i].is_used) {
			continue;
		}

		iface->ipv6.unicast[i].is_used = true;
		iface->ipv6.unicast[i].address.family = AF_INET6;
		iface->ipv6.unicast[i].addr_type = addr_type;
		memcpy(&iface->ipv6.unicast[i].address.in6_addr, addr, 16);

		/* FIXME - set the mcast addr for this node */

		if (vlifetime) {
			iface->ipv6.unicast[i].is_infinite = false;

			/* FIXME - set the timer */
		} else {
			iface->ipv6.unicast[i].is_infinite = true;
		}

		NET_DBG("[%d] interface %p address %s type %s added", i, iface,
			net_sprint_ipv6_addr(addr),
			net_addr_type2str(addr_type));

		return &iface->ipv6.unicast[i];
	}

	return NULL;
}

struct net_if_mcast_addr *net_if_ipv6_maddr_add(struct net_if *iface,
						struct in6_addr *addr)
{
	int i;

	if (!net_is_ipv6_addr_mcast(addr)) {
		NET_DBG("Address %s is not a multicast address.",
			net_sprint_ipv6_addr(addr));
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (iface->ipv6.mcast[i].is_used) {
			continue;
		}

		iface->ipv6.mcast[i].is_used = true;
		memcpy(&iface->ipv6.mcast[i].address.in6_addr, addr, 16);

		NET_DBG("[%d] interface %p address %s added", i, iface,
			net_sprint_ipv6_addr(addr));

		return &iface->ipv6.mcast[i];
	}

	return NULL;
}

struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(struct in6_addr *maddr)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		int i;

		for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
			if (!iface->ipv6.mcast[i].is_used ||
			    iface->ipv6.mcast[i].address.family != AF_INET6) {
				continue;
			}

			if (net_is_ipv6_prefix(maddr->s6_addr,
				iface->ipv6.mcast[i].address.in6_addr.s6_addr,
					       128)) {
				return &iface->ipv6.mcast[i];
			}
		}
	}

	return NULL;
}

struct net_if_addr *net_if_ipv4_addr_lookup(struct in_addr *addr)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		int i;

		for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (!iface->ipv4.unicast[i].is_used ||
			    iface->ipv4.unicast[i].address.family != AF_INET) {
				continue;
			}

			if (addr->s4_addr32[0] ==
			    iface->ipv4.unicast[i].address.in_addr.s_addr[0]) {
				return &iface->ipv4.unicast[i];
			}
		}
	}

	return NULL;
}

struct net_if_addr *net_if_ipv4_addr_add(struct net_if *iface,
					 struct in_addr *addr,
					 enum net_addr_type addr_type,
					 uint32_t vlifetime)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->ipv4.unicast[i].is_used) {
			continue;
		}

		iface->ipv4.unicast[i].is_used = true;
		iface->ipv4.unicast[i].address.family = AF_INET;
		iface->ipv4.unicast[i].address.in_addr.s4_addr32[0] =
						addr->s4_addr32[0];
		iface->ipv4.unicast[i].addr_type = addr_type;

		if (vlifetime) {
			iface->ipv4.unicast[i].is_infinite = false;

			/* FIXME - set the timer */
		} else {
			iface->ipv4.unicast[i].is_infinite = true;
		}

		NET_DBG("[%d] interface %p address %s type %s added", i, iface,
			net_sprint_ipv4_addr(addr),
			net_addr_type2str(addr_type));

		return &iface->ipv4.unicast[i];
	}

	return NULL;
}

int net_if_init(void)
{
	struct net_if_api *api;
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		api = (struct net_if_api *) iface->dev->driver_api;

		if (api && api->init) {
			api->init(iface);
			init_tx_queue(iface);
		}
	}

	return 0;
}
