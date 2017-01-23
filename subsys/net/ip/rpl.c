/** @file
 * @brief RPL (Ripple, RFC 6550) handling.
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
#include <stdint.h>

#include <net/nbuf.h>
#include <net/net_core.h>

#include "net_private.h"
#include "ipv6.h"
#include "icmpv6.h"
#include "nbr.h"
#include "route.h"
#include "rpl.h"
#include "net_stats.h"

#define NET_RPL_DIO_GROUNDED         0x80
#define NET_RPL_DIO_MOP_SHIFT        3
#define NET_RPL_DIO_MOP_MASK         0x38
#define NET_RPL_DIO_PREFERENCE_MASK  0x07

/* RPL IPv6 extension header option. */
#define NET_RPL_HDR_OPT_LEN		4
#define NET_RPL_HOP_BY_HOP_LEN		(NET_RPL_HDR_OPT_LEN + 2 + 2)
#define NET_RPL_HDR_OPT_DOWN		0x80
#define NET_RPL_HDR_OPT_DOWN_SHIFT	7
#define NET_RPL_HDR_OPT_RANK_ERR	0x40
#define NET_RPL_HDR_OPT_RANK_ERR_SHIFT	6
#define NET_RPL_HDR_OPT_FWD_ERR		0x20
#define NET_RPL_HDR_OPT_FWD_ERR_SHIFT	5

/* Hop By Hop extension headers option type */
#define NET_RPL_EXT_HDR_OPT_RPL		0x63

/* Special value indicating immediate removal. */
#define NET_RPL_ZERO_LIFETIME		0

/* Expire DAOs from neighbors that do not respond in this time. (seconds) */
#define NET_RPL_DAO_EXPIRATION_TIMEOUT      60

#if defined(CONFIG_NET_RPL_MOP3)
#define NET_RPL_MOP_DEFAULT		NET_RPL_MOP_STORING_MULTICAST
#else
#define NET_RPL_MOP_DEFAULT		NET_RPL_MOP_STORING_NO_MULTICAST
#endif

#if defined(CONFIG_NET_RPL_MOP3)
#define NET_RPL_MULTICAST 1
#else
#define NET_RPL_MULTICAST 0
#endif

#if defined(CONFIG_NET_RPL_GROUNDED)
#define NET_RPL_GROUNDED true
#else
#define NET_RPL_GROUNDED false
#endif

#define NET_RPL_PARENT_FLAG_UPDATED           0x1
#define NET_RPL_PARENT_FLAG_LINK_METRIC_VALID 0x2

static struct net_rpl_instance rpl_instances[CONFIG_NET_RPL_MAX_INSTANCES];
static struct net_rpl_instance *rpl_default_instance;
static enum net_rpl_mode rpl_mode = NET_RPL_MODE_MESH;
static net_rpl_join_callback_t rpl_join_callback;
static uint8_t rpl_dao_sequence;

#if defined(CONFIG_NET_RPL_DIS_SEND)
/* DODAG Information Solicitation timer. */
struct k_delayed_work dis_timer;

#define NET_RPL_DIS_START_DELAY  5 /* in seconds */
#endif

/* This is set to true if we are able and ready to send any DIOs */
static bool rpl_dio_send_ok;

static int dao_send(struct net_rpl_parent *parent, uint8_t lifetime,
		    struct net_if *iface);
static void dao_send_timer(struct k_work *work);
static void set_dao_lifetime_timer(struct net_rpl_instance *instance);

static struct net_rpl_parent *find_parent(struct net_if *iface,
					  struct net_rpl_dag *dag,
					  struct in6_addr *addr);
static void remove_parents(struct net_if *iface,
			   struct net_rpl_dag *dag,
			   uint16_t minimum_rank);

static void net_rpl_schedule_dao_now(struct net_rpl_instance *instance);

void net_rpl_set_join_callback(net_rpl_join_callback_t cb)
{
	rpl_join_callback = cb;
}

enum net_rpl_mode net_rpl_get_mode(void)
{
	return rpl_mode;
}

static inline void net_rpl_cancel_dao(struct net_rpl_instance *instance)
{
	k_delayed_work_cancel(&instance->dao_timer);
}

void net_rpl_set_mode(enum net_rpl_mode new_mode)
{
	NET_ASSERT_INFO(new_mode >= NET_RPL_MODE_MESH &&
			new_mode <= NET_RPL_MODE_LEAF,
			"Invalid RPL mode %d", new_mode);

	rpl_mode = new_mode;

	/* We need to do different things depending on what mode we are
	 * switching to.
	 */
	if (rpl_mode == NET_RPL_MODE_MESH) {

		/* If we switch to mesh mode, we should send out a DAO message
		 * to inform our parent that we now are reachable. Before we do
		 * this,  we must set the mode variable, since DAOs will not be
		 * sent if we are in feather mode.
		 */
		NET_DBG("Switching to mesh mode");

		if (rpl_default_instance) {
			net_rpl_schedule_dao_now(rpl_default_instance);
		}

	} else if (rpl_mode == NET_RPL_MODE_FEATHER) {

		NET_DBG("Switching to feather mode");

		if (rpl_default_instance) {
			net_rpl_cancel_dao(rpl_default_instance);
		}
	}
}

static inline uint32_t net_rpl_lifetime(struct net_rpl_instance *instance,
					uint8_t lifetime)
{
	return (uint32_t)instance->lifetime_unit * (uint32_t)lifetime;
}

static void net_rpl_neighbor_data_remove(struct net_nbr *nbr)
{
	NET_DBG("Neighbor %p removed", nbr);
}

static void net_rpl_neighbor_table_clear(struct net_nbr_table *table)
{
	NET_DBG("Neighbor table %p cleared", table);
}

NET_NBR_POOL_INIT(net_rpl_neighbor_pool, CONFIG_NET_IPV6_MAX_NEIGHBORS,
		  sizeof(struct net_rpl_parent),
		  net_rpl_neighbor_data_remove, 0);

NET_NBR_TABLE_INIT(NET_NBR_LOCAL, rpl_parents, net_rpl_neighbor_pool,
		   net_rpl_neighbor_table_clear);

#if defined(CONFIG_NET_DEBUG_RPL)
#define net_rpl_info(buf, req)						     \
	do {								     \
		char out[NET_IPV6_ADDR_LEN];				     \
									     \
		snprintk(out, sizeof(out), "%s",			     \
			 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));     \
		NET_DBG("Received %s from %s to %s", req,		     \
			net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out); \
	} while (0)

#define net_rpl_dao_info(buf, src, dst, prefix)				  \
	do {								  \
		char out[NET_IPV6_ADDR_LEN];				  \
		char prf[NET_IPV6_ADDR_LEN];				  \
									  \
		snprintk(out, sizeof(out), "%s",			  \
			 net_sprint_ipv6_addr(dst));			  \
		snprintk(prf, sizeof(prf), "%s",			  \
			 net_sprint_ipv6_addr(prefix));			  \
		NET_DBG("Send DAO with prefix %s from %s to %s",	  \
			prf, net_sprint_ipv6_addr(src), out);		  \
	} while (0)

#define net_rpl_dao_ack_info(buf, src, dst, id, seq)			\
	do {								\
		char out[NET_IPV6_ADDR_LEN];				\
									\
		snprintk(out, sizeof(out), "%s",			\
			 net_sprint_ipv6_addr(dst));			\
		NET_DBG("Send DAO-ACK (id %d, seq %d) from %s to %s",	\
			id, seq, net_sprint_ipv6_addr(src), out);	\
	} while (0)
#else /* CONFIG_NET_DEBUG_RPL */
#define net_rpl_info(...)
#define net_rpl_dao_info(...)
#define net_rpl_dao_ack_info(...)
#endif /* CONFIG_NET_DEBUG_RPL */

static void new_dio_interval(struct net_rpl_instance *instance);

static inline struct net_nbr *get_nbr(int idx)
{
	return &net_rpl_neighbor_pool[idx].nbr;
}

static inline struct net_rpl_parent *nbr_data(struct net_nbr *nbr)
{
	return (struct net_rpl_parent *)nbr->data;
}

struct net_nbr *net_rpl_get_nbr(struct net_rpl_parent *data)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (nbr->data == (uint8_t *)data) {
			return nbr;
		}
	}

	return NULL;
}

static struct net_nbr *nbr_lookup(struct net_nbr_table *table,
				  struct net_if *iface,
				  struct in6_addr *addr)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (nbr->ref && nbr->iface == iface &&
		    net_ipv6_addr_cmp(&nbr_data(nbr)->dag->dag_id, addr)) {
			return nbr;
		}
	}

	return NULL;
}

static inline void nbr_free(struct net_nbr *nbr)
{
	NET_DBG("nbr %p", nbr);

	net_nbr_unref(nbr);
}

static struct net_nbr *nbr_add(struct net_if *iface,
			       struct in6_addr *addr,
			       struct net_linkaddr *lladdr)
{
	struct net_nbr *nbr = net_nbr_get(&net_rpl_parents.table);
	int ret;

	if (!nbr) {
		return NULL;
	}

	ret = net_nbr_link(nbr, iface, lladdr);
	if (ret) {
		NET_DBG("nbr linking failure (%d)", ret);
		nbr_free(nbr);
		return NULL;
	}

	NET_DBG("[%d] nbr %p IPv6 %s ll %s",
		nbr->idx, nbr, net_sprint_ipv6_addr(addr),
		net_sprint_ll_addr(lladdr->addr, lladdr->len));

	return nbr;
}

struct in6_addr *net_rpl_get_parent_addr(struct net_if *iface,
					 struct net_rpl_parent *parent)
{
	struct net_nbr *nbr;

	nbr = net_rpl_get_nbr(parent);
	if (!nbr) {
		NET_DBG("Parent %p unknown", parent);
		return NULL;
	}

	return net_ipv6_nbr_lookup_by_index(iface, nbr->idx);
}

#if defined(CONFIG_NET_DEBUG_RPL)
static void net_rpl_print_neighbors(void)
{
	if (rpl_default_instance && rpl_default_instance->current_dag) {
		int curr_interval = rpl_default_instance->dio_interval_current;
		int curr_rank = rpl_default_instance->current_dag->rank;
		uint32_t now = k_uptime_get_32();
		struct net_rpl_parent *parent;
		int i;

		NET_DBG("rank %u DIO interval %u", curr_rank,
			curr_interval);

		for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
			struct net_nbr *ipv6_nbr, *nbr = get_nbr(i);
			struct in6_addr *parent_addr;

			if (!nbr->ref) {
				continue;
			}

			parent = nbr_data(nbr);

			parent_addr =
				net_rpl_get_parent_addr(net_if_get_default(),
							parent);

			ipv6_nbr = net_ipv6_nbr_lookup(net_if_get_default(),
						       parent_addr);

			NET_DBG("[%d] nbr %s %5u %5u => %5u %c "
				"(last tx %u min ago)",
				nbr->idx,
				net_sprint_ll_addr(
					net_nbr_get_lladdr(nbr->idx)->addr,
					net_nbr_get_lladdr(nbr->idx)->len),
				parent->rank,
				ipv6_nbr ?
				net_ipv6_nbr_data(ipv6_nbr)->link_metric : 0,
				net_rpl_of_calc_rank(parent, 0),
				parent == rpl_default_instance->current_dag->
						preferred_parent ? '*' : ' ',
				(unsigned)((now - parent->last_tx_time) /
					   (60 * MSEC_PER_SEC)));
		}
	}
}

#define net_route_info(str, route, addr, len, nexthop)			\
	do {								\
		char out[NET_IPV6_ADDR_LEN];				\
									\
		snprintk(out, sizeof(out), "%s",			\
			 net_sprint_ipv6_addr(addr));			\
		NET_DBG("%s route to %s via %s (iface %p)", str, out,	\
			net_sprint_ipv6_addr(nexthop), route->iface);	\
	} while (0)
#else
#define net_rpl_print_neighbors(...)
#define net_route_info(...)
#endif /* CONFIG_NET_DEBUG_RPL */

struct net_route_entry *net_rpl_add_route(struct net_rpl_dag *dag,
					  struct net_if *iface,
					  struct in6_addr *addr,
					  int prefix_len,
					  struct in6_addr *nexthop)
{
	struct net_rpl_route_entry *extra;
	struct net_route_entry *route;
	struct net_nbr *nbr;

	route = net_route_add(iface, addr, prefix_len, nexthop);
	if (!route) {
		return NULL;
	}

	nbr = net_route_get_nbr(route);

	extra = net_nbr_extra_data(nbr);

	extra->dag = dag;
	extra->lifetime = net_rpl_lifetime(dag->instance,
					   dag->instance->default_lifetime);
	extra->route_source = NET_RPL_ROUTE_INTERNAL;

	net_route_info("Added", route, addr, prefix_len, nexthop);

	return route;
}

static inline void setup_icmpv6_hdr(struct net_buf *buf, uint8_t type,
				    uint8_t code)
{
	net_nbuf_append_u8(buf, type);
	net_nbuf_append_u8(buf, code);
	net_nbuf_append_be16(buf, 0); /* checksum */
}

