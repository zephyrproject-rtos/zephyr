/** @file
 * @brief RPL MRH Objective Function handling.
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

/* Reject parents that have a higher link metric than the following. */
#define MRHOF_MAX_LINK_METRIC			10

/* Reject parents that have a higher path cost than the following. */
#define MRHOF_MAX_PATH_COST			100

/* The rank must differ more than 1/PARENT_SWITCH_THRESHOLD_DIV in order
 * to switch preferred parent.
 */
#define MRHOF_PARENT_SWITCH_THRESHOLD_DIV	2

/* Constants for the ETX moving average */
#define MRHOF_ETX_SCALE   100
#define MRHOF_ETX_ALPHA   90

static u16_t net_rpl_mrhof_get(void)
{
	return 1;
}

u16_t net_rpl_of_get(void) ALIAS_OF(net_rpl_mrhof_get);

static bool net_rpl_mrhof_find(u16_t ocp)
{
	if (ocp != 1) {
		return false;
	}

	return true;
}

bool net_rpl_of_find(u16_t ocp) ALIAS_OF(net_rpl_mrhof_find);

static void net_rpl_mrhof_reset(struct net_rpl_dag *dag)
{
	NET_DBG("Reset MRHOF");
}

void net_rpl_of_reset(struct net_rpl_dag *dag) ALIAS_OF(net_rpl_mrhof_reset);

static int net_rpl_mrhof_neighbor_link_cb(struct net_if *iface,
					  struct net_rpl_parent *parent,
					  int status, int numtx)
{
	u16_t packet_etx = numtx * NET_RPL_MC_ETX_DIVISOR;
	u16_t recorded_etx = 0;
	struct net_ipv6_nbr_data *data;
	u16_t new_etx;

	data = net_rpl_get_ipv6_nbr_data(parent);
	if (!data) {
		/* No neighbor data for this parent,
		 * something bad has occurred.
		 */
		return -ENOENT;
	}

	recorded_etx = data->link_metric;

	/* Do not penalize the ETX when collisions or transmission errors
	 * occur.
	 */
	if (status < 0) {
		/* FIXME - The status values need to be set properly */
		if (status == -EIO) {
			packet_etx = MRHOF_MAX_LINK_METRIC *
				NET_RPL_MC_ETX_DIVISOR;
		}

		if (parent->flags & NET_RPL_PARENT_FLAG_LINK_METRIC_VALID) {
			/* We already have a valid link metric,
			 * use weighted moving average to update it
			 */
			new_etx = ((u32_t)recorded_etx * MRHOF_ETX_ALPHA +
				   (u32_t)packet_etx *
				   (MRHOF_ETX_SCALE - MRHOF_ETX_ALPHA)) /
				MRHOF_ETX_SCALE;
		} else {
			/* We don't have a valid link metric,
			 * set it to the current packet's ETX
			 */
			new_etx = packet_etx;

			/* Set link metric as valid */
			parent->flags |= NET_RPL_PARENT_FLAG_LINK_METRIC_VALID;
		}

		NET_DBG("ETX changed from %d to %d packet ETX %d",
			recorded_etx / NET_RPL_MC_ETX_DIVISOR,
			new_etx  / NET_RPL_MC_ETX_DIVISOR,
			packet_etx / NET_RPL_MC_ETX_DIVISOR);

		/* Update the link metric for this IPv6 nbr */
		data->link_metric = new_etx;
	}

	return 0;
}

int net_rpl_of_neighbor_link_cb(struct net_if *iface,
				struct net_rpl_parent *parent,
				int status, int numtx)
	ALIAS_OF(net_rpl_mrhof_neighbor_link_cb);

static u16_t calculate_path_metric(struct net_rpl_parent *parent)
{
	struct net_ipv6_nbr_data *data;
	struct net_nbr *nbr;

	if (!parent) {
		return MRHOF_MAX_PATH_COST * NET_RPL_MC_ETX_DIVISOR;
	}

	nbr = net_rpl_get_nbr(parent);
	if (!nbr) {
		return MRHOF_MAX_PATH_COST * NET_RPL_MC_ETX_DIVISOR;
	}

	data = net_rpl_get_ipv6_nbr_data(parent);
	if (!data) {
		/* No neighbor data for this parent,
		 * something bad has occurred.
		 */
		return 0;
	}

#if defined(CONFIG_NET_RPL_MC_NONE)
	return parent->rank + data->link_metric;

#elif defined(CONFIG_NET_RPL_MC_ETX)
	return parent->mc.obj.etx + data->link_metric;

#elif defined(CONFIG_NET_RPL_MC_ENERGY)
	return parent->mc.obj.energy.estimation + data->link_metric;
#else
#error "Unsupported routing metric configured"
#endif

	return 0;
}

static struct net_rpl_parent *
net_rpl_mrhof_best_parent(struct net_if *iface,
			  struct net_rpl_parent *parent1,
			  struct net_rpl_parent *parent2)
{
	struct net_rpl_dag *dag;
	u16_t min_diff;
	u16_t p1_metric;
	u16_t p2_metric;

	dag = parent1->dag; /* Both parents are in the same DAG. */

	min_diff = NET_RPL_MC_ETX_DIVISOR /
		MRHOF_PARENT_SWITCH_THRESHOLD_DIV;

	p1_metric = calculate_path_metric(parent1);
	p2_metric = calculate_path_metric(parent2);

