/** @file
 * @brief RPL Zero Objective Function handling.
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(CONFIG_NET_DEBUG_RPL)
#define SYS_LOG_DOMAIN "net/rpl"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <limits.h>
#include <zephyr/types.h>

#include <net/net_core.h>
#include <net/net_stats.h>

#include "net_private.h"
#include "ipv6.h"
#include "icmpv6.h"
#include "nbr.h"
#include "route.h"
#include "rpl.h"

#define DEFAULT_RANK_INCREMENT CONFIG_NET_RPL_MIN_HOP_RANK_INC
#define MIN_DIFFERENCE (CONFIG_NET_RPL_MIN_HOP_RANK_INC + \
			CONFIG_NET_RPL_MIN_HOP_RANK_INC / 2)

static u16_t net_rpl_of0_get(void)
{
	return 0;
}

u16_t net_rpl_of_get(void) ALIAS_OF(net_rpl_of0_get);

static bool net_rpl_of0_find(u16_t ocp)
{
	if (ocp != 0) {
		return false;
	}

	return true;
}

bool net_rpl_of_find(u16_t ocp) ALIAS_OF(net_rpl_of0_find);

static void net_rpl_of0_reset(struct net_rpl_dag *dag)
{
	NET_DBG("Reset OF0");
}

void net_rpl_of_reset(struct net_rpl_dag *dag) ALIAS_OF(net_rpl_of0_reset);

int net_rpl_of0_neighbor_link_cb(struct net_if *iface,
				 struct net_rpl_parent *parent,
				 int status, int numtx)
{
	return 0;
}

int net_rpl_of_neighbor_link_cb(struct net_if *iface,
				struct net_rpl_parent *parent,
				int status, int numtx)
	ALIAS_OF(net_rpl_of0_neighbor_link_cb);

static struct net_rpl_parent *
net_rpl_of0_best_parent(struct net_if *iface,
			struct net_rpl_parent *parent1,
			struct net_rpl_parent *parent2)
{
	u16_t rank1, rank2;
	struct net_rpl_dag *dag;
	struct net_nbr *nbr1, *nbr2;

	nbr1 = net_rpl_get_nbr(parent1);
	nbr2 = net_rpl_get_nbr(parent2);

	dag = (struct net_rpl_dag *)parent1->dag;

	if (!nbr1 || !nbr2) {
		return dag->preferred_parent;
	}

	NET_DBG("Comparing parent %s (confidence %d, rank %d) with "
		"parent %s (confidence %d, rank %d)",
		net_sprint_ipv6_addr(net_rpl_get_parent_addr(iface, parent1)),
		net_ipv6_nbr_data(nbr1)->link_metric,
		parent1->rank,
		net_sprint_ipv6_addr(net_rpl_get_parent_addr(iface, parent2))),
		net_ipv6_nbr_data(nbr2)->link_metric,
		parent2->rank);

	rank1 = NET_RPL_DAG_RANK(parent1->rank,
				 parent1->dag->instance) *
		CONFIG_NET_RPL_MIN_HOP_RANK_INC +
		net_ipv6_nbr_data(nbr1)->link_metric;

	rank2 = NET_RPL_DAG_RANK(parent2->rank,
				 parent1->dag->instance) *
		CONFIG_NET_RPL_MIN_HOP_RANK_INC +
		net_ipv6_nbr_data(nbr2)->link_metric;

	/* Compare two parents by looking both and their rank and at the ETX
	 * for that parent. We choose the parent that has the most
	 * favourable combination.
	 */

	if (rank1 < rank2 + MIN_DIFFERENCE && rank1 > rank2 - MIN_DIFFERENCE) {
		return dag->preferred_parent;
	} else if (rank1 < rank2) {
		return parent1;
	}

	return parent2;
}

struct net_rpl_parent *net_rpl_of_best_parent(struct net_if *iface,
					      struct net_rpl_parent *parent1,
					      struct net_rpl_parent *parent2)
	ALIAS_OF(net_rpl_of0_best_parent);

static struct net_rpl_dag *net_rpl_of0_best_dag(struct net_rpl_dag *dag1,
						struct net_rpl_dag *dag2)
{
	if (net_rpl_dag_is_grounded(dag1)) {
		if (!net_rpl_dag_is_grounded(dag2)) {
			return dag1;
		}
	} else if (net_rpl_dag_is_grounded(dag2)) {
		return dag2;
	}

	if (net_rpl_dag_get_preference(dag1) <
	    net_rpl_dag_get_preference(dag2)) {
		return dag2;
	}

	if (net_rpl_dag_get_preference(dag1) >
	    net_rpl_dag_get_preference(dag2)) {
		return dag1;
	}

	if (dag2->rank < dag1->rank) {
		return dag2;
	}

	return dag1;
}

struct net_rpl_dag *net_rpl_of_best_dag(struct net_rpl_dag *dag1,
					struct net_rpl_dag *dag2)
	ALIAS_OF(net_rpl_of0_best_dag);

static u16_t net_rpl_of0_calc_rank(struct net_rpl_parent *parent,
				   u16_t base_rank)
{
	u16_t increment;

	if (base_rank == 0) {
		if (!parent) {
			return NET_RPL_INFINITE_RANK;
		}

		base_rank = parent->rank;
	}

	increment = parent ? parent->dag->instance->min_hop_rank_inc :
		DEFAULT_RANK_INCREMENT;

	if ((u16_t)(base_rank + increment) < base_rank) {
		NET_DBG("OF0 rank %d incremented to infinite rank due to "
			"wrapping", base_rank);

		return NET_RPL_INFINITE_RANK;
	}

	return base_rank + increment;
}

u16_t net_rpl_of_calc_rank(struct net_rpl_parent *parent,
			   u16_t base_rank)
	ALIAS_OF(net_rpl_of0_calc_rank);

static int net_rpl_of0_update_mc(struct net_rpl_instance *instance)
{
	instance->mc.type = NET_RPL_MC_NONE;

	return 0;
}

int net_rpl_of_update_mc(struct net_rpl_instance *instance)
	ALIAS_OF(net_rpl_of0_update_mc);