int net_rpl_dio_send(struct net_if *iface,
		     struct net_rpl_instance *instance,
		     struct in6_addr *src,
		     struct in6_addr *dst)
{
	struct net_rpl_dag *dag = instance->current_dag;
	struct in6_addr addr, *dst_addr;
	struct net_buf *buf;
	uint16_t value;
	int ret;

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return -ENOMEM;
	}

	if (!dst) {
		net_rpl_create_mcast_address(&addr);
		dst_addr = &addr;
	} else {
		dst_addr = dst;
	}

	buf = net_ipv6_create_raw(buf, net_if_get_ll_reserve(iface, dst),
				  src, dst_addr, iface, IPPROTO_ICMPV6);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_if_get_ll_reserve(iface, dst));

	setup_icmpv6_hdr(buf, NET_ICMPV6_RPL, NET_RPL_DODAG_INFO_OBJ);

	net_nbuf_append_u8(buf, instance->instance_id);
	net_nbuf_append_u8(buf, dag->version);
	net_nbuf_append_be16(buf, dag->rank);

	value = net_rpl_dag_is_grounded(dag) << 8;
	value |= instance->mop << NET_RPL_DIO_MOP_SHIFT;
	value |= net_rpl_dag_get_preference(dag) & NET_RPL_DIO_PREFERENCE_MASK;
	net_nbuf_append_u8(buf, value);
	net_nbuf_append_u8(buf, instance->dtsn);

	if (!dst) {
		net_rpl_lollipop_increment(&instance->dtsn);
	}

	/* Flags and reserved are set to 0 */
	net_nbuf_append_be16(buf, 0);

	net_nbuf_append(buf, sizeof(struct in6_addr), dag->dag_id.s6_addr);

	if (instance->mc.type != NET_RPL_MC_NONE) {
		net_rpl_of_update_mc(instance);

		net_nbuf_append_u8(buf, NET_RPL_OPTION_DAG_METRIC_CONTAINER);
		net_nbuf_append_u8(buf, 6);
		net_nbuf_append_u8(buf, instance->mc.type);
		net_nbuf_append_u8(buf, instance->mc.flags >> 1);
		value = (instance->mc.flags & 1) << 7;
		net_nbuf_append_u8(buf, value |
			       (instance->mc.aggregated << 4) |
			       instance->mc.precedence);

		if (instance->mc.type == NET_RPL_MC_ETX) {
			net_nbuf_append_u8(buf, 2);
			net_nbuf_append_be16(buf, instance->mc.obj.etx);

		} else if (instance->mc.type == NET_RPL_MC_ENERGY) {
			net_nbuf_append_u8(buf, 2);
			net_nbuf_append_u8(buf, instance->mc.obj.energy.flags);
			net_nbuf_append_u8(buf,
					  instance->mc.obj.energy.estimation);

		} else {
			NET_DBG("Cannot send DIO, unknown DAG MC type %u",
				instance->mc.type);
			net_nbuf_unref(buf);
			return -EINVAL;
		}
	}

	net_nbuf_append_u8(buf, NET_RPL_OPTION_DAG_CONF);
	net_nbuf_append_u8(buf, 14);
	net_nbuf_append_u8(buf, 0); /* No Auth */
	net_nbuf_append_u8(buf, instance->dio_interval_doublings);
	net_nbuf_append_u8(buf, instance->dio_interval_min);
	net_nbuf_append_u8(buf, instance->dio_redundancy);
	net_nbuf_append_be16(buf, instance->max_rank_inc);
	net_nbuf_append_be16(buf, instance->min_hop_rank_inc);

	net_nbuf_append_be16(buf, instance->ocp);
	net_nbuf_append_u8(buf, 0); /* Reserved */
	net_nbuf_append_u8(buf, instance->default_lifetime);
	net_nbuf_append_be16(buf, instance->lifetime_unit);

	if (dag->prefix_info.length > 0) {
		net_nbuf_append_u8(buf, NET_RPL_OPTION_PREFIX_INFO);
		net_nbuf_append_u8(buf, 30); /* length */
		net_nbuf_append_u8(buf, dag->prefix_info.length);
		net_nbuf_append_u8(buf, dag->prefix_info.flags);

		/* First valid lifetime and the second one is
		 * preferred lifetime.
		 */
		net_nbuf_append_be32(buf, dag->prefix_info.lifetime);
		net_nbuf_append_be32(buf, dag->prefix_info.lifetime);

		net_nbuf_append_be32(buf, 0); /* reserved */
		net_nbuf_append(buf, sizeof(struct in6_addr),
			       dag->prefix_info.prefix.s6_addr);

		NET_DBG("Sending prefix info in DIO for %s",
			net_sprint_ipv6_addr(&dag->prefix_info.prefix));
	} else {
		NET_DBG("Prefix info not sent because length was %d",
			dag->prefix_info.length);
	}

	buf = net_ipv6_finalize_raw(buf, IPPROTO_ICMPV6);

	ret = net_send_data(buf);
	if (ret >= 0) {
		if (!dst) {
			NET_DBG("Sent a multicast DIO with rank %d",
				instance->current_dag->rank);
		} else {
			NET_DBG("Sent a unicast DIO with rank %d to %s",
				instance->current_dag->rank,
				net_sprint_ipv6_addr(dst));
		}

		net_stats_update_icmp_sent();
		net_stats_update_rpl_dio_sent();

		return 0;
	}

	net_nbuf_unref(buf);

	return ret;
}

#define DIO_TIMEOUT (MSEC_PER_SEC)

static void dio_timer(struct k_work *work)
{
	struct net_rpl_instance *instance = CONTAINER_OF(work,
						       struct net_rpl_instance,
						       dio_timer);

	NET_DBG("DIO Timer triggered at %u", k_uptime_get_32());

	if (!rpl_dio_send_ok) {
		struct in6_addr *tmp;

		tmp = net_if_ipv6_get_ll_addr(NET_ADDR_PREFERRED, NULL);
		if (tmp) {
			rpl_dio_send_ok = true;
		} else {
			NET_DBG("Sending DIO later because IPv6 link local "
				"address is not found");

			k_delayed_work_submit(&instance->dio_timer,
					      DIO_TIMEOUT);
			return;
		}
	}

	if (instance->dio_send) {
		if (instance->dio_redundancy &&
		    instance->dio_counter < instance->dio_redundancy) {
			struct net_if *iface;
			struct in6_addr *addr =
				net_if_ipv6_get_ll_addr(NET_ADDR_PREFERRED,
							&iface);

			net_rpl_dio_send(iface, instance, addr, NULL);

#if defined(CONFIG_NET_RPL_STATS)
			instance->dio_send_pkt++;
#endif
		} else {
			NET_DBG("Supressing DIO transmission as %d >= %d",
				instance->dio_counter,
				instance->dio_redundancy);
		}
		instance->dio_send = false;

		NET_DBG("Next DIO send after %lu ms",
			instance->dio_next_delay);

		k_delayed_work_submit(&instance->dio_timer,
				      instance->dio_next_delay);
	} else {
		if (instance->dio_interval_current <
		    instance->dio_interval_min +
		    instance->dio_interval_doublings) {
			instance->dio_interval_current++;

			NET_DBG("DIO Timer interval doubled to %d",
				instance->dio_interval_current);
		}

		new_dio_interval(instance);
	}

	net_rpl_print_neighbors();
}

static void new_dio_interval(struct net_rpl_instance *instance)
{
	uint32_t time;

	time = 1 << instance->dio_interval_current;

	NET_ASSERT(time);

	instance->dio_next_delay = time;
	time = time / 2 + (time / 2 * sys_rand32_get()) / UINT32_MAX;

	/* Adjust the interval so that they are equally long among the nodes.
	 * This is needed so that Trickle algo can operate efficiently.
	 */
	instance->dio_next_delay -= time;

#if defined(CONFIG_NET_RPL_STATS)
	instance->dio_intervals++;
	instance->dio_recv_pkt += instance->dio_counter;

	NET_DBG("rank %u.%u (%u) stats %d/%d/%d/%d %s",
		NET_RPL_DAG_RANK(instance->current_dag->rank, instance),
		((10 * (instance->current_dag->rank %
		       instance->min_hop_rank_inc)) /
		 instance->min_hop_rank_inc),
		instance->current_dag->version,
		instance->dio_intervals,
		instance->dio_send_pkt,
		instance->dio_recv_pkt,
		instance->dio_interval_current,
		instance->current_dag->rank == NET_RPL_ROOT_RANK(instance) ?
		"ROOT" : "");
#endif /* CONFIG_NET_RPL_STATS */

	instance->dio_counter = 0;
	instance->dio_send = true;

	NET_DBG("DIO Timer interval set to %d", time);

	k_delayed_work_cancel(&instance->dio_timer);
	k_delayed_work_init(&instance->dio_timer, dio_timer);
	k_delayed_work_submit(&instance->dio_timer, time);
}

static void net_rpl_dio_reset_timer(struct net_rpl_instance *instance)
{
	if (instance->dio_interval_current > instance->dio_interval_min) {
		instance->dio_interval_current = instance->dio_interval_min;
		instance->dio_counter = 0;

		new_dio_interval(instance);
	}

	net_stats_update_rpl_resets();
}

static inline void send_dis_all_interfaces(struct net_if *iface,
					   void *user_data)
{
	net_rpl_dis_send(user_data, iface);
}

int net_rpl_dis_send(struct in6_addr *dst, struct net_if *iface)
{
	struct in6_addr addr, *dst_addr;
	const struct in6_addr *src;
	struct net_buf *buf;
	uint16_t pos;
	int ret;

	if (!iface) {
		/* Go through all interface to send the DIS */
		net_if_foreach(send_dis_all_interfaces,
			       dst);
		return 0;
	}

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return -ENOMEM;
	}

	if (!dst) {
		net_rpl_create_mcast_address(&addr);
		dst_addr = &addr;
	} else {
		dst_addr = dst;
	}

	src = net_if_ipv6_select_src_addr(iface, dst_addr);

	buf = net_ipv6_create_raw(buf, net_if_get_ll_reserve(iface, dst_addr),
				  src, dst_addr, iface, IPPROTO_ICMPV6);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_if_get_ll_reserve(iface, dst_addr));

	setup_icmpv6_hdr(buf, NET_ICMPV6_RPL, NET_RPL_DODAG_SOLICIT);

	/* Add flags and reserved fields */
	net_nbuf_write_u8(buf, buf->frags,
			  sizeof(struct net_ipv6_hdr) +
			  sizeof(struct net_icmp_hdr),
			  &pos, 0);
	net_nbuf_write_u8(buf, buf->frags, pos, &pos, 0);

	buf = net_ipv6_finalize_raw(buf, IPPROTO_ICMPV6);

	ret = net_send_data(buf);
	if (ret >= 0) {
		NET_DBG("Sent a %s DIS to %s", dst ? "unicast" : "multicast",
			net_sprint_ipv6_addr(dst_addr));

		net_stats_update_icmp_sent();
		net_stats_update_rpl_dis_sent();
	} else {
		net_nbuf_unref(buf);
	}

	return ret;
}

static enum net_verdict handle_dis(struct net_buf *buf)
{
	struct net_rpl_instance *instance;

	net_rpl_info(buf, "DODAG Information Solicitation");

	for (instance = &rpl_instances[0];
	     instance < &rpl_instances[0] + CONFIG_NET_RPL_MAX_INSTANCES;
	     instance++) {
		if (!instance->is_used) {
			continue;
		}

		if (net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
			net_rpl_dio_reset_timer(instance);
		} else {
			net_rpl_dio_send(net_nbuf_iface(buf),
					 instance,
					 &NET_IPV6_BUF(buf)->src,
					 &NET_IPV6_BUF(buf)->dst);
		}
	}

	return NET_DROP;
}

static struct net_rpl_instance *net_rpl_get_instance(uint8_t instance_id)
{
	int i;

	for (i = 0; i < CONFIG_NET_RPL_MAX_INSTANCES; ++i) {
		if (rpl_instances[i].is_used &&
		    rpl_instances[i].instance_id == instance_id) {
			return &rpl_instances[i];
		}
	}

	return NULL;
}

#if defined(CONFIG_NET_RPL_PROBING)

#if !defined(NET_RPL_PROBING_INTERVAL)
#define NET_RPL_PROBING_INTERVAL (120 * MSEC_PER_SEC)
#endif

#if !defined(NET_RPL_PROBING_EXPIRATION_TIME)
#define NET_RPL_PROBING_EXPIRATION_TIME ((10 * 60) * MSEC_PER_SEC)
#endif

static void net_rpl_schedule_probing(struct net_rpl_instance *instance);

static struct net_rpl_parent *get_probing_target(struct net_rpl_dag *dag)
{
	/* Returns the next probing target. The probes are sent to the current
	 * preferred parent if we have not updated its link for
	 * NET_RPL_PROBING_EXPIRATION_TIME.
	 * Otherwise, it picks at random between:
	 * (1) selecting the best parent not updated
	 *     for NET_RPL_PROBING_EXPIRATION_TIME
	 * (2) selecting the least recently updated parent
	 */
	struct net_rpl_parent *probing_target = NULL;
	uint16_t probing_target_rank = NET_RPL_INFINITE_RANK;

	/* min_last_tx is the clock time NET_RPL_PROBING_EXPIRATION_TIME in
	 * the past
	 */
	uint32_t min_last_tx = k_uptime_get_32();
	struct net_rpl_parent *parent;
	int i;

	min_last_tx = min_last_tx > 2 *
		(NET_RPL_PROBING_EXPIRATION_TIME ?
		 min_last_tx - NET_RPL_PROBING_EXPIRATION_TIME : 1);

	if (!dag || !dag->instance || !dag->preferred_parent) {
		return NULL;
	}

	/* Our preferred parent needs probing */
	if (dag->preferred_parent->last_tx_time < min_last_tx) {
		probing_target = dag->preferred_parent;
	}

	/* With 50% probability: probe best parent not updated
	 * for NET_RPL_PROBING_EXPIRATION_TIME
	 */
	if (!probing_target && (sys_rand32_get() % 2) == 0) {
		for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
			struct net_nbr *nbr = get_nbr(i);

			parent = nbr_data(nbr);
			if (parent->dag == dag &&
			    parent->last_tx_time < min_last_tx) {
				/* parent is in our dag and needs probing */
				uint16_t parent_rank =
					net_rpl_of_calc_rank(parent, 0);

				if (!probing_target ||
				    parent_rank < probing_target_rank) {
					probing_target = parent;
					probing_target_rank = parent_rank;
				}
			}
		}
	}

	/* The default probing target is the least recently updated parent */
	if (!probing_target) {
		for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
			struct net_nbr *nbr = get_nbr(i);

			parent = nbr_data(nbr);
			if (parent->dag == dag) {
				if (!probing_target ||
				    parent->last_tx_time <
				    probing_target->last_tx_time) {
					probing_target = parent;
				}
			}
		}
	}

	return probing_target;
}

static void rpl_probing_timer(struct k_work *work)
{
	struct net_rpl_instance *instance =
		CONTAINER_OF(work, struct net_rpl_instance, probing_timer);

	struct net_rpl_parent *probing_target =
		get_probing_target(instance->current_dag);

	NET_DBG("Probing target %p dag %p", probing_target,
		instance->current_dag);

	if (probing_target) {
		struct net_nbr *nbr = net_rpl_get_nbr(probing_target);
		struct net_linkaddr_storage *lladdr;
		struct in6_addr *src;
		struct in6_addr *dst;
		int ret;

		dst = net_ipv6_nbr_lookup_by_index(instance->iface, nbr->idx);

		lladdr = net_nbr_get_lladdr(nbr->idx);

		NET_DBG("Probing %s [%s]", net_sprint_ipv6_addr(dst),
			net_sprint_ll_addr(lladdr->addr, lladdr->len));

		src = (struct in6_addr *)
			net_if_ipv6_select_src_addr(instance->iface, dst);

		/* Send probe (currently DIO) */
		ret = net_rpl_dio_send(instance->iface, instance, src, dst);
		if (ret) {
			NET_DBG("DIO probe failed (%d)", ret);
		}
	}

	/* Schedule next probing */
	net_rpl_schedule_probing(instance);

	net_rpl_print_neighbors();
}

static void net_rpl_schedule_probing(struct net_rpl_instance *instance)
{
	int expiration;

	expiration = ((NET_RPL_PROBING_INTERVAL / 2) +
		      sys_rand32_get() % NET_RPL_PROBING_INTERVAL) *
		MSEC_PER_SEC;

	NET_DBG("Send probe in %lu ms, instance %p (%d)",
		expiration, instance, instance->instance_id);

	k_delayed_work_init(&instance->probing_timer, rpl_probing_timer);
	k_delayed_work_submit(&instance->probing_timer, expiration);
}
#endif /* CONFIG_NET_RPL_PROBING */

static struct net_rpl_instance *net_rpl_alloc_instance(uint8_t instance_id)
{
	int i;

	for (i = 0; i < CONFIG_NET_RPL_MAX_INSTANCES; ++i) {
		if (rpl_instances[i].is_used) {
			continue;
		}

		memset(&rpl_instances[i], 0, sizeof(struct net_rpl_instance));

		rpl_instances[i].instance_id = instance_id;
		rpl_instances[i].default_route = NULL;
		rpl_instances[i].is_used = true;

#if defined(CONFIG_NET_RPL_PROBING)
		net_rpl_schedule_probing(&rpl_instances[i]);
#endif
		return &rpl_instances[i];
	}

	return NULL;
}

static struct net_rpl_dag *alloc_dag(uint8_t instance_id,
				     struct in6_addr *dag_id)
{
	struct net_rpl_instance *instance;
	struct net_rpl_dag *dag;
	int i;

	instance = net_rpl_get_instance(instance_id);
	if (!instance) {
		instance = net_rpl_alloc_instance(instance_id);
		if (!instance) {
			NET_DBG("Cannot allocate instance id %d", instance_id);
			net_stats_update_rpl_mem_overflows();

			return NULL;
		}
	}

