/* nbr.c - Neighbor table management */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_nbr, CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL);

#include <errno.h>

#include <zephyr/net/net_core.h>

#include "net_private.h"

#include "nbr.h"

NET_NBR_LLADDR_INIT(net_neighbor_lladdr, CONFIG_NET_IPV6_MAX_NEIGHBORS);

#if defined(CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL_DBG)
void net_nbr_unref_debug(struct net_nbr *nbr, const char *caller, int line)
#define net_nbr_unref(nbr) net_nbr_unref_debug(nbr, __func__, __LINE__)
#else
void net_nbr_unref(struct net_nbr *nbr)
#endif
{
#if defined(CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL_DBG)
	NET_DBG("nbr %p ref %u (%s():%d)", nbr, nbr->ref - 1, caller, line);
#else
	NET_DBG("nbr %p ref %u", nbr, nbr->ref - 1);
#endif
	if (--nbr->ref) {
		return;
	}

	if (nbr->remove) {
		nbr->remove(nbr);
	}
}

#if defined(CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL_DBG)
struct net_nbr *net_nbr_ref_debug(struct net_nbr *nbr, const char *caller,
				  int line)
#else
struct net_nbr *net_nbr_ref(struct net_nbr *nbr)
#endif
{
#if defined(CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL_DBG)
	NET_DBG("nbr %p ref %u (%s():%d)", nbr, nbr->ref + 1, caller, line);
#else
	NET_DBG("nbr %p ref %u", nbr, nbr->ref + 1);
#endif
	nbr->ref++;

	return nbr;
}

static inline struct net_nbr *get_nbr(struct net_nbr *start, int idx)
{
	NET_ASSERT(idx < CONFIG_NET_IPV6_MAX_NEIGHBORS);

	return (struct net_nbr *)((uint8_t *)start +
			((sizeof(struct net_nbr) + start->size) * idx));
}

struct net_nbr *net_nbr_get(struct net_nbr_table *table)
{
	int i;

	for (i = 0; i < table->nbr_count; i++) {
		struct net_nbr *nbr = get_nbr(table->nbr, i);

		if (!nbr->ref) {
			nbr->data = nbr->__nbr;

			return net_nbr_ref(nbr);
		}
	}

	return NULL;
}

int net_nbr_link(struct net_nbr *nbr, struct net_if *iface,
		 const struct net_linkaddr *lladdr)
{
	int i, avail = -1;

	if (nbr->idx != NET_NBR_LLADDR_UNKNOWN) {
		return -EALREADY;
	}

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		if (avail < 0 && !net_neighbor_lladdr[i].ref) {
			avail = i;
		}

		if (net_neighbor_lladdr[i].ref &&
		    !memcmp(lladdr->addr,
			    net_neighbor_lladdr[i].lladdr.addr,
			    lladdr->len)) {
			/* We found same lladdr in nbr cache so just
			 * increase the ref count.
			 */
			net_neighbor_lladdr[i].ref++;

			nbr->idx = i;
			nbr->iface = iface;

			return 0;
		}
	}

	if (avail < 0) {
		return -ENOENT;
	}

	/* There was no existing entry in the lladdr cache,
	 * so allocate one for this lladdr.
	 */
	net_neighbor_lladdr[avail].ref++;
	nbr->idx = avail;

	net_linkaddr_set(&net_neighbor_lladdr[avail].lladdr,
			 (uint8_t *)lladdr->addr,
			 lladdr->len);
	net_neighbor_lladdr[avail].lladdr.len = lladdr->len;
	net_neighbor_lladdr[avail].lladdr.type = lladdr->type;

	nbr->iface = iface;

	return 0;
}

int net_nbr_unlink(struct net_nbr *nbr, struct net_linkaddr *lladdr)
{
	ARG_UNUSED(lladdr);

	if (nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
		return -EALREADY;
	}

	NET_ASSERT(nbr->idx < CONFIG_NET_IPV6_MAX_NEIGHBORS);
	NET_ASSERT(net_neighbor_lladdr[nbr->idx].ref > 0);

	net_neighbor_lladdr[nbr->idx].ref--;

	if (!net_neighbor_lladdr[nbr->idx].ref) {
		(void)memset(net_neighbor_lladdr[nbr->idx].lladdr.addr, 0,
			     sizeof(net_neighbor_lladdr[nbr->idx].lladdr.addr));
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

	for (i = 0; i < table->nbr_count; i++) {
		struct net_nbr *nbr = get_nbr(table->nbr, i);

		if (nbr->ref && nbr->iface == iface &&
		    net_neighbor_lladdr[nbr->idx].ref &&
		    !memcmp(net_neighbor_lladdr[nbr->idx].lladdr.addr,
			    lladdr->addr, lladdr->len)) {
			return nbr;
		}
	}

	return NULL;
}

struct net_linkaddr *net_nbr_get_lladdr(uint8_t idx)
{
	NET_ASSERT(idx < CONFIG_NET_IPV6_MAX_NEIGHBORS,
		   "idx %d >= max %d", idx,
		   CONFIG_NET_IPV6_MAX_NEIGHBORS);

	return &net_neighbor_lladdr[idx].lladdr;
}

void net_nbr_clear_table(struct net_nbr_table *table)
{
	int i;

	for (i = 0; i < table->nbr_count; i++) {
		struct net_nbr *nbr = get_nbr(table->nbr, i);
		struct net_linkaddr lladdr;

		(void)net_linkaddr_set(&lladdr, net_neighbor_lladdr[i].lladdr.addr,
				       net_neighbor_lladdr[i].lladdr.len);

		net_nbr_unlink(nbr, &lladdr);
	}

	if (table->clear) {
		table->clear(table);
	}
}

void net_nbr_print(struct net_nbr_table *table)
{
	if (CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL >= LOG_LEVEL_DBG) {
		int i;

		for (i = 0; i < table->nbr_count; i++) {
			struct net_nbr *nbr = get_nbr(table->nbr, i);

			if (!nbr->ref) {
				continue;
			}

			NET_DBG("[%d] nbr %p data %p ref %d iface %p idx %d "
				"ll %s",
				i, nbr, nbr->data, nbr->ref, nbr->iface,
				nbr->idx,
				nbr->idx == NET_NBR_LLADDR_UNKNOWN ?
				"<unknown>" :
				net_sprint_ll_addr(
				   net_neighbor_lladdr[nbr->idx].lladdr.addr,
				   net_neighbor_lladdr[nbr->idx].lladdr.len));
		}
	}
}