	/* Maintain stability of the preferred parent in case of similar
	 * ranks.
	 */
	if (parent1 == dag->preferred_parent ||
	    parent2 == dag->preferred_parent) {
		if (p1_metric < p2_metric + min_diff &&
		    p1_metric > p2_metric - min_diff) {
			NET_DBG("MRHOF hysteresis %u <= %u <= %u",
				p2_metric - min_diff,
				p1_metric,
				p2_metric + min_diff);
			return dag->preferred_parent;
		}
	}

	return p1_metric < p2_metric ? parent1 : parent2;
}

struct net_rpl_parent *net_rpl_of_best_parent(struct net_if *iface,
					      struct net_rpl_parent *parent1,
					      struct net_rpl_parent *parent2)
	ALIAS_OF(net_rpl_mrhof_best_parent);

static struct net_rpl_dag *net_rpl_mrhof_best_dag(struct net_rpl_dag *dag1,
						  struct net_rpl_dag *dag2)
{
	if (net_rpl_dag_is_grounded(dag1) != net_rpl_dag_is_grounded(dag2)) {
		return net_rpl_dag_is_grounded(dag1) ? dag1 : dag2;
	}

	if (net_rpl_dag_get_preference(dag1) !=
	    net_rpl_dag_get_preference(dag2)) {
		return net_rpl_dag_get_preference(dag1) >
			net_rpl_dag_get_preference(dag2) ? dag1 : dag2;
	}

	return dag1->rank < dag2->rank ? dag1 : dag2;
}

struct net_rpl_dag *net_rpl_of_best_dag(struct net_rpl_dag *dag1,
					struct net_rpl_dag *dag2)
	ALIAS_OF(net_rpl_mrhof_best_dag);

static u16_t net_rpl_mrhof_calc_rank(struct net_rpl_parent *parent,
				     u16_t base_rank)
{
	u16_t new_rank;
	u16_t rank_increase = 0;
	struct net_ipv6_nbr_data *data;

	if (!parent) {
		return NET_RPL_INFINITE_RANK;
	}

	data = net_rpl_get_ipv6_nbr_data(parent);
	if (!data) {
		if (base_rank == 0) {
			return NET_RPL_INFINITE_RANK;
		}

		rank_increase = CONFIG_NET_RPL_INIT_LINK_METRIC *
			NET_RPL_MC_ETX_DIVISOR;
	} else {
		rank_increase = data->link_metric;

		if (base_rank == 0) {
			base_rank = parent->rank;
		}
	}

	if (NET_RPL_INFINITE_RANK - base_rank < rank_increase) {
		/* Reached the maximum rank. */
		new_rank = NET_RPL_INFINITE_RANK;
	} else {
		/* Calculate the rank based on the new rank information
		 * from DIO or stored otherwise.
		 */
		new_rank = base_rank + rank_increase;
	}

	return new_rank;
}

u16_t net_rpl_of_calc_rank(struct net_rpl_parent *parent,
			   u16_t base_rank)
	ALIAS_OF(net_rpl_mrhof_calc_rank);

static int net_rpl_mrhof_update_mc(struct net_rpl_instance *instance)
{
#if defined(CONFIG_NET_RPL_MC_NONE)
	instance->mc.type = NET_RPL_MC_NONE;
	return 0;
#else
	u16_t path_metric;
	struct net_rpl_dag *dag;

#if defined(CONFIG_NET_RPL_MC_ENERGY)
	u8_t type;

	instance->mc.type = NET_RPL_MC_ENERGY;
#else
	instance->mc.type = NET_RPL_MC_ETX;
#endif

	instance->mc.flags = NET_RPL_MC_FLAG_P;
	instance->mc.aggregated = NET_RPL_MC_A_ADDITIVE;
	instance->mc.precedence = 0;

	dag = instance->current_dag;

	if (!net_rpl_dag_is_joined(dag)) {
		NET_DBG("Cannot update the metric container when not joined.");
		return -EINVAL;
	}

	if (dag->rank == NET_RPL_ROOT_RANK(instance)) {
		path_metric = 0;
	} else {
		path_metric = calculate_path_metric(dag->preferred_parent);
	}

#if defined(CONFIG_NET_RPL_MC_ETX)
	instance->mc.length = sizeof(instance->mc.obj.etx);
	instance->mc.obj.etx = path_metric;

	NET_DBG("My path ETX to the root is %u.%u\n",
		instance->mc.obj.etx / RPL_DAG_MC_ETX_DIVISOR,
		(instance->mc.obj.etx % RPL_DAG_MC_ETX_DIVISOR * 100) /
		NET_RPL_DAG_MC_ETX_DIVISOR);

#elif defined(CONFIG_NET_RPL_MC_ENERGY)
	instance->mc.length = sizeof(instance->mc.obj.energy);

	if (dag->rank == NET_RPL_ROOT_RANK(instance)) {
		type = NET_RPL_MC_NODE_TYPE_MAINS;
	} else {
		type = NET_RPL_MC_NODE_TYPE_BATTERY;
	}

	instance->mc.obj.energy.flags = type << NET_RPL_MC_ENERGY_TYPE;
	instance->mc.obj.energy.estimation = path_metric;
#endif

	return 0;
#endif /* CONFIG_NET_RPL_MC_NONE */
}

int net_rpl_of_update_mc(struct net_rpl_instance *instance)
	ALIAS_OF(net_rpl_mrhof_update_mc);