	for (i = 0; i < CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE; ++i) {
		dag = &instance->dags[i];

		if (net_rpl_dag_is_used(dag)) {
			continue;
		}

		memset(dag, 0, sizeof(*dag));

		net_rpl_dag_set_used(dag);
		dag->rank = NET_RPL_INFINITE_RANK;
		dag->min_rank = NET_RPL_INFINITE_RANK;
		dag->instance = instance;

		return dag;
	}

	return NULL;
}

static struct net_rpl_dag *get_dag(uint8_t instance_id,
				   struct in6_addr *dag_id)
{
	struct net_rpl_instance *instance;
	struct net_rpl_dag *dag;
	int i;

	instance = net_rpl_get_instance(instance_id);
	if (!instance) {
		NET_DBG("Cannot get instance id %d", instance_id);
		return NULL;
	}

	for (i = 0; i < CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE; ++i) {
		dag = &instance->dags[i];

		if (net_rpl_dag_is_used(dag) &&
		    net_ipv6_addr_cmp(&dag->dag_id, dag_id)) {
			return dag;
		}
	}

	return NULL;
}

static void route_rm_cb(struct net_route_entry *entry, void *user_data)
{
	struct net_rpl_dag *dag = user_data;
	struct net_rpl_route_entry *extra;

	extra = net_nbr_extra_data(net_route_get_nbr(entry));

	if (extra->dag == dag) {
		net_route_del(entry);
	}
}

#if NET_RPL_MULTICAST
static void route_mcast_rm_cb(struct net_route_entry_mcast *route,
			      void *user_data)
{
	struct net_rpl_dag *dag = user_data;
	struct net_rpl_route_entry *extra;

	extra = net_nbr_extra_data(net_route_get_nbr(entry));

	if (extra->dag == dag) {
		net_route_mcast_del(route)
	}
}
#endif

static void net_rpl_remove_routes(struct net_rpl_dag *dag)
{
	net_route_foreach(route_rm_cb, dag);

#if NET_RPL_MULTICAST
	net_route_mcast_foreach(route_mcast_rm_cb, dag);
#endif
}

static inline void set_ip_from_prefix(struct net_linkaddr *lladdr,
				      struct net_rpl_prefix *prefix,
				      struct in6_addr *addr)
{
	memset(addr, 0, sizeof(struct in6_addr));

	net_ipv6_addr_create_iid(addr, lladdr);

	memcpy(addr->s6_addr, &prefix->prefix, (prefix->length + 7) / 8);
}

static void check_prefix(struct net_if *iface,
			 struct net_rpl_prefix *last_prefix,
			 struct net_rpl_prefix *new_prefix)
{
	struct in6_addr addr;

	if (last_prefix && new_prefix &&
	    last_prefix->length == new_prefix->length &&
	    net_is_ipv6_prefix(last_prefix->prefix.s6_addr,
			       new_prefix->prefix.s6_addr,
			       new_prefix->length) &&
	    last_prefix->flags == new_prefix->flags) {
		/* Nothing has changed. */
		NET_DBG("Same prefix %s/%d flags 0x%x",
			net_sprint_ipv6_addr(&new_prefix->prefix),
			new_prefix->length, new_prefix->flags);
		return;
	}

	if (last_prefix) {
		set_ip_from_prefix(&iface->link_addr, last_prefix, &addr);

		if (net_if_ipv6_addr_rm(iface, &addr)) {
			NET_DBG("Removed global IP address %s",
				net_sprint_ipv6_addr(&addr));
		}
	}

	if (new_prefix) {
		set_ip_from_prefix(&iface->link_addr, new_prefix, &addr);

		if (net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF, 0)) {
			NET_DBG("Added global IP address %s",
				net_sprint_ipv6_addr(&addr));
		}
	}
}

static void net_rpl_free_dag(struct net_if *iface, struct net_rpl_dag *dag)
{
	if (net_rpl_dag_is_joined(dag)) {
		NET_DBG("Leaving the DAG %s",
			net_sprint_ipv6_addr(&dag->dag_id));

		net_rpl_dag_unjoin(dag);

		/* Remove routes installed by DAOs. */
		net_rpl_remove_routes(dag);

		/* Remove autoconfigured address */
		if ((dag->prefix_info.flags & NET_ICMPV6_RA_FLAG_AUTONOMOUS)) {
			check_prefix(iface, &dag->prefix_info, NULL);
		}

		remove_parents(iface, dag, 0);
	}

	net_rpl_dag_set_not_used(dag);
}

static void net_rpl_set_preferred_parent(struct net_if *iface,
					 struct net_rpl_dag *dag,
					 struct net_rpl_parent *parent)
{
	if (dag && dag->preferred_parent != parent) {
		struct in6_addr *addr = net_rpl_get_parent_addr(iface, parent);

		NET_DBG("Preferred parent %s",
			parent ? net_sprint_ipv6_addr(addr) : "not set");

		addr = net_rpl_get_parent_addr(iface, dag->preferred_parent);

		NET_DBG("It used to be %s",
			dag->preferred_parent ?
			net_sprint_ipv6_addr(addr) : "not set");

		dag->preferred_parent = parent;
	}
}

static void net_rpl_reset_dio_timer(struct net_rpl_instance *instance)
{
	NET_DBG("instance %p current %d min %d", instance,
		instance->dio_interval_current,
		instance->dio_interval_min);

	/* Do not reset if we are already on the minimum interval,
	 * unless forced to do so.
	 */
	if (instance->dio_interval_current > instance->dio_interval_min) {
		instance->dio_counter = 0;
		instance->dio_interval_current = instance->dio_interval_min;
		new_dio_interval(instance);
	}

	net_stats_update_rpl_resets();
}

static
struct net_rpl_dag *net_rpl_set_root_with_version(struct net_if *iface,
						  uint8_t instance_id,
						  struct in6_addr *dag_id,
						  uint8_t version)
{
	struct net_rpl_dag *dag;
	struct net_rpl_instance *instance;
	int i;

	instance = net_rpl_get_instance(instance_id);
	if (instance) {
		for (i = 0; i < CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE; i++) {
			dag = &instance->dags[i];

			if (net_rpl_dag_is_used(dag)) {
				if (net_ipv6_addr_cmp(&dag->dag_id, dag_id)) {
					version = dag->version;

					net_rpl_lollipop_increment(&version);
				}

				if (dag == dag->instance->current_dag) {
					NET_DBG("Dropping a joined DAG when "
						"setting this node as root");
					dag->instance->current_dag = NULL;
				} else {
					NET_DBG("Dropping a DAG when "
						"setting this node as root");
				}

				net_rpl_free_dag(iface, dag);
			}
		}
	}

	dag = alloc_dag(instance_id, dag_id);
	if (!dag) {
		NET_DBG("Failed to allocate a DAG");
		return NULL;
	}

	instance = dag->instance;

	net_rpl_dag_join(dag);
	net_rpl_dag_set_preference(dag, CONFIG_NET_RPL_PREFERENCE);
	net_rpl_dag_set_grounded_status(dag, NET_RPL_GROUNDED);
	dag->version = version;

	instance->mop = NET_RPL_MOP_DEFAULT;

	instance->ocp = net_rpl_of_get();

	net_rpl_set_preferred_parent(iface, dag, NULL);

	net_ipaddr_copy(&dag->dag_id, dag_id);

	instance->dio_interval_doublings =
					CONFIG_NET_RPL_DIO_INTERVAL_DOUBLINGS;
	instance->dio_interval_min = CONFIG_NET_RPL_DIO_INTERVAL_MIN;

	/* The current interval must differ from the minimum interval in
	 * order to trigger a DIO timer reset.
	 */
	instance->dio_interval_current = CONFIG_NET_RPL_DIO_INTERVAL_MIN +
		CONFIG_NET_RPL_DIO_INTERVAL_DOUBLINGS;
	instance->dio_redundancy = CONFIG_NET_RPL_DIO_REDUNDANCY;
	instance->max_rank_inc = NET_RPL_MAX_RANK_INC;
	instance->min_hop_rank_inc = CONFIG_NET_RPL_MIN_HOP_RANK_INC;
	instance->default_lifetime = CONFIG_NET_RPL_DEFAULT_LIFETIME;
	instance->lifetime_unit = CONFIG_NET_RPL_DEFAULT_LIFETIME_UNIT;

	dag->rank = NET_RPL_ROOT_RANK(instance);

	if (instance->current_dag != dag && instance->current_dag) {
		/* Remove routes installed by DAOs. */
		net_rpl_remove_routes(instance->current_dag);

		net_rpl_dag_unjoin(instance->current_dag);
	}

	instance->current_dag = dag;
	instance->dtsn = net_rpl_lollipop_init();
	net_rpl_of_update_mc(instance);
	rpl_default_instance = instance;

	NET_DBG("Node set to be a DAG root with DAG ID %s",
		net_sprint_ipv6_addr(&dag->dag_id));

	net_rpl_reset_dio_timer(instance);

	return dag;
}

struct net_rpl_dag *net_rpl_get_any_dag(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_RPL_MAX_INSTANCES; ++i) {
		if (rpl_instances[i].is_used &&
		    net_rpl_dag_is_joined(rpl_instances[i].current_dag)) {
			return rpl_instances[i].current_dag;
		}
	}

	return NULL;
}

struct net_rpl_dag *net_rpl_set_root(struct net_if *iface,
				     uint8_t instance_id,
				     struct in6_addr *dag_id)
{
	return net_rpl_set_root_with_version(iface, instance_id, dag_id,
					     net_rpl_lollipop_init());
}

static int lollipop_greater_than(int a, int b)
{
	/* Check if we are comparing an initial value with an old value */
	if (a > NET_RPL_LOLLIPOP_CIRCULAR_REGION &&
	    b <= NET_RPL_LOLLIPOP_CIRCULAR_REGION) {
		return (NET_RPL_LOLLIPOP_MAX_VALUE + 1 + b - a) >
			NET_RPL_LOLLIPOP_SEQUENCE_WINDOWS;
	}

	/* Otherwise check if a > b and comparable => ok, or
	 * if they have wrapped and are still comparable
	 */
	return (a > b && (a - b) < NET_RPL_LOLLIPOP_SEQUENCE_WINDOWS) ||
		(a < b && (b - a) > (NET_RPL_LOLLIPOP_CIRCULAR_REGION + 1 -
				     NET_RPL_LOLLIPOP_SEQUENCE_WINDOWS));
}

bool net_rpl_set_prefix(struct net_if *iface,
			struct net_rpl_dag *dag,
			struct in6_addr *prefix,
			uint8_t prefix_len)
{
	uint8_t last_len = dag->prefix_info.length;
	struct net_rpl_prefix last_prefix;

	if (prefix_len > 128) {
		return false;
	}

	if (dag->prefix_info.length) {
		memcpy(&last_prefix, &dag->prefix_info,
		       sizeof(struct net_rpl_prefix));
	}

	memset(&dag->prefix_info.prefix, 0,
	       sizeof(dag->prefix_info.prefix));
	memcpy(&dag->prefix_info.prefix, prefix, (prefix_len + 7) / 8);
	dag->prefix_info.length = prefix_len;
	dag->prefix_info.flags = NET_ICMPV6_RA_FLAG_AUTONOMOUS;

	/* Autoconfigure an address if this node does not already have
	 * an address with this prefix. Otherwise, update the prefix.
	 */
	NET_DBG("Prefix is %s, will announce this in DIOs",
		last_len ? "non-NULL" : "NULL");
	if (last_len == 0) {
		check_prefix(iface, NULL, &dag->prefix_info);
	} else {
		check_prefix(iface, &last_prefix, &dag->prefix_info);
	}

	return true;
}

static void net_rpl_nullify_parent(struct net_if *iface,
				   struct net_rpl_parent *parent)
{
	struct net_rpl_dag *dag = parent->dag;
#if defined(CONFIG_NET_DEBUG_RPL)
	struct in6_addr *addr = net_rpl_get_parent_addr(iface, parent);
#endif

	/* This function can be called when the preferred parent is NULL,
	 * so we need to handle this condition properly.
	 */
	if (parent == dag->preferred_parent || !dag->preferred_parent) {
		dag->rank = NET_RPL_INFINITE_RANK;

		if (net_rpl_dag_is_joined(dag)) {
			if (dag->instance->default_route) {
				NET_DBG("Removing default route %s",
					net_sprint_ipv6_addr(addr));

				net_if_router_rm(dag->instance->default_route);

				dag->instance->default_route = NULL;
			}

			/* Send no-path DAO only to preferred parent, if any */
			if (parent == dag->preferred_parent) {
				dao_send(parent, NET_RPL_ZERO_LIFETIME, NULL);
				net_rpl_set_preferred_parent(iface, dag, NULL);
			}
		}
	}

	NET_DBG("Nullifying parent %s", net_sprint_ipv6_addr(addr));
}

static void net_rpl_remove_parent(struct net_if *iface,
				  struct net_rpl_parent *parent,
				  struct net_nbr *nbr)
{
	if (!nbr) {
		nbr = net_rpl_get_nbr(parent);
	}

	NET_ASSERT(iface);

	if (nbr) {
#if defined(CONFIG_NET_DEBUG_RPL)
		struct in6_addr *addr;
		struct net_linkaddr_storage *lladdr;

		addr = net_rpl_get_parent_addr(iface, parent);
		lladdr = net_nbr_get_lladdr(nbr->idx);

		NET_DBG("Removing parent %s [%s]",
			net_sprint_ipv6_addr(addr),
			net_sprint_ll_addr(lladdr->addr, lladdr->len));
#endif /* CONFIG_NET_DEBUG_RPL */

		net_rpl_nullify_parent(iface, parent);

		nbr_free(nbr);
	}
}

/* Remove DAG parents with a rank that is at least the same as minimum_rank.
 */
static void remove_parents(struct net_if *iface,
			   struct net_rpl_dag *dag,
			   uint16_t minimum_rank)
{
	int i;

	NET_DBG("Removing parents minimum rank %u", minimum_rank);

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_rpl_parent *parent;

		parent = nbr_data(nbr);

		if (dag == parent->dag && parent->rank >= minimum_rank) {
			net_rpl_remove_parent(iface, parent, nbr);
		}
	}
}

static struct net_rpl_parent *net_rpl_add_parent(struct net_if *iface,
						 struct net_rpl_dag *dag,
						 struct net_rpl_dio *dio,
						 struct in6_addr *addr)
{
	struct net_nbr *nbr;

	/* Is the parent known in neighbor cache? Drop this request if not.
	 * Typically, the parent is added upon receiving a DIO.
	 */
	nbr = net_ipv6_nbr_lookup(iface, addr);
	if (nbr) {
		struct net_linkaddr_storage *lladdr_storage;
		struct net_ipv6_nbr_data *data;
		struct net_rpl_parent *parent;
		struct net_linkaddr lladdr;
		struct net_nbr *rpl_nbr;

		lladdr_storage = net_nbr_get_lladdr(nbr->idx);

		lladdr.addr = lladdr_storage->addr;
		lladdr.len = lladdr_storage->len;

		rpl_nbr = net_nbr_lookup(&net_rpl_parents.table,
					 iface,
					 &lladdr);
		if (!rpl_nbr) {
			NET_DBG("Add parent %s [%s]",
				net_sprint_ipv6_addr(addr),
				net_sprint_ll_addr(lladdr.addr, lladdr.len));

			rpl_nbr = nbr_add(iface, addr, &lladdr);
			if (!rpl_nbr) {
				NET_DBG("Cannot add RPL neighbor");
				return NULL;
			}
		}

		parent = nbr_data(rpl_nbr);

		NET_DBG("[%d] nbr %p parent %p", rpl_nbr->idx, rpl_nbr,
			parent);

		parent->dag = dag;
		parent->rank = dio->rank;
		parent->dtsn = dio->dtsn;

		/* Check whether we have a neighbor that has not gotten
		 * a link metric yet.
		 */
		data = net_ipv6_nbr_data(nbr);

		if (data->link_metric == 0) {
			data->link_metric = CONFIG_NET_RPL_INIT_LINK_METRIC *
				NET_RPL_MC_ETX_DIVISOR;
		}

#if !defined(CONFIG_NET_RPL_DAG_MC_NONE)
		memcpy(&parent->mc, &dio->mc, sizeof(parent->mc));
#endif

		return parent;
	}

