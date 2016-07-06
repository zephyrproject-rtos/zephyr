/* nbr.c - Neighbor table management */

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

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_IPV6_NBR_CACHE)
#define SYS_LOG_DOMAIN "net/nbr"
#define NET_DEBUG 1
#endif

#include <errno.h>

#include <net/net_core.h>

#include "net_private.h"

#include "nbr.h"

NET_NBR_LLADDR_INIT(net_neighbor_lladdr, CONFIG_NET_IPV6_MAX_NEIGHBORS);

void net_nbr_unref(struct net_nbr *nbr)
{
	NET_DBG("nbr %p ref %u", nbr, nbr->ref - 1);

	if (--nbr->ref) {
		return;
	}

	if (nbr->remove) {
		nbr->remove(nbr);
	}
}

struct net_nbr *net_nbr_ref(struct net_nbr *nbr)
{
	NET_DBG("nbr %p ref %u", nbr, nbr->ref + 1);

	nbr->ref++;

	return nbr;
}

struct net_nbr *net_nbr_get(struct net_nbr_table *table)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = &table->nbr[i];

		if (!nbr->ref) {
			return net_nbr_ref(nbr);
		}
	}

	return NULL;
}

int net_nbr_link(struct net_nbr *nbr, struct net_if *iface,
		 struct net_linkaddr *lladdr)
{
	int i;

	if (nbr->idx != NET_NBR_LLADDR_UNKNOWN) {
		return -EALREADY;
	}

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (net_neighbor_lladdr[i].ref) {
			continue;
		}

		net_neighbor_lladdr[i].ref++;
		nbr->idx = i;

		memcpy(net_neighbor_lladdr[i].lladdr.addr,
		       lladdr->addr, lladdr->len);
		net_neighbor_lladdr[i].lladdr.len = lladdr->len;

		nbr->iface = iface;

		return 0;
	}

	return -ENOENT;
}

int net_nbr_unlink(struct net_nbr *nbr, struct net_linkaddr *lladdr)
{
	if (nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
		return -EALREADY;
	}

	NET_ASSERT(nbr->idx < CONFIG_NET_IPV6_MAX_NEIGHBORS);
	NET_ASSERT(net_neighbor_lladdr[nbr->idx].ref > 0);

	net_neighbor_lladdr[nbr->idx].ref--;

	if (!net_neighbor_lladdr[nbr->idx].ref) {
		memset(net_neighbor_lladdr[nbr->idx].lladdr.addr, 0,
		       sizeof(net_neighbor_lladdr[nbr->idx].lladdr.storage));
	}

	nbr->idx = NET_NBR_LLADDR_UNKNOWN;
	nbr->iface = NULL;

	return 0;
}

struct net_nbr *net_nbr_lookup(struct net_nbr_table *table,
			       struct net_if *iface,
			       struct net_linkaddr *lladdr)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = &table->nbr[i];

		if (nbr->ref && nbr->iface == iface &&
		    net_neighbor_lladdr[nbr->idx].ref &&
		    !memcmp(net_neighbor_lladdr[nbr->idx].lladdr.addr,
			    lladdr->addr, lladdr->len)) {
			return nbr;
		}
	}

	return NULL;
}

struct net_linkaddr_storage *net_nbr_get_lladdr(uint8_t idx)
{
	NET_ASSERT(idx < CONFIG_NET_IPV6_MAX_NEIGHBORS);

	return &net_neighbor_lladdr[idx].lladdr;
}

void net_nbr_clear_table(struct net_nbr_table *table)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = &table->nbr[i];
		struct net_linkaddr lladdr = {
			.addr = net_neighbor_lladdr[i].lladdr.addr,
			.len = net_neighbor_lladdr[i].lladdr.len
		};

		net_nbr_unlink(nbr, &lladdr);
	}

	if (table->clear) {
		table->clear(table);
	}
}

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_IPV6_NBR_CACHE)
void net_nbr_print(struct net_nbr_table *table)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = &table->nbr[i];

		if (!nbr->ref) {
			continue;
		}

		NET_DBG("[%d] ref %d iface %p idx %d ll %s",
			i, nbr->ref, nbr->iface, nbr->idx,
			net_sprint_ll_addr(
				net_neighbor_lladdr[nbr->idx].lladdr.addr,
				net_neighbor_lladdr[nbr->idx].lladdr.len));
	}
}
#endif