	return NULL;
}

static void dao_timer(struct net_rpl_instance *instance)
{
	/* Send the DAO to the preferred parent. */
	if (instance->current_dag->preferred_parent) {
		NET_DBG("Sending DAO iface %p", instance->iface);

		dao_send(instance->current_dag->preferred_parent,
			 instance->default_lifetime,
			 instance->iface);

#if NET_RPL_MULTICAST
		/* Send DAOs for multicast prefixes only if the instance is
		 * in MOP 3.
		 */
		if (instance->mop == NET_RPL_MOP_STORING_MULTICAST) {
			send_mcast_dao(instance);
		}
#endif
	} else {
		NET_DBG("No suitable DAO parent found.");
	}
}

static void dao_lifetime_timer(struct k_work *work)
{
	struct net_rpl_instance *instance =
		CONTAINER_OF(work, struct net_rpl_instance,
			     dao_lifetime_timer);

	dao_timer(instance);

	instance->dao_lifetime_timer_active = false;

	set_dao_lifetime_timer(instance);
}

static void set_dao_lifetime_timer(struct net_rpl_instance *instance)
{
	if (net_rpl_get_mode() == NET_RPL_MODE_FEATHER) {
		return;
	}

	/* Set up another DAO within half the expiration time,
	 * if such a time has been configured.
	 */
	if (instance && instance->lifetime_unit != 0xffff &&
	    instance->default_lifetime != 0xff) {
		uint32_t expiration_time;

		expiration_time = (uint32_t)instance->default_lifetime *
			(uint32_t)instance->lifetime_unit *
			MSEC_PER_SEC / 2;

		instance->dao_lifetime_timer_active = true;

		NET_DBG("Scheduling DAO lifetime timer %d ms in the future",
			expiration_time);

		k_delayed_work_init(&instance->dao_lifetime_timer,
				    dao_lifetime_timer);
		k_delayed_work_submit(&instance->dao_lifetime_timer,
				      expiration_time);
	}
}

static void dao_send_timer(struct k_work *work)
{
	struct net_rpl_instance *instance =
		CONTAINER_OF(work, struct net_rpl_instance, dao_timer);

	instance->dao_timer_active = false;

	if (!rpl_dio_send_ok &&
	    !net_if_ipv6_get_ll(instance->iface, NET_ADDR_PREFERRED)) {
		NET_DBG("Postpone DAO transmission, trying again later.");

		instance->dao_timer_active = true;

		k_delayed_work_submit(&instance->dao_timer, MSEC_PER_SEC);
		return;
	}

	dao_timer(instance);
}

static void schedule_dao(struct net_rpl_instance *instance, int latency)
{
	int expiration;

	if (net_rpl_get_mode() == NET_RPL_MODE_FEATHER) {
		return;
	}

	if (instance->dao_timer_active) {
		NET_DBG("DAO timer already scheduled");
		return;
	}

	if (latency != 0) {
		latency = latency * MSEC_PER_SEC;
		expiration = latency / 2 + (sys_rand32_get() % latency);
	} else {
		expiration = 0;
	}

	NET_DBG("Scheduling DAO timer %u ms in the future",
		(unsigned int)expiration);

	instance->dao_timer_active = true;

	k_delayed_work_init(&instance->dao_timer, dao_send_timer);
	k_delayed_work_submit(&instance->dao_timer, expiration);

	if (!instance->dao_lifetime_timer_active) {
		set_dao_lifetime_timer(instance);
	}
}

static inline void net_rpl_schedule_dao(struct net_rpl_instance *instance)
{
	schedule_dao(instance, CONFIG_NET_RPL_DAO_TIMER);
}

static inline void net_rpl_schedule_dao_now(struct net_rpl_instance *instance)
{
	schedule_dao(instance, 0);
}

static int net_rpl_set_default_route(struct net_if *iface,
				     struct net_rpl_instance *instance,
				     struct in6_addr *from)
{
	if (instance->default_route) {
		NET_DBG("Removing default route through %s",
			net_sprint_ipv6_addr(&instance->default_route->address.
					     in6_addr));
		net_if_router_rm(instance->default_route);
		instance->default_route = NULL;
	}

	if (from) {
		NET_DBG("Adding default route through %s",
			net_sprint_ipv6_addr(from));

		instance->default_route =
			net_if_ipv6_router_add(iface, from,
					       net_rpl_lifetime(instance,
						  instance->default_lifetime));
		if (!instance->default_route) {
			return -EINVAL;
		}
	} else {
		if (instance->default_route) {
			NET_DBG("Removing default route through %s",
				net_sprint_ipv6_addr(&instance->
						     default_route->address.
						     in6_addr));
			net_if_router_rm(instance->default_route);
			instance->default_route = NULL;
		} else {
			NET_DBG("Not removing default route because it is "
				"missing");
		}
	}

	return 0;
}

static inline
struct net_rpl_dag *get_best_dag(struct net_rpl_instance *instance,
				 struct net_rpl_parent *parent)
{
	struct net_rpl_dag *dag, *end, *best_dag = NULL;

	for (dag = &instance->dags[0],
		     end = dag + CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE;
	     dag < end; ++dag) {

		if (dag->is_used && dag->preferred_parent &&
		    dag->preferred_parent->rank != NET_RPL_INFINITE_RANK) {

			if (!best_dag) {
				best_dag = dag;
			} else {
				best_dag = net_rpl_of_best_dag(best_dag,
							       dag);
			}
		}
	}

	return best_dag;
}

static struct net_rpl_parent *best_parent(struct net_if *iface,
					  struct net_rpl_dag *dag)
{
	struct net_rpl_parent *best = NULL;
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_rpl_parent *parent;

		parent = nbr_data(nbr);

		if (parent->dag != dag ||
		    parent->rank == NET_RPL_INFINITE_RANK) {
			/* ignore this neighbor */
		} else if (!best) {
			best = parent;
		} else {
			best = net_rpl_of_best_parent(iface, best, parent);
		}
	}

	return best;
}

static struct net_rpl_parent *net_rpl_select_parent(struct net_if *iface,
						    struct net_rpl_dag *dag)
{
	struct net_rpl_parent *best = best_parent(iface, dag);

	if (best) {
		net_rpl_set_preferred_parent(iface, dag, best);
	}

	return best;
}

static int acceptable_rank(struct net_rpl_dag *dag, uint16_t rank)
{
	return rank != NET_RPL_INFINITE_RANK &&
		(dag->instance->max_rank_inc == 0 ||
		 NET_RPL_DAG_RANK(rank, dag->instance) <=
		 NET_RPL_DAG_RANK(dag->min_rank + dag->instance->max_rank_inc,
				  dag->instance));
}

static
struct net_rpl_dag *net_rpl_select_dag(struct net_if *iface,
				       struct net_rpl_instance *instance,
				       struct net_rpl_parent *parent)
{
	struct net_rpl_parent *last_parent;
	struct net_rpl_dag *best_dag;
	uint16_t old_rank;

	old_rank = instance->current_dag->rank;
	last_parent = instance->current_dag->preferred_parent;

	best_dag = instance->current_dag;

	if (best_dag->rank != NET_RPL_ROOT_RANK(instance)) {

		if (net_rpl_select_parent(iface, parent->dag)) {
			if (parent->dag != best_dag) {
				best_dag = net_rpl_of_best_dag(best_dag,
							       parent->dag);
			}
		} else if (parent->dag == best_dag) {
			best_dag = get_best_dag(instance, parent);
		}
	}

	if (!best_dag) {
		/* No parent found: the calling function handle this problem. */
		return NULL;
	}

	if (instance->current_dag != best_dag) {
		/* Remove routes installed by DAOs. */
		net_rpl_remove_routes(instance->current_dag);

		NET_DBG("New preferred DAG %s",
			net_sprint_ipv6_addr(&best_dag->dag_id));

		if (best_dag->prefix_info.flags &
		    NET_ICMPV6_RA_FLAG_AUTONOMOUS) {
			check_prefix(iface,
				     &instance->current_dag->prefix_info,
				     &best_dag->prefix_info);

		} else if (instance->current_dag->prefix_info.flags &
			   NET_ICMPV6_RA_FLAG_AUTONOMOUS) {
			check_prefix(iface,
				     &instance->current_dag->prefix_info,
				     NULL);
		}

		net_rpl_dag_join(best_dag);
		net_rpl_dag_unjoin(instance->current_dag);
		instance->current_dag = best_dag;
	}

	net_rpl_of_update_mc(instance);

	/* Update the DAG rank. */
	best_dag->rank = net_rpl_of_calc_rank(best_dag->preferred_parent, 0);

	if (!last_parent || best_dag->rank < best_dag->min_rank) {
		best_dag->min_rank = best_dag->rank;

	} else if (!acceptable_rank(best_dag, best_dag->rank)) {
		NET_DBG("New rank unacceptable!");

		net_rpl_set_preferred_parent(iface, instance->current_dag,
					     NULL);

		if (instance->mop != NET_RPL_MOP_NO_DOWNWARD_ROUTES &&
		    last_parent) {

			/* Send a No-Path DAO to the removed preferred parent.
			 */
			dao_send(last_parent, NET_RPL_ZERO_LIFETIME, iface);
		}

		return NULL;
	}

	if (best_dag->preferred_parent != last_parent) {
		net_rpl_set_default_route(iface, instance,
			net_rpl_get_parent_addr(iface,
						best_dag->preferred_parent));

		NET_DBG("Changed preferred parent, rank changed from %u to %u",
			old_rank, best_dag->rank);

		net_stats_update_rpl_parent_switch();

		if (instance->mop != NET_RPL_MOP_NO_DOWNWARD_ROUTES) {
			if (last_parent) {
				/* Send a No-Path DAO to the removed preferred
				 * parent.
				 */
				dao_send(last_parent,
					 NET_RPL_ZERO_LIFETIME,
					 iface);
			}

			/* The DAO parent set changed, so schedule a DAO
			 * transmission.
			 */
			net_rpl_lollipop_increment(&instance->dtsn);

			net_rpl_schedule_dao(instance);
		}

		net_rpl_reset_dio_timer(instance);

		net_rpl_print_neighbors();

	} else if (best_dag->rank != old_rank) {
		NET_DBG("Preferred parent update, rank changed from %u to %u",
			old_rank, best_dag->rank);
	}

	return best_dag;
}

static void nullify_parents(struct net_if *iface,
			    struct net_rpl_dag *dag,
			    uint16_t minimum_rank)
{
	int i;

	NET_DBG("Nullifying parents (minimum rank %u)", minimum_rank);

	for (i = 0; i < CONFIG_NET_IPV6_MAX_NEIGHBORS; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_rpl_parent *parent;

		parent = nbr_data(nbr);

		if (dag == parent->dag &&
		    parent->rank >= minimum_rank) {
			net_rpl_nullify_parent(iface, parent);
		}
	}
}

static void net_rpl_local_repair(struct net_if *iface,
				 struct net_rpl_instance *instance)
{
	int i;

	if (!instance) {
		return;
	}

	NET_DBG("Starting a local instance repair");

	for (i = 0; i < CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE; i++) {
		if (instance->dags[i].is_used) {
			instance->dags[i].rank = NET_RPL_INFINITE_RANK;

			nullify_parents(iface, &instance->dags[i], 0);
		}
	}

	net_rpl_reset_dio_timer(instance);

	net_stats_update_rpl_local_repairs();
}

/* Return true if parent is kept, false if it is dropped */
static bool net_rpl_process_parent_event(struct net_if *iface,
					 struct net_rpl_instance *instance,
					 struct net_rpl_parent *parent)
{
	bool ret = true;

#if defined(CONFIG_NET_DEBUG_RPL)
	uint16_t old_rank = instance->current_dag->rank;
#endif

	if (!acceptable_rank(parent->dag, parent->rank)) {
		/* The candidate parent is no longer valid: the rank increase
		 * resulting from the choice of it as a parent would be too
		 * high.
		 */
		NET_DBG("Unacceptable rank %u", parent->rank);

		net_rpl_nullify_parent(iface, parent);

		if (parent != instance->current_dag->preferred_parent) {
			return false;
		}

		ret = false;
	}

	if (!net_rpl_select_dag(iface, instance, parent)) {
		/* No suitable parent; trigger a local repair. */
		NET_DBG("No parents found in any DAG");

		net_rpl_local_repair(iface, instance);

		return false;
	}

#if defined(CONFIG_NET_DEBUG_RPL)
	if (NET_RPL_DAG_RANK(old_rank, instance) !=
	    NET_RPL_DAG_RANK(instance->current_dag->rank, instance)) {
		NET_DBG("Moving in the instance from rank %u to %u",
			NET_RPL_DAG_RANK(old_rank, instance),
			NET_RPL_DAG_RANK(instance->current_dag->rank,
					 instance));

		if (instance->current_dag->rank != NET_RPL_INFINITE_RANK) {
			NET_DBG("The preferred parent is %s (rank %u)",
				net_sprint_ipv6_addr(
					net_rpl_get_parent_addr(iface,
						instance->current_dag->
						preferred_parent)),

				NET_RPL_DAG_RANK(instance->current_dag->
						 preferred_parent->rank,
						 instance));
		} else {
			NET_DBG("We don't have any parent");
		}
	}
#endif /* CONFIG_NET_DEBUG_RPL */

	return ret;
}

static bool net_rpl_repair_root(uint8_t instance_id)
{
	struct net_rpl_instance *instance;

	instance = net_rpl_get_instance(instance_id);
	if (!instance ||
	    instance->current_dag->rank != NET_RPL_ROOT_RANK(instance)) {
		NET_DBG("RPL repair root triggered but not root");
		return false;
	}

	net_stats_update_rpl_root_repairs();

	net_rpl_lollipop_increment(&instance->current_dag->version);
	net_rpl_lollipop_increment(&instance->dtsn);

	NET_DBG("RPL repair root initiating global repair with version %d",
		instance->current_dag->version);

	net_rpl_reset_dio_timer(instance);

	return true;
}

void net_rpl_global_repair(struct net_route_entry *route)
{
	/* Trigger a global repair. */
	struct net_rpl_route_entry *extra;
	struct net_rpl_instance *instance;
	struct net_rpl_dag *dag;
	struct net_nbr *nbr;

	nbr = net_route_get_nbr(route);
	if (!nbr) {
		NET_DBG("No neighbor for route %p", route);
		return;
	}

	extra = net_nbr_extra_data(nbr);

	dag = extra->dag;
	if (dag) {
		instance = dag->instance;

		net_rpl_repair_root(instance->instance_id);
	}
}

static void global_repair(struct net_if *iface,
			  struct in6_addr *from,
			  struct net_rpl_dag *dag,
			  struct net_rpl_dio *dio)
{
	struct net_rpl_parent *parent;

	remove_parents(iface, dag, 0);

	dag->version = dio->version;

	/* Copy parts of the configuration so that it propagates
	 * in the network.
	 */
	dag->instance->dio_interval_doublings = dio->dag_interval_doublings;
	dag->instance->dio_interval_min = dio->dag_interval_min;
	dag->instance->dio_redundancy = dio->dag_redundancy;
	dag->instance->default_lifetime = dio->default_lifetime;
	dag->instance->lifetime_unit = dio->lifetime_unit;

	net_rpl_of_reset(dag);
	dag->min_rank = NET_RPL_INFINITE_RANK;
	net_rpl_lollipop_increment(&dag->instance->dtsn);

	parent = net_rpl_add_parent(iface, dag, dio, from);
	if (!parent) {
		NET_DBG("Failed to add a parent during the global repair");
		dag->rank = NET_RPL_INFINITE_RANK;
	} else {
		dag->rank = net_rpl_of_calc_rank(parent, 0);
		dag->min_rank = dag->rank;

		NET_DBG("Starting global repair");
		net_rpl_process_parent_event(iface, dag->instance, parent);
	}

	NET_DBG("Participating in a global repair version %d rank %d",
		dag->version, dag->rank);

	net_stats_update_rpl_global_repairs();
}

#define net_rpl_print_parent_info(parent, instance)			\
	do {								\
		struct net_nbr *nbr;					\
		struct net_ipv6_nbr_data *data = NULL;			\
									\
		nbr = net_rpl_get_nbr(parent);				\
		if (nbr->idx != NET_NBR_LLADDR_UNKNOWN) {		\
			data = net_ipv6_get_nbr_by_index(nbr->idx);	\
		}							\
									\
		NET_DBG("Preferred DAG %s rank %d min_rank %d "		\
			"parent rank %d parent etx %d link metric %d "	\
			"instance etx %d",				\
			net_sprint_ipv6_addr(&(instance)->current_dag-> \
					     dag_id),			\
			(instance)->current_dag->rank,			\
			(instance)->current_dag->min_rank,		\
			(parent)->rank, -1,				\
			data ? data->link_metric : 0,			\
			(instance)->mc.obj.etx);			\
	} while (0)

#if NET_RPL_MULTICAST
static void send_mcast_dao(struct net_route_entry_mcast *route,
			   void *user_data)
{
	struct net_rpl_instance *instance = user_data;

	/* Don't send if it's also our own address, done that already */
	if (!net_route_mcast_lookup(&route->group)) {
		net_rpl_dao_send(instance->current_dag->preferred_parent,
				 &route->group,
				 CONFIG_NET_RPL_MCAST_LIFETIME);
	}
}

static inline void send_mcast_dao(struct net_rpl_instance *instance)
{
	uip_mcast6_route_t *mcast_route;
	uint8_t i;

	/* Send a DAO for own multicast addresses */
	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		struct in6_addr *addr;

		addr = &instance->iface->ipv6.mcast[i].address.in6_addr;

		if (instance->iface->ipv6.mcast[i].is_used &&
		    net_is_ipv6_addr_mcast_global(addr)) {

			net_rpl_dao_send(instance->iface,
				       instance->current_dag->preferred_parent,
					 addr,
					 CONFIG_NET_RPL_MCAST_LIFETIME);
		}
	}

	/* Iterate over multicast routes and send DAOs */
	net_route_mcast_foreach(send_mcast_dao, addr, instance);
}
#endif

static void net_rpl_join_instance(struct net_if *iface,
				  struct in6_addr *from,
				  struct net_rpl_dio *dio)
{
	struct net_rpl_instance *instance;
	struct net_rpl_parent *parent;
	struct net_rpl_dag *dag;

	dag = alloc_dag(dio->instance_id, &dio->dag_id);
	if (!dag) {
		NET_DBG("Failed to allocate a DAG object!");
		return;
	}

	instance = dag->instance;

	parent = net_rpl_add_parent(iface, dag, dio, from);
	if (!parent) {
		instance->is_used = false;

		NET_DBG("Cannot add %s as a parent",
			net_sprint_ipv6_addr(from));

		return;
	}

	NET_DBG("Add %s as a parent", net_sprint_ipv6_addr(from));

	parent->dtsn = dio->dtsn;

	/* Determine the objective function by using the objective code point
	 * of the DIO.
	 */
	if (!net_rpl_of_find(dio->ocp)) {
		NET_DBG("DIO for DAG instance %d does not specify "
			"a supported OF", dio->instance_id);

		instance->is_used = false;

		net_rpl_remove_parent(iface, parent, NULL);

		return;
	}

	/* Autoconfigure an address if this node does not already have
	 * an address with this prefix.
	 */
	if (dio->prefix_info.flags & NET_ICMPV6_RA_FLAG_AUTONOMOUS) {
		check_prefix(iface, NULL, &dio->prefix_info);
	}

	net_rpl_dag_join(dag);
	net_rpl_dag_set_preference(dag, dio->preference);
	net_rpl_dag_set_grounded_status(dag, dio->grounded);
	dag->version = dio->version;

	instance->ocp = dio->ocp;
	instance->mop = dio->mop;
	instance->current_dag = dag;
	instance->dtsn = net_rpl_lollipop_init();

	instance->max_rank_inc = dio->max_rank_inc;
	instance->min_hop_rank_inc = dio->min_hop_rank_inc;
	instance->dio_interval_doublings = dio->dag_interval_doublings;
	instance->dio_interval_min = dio->dag_interval_min;
	instance->dio_interval_current =
		instance->dio_interval_min + instance->dio_interval_doublings;
	instance->dio_redundancy = dio->dag_redundancy;
	instance->default_lifetime = dio->default_lifetime;
	instance->lifetime_unit = dio->lifetime_unit;
	instance->iface = iface;

	net_ipaddr_copy(&dag->dag_id, &dio->dag_id);

	memcpy(&dag->prefix_info, &dio->prefix_info,
	       sizeof(struct net_rpl_prefix));

	net_rpl_set_preferred_parent(iface, dag, parent);

	net_rpl_of_update_mc(instance);

	dag->rank = net_rpl_of_calc_rank(parent, 0);

	/* So far this is the lowest rank we are aware of. */
	dag->min_rank = dag->rank;

	if (!rpl_default_instance) {
		rpl_default_instance = instance;
	}

	NET_DBG("Joined DAG with instance ID %d rank %d DAG ID %s",
		dio->instance_id, dag->min_rank,
		net_sprint_ipv6_addr(&dag->dag_id));

	net_rpl_reset_dio_timer(instance);
	net_rpl_set_default_route(iface, instance, from);

	if (instance->mop != NET_RPL_MOP_NO_DOWNWARD_ROUTES) {
		net_rpl_schedule_dao(instance);
	} else {
		NET_DBG("DIO does not meet the prerequisites for "
			"sending a DAO");
	}
}

static
struct net_rpl_parent *find_parent_any_dag_any_instance(struct net_if *iface,
							struct in6_addr *addr)
{
	struct net_nbr *nbr, *rpl_nbr;

	nbr = net_ipv6_nbr_lookup(iface, addr);
	if (!nbr) {
		return NULL;
	}

	rpl_nbr = nbr_lookup(&net_rpl_parents.table, iface, addr);
	if (!rpl_nbr) {
		return NULL;
	}

	return nbr_data(rpl_nbr);
}

static struct net_rpl_parent *find_parent(struct net_if *iface,
					  struct net_rpl_dag *dag,
					  struct in6_addr *addr)
{
	struct net_rpl_parent *parent;

	parent = find_parent_any_dag_any_instance(iface, addr);
	if (parent && parent->dag == dag) {
		return parent;
	}

	return NULL;
}

static struct net_rpl_dag *find_parent_dag(struct net_if *iface,
					   struct net_rpl_instance *instance,
					   struct in6_addr *addr)
{
	struct net_rpl_parent *parent;

	parent = find_parent_any_dag_any_instance(iface, addr);
	if (parent) {
		return parent->dag;
	}

	return NULL;
}

static
struct net_rpl_parent *find_parent_any_dag(struct net_if *iface,
					   struct net_rpl_instance *instance,
					   struct in6_addr *addr)
{
	struct net_rpl_parent *parent;

	parent = find_parent_any_dag_any_instance(iface, addr);
	if (parent && parent->dag && parent->dag->instance == instance) {
		return parent;
	}

	return NULL;
}

static void net_rpl_move_parent(struct net_if *iface,
				struct net_rpl_dag *dag_src,
				struct net_rpl_dag *dag_dst,
				struct net_rpl_parent *parent)
{
	struct in6_addr *addr = net_rpl_get_parent_addr(iface, parent);

	if (parent == dag_src->preferred_parent) {
		net_rpl_set_preferred_parent(iface, dag_src, NULL);
		dag_src->rank = NET_RPL_INFINITE_RANK;

		if (net_rpl_dag_is_joined(dag_src) &&
		    dag_src->instance->default_route) {
			NET_DBG("Removing default route %s",
				net_sprint_ipv6_addr(addr));

			net_if_router_rm(dag_src->instance->default_route);
			dag_src->instance->default_route = NULL;
		}

	} else if (net_rpl_dag_is_joined(dag_src)) {
		/* Remove routes that have this parent as the next hop and
		 * which has correct DAG pointer.
		 */
		net_route_del_by_nexthop_data(iface, addr, dag_src);
	}

	NET_DBG("Moving parent %s", net_sprint_ipv6_addr(addr));

	parent->dag = dag_dst;
}

static void net_rpl_link_neighbor_callback(struct net_if *iface,
					   struct net_linkaddr *lladdr,
					   int status)
{
	struct net_rpl_instance *instance;
	struct in6_addr addr;

	net_ipv6_addr_create_iid(&addr, lladdr);

	for (instance = &rpl_instances[0];
	     instance < &rpl_instances[0] + CONFIG_NET_RPL_MAX_INSTANCES;
	     instance++) {
		struct net_rpl_parent *parent;

		if (!instance->is_used) {
			continue;
		}

		parent = find_parent_any_dag(iface, instance, &addr);
		if (parent) {
			/* Trigger DAG rank recalculation. */
			NET_DBG("Neighbor link callback triggering update");

			parent->flags |= NET_RPL_PARENT_FLAG_UPDATED;

			/* FIXME - Last parameter value (number of
			 * transmissions) needs adjusting if possible.
			 */
			net_rpl_of_neighbor_link_cb(iface, parent, status, 1);

			parent->last_tx_time = k_uptime_get_32();
		}
	}
}

#if CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE > 1
static void net_rpl_add_dag(struct net_if *iface,
			    struct in6_addr *from,
			    struct net_rpl_dio *dio)
{
	struct net_rpl_instance *instance;
	struct net_rpl_dag *dag, *previous_dag;
	struct net_rpl_parent *parent;

	dag = alloc_dag(dio->instance_id, &dio->dag_id);
	if (!dag) {
		NET_DBG("Failed to allocate a DAG object!");
		return;
	}

	instance = dag->instance;

	previous_dag = find_parent_dag(iface, instance, from);
	if (!previous_dag) {
		parent = net_rpl_add_parent(iface, dag, dio, from);
		if (!parent) {
			NET_DBG("Adding %s as a parent failed.",
				net_sprint_ipv6_addr(from));

			net_rpl_dag_set_not_used(dag);
			return;
		}

		NET_DBG("Adding %s as a parent.",
			net_sprint_ipv6_addr(from));
	} else {
		parent = find_parent(iface, previous_dag, from);
		if (parent) {
			net_rpl_move_parent(iface, previous_dag, dag, parent);
		}
	}

	if (net_rpl_of_find(dio->ocp) ||
	    instance->mop != dio->mop ||
	    instance->max_rank_inc != dio->max_rank_inc ||
	    instance->min_hop_rank_inc != dio->min_hop_rank_inc ||
	    instance->dio_interval_doublings != dio->dag_interval_doublings ||
	    instance->dio_interval_min != dio->dag_interval_min ||
	    instance->dio_redundancy != dio->dag_redundancy ||
	    instance->default_lifetime != dio->default_lifetime ||
	    instance->lifetime_unit != dio->lifetime_unit) {
		NET_DBG("DIO for DAG instance %u incompatible with "
			"previous DIO", dio->instance_id);
		net_rpl_remove_parent(iface, parent, NULL);
		net_rpl_dag_set_not_used(dag);

		return;
	}

	net_rpl_dag_set_used(dag);
	net_rpl_dag_set_grounded_status(dag, dio->grounded);
	net_rpl_dag_set_preference(dag, dio->preference);
	dag->version = dio->version;

	net_ipaddr_copy(&dag->dag_id, &dio->dag_id);

	memcpy(&dag->prefix_info, &dio->prefix_info,
	       sizeof(struct net_rpl_prefix));

	net_rpl_set_preferred_parent(iface, dag, parent);

	dag->rank = net_rpl_of_calc_rank(parent, 0);

	/* So far this is the lowest rank we are aware of. */
	dag->min_rank = dag->rank;

	NET_DBG("Joined DAG with instance ID %d rank %d DAG ID %s",
		dio->instance_id, dag->min_rank,
		net_sprint_ipv6_addr(&dag->dag_id));

	net_rpl_process_parent_event(iface, instance, parent);
	parent->dtsn = dio->dtsn;
}
#endif /* CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE > 1 */

static int should_send_dao(struct net_rpl_instance *instance,
			   struct net_rpl_dio *dio,
			   struct net_rpl_parent *parent)
{
	/* If MOP is set to no downward routes no DAO should be sent. */
	if (instance->mop == NET_RPL_MOP_NO_DOWNWARD_ROUTES) {
		return 0;
	}

	/* Check if the new DTSN is more recent */
	return parent == instance->current_dag->preferred_parent &&
		(lollipop_greater_than(dio->dtsn, parent->dtsn));
}

static void net_rpl_process_dio(struct net_if *iface,
				struct in6_addr *from,
				struct net_rpl_dio *dio)
{
	struct net_rpl_dag *dag, *previous_dag;
	struct net_rpl_instance *instance;
	struct net_rpl_parent *parent;

#if defined(CONFIG_NET_RPL_MOP3)
	/* If the root is advertising MOP 2 but we support MOP 3 we can still
	 * join. In that scenario, we suppress DAOs for multicast targets.
	 */
	if (dio->mop < NET_RPL_MOP_STORING_NO_MULTICAST)
#else
	if (dio->mop != NET_RPL_MOP_DEFAULT)
#endif
	{
		NET_DBG("Ignoring a DIO with an unsupported MOP %d", dio->mop);
		return;
	}

	dag = get_dag(dio->instance_id, &dio->dag_id);
	instance = net_rpl_get_instance(dio->instance_id);

	if (dag && instance) {
		if (lollipop_greater_than(dio->version, dag->version)) {
			if (dag->rank == NET_RPL_ROOT_RANK(instance)) {
				uint8_t version;

				NET_DBG("Root received inconsistent DIO "
					"version number %d rank %d",
					dio->version, dag->rank);

				version = dio->version;
				net_rpl_lollipop_increment(&version);
				dag->version = version;

				net_rpl_reset_dio_timer(instance);
			} else {
				NET_DBG("Global repair");

				if (dio->prefix_info.length != 0 &&
				    dio->prefix_info.flags &
				    NET_ICMPV6_RA_FLAG_AUTONOMOUS) {
					NET_DBG("Prefix announced in DIO");

					net_rpl_set_prefix(iface, dag,
						      &dio->prefix_info.prefix,
						      dio->prefix_info.length);
				}

				global_repair(iface, from, dag, dio);
			}

			return;
		}

		if (lollipop_greater_than(dag->version, dio->version)) {
			/* The DIO sender is on an older version of the DAG. */
			NET_DBG("old version received => inconsistency "
				"detected");
			if (net_rpl_dag_is_joined(dag)) {
				net_rpl_reset_dio_timer(instance);
				return;
			}
		}
	}

	if (!instance) {
		/* Join the RPL DAG if there is no join callback or the join
		 * callback tells us to join.
		 */
		if (!rpl_join_callback || rpl_join_callback(dio)) {
			NET_DBG("New instance detected: joining...");
			net_rpl_join_instance(iface, from, dio);
		} else {
			NET_DBG("New instance detected: not joining, "
				"rejected by join callback");
		}

		return;
	}

	if (instance->current_dag->rank == NET_RPL_ROOT_RANK(instance) &&
	    instance->current_dag != dag) {
		NET_DBG("Root ignored DIO for different DAG");
		return;
	}

	if (!dag) {
#if CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE > 1
		NET_DBG("Adding new DAG to known instance.");
		net_rpl_add_dag(iface, from, dio);
#else
		NET_DBG("Only one instance supported.");
#endif /* CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE > 1 */

		return;
	}

	if (dio->rank < NET_RPL_ROOT_RANK(instance)) {
		NET_DBG("Ignoring DIO with too low rank %d", dio->rank);
		return;

	} else if (dio->rank == NET_RPL_INFINITE_RANK &&
		   net_rpl_dag_is_joined(dag)) {
		net_rpl_reset_dio_timer(instance);
	}

	/* Prefix Information Option treated to add new prefix */
	if (dio->prefix_info.length != 0) {
		if (dio->prefix_info.flags & NET_ICMPV6_RA_FLAG_AUTONOMOUS) {
			NET_DBG("Prefix announced in DIO");

			net_rpl_set_prefix(iface, dag, &dio->prefix_info.prefix,
					   dio->prefix_info.length);
		}
	}

	if (dag->rank == NET_RPL_ROOT_RANK(instance)) {
		if (dio->rank != NET_RPL_INFINITE_RANK) {
			instance->dio_counter++;
		}

		return;
	}

	/*
	 * At this point, we know that this DIO pertains to a DAG that
	 * we are already part of. We consider the sender of the DIO to be
	 * a candidate parent, and let net_rpl_process_parent_event decide
	 * whether to keep it in the set.
	 */

	parent = find_parent(iface, dag, from);
	if (!parent) {
		previous_dag = find_parent_dag(iface, instance, from);
		if (!previous_dag) {
			/* Add the DIO sender as a candidate parent. */
			parent = net_rpl_add_parent(iface, dag, dio, from);
			if (!parent) {
				NET_DBG("Failed to add a new parent %s",
					net_sprint_ipv6_addr(from));
				return;
			}

			NET_DBG("New candidate parent %s with rank %d",
				net_sprint_ipv6_addr(from), parent->rank);
		} else {
			parent = find_parent(iface, previous_dag, from);
			if (parent) {
				net_rpl_move_parent(iface, previous_dag,
						    dag, parent);
			}
		}
	} else {
		if (parent->rank == dio->rank) {
			NET_DBG("Received consistent DIO");

			if (net_rpl_dag_is_joined(dag)) {
				instance->dio_counter++;
			}
		} else {
			parent->rank = dio->rank;
		}
	}

	/* Parent info has been updated, trigger rank recalculation */
	parent->flags |= NET_RPL_PARENT_FLAG_UPDATED;

	net_rpl_print_parent_info(parent, instance);

	/* We have allocated a candidate parent; process the DIO further. */

#if !defined(CONFIG_NET_RPL_DAG_MC_NONE)
	memcpy(&parent->mc, &dio->mc, sizeof(parent->mc));
#endif

	if (!net_rpl_process_parent_event(iface, instance, parent)) {
		NET_DBG("The candidate parent is rejected.");
		return;
	}

	/* We don't use route control, so we can have only one official
	 * parent.
	 */
	if (net_rpl_dag_is_joined(dag) && parent == dag->preferred_parent) {
		if (should_send_dao(instance, dio, parent)) {
			net_rpl_lollipop_increment(&instance->dtsn);
			net_rpl_schedule_dao(instance);
		}

		/* We received a new DIO from our preferred parent.
		 * Add default route to set a fresh value for the lifetime
		 * counter.
		 */
		net_if_ipv6_router_add(iface, from,
				       net_rpl_lifetime(instance,
						instance->default_lifetime));
	}

	parent->dtsn = dio->dtsn;
}

static enum net_verdict handle_dio(struct net_buf *buf)
{
	struct net_rpl_dio dio = { 0 };
	struct net_buf *frag;
	struct net_nbr *nbr;
	uint16_t offset, pos;
	uint8_t subopt_type;
	uint8_t flags, len, tmp;

	net_rpl_info(buf, "DODAG Information Object");

	/* Default values can be overwritten by DIO config option.
	 */
	dio.dag_interval_doublings = CONFIG_NET_RPL_DIO_INTERVAL_DOUBLINGS;
	dio.dag_interval_min = CONFIG_NET_RPL_DIO_INTERVAL_MIN;
	dio.dag_redundancy = CONFIG_NET_RPL_DIO_REDUNDANCY;
	dio.min_hop_rank_inc = CONFIG_NET_RPL_MIN_HOP_RANK_INC;
	dio.max_rank_inc = NET_RPL_MAX_RANK_INC;
	dio.ocp = net_rpl_of_get();
	dio.default_lifetime = CONFIG_NET_RPL_DEFAULT_LIFETIME;
	dio.lifetime_unit = CONFIG_NET_RPL_DEFAULT_LIFETIME_UNIT;

	nbr = net_ipv6_nbr_lookup(net_nbuf_iface(buf),
				  &NET_IPV6_BUF(buf)->src);
	if (!nbr) {
		NET_ASSERT_INFO(net_nbuf_ll_src(buf)->len,
				"Link layer address not set");

		nbr = net_ipv6_nbr_add(net_nbuf_iface(buf),
				       &NET_IPV6_BUF(buf)->src,
				       net_nbuf_ll_src(buf),
				       0,
				       NET_NBR_REACHABLE);
		if (!nbr) {
			NET_DBG("Cannot add neighbor by DIO");
			goto out;
		}

		net_ipv6_nbr_set_reachable_timer(net_nbuf_iface(buf), nbr);
	}

	/* offset tells now where the ICMPv6 header is starting */
	offset = net_nbuf_icmp_data(buf) - net_nbuf_ip_data(buf);

	offset += sizeof(struct net_icmp_hdr);

	/* First the DIO option. */
	frag = net_nbuf_read_u8(buf->frags, offset, &pos, &dio.instance_id);
	frag = net_nbuf_read_u8(frag, pos, &pos, &dio.version);
	frag = net_nbuf_read_be16(frag, pos, &pos, &dio.rank);

	NET_DBG("Incoming DIO len %d id %d ver %d rank %d",
		net_buf_frags_len(buf) - offset,
		dio.instance_id, dio.version, dio.rank);

	frag = net_nbuf_read_u8(frag, pos, &pos, &flags);

	dio.grounded = flags & NET_RPL_DIO_GROUNDED;
	dio.mop = (flags & NET_RPL_DIO_MOP_MASK) >> NET_RPL_DIO_MOP_SHIFT;
	dio.preference = flags & NET_RPL_DIO_PREFERENCE_MASK;

	frag = net_nbuf_read_u8(frag, pos, &pos, &dio.dtsn);

	/* two reserved bytes */
	frag = net_nbuf_skip(frag, pos, &pos, 2);

	frag = net_nbuf_read(frag, pos, &pos, sizeof(dio.dag_id),
			     dio.dag_id.s6_addr);

	NET_DBG("Incoming DIO dag_id %s pref %d",
		net_sprint_ipv6_addr(&dio.dag_id), dio.preference);

	/* Handle any DIO suboptions */
	while (frag) {
		frag = net_nbuf_read_u8(frag, pos, &pos, &subopt_type);

		if (pos == 0) {
			/* We are at the end of the message */
			frag = NULL;
			break;
		}

		if (subopt_type == NET_RPL_OPTION_PAD1) {
			len = 1;
		} else {
			/* Suboption with a two-byte header + payload */
			frag = net_nbuf_read_u8(frag, pos, &pos, &tmp);

			len = 2 + tmp;
		}

		if (!frag && pos) {
			NET_DBG("Invalid DIO packet");
			net_stats_update_rpl_malformed_msgs();
			goto out;
		}

		NET_DBG("DIO option %u length %d", subopt_type, len - 2);

		switch (subopt_type) {
		case NET_RPL_OPTION_DAG_METRIC_CONTAINER:
			if (len < 6) {
				NET_DBG("Invalid DAG MC len %d", len);
				net_stats_update_rpl_malformed_msgs();
				goto out;
			}

			frag = net_nbuf_read_u8(frag, pos, &pos, &dio.mc.type);
			frag = net_nbuf_read_u8(frag, pos, &pos, &tmp);
			dio.mc.flags = tmp << 1;
			frag = net_nbuf_read_u8(frag, pos, &pos, &tmp);
			dio.mc.flags |= tmp >> 7;
			dio.mc.aggregated = (tmp >> 4) & 0x3;
			dio.mc.precedence = tmp & 0xf;
			frag = net_nbuf_read_u8(frag, pos, &pos,
						&dio.mc.length);

			if (dio.mc.type == NET_RPL_MC_ETX) {
				frag = net_nbuf_read_be16(frag, pos, &pos,
							  &dio.mc.obj.etx);

				NET_DBG("DAG MC type %d flags %d aggr %d "
					"prec %d length %d ETX %d",
					dio.mc.type, dio.mc.flags,
					dio.mc.aggregated,
					dio.mc.precedence, dio.mc.length,
					dio.mc.obj.etx);

			} else if (dio.mc.type == NET_RPL_MC_ENERGY) {
				frag = net_nbuf_read_u8(frag, pos, &pos,
						     &dio.mc.obj.energy.flags);
				frag = net_nbuf_read_u8(frag, pos, &pos,
						&dio.mc.obj.energy.estimation);
			} else {
				NET_DBG("Unhandled DAG MC type %d",
					dio.mc.type);
				goto out;
			}

			break;
		case NET_RPL_OPTION_ROUTE_INFO:
			if (len < 9) {
				NET_DBG("Invalid destination prefix "
					"option len %d", len);
				net_stats_update_rpl_malformed_msgs();
				goto out;
			}

			frag = net_nbuf_read_u8(frag, pos, &pos,
					&dio.destination_prefix.length);
			frag = net_nbuf_read_u8(frag, pos, &pos,
					&dio.destination_prefix.flags);
			frag = net_nbuf_read_be32(frag, pos, &pos,
					&dio.destination_prefix.lifetime);

			if (((dio.destination_prefix.length + 7) / 8) + 8 <=
			    len && dio.destination_prefix.length <= 128) {
				frag = net_nbuf_read(frag, pos, &pos,
				       (dio.destination_prefix.length + 7) / 8,
				       dio.destination_prefix.prefix.s6_addr);

				NET_DBG("Copying destination prefix %s/%d",
					net_sprint_ipv6_addr(
						&dio.destination_prefix.prefix),
					dio.destination_prefix.length);
			} else {
				NET_DBG("Invalid route info option len %d",
					len);
				net_stats_update_rpl_malformed_msgs();
				goto out;
			}

			break;
		case NET_RPL_OPTION_DAG_CONF:
			if (len != 16) {
				NET_DBG("Invalid DAG configuration option "
					"len %d", len);
				net_stats_update_rpl_malformed_msgs();
				goto out;
			}

			/* Path control field not yet implemented (1 byte) */
			frag = net_nbuf_skip(frag, pos, &pos, 1);

			frag = net_nbuf_read_u8(frag, pos, &pos,
					     &dio.dag_interval_doublings);
			frag = net_nbuf_read_u8(frag, pos, &pos,
					     &dio.dag_interval_min);
			frag = net_nbuf_read_u8(frag, pos, &pos,
					     &dio.dag_redundancy);
			frag = net_nbuf_read_be16(frag, pos, &pos,
						  &dio.max_rank_inc);
			frag = net_nbuf_read_be16(frag, pos, &pos,
						  &dio.min_hop_rank_inc);
			frag = net_nbuf_read_be16(frag, pos, &pos,
						  &dio.ocp);

			/* one reserved byte */
			frag = net_nbuf_skip(frag, pos, &pos, 1);

			frag = net_nbuf_read_u8(frag, pos, &pos,
					     &dio.default_lifetime);
			frag = net_nbuf_read_be16(frag, pos, &pos,
						  &dio.lifetime_unit);

			NET_DBG("DAG conf dbl %d min %d red %d maxinc %d "
				"mininc %d ocp %d d_l %d l_u %d",
				dio.dag_interval_doublings,
				dio.dag_interval_min,
				dio.dag_redundancy, dio.max_rank_inc,
				dio.min_hop_rank_inc, dio.ocp,
				dio.default_lifetime, dio.lifetime_unit);

			break;
		case NET_RPL_OPTION_PREFIX_INFO:
			if (len != 32) {
				NET_DBG("Invalid DAG prefix info len %d != 32",
					len);
				net_stats_update_rpl_malformed_msgs();
				goto out;
			}

			frag = net_nbuf_read_u8(frag, pos, &pos,
					     &dio.prefix_info.length);
			frag = net_nbuf_read_u8(frag, pos, &pos,
					     &dio.prefix_info.flags);

			/* skip valid lifetime atm */
			frag = net_nbuf_skip(frag, pos, &pos, 4);

			/* preferred lifetime stored in lifetime */
			frag = net_nbuf_read_be32(frag, pos, &pos,
						  &dio.prefix_info.lifetime);

			/* 32-bit reserved */
			frag = net_nbuf_skip(frag, pos, &pos, 4);

			frag = net_nbuf_read(frag, pos, &pos,
					     sizeof(struct in6_addr),
					     dio.prefix_info.prefix.s6_addr);

			NET_DBG("Prefix %s/%d",
				net_sprint_ipv6_addr(&dio.prefix_info.prefix),
				dio.prefix_info.length);

			break;
		default:
			NET_DBG("Unsupported suboption type in DIO %d",
				subopt_type);
		}
	}

	NET_ASSERT_INFO(!pos && !frag, "DIO reading failure");

	net_rpl_process_dio(net_nbuf_iface(buf), &NET_IPV6_BUF(buf)->src,
			    &dio);

out:
	return NET_DROP;
}

int net_rpl_dao_send(struct net_if *iface,
		     struct net_rpl_parent *parent,
		     struct in6_addr *prefix,
		     uint8_t lifetime)
{
	uint16_t value = 0;
	struct net_rpl_instance *instance;
	const struct in6_addr *src;
	struct net_rpl_dag *dag;
	struct in6_addr *dst;
	struct net_buf *buf;
	uint8_t prefix_bytes;
	uint8_t prefixlen;
	int ret;

	/* No DAOs in feather mode. */
	if (net_rpl_get_mode() == NET_RPL_MODE_FEATHER) {
		return -EINVAL;
	}

	if (!parent || !parent->dag) {
		NET_DBG("DAO error parent %p dag %p",
			parent, parent ? parent->dag : NULL);
		return -EINVAL;
	}

	dag = parent->dag;

	instance = dag->instance;
	if (!instance) {
		NET_DBG("RPL DAO error no instance");
		return -EINVAL;
	}

	dst = net_rpl_get_parent_addr(iface, parent);
	if (!dst) {
		NET_DBG("No destination address for parent %p", parent);
		return -EINVAL;
	}

	src = net_if_ipv6_select_src_addr(iface, dst);

	if (net_ipv6_addr_cmp(src, net_ipv6_unspecified_address())) {
		NET_DBG("Invalid src addr %s found",
			net_sprint_ipv6_addr(src));
		return -EINVAL;
	}

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return -ENOMEM;
	}

	buf = net_ipv6_create_raw(buf, net_if_get_ll_reserve(iface, src),
				  src, dst, iface, IPPROTO_ICMPV6);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_if_get_ll_reserve(iface, dst));

	setup_icmpv6_hdr(buf, NET_ICMPV6_RPL, NET_RPL_DEST_ADV_OBJ);

	net_rpl_lollipop_increment(&rpl_dao_sequence);

	net_nbuf_append_u8(buf, instance->instance_id);

#if defined(CONFIG_NET_RPL_DAO_SPECIFY_DAG)
	value |= NET_RPL_DAO_D_FLAG;
#endif

#if defined(CONFIG_NET_RPL_DAO_ACK)
	value |= NET_RPL_DAO_K_FLAG;
#endif
	net_nbuf_append_u8(buf, value);
	net_nbuf_append_u8(buf, 0); /* reserved */
	net_nbuf_append_u8(buf, rpl_dao_sequence);

#if defined(CONFIG_NET_RPL_DAO_SPECIFY_DAG)
	net_nbuf_append(buf, sizeof(dag->dag_id), dag->dag_id.s6_addr);
#endif

	prefixlen = sizeof(*prefix) * CHAR_BIT;
	prefix_bytes = (prefixlen + 7) / CHAR_BIT;

	net_nbuf_append_u8(buf, NET_RPL_OPTION_TARGET);
	net_nbuf_append_u8(buf, 2 + prefix_bytes);
	net_nbuf_append_u8(buf, 0); /* reserved */
	net_nbuf_append_u8(buf, prefixlen);
	net_nbuf_append(buf, prefix_bytes, prefix->s6_addr);

	net_nbuf_append_u8(buf, NET_RPL_OPTION_TRANSIT);
	net_nbuf_append_u8(buf, 4); /* length */
	net_nbuf_append_u8(buf, 0); /* flags */
	net_nbuf_append_u8(buf, 0); /* path control */
	net_nbuf_append_u8(buf, 0); /* path seq */
	net_nbuf_append_u8(buf, lifetime);

	buf = net_ipv6_finalize_raw(buf, IPPROTO_ICMPV6);

	ret = net_send_data(buf);
	if (ret >= 0) {
		net_rpl_dao_info(buf, src, dst, prefix);

		net_stats_update_icmp_sent();
		net_stats_update_rpl_dao_sent();
	} else {
		net_nbuf_unref(buf);
	}

	return ret;
}

static int dao_send(struct net_rpl_parent *parent,
		    uint8_t lifetime,
		    struct net_if *iface)
{
	struct in6_addr *prefix;

	prefix = net_if_ipv6_get_global_addr(&iface);
	if (!prefix) {
		NET_DBG("Will not send DAO as no global address was found.");
		return -EDESTADDRREQ;
	}

	NET_ASSERT_INFO(iface, "Interface not set");

	return net_rpl_dao_send(iface, parent, prefix, lifetime);
}

static inline int dao_forward(struct net_if *iface,
			      struct net_buf *orig,
			      struct in6_addr *dst)
{
	struct net_buf *buf;
	int ret;

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return -ENOMEM;
	}

	/* Steal the fragment chain */
	buf->frags = orig->frags;
	orig->frags = NULL;

	net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, dst);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_if_get_ll_reserve(iface, dst));

	ret = net_send_data(buf);
	if (ret >= 0) {
		net_stats_update_icmp_sent();
		net_stats_update_rpl_dao_forwarded();
	} else {
		net_nbuf_unref(buf);
	}

	return ret;
}

static int dao_ack_send(struct net_buf *orig,
			struct net_rpl_instance *instance,
			struct in6_addr *dst,
			uint8_t sequence)
{
	struct in6_addr *src = &NET_IPV6_BUF(orig)->dst;
	struct net_if *iface = net_nbuf_iface(orig);
	struct net_buf *buf;
	int ret;

	NET_DBG("Sending a DAO ACK with sequence number %d to %s",
		sequence, net_sprint_ipv6_addr(dst));

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return -ENOMEM;
	}

	buf = net_ipv6_create_raw(buf, net_if_get_ll_reserve(iface, src),
				  src, dst, iface, IPPROTO_ICMPV6);

	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_if_get_ll_reserve(iface, dst));

	setup_icmpv6_hdr(buf, NET_ICMPV6_RPL, NET_RPL_DEST_ADV_OBJ_ACK);

	net_nbuf_append_u8(buf, instance->instance_id);
	net_nbuf_append_u8(buf, 0); /* reserved */
	net_nbuf_append_u8(buf, sequence);
	net_nbuf_append_u8(buf, 0); /* status */

	buf = net_ipv6_finalize_raw(buf, IPPROTO_ICMPV6);

	ret = net_send_data(buf);
	if (ret >= 0) {
		net_rpl_dao_ack_info(buf, src, dst, instance->instance_id,
				     sequence);

		net_stats_update_icmp_sent();
		net_stats_update_rpl_dao_ack_sent();
	} else {
		net_nbuf_unref(buf);
	}

	return 0;
}

static void forwarding_dao(struct net_rpl_instance *instance,
			   struct net_rpl_dag *dag,
			   struct in6_addr *dao_sender,
			   struct net_buf *buf,
			   uint8_t sequence,
			   uint8_t flags,
			   char *str)
{
	struct in6_addr *paddr;

	paddr = net_rpl_get_parent_addr(instance->iface,
					dag->preferred_parent);
	if (paddr) {
		NET_DBG("%s %s", str, net_sprint_ipv6_addr(paddr));

		dao_forward(dag->instance->iface, buf, paddr);

		if (flags & NET_RPL_DAO_K_FLAG) {
			dao_ack_send(buf, instance, dao_sender, sequence);
		}
	}
}

static enum net_verdict handle_dao(struct net_buf *buf)
{
	struct in6_addr *dao_sender = &NET_IPV6_BUF(buf)->src;
	struct net_rpl_route_entry *extra = NULL;
	struct net_rpl_parent *parent = NULL;
	enum net_rpl_route_source learned_from;
	struct net_rpl_instance *instance;
	struct net_route_entry *route;
	struct net_rpl_dag *dag;
	struct net_buf *frag;
	struct in6_addr addr;
	struct net_nbr *nbr;
	uint16_t offset;
	uint16_t pos;
	uint8_t sequence;
	uint8_t instance_id;
	uint8_t lifetime;
	uint8_t target_len;
	uint8_t flags;
	uint8_t subopt_type;
	int len;

	net_rpl_info(buf, "Destination Advertisement Object");

	/* offset tells now where the ICMPv6 header is starting */
	offset = net_nbuf_icmp_data(buf) - net_nbuf_ip_data(buf);

	offset += sizeof(struct net_icmp_hdr);

	frag = net_nbuf_read_u8(buf->frags, offset, &pos, &instance_id);

	instance = net_rpl_get_instance(instance_id);
	if (!instance) {
		NET_DBG("Ignoring DAO for an unknown instance %d",
			instance_id);
		return NET_DROP;
	}

	lifetime = instance->default_lifetime;

	frag = net_nbuf_read_u8(frag, pos, &pos, &flags);
	frag = net_nbuf_skip(frag, pos, &pos, 1); /* reserved */
	frag = net_nbuf_read_u8(frag, pos, &pos, &sequence);

	dag = instance->current_dag;

	/* Is the DAG ID present? */
	if (flags & NET_RPL_DAO_D_FLAG) {
		frag = net_nbuf_read(frag, pos, &pos, sizeof(addr),
				     addr.s6_addr);

		if (memcmp(&dag->dag_id, &addr, sizeof(dag->dag_id))) {
			NET_DBG("Ignoring DAO for a DAG %s different from ours",
				net_sprint_ipv6_addr(&dag->dag_id));
			return NET_DROP;
		}
	}

	learned_from = net_is_ipv6_addr_mcast(dao_sender) ?
		NET_RPL_ROUTE_MULTICAST_DAO :
		NET_RPL_ROUTE_UNICAST_DAO;

	NET_DBG("DAO from %s", learned_from == NET_RPL_ROUTE_UNICAST_DAO ?
		"unicast" : "multicast");

	if (learned_from == NET_RPL_ROUTE_UNICAST_DAO) {
		/* Check whether this is a DAO forwarding loop. */
		parent = find_parent(instance->iface, dag, dao_sender);

		/* Check if this is a new DAO registration with an "illegal"
		 * rank if we already route to this node it is likely.
		 */
		if (parent &&
		    NET_RPL_DAG_RANK(parent->rank, instance) <
		    NET_RPL_DAG_RANK(dag->rank, instance)) {
			NET_DBG("Loop detected when receiving a unicast DAO "
				"from a node with a lower rank! (%d < %d)",
				NET_RPL_DAG_RANK(parent->rank, instance),
				NET_RPL_DAG_RANK(dag->rank, instance));
			parent->rank = NET_RPL_INFINITE_RANK;
			parent->flags |= NET_RPL_PARENT_FLAG_UPDATED;
			return NET_DROP;
		}

		/* If we get the DAO from our parent, we also have a loop. */
		if (parent && parent == dag->preferred_parent) {
			NET_DBG("Loop detected when receiving a unicast DAO "
				"from our parent");
			parent->rank = NET_RPL_INFINITE_RANK;
			parent->flags |= NET_RPL_PARENT_FLAG_UPDATED;
			return NET_DROP;
		}
	}

	target_len = 0;

	/* Handle any DAO suboptions */
	while (frag) {
		frag = net_nbuf_read_u8(frag, pos, &pos, &subopt_type);

		if (pos == 0) {
			/* We are at the end of the message */
			frag = NULL;
			break;
		}

		if (subopt_type == NET_RPL_OPTION_PAD1) {
			len = 1;
		} else {
			uint8_t tmp;

			/* Suboption with a two-byte header + payload */
			frag = net_nbuf_read_u8(frag, pos, &pos, &tmp);

			len = 2 + tmp;
		}

		if (!frag && pos) {
			NET_DBG("Invalid DAO packet");
			net_stats_update_rpl_malformed_msgs();
			goto out;
		}

		NET_DBG("DAO option %u length %d", subopt_type, len - 2);

		switch (subopt_type) {
		case NET_RPL_OPTION_TARGET:
			frag = net_nbuf_skip(frag, pos, &pos, 1); /* reserved */
			frag = net_nbuf_read_u8(frag, pos, &pos, &target_len);
			frag = net_nbuf_read(frag, pos, &pos,
					     (target_len + 7) / 8,
					     addr.s6_addr);
			break;
		case NET_RPL_OPTION_TRANSIT:
			/* The path sequence and control are ignored. */
			frag = net_nbuf_skip(frag, pos, &pos, 2);
			frag = net_nbuf_read_u8(frag, pos, &pos, &lifetime);
			break;
		}
	}

	NET_DBG("DAO lifetime %d addr %s/%d", lifetime,
		net_sprint_ipv6_addr(&addr), target_len);

#if NET_RPL_MULTICAST
	if (net_is_ipv6_addr_mcast_global(&addr)) {
		struct net_route_entry_mcast *mcast_group;

		mcast_group = net_route_mcast_add(net_nbuf_iface(buf), &addr);
		if (mcast_group) {
			mcast_group->data = (void *)dag;
			mcast_group->lifetime = net_rpl_lifetime(instance,
								 lifetime);
		}

		goto fwd_dao;
	}
#endif

	route = net_route_lookup(net_nbuf_iface(buf), &addr);

	if (lifetime == NET_RPL_ZERO_LIFETIME) {
		struct in6_addr *nexthop;

		NET_DBG("No-Path DAO received");

		nbr = net_route_get_nbr(route);
		extra = net_nbr_extra_data(nbr);
		nexthop = net_route_get_nexthop(route);

		/* No-Path DAO received; invoke the route purging routine. */
		if (route && !extra->no_path_received &&
		    route->prefix_len == target_len && nexthop &&
		    net_ipv6_addr_cmp(nexthop, dao_sender)) {
			NET_DBG("Setting expiration timer for target %s",
				net_sprint_ipv6_addr(&addr));

			extra->no_path_received = true;
			extra->lifetime = NET_RPL_DAO_EXPIRATION_TIMEOUT;

			/* We forward the incoming no-path DAO to our parent,
			 * if we have one.
			 */
			if (dag->preferred_parent) {
				forwarding_dao(instance, dag, dao_sender,
					       buf, sequence, flags,
#if defined(CONFIG_NET_DEBUG_RPL)
					       "Forwarding no-path DAO to "
					       "parent"
#else
					       ""
#endif
					);
			}
		}

		return NET_DROP;
	}

	NET_DBG("Adding DAO route");

	nbr = net_ipv6_nbr_lookup(net_nbuf_iface(buf), dao_sender);
	if (!nbr) {
		nbr = net_ipv6_nbr_add(net_nbuf_iface(buf), dao_sender,
				       net_nbuf_ll_src(buf), false,
				       NET_NBR_REACHABLE);
		if (nbr) {
			/* Set reachable timer */
			net_ipv6_nbr_set_reachable_timer(net_nbuf_iface(buf),
							 nbr);

			NET_DBG("Neighbor %s [%s] added to neighbor cache",
				net_sprint_ipv6_addr(dao_sender),
				net_sprint_ll_addr(net_nbuf_ll_src(buf)->addr,
						   net_nbuf_ll_src(buf)->len));
		} else {
			NET_DBG("Out of memory, dropping DAO from %s [%s]",
				net_sprint_ipv6_addr(dao_sender),
				net_sprint_ll_addr(net_nbuf_ll_src(buf)->addr,
						   net_nbuf_ll_src(buf)->len));
			return NET_DROP;
		}
	} else {
		NET_DBG("Neighbor %s [%s] already in neighbor cache",
			net_sprint_ipv6_addr(dao_sender),
			net_sprint_ll_addr(net_nbuf_ll_src(buf)->addr,
					   net_nbuf_ll_src(buf)->len));
	}

	route = net_rpl_add_route(dag, net_nbuf_iface(buf),
				  &addr, target_len, dao_sender);
	if (!route) {
		net_stats_update_rpl_mem_overflows();

		NET_DBG("Could not add a route after receiving a DAO");
		return NET_DROP;
	}

	if (extra) {
		extra->lifetime = net_rpl_lifetime(instance, lifetime);
		extra->route_source = learned_from;
		extra->no_path_received = false;
	}

#if NET_RPL_MULTICAST
fwd_dao:
#endif

	if (learned_from == NET_RPL_ROUTE_UNICAST_DAO) {
		if (dag->preferred_parent) {
			forwarding_dao(instance, dag,
				       dao_sender, buf,
				       sequence, flags,
#if defined(CONFIG_NET_DEBUG_RPL)
				       "Forwarding DAO to parent"
#else
				       ""
#endif
				);
		}
	}

out:
	return NET_DROP;
}

static enum net_verdict handle_dao_ack(struct net_buf *buf)
{
	net_rpl_info(buf, "Destination Advertisement Object Ack");

	net_stats_update_rpl_dao_ack_recv();

	return NET_DROP;
}

static struct net_icmpv6_handler dodag_info_solicitation_handler = {
	.type = NET_ICMPV6_RPL,
	.code = NET_RPL_DODAG_SOLICIT,
	.handler = handle_dis,
};

static struct net_icmpv6_handler dodag_information_object_handler = {
	.type = NET_ICMPV6_RPL,
	.code = NET_RPL_DODAG_INFO_OBJ,
	.handler = handle_dio,
};

static struct net_icmpv6_handler destination_advertisement_object_handler = {
	.type = NET_ICMPV6_RPL,
	.code = NET_RPL_DEST_ADV_OBJ,
	.handler = handle_dao,
};

static struct net_icmpv6_handler dao_ack_handler = {
	.type = NET_ICMPV6_RPL,
	.code = NET_RPL_DEST_ADV_OBJ_ACK,
	.handler = handle_dao_ack,
};

int net_rpl_update_header(struct net_buf *buf, struct in6_addr *addr)
{
	uint16_t pos = 0;
	struct net_rpl_parent *parent;
	struct net_buf *frag;
	uint8_t next;
	uint8_t len;

	frag = buf->frags;

	if (NET_IPV6_BUF(buf)->nexthdr == NET_IPV6_NEXTHDR_HBHO) {
		/* The HBHO will start right after IPv6 header */
		frag = net_nbuf_skip(frag, pos, &pos,
				     sizeof(struct net_ipv6_hdr));
		if (!frag && pos) {
			/* Not enough data in the message */
			return -EMSGSIZE;
		}

		frag = net_nbuf_read(frag, pos, &pos, 1, &next);
		frag = net_nbuf_read(frag, pos, &pos, 1, &len);
		if (!frag && pos) {
			return -EMSGSIZE;
		}

		if (len != NET_RPL_HOP_BY_HOP_LEN - 8) {
			NET_DBG("Non RPL Hop-by-hop options support not "
				"implemented");
			return 0;
		}

		if (next == NET_RPL_EXT_HDR_OPT_RPL) {
			uint16_t sender_rank, offset;

			frag = net_nbuf_skip(frag, pos, &pos, 1); /* opt type */
			frag = net_nbuf_skip(frag, pos, &pos, 1); /* opt len */

			offset = pos; /* Where the flags is located in the
				       * packet, that info is need few lines
				       * below.
				       */

			frag = net_nbuf_skip(frag, pos, &pos, 1); /* flags */
			frag = net_nbuf_skip(frag, pos, &pos, 1); /* instance */

			frag = net_nbuf_read(frag, pos, &pos, 2,
					     (uint8_t *)&sender_rank);
			if (!frag && pos) {
				return -EMSGSIZE;
			}

			if (sender_rank == 0) {
				NET_DBG("Updating RPL option");
				if (!rpl_default_instance ||
				    !rpl_default_instance->is_used ||
				    !net_rpl_dag_is_joined(
					  rpl_default_instance->current_dag)) {

					NET_DBG("Unable to add hop-by-hop "
						"extension header: incorrect "
						"default instance");
					return -EINVAL;
				}

				parent = find_parent(net_nbuf_iface(buf),
						     rpl_default_instance->
						     current_dag,
						     addr);

				if (!parent ||
				    parent != parent->dag->preferred_parent) {
					net_nbuf_write_u8(buf, buf->frags,
							  offset, &pos,
							  NET_RPL_HDR_OPT_DOWN);
				}

				offset++;

				net_nbuf_write_u8(buf, buf->frags, offset, &pos,
					     rpl_default_instance->instance_id);

				net_nbuf_write_be16(buf, buf->frags, pos, &pos,
					     htons(rpl_default_instance->
						   current_dag->rank));
			}
		}
	}

	return 0;
}

bool net_rpl_verify_header(struct net_buf *buf,
			   uint16_t offset, uint16_t *pos)
{
	struct net_rpl_instance *instance;
	struct net_buf *frag;
	uint16_t sender_rank;
	uint8_t instance_id, flags;
	bool down, sender_closer;

	frag = net_nbuf_read_u8(buf, offset, pos, &flags);
	frag = net_nbuf_read_u8(frag, *pos, pos, &instance_id);
	frag = net_nbuf_read_be16(frag, *pos, pos, &sender_rank);

	if (!frag && *pos == 0xffff) {
		return false;
	}

	instance = net_rpl_get_instance(instance_id);
	if (!instance) {
		NET_DBG("Unknown instance %u", instance_id);
		return false;
	}

	if (flags & NET_RPL_HDR_OPT_FWD_ERR) {
		struct net_route_entry *route;

		/* We should try to repair it by removing the neighbor that
		 * caused the packet to be forwareded in the first place.
		 * We drop any routes that go through the neighbor that sent
		 * the packet to us.
		 */
		NET_DBG("Forward error!");

		route = net_route_lookup(net_nbuf_iface(buf),
					 &NET_IPV6_BUF(buf)->dst);
		if (route) {
			net_route_del(route);
		}

		net_stats_update_rpl_forward_errors();

		/* Trigger DAO retransmission */
		net_rpl_reset_dio_timer(instance);

		/* drop the packet as it is not routable */
		return false;
	}

	if (!net_rpl_dag_is_joined(instance->current_dag)) {
		NET_DBG("No DAG in the instance");
		return false;
	}

	if (flags & NET_RPL_HDR_OPT_DOWN) {
		down = true;
	} else {
		down = false;
	}

	sender_rank = ntohs(sender_rank);
	sender_closer = sender_rank < instance->current_dag->rank;

	NET_DBG("Packet going %s, sender closer %d (%d < %d)",
		down ? "down" : "up", sender_closer, sender_rank,
		instance->current_dag->rank);

	if ((down && !sender_closer) || (!down && sender_closer)) {
		NET_DBG("Loop detected - sender rank %d my-rank %d "
			"sender_closer %d",
			sender_rank, instance->current_dag->rank,
			sender_closer);

		if (flags & NET_RPL_HDR_OPT_RANK_ERR) {
			net_stats_update_rpl_loop_errors();

			NET_DBG("Rank error signalled in RPL option!");

			/* Packet must be dropped and dio trickle timer reset,
			 * see RFC 6550 - 11.2.2.2
			 */
			net_rpl_reset_dio_timer(instance);

			return false;
		}

		NET_DBG("Single error tolerated.");
		net_stats_update_rpl_loop_warnings();

		net_nbuf_write_u8(buf, buf->frags, offset, pos,
				  flags | NET_RPL_HDR_OPT_RANK_ERR);

		return true;
	}

	NET_DBG("Rank OK");
	return true;
}

static inline int add_rpl_opt(struct net_buf *buf, uint16_t offset)
{
	int ext_len = net_nbuf_ext_len(buf);
	bool ret;

	/* next header */
	ret = net_nbuf_insert_u8(buf, buf->frags, offset++,
				 NET_IPV6_BUF(buf)->nexthdr);
	if (!ret) {
		return -EINVAL;
	}

	/* Option len */
	ret = net_nbuf_insert_u8(buf, buf->frags, offset++,
				 NET_RPL_HOP_BY_HOP_LEN - 8);
	if (!ret) {
		return -EINVAL;
	}

	/* Sub-option type */
	ret = net_nbuf_insert_u8(buf, buf->frags, offset++,
				 NET_IPV6_EXT_HDR_OPT_RPL);
	if (!ret) {
		return -EINVAL;
	}

	/* Sub-option length */
	ret = net_nbuf_insert_u8(buf, buf->frags, offset++,
				 NET_RPL_HDR_OPT_LEN);
	if (!ret) {
		return -EINVAL;
	}

	/* RPL option flags */
	ret = net_nbuf_insert_u8(buf, buf->frags, offset++, 0);
	if (!ret) {
		return -EINVAL;
	}

	/* RPL Instance id */
	ret = net_nbuf_insert_u8(buf, buf->frags, offset++, 0);
	if (!ret) {
		return -EINVAL;
	}

	/* RPL sender rank */
	ret = net_nbuf_insert_be16(buf, buf->frags, offset++, 0);
	if (!ret) {
		return -EINVAL;
	}

	NET_IPV6_BUF(buf)->nexthdr = NET_IPV6_NEXTHDR_HBHO;

	net_nbuf_set_ext_len(buf, ext_len + NET_RPL_HOP_BY_HOP_LEN);

	return 0;
}

static int net_rpl_update_header_empty(struct net_buf *buf)
{
	uint16_t offset = sizeof(struct net_ipv6_hdr);
	uint8_t next = NET_IPV6_BUF(buf)->nexthdr;
	struct net_buf *frag = buf->frags;
	struct net_rpl_instance *instance;
	struct net_rpl_parent *parent;
	struct net_route_entry *route;
	uint8_t next_hdr, len, length;
	uint8_t opt_type = 0, opt_len;
	uint8_t instance_id, flags;
	uint16_t pos;

	NET_DBG("Verifying the presence of the RPL header option");

	frag = net_nbuf_read_u8(frag, offset, &offset, &next_hdr);
	frag = net_nbuf_read_u8(frag, offset, &offset, &len);
	if (!frag) {
		return 0;
	}

	length = 0;

	if (next != NET_IPV6_NEXTHDR_HBHO) {
		NET_DBG("No hop-by-hop option found, creating it");

		 /* We already read 2 bytes so go back accordingly. */
		if (add_rpl_opt(buf, offset - 2) < 0) {
			NET_DBG("Cannot add RPL options");
			return -EINVAL;
		}

		return 0;
	}

	if (len != NET_RPL_HOP_BY_HOP_LEN - 8) {
		NET_DBG("Hop-by-hop ext header is wrong size "
			"(%d vs %d)", length,
			NET_RPL_HOP_BY_HOP_LEN - 8);

		return 0;
	}

	length += 2;

	/* Each extension option has type and length */
	frag = net_nbuf_read_u8(frag, offset, &offset, &opt_type);
	frag = net_nbuf_read_u8(frag, offset, &offset, &opt_len);

	if (opt_type != NET_IPV6_EXT_HDR_OPT_RPL) {
		/* FIXME: go through all the options instead */
		NET_DBG("Non RPL Hop-by-hop option check not "
			"implemented");
		return 0;
	}

	if (opt_len != NET_RPL_HDR_OPT_LEN) {
		NET_DBG("RPL Hop-by-hop option has wrong length");
		return 0;
	}

	frag = net_nbuf_read_u8(buf, offset, &offset, &flags);
	frag = net_nbuf_read_u8(frag, offset, &offset, &instance_id);

	instance = net_rpl_get_instance(instance_id);
	if (!instance || !instance->is_used ||
	    !instance->current_dag->is_joined) {
		NET_DBG("Incorrect instance so hop-by-hop ext header "
			"not added");
		return 0;
	}

	if (opt_type != NET_IPV6_EXT_HDR_OPT_RPL) {
		NET_DBG("Multi Hop-by-hop options not implemented");
		return 0;
	}

	NET_DBG("Updating RPL option");

	/* The offset should point to "rank" right now */
	net_nbuf_write_be16(buf, frag, offset, &pos,
			    instance->current_dag->rank);

	offset -= 2; /* move back to flags */

	route = net_route_lookup(net_nbuf_iface(buf), &NET_IPV6_BUF(buf)->dst);

	/*
	 * Check the direction of the down flag, as per
	 * Section 11.2.2.3, which states that if a packet is going
	 * down it should in general not go back up again. If this
	 * happens, a NET_RPL_HDR_OPT_FWD_ERR should be flagged.
	 */
	if (flags & NET_RPL_HDR_OPT_DOWN) {
		struct net_nbr *nbr;

		if (!route) {
			net_nbuf_write_u8(buf, frag, offset, &pos,
					  flags |= NET_RPL_HDR_OPT_FWD_ERR);

			NET_DBG("RPL forwarding error");

			/*
			 * We should send back the packet to the
			 * originating parent, but it is not feasible
			 * yet, so we send a No-Path DAO instead.
			 */
			NET_DBG("RPL generate No-Path DAO");

			nbr = net_nbr_lookup(&net_rpl_parents.table,
					     net_nbuf_iface(buf),
					     net_nbuf_ll_src(buf));

			parent = nbr_data(nbr);
			if (parent) {
				net_rpl_dao_send(net_nbuf_iface(buf),
						 parent,
						 &NET_IPV6_BUF(buf)->dst,
						 NET_RPL_ZERO_LIFETIME);
			}

			/* Drop packet */
			return -EINVAL;
		}

		return 0;
	}

	/*
	 * Set the down extension flag correctly as described
	 * in Section 11.2 of RFC6550. If the packet progresses
	 * along a DAO route, the down flag should be set.
	 */

	if (!route) {
		/* No route was found, so this packet will go
		 * towards the RPL root. If so, we should not
		 * set the down flag.
		 */
		net_nbuf_write_u8(buf, frag, offset, &pos,
				  flags &= ~NET_RPL_HDR_OPT_DOWN);

		NET_DBG("RPL option going up");
	} else {
		/* A DAO route was found so we set the down
		 * flag.
		 */
		net_nbuf_write_u8(buf, frag, offset, &pos,
				  flags |= NET_RPL_HDR_OPT_DOWN);

		NET_DBG("RPL option going down");
	}

	return 0;
}

int net_rpl_insert_header(struct net_buf *buf)
{
#if defined(CONFIG_NET_RPL_INSERT_HBH_OPTION)
	if (rpl_default_instance &&
	    !net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
		return net_rpl_update_header_empty(buf);
	}
#endif

	return 0;
}

static inline void create_linklocal_rplnodes_mcast(struct in6_addr *addr)
{
	net_ipv6_addr_create(addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x001a);
}

#if defined(CONFIG_NET_RPL_DIS_SEND)
static void dis_timeout(struct k_work *work)
{
	uint32_t dis_interval;

	NET_DBG("DIS Timer triggered at %u", k_uptime_get_32());

	net_rpl_dis_send(NULL, NULL);

	dis_interval = CONFIG_NET_RPL_DIS_INTERVAL * MSEC_PER_SEC;

	k_delayed_work_submit(&dis_timer, dis_interval);
}
#endif

static inline void net_rpl_init_timers(void)
{
#if defined(CONFIG_NET_RPL_DIS_SEND)
	/* Randomize the first DIS sending*/
	uint32_t dis_interval;

	dis_interval = (CONFIG_NET_RPL_DIS_INTERVAL / 2 +
			((uint32_t)CONFIG_NET_RPL_DIS_INTERVAL *
			 (uint32_t)sys_rand32_get()) / UINT_MAX -
			NET_RPL_DIS_START_DELAY) * MSEC_PER_SEC;

	k_delayed_work_init(&dis_timer, dis_timeout);
	k_delayed_work_submit(&dis_timer, dis_interval);
#endif
}

void net_rpl_init(void)
{
	/* Note that link_cb needs to be static as it is added
	 * to linked list of callbacks.
	 */
	static struct net_if_link_cb link_cb;
	struct in6_addr addr;

	NET_DBG("Allocated %d routing entries (%d bytes)",
		CONFIG_NET_IPV6_MAX_NEIGHBORS,
		sizeof(net_rpl_neighbor_pool));

#if defined(CONFIG_NET_RPL_STATS)
	memset(&net_stats.rpl, 0, sizeof(net_stats.rpl));
#endif

	rpl_dao_sequence = net_rpl_lollipop_init();

	net_rpl_init_timers();

	create_linklocal_rplnodes_mcast(&addr);
	if (!net_if_ipv6_maddr_add(net_if_get_default(), &addr)) {
		NET_ERR("Cannot create RPL multicast address");

		/* Ignore error at this point */
	}

	net_rpl_of_reset(NULL);

	net_if_register_link_cb(&link_cb, net_rpl_link_neighbor_callback);

	net_icmpv6_register_handler(&dodag_info_solicitation_handler);
	net_icmpv6_register_handler(&dodag_information_object_handler);
	net_icmpv6_register_handler(&destination_advertisement_object_handler);
	net_icmpv6_register_handler(&dao_ack_handler);
}
