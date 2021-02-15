/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_if, CONFIG_NET_IF_LOG_LEVEL);

#include <init.h>
#include <kernel.h>
#include <linker/sections.h>
#include <random/rand32.h>
#include <syscall_handler.h>
#include <stdlib.h>
#include <string.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/ethernet.h>

#include "net_private.h"
#include "ipv6.h"
#include "ipv4_autoconf_internal.h"

#include "net_stats.h"

#define REACHABLE_TIME (MSEC_PER_SEC * 30) /* in ms */
/*
 * split the min/max random reachable factors into numerator/denominator
 * so that integer-based math works better
 */
#define MIN_RANDOM_NUMER (1)
#define MIN_RANDOM_DENOM (2)
#define MAX_RANDOM_NUMER (3)
#define MAX_RANDOM_DENOM (2)

/* net_if dedicated section limiters */
extern struct net_if _net_if_list_start[];
extern struct net_if _net_if_list_end[];

#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
static struct net_if_router routers[CONFIG_NET_MAX_ROUTERS];
static struct k_delayed_work router_timer;
static sys_slist_t active_router_timers;
#endif

#if defined(CONFIG_NET_NATIVE_IPV6)
/* Timer that triggers network address renewal */
static struct k_delayed_work address_lifetime_timer;

/* Track currently active address lifetime timers */
static sys_slist_t active_address_lifetime_timers;

/* Timer that triggers IPv6 prefix lifetime */
static struct k_delayed_work prefix_lifetime_timer;

/* Track currently active IPv6 prefix lifetime timers */
static sys_slist_t active_prefix_lifetime_timers;

#if defined(CONFIG_NET_IPV6_DAD)
/** Duplicate address detection (DAD) timer */
static struct k_delayed_work dad_timer;
static sys_slist_t active_dad_timers;
#endif

#if defined(CONFIG_NET_IPV6_ND)
static struct k_delayed_work rs_timer;
static sys_slist_t active_rs_timers;
#endif

static struct {
	struct net_if_ipv6 ipv6;
	struct net_if *iface;
} ipv6_addresses[CONFIG_NET_IF_MAX_IPV6_COUNT];
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_NATIVE_IPV4)
static struct {
	struct net_if_ipv4 ipv4;
	struct net_if *iface;
} ipv4_addresses[CONFIG_NET_IF_MAX_IPV4_COUNT];
#endif /* CONFIG_NET_IPV4 */

/* We keep track of the link callbacks in this list.
 */
static sys_slist_t link_callbacks;

#if defined(CONFIG_NET_NATIVE_IPV6)
/* Multicast join/leave tracking.
 */
static sys_slist_t mcast_monitor_callbacks;
#endif

#if defined(CONFIG_NET_PKT_TIMESTAMP_THREAD)
#if !defined(CONFIG_NET_PKT_TIMESTAMP_STACK_SIZE)
#define CONFIG_NET_PKT_TIMESTAMP_STACK_SIZE 1024
#endif

K_KERNEL_STACK_DEFINE(tx_ts_stack, CONFIG_NET_PKT_TIMESTAMP_STACK_SIZE);
K_FIFO_DEFINE(tx_ts_queue);

static struct k_thread tx_thread_ts;

/* We keep track of the timestamp callbacks in this list.
 */
static sys_slist_t timestamp_callbacks;
#endif /* CONFIG_NET_PKT_TIMESTAMP_THREAD */

#if CONFIG_NET_IF_LOG_LEVEL >= LOG_LEVEL_DBG
#define debug_check_packet(pkt)						\
	do {								\
		NET_DBG("Processing (pkt %p, prio %d) network packet "	\
			"iface %p/%d",					\
			pkt, net_pkt_priority(pkt),			\
			net_pkt_iface(pkt),				\
			net_if_get_by_iface(net_pkt_iface(pkt)));	\
									\
		NET_ASSERT(pkt->frags);					\
	} while (0)
#else
#define debug_check_packet(...)
#endif /* CONFIG_NET_IF_LOG_LEVEL >= LOG_LEVEL_DBG */

struct net_if *z_impl_net_if_get_by_index(int index)
{
	if (index <= 0) {
		return NULL;
	}

	if (&_net_if_list_start[index - 1] >= _net_if_list_end) {
		NET_DBG("Index %d is too large", index);
		return NULL;
	}

	return &_net_if_list_start[index - 1];
}

#ifdef CONFIG_USERSPACE
struct net_if *z_vrfy_net_if_get_by_index(int index)
{
	struct net_if *iface;
	struct z_object *zo;
	int ret;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return NULL;
	}

	zo = z_object_find(iface);

	ret = z_object_validate(zo, K_OBJ_NET_IF, _OBJ_INIT_TRUE);
	if (ret != 0) {
		z_dump_object_error(ret, iface, zo, K_OBJ_NET_IF);
		return NULL;
	}

	return iface;
}

#include <syscalls/net_if_get_by_index_mrsh.c>
#endif

static inline void net_context_send_cb(struct net_context *context,
				       int status)
{
	if (!context) {
		return;
	}

	if (context->send_cb) {
		context->send_cb(context, status, context->user_data);
	}

	if (IS_ENABLED(CONFIG_NET_UDP) &&
	    net_context_get_ip_proto(context) == IPPROTO_UDP) {
		net_stats_update_udp_sent(net_context_get_iface(context));
	} else if (IS_ENABLED(CONFIG_NET_TCP) &&
		   net_context_get_ip_proto(context) == IPPROTO_TCP) {
		net_stats_update_tcp_seg_sent(net_context_get_iface(context));
	}
}

static void update_txtime_stats_detail(struct net_pkt *pkt,
				       uint32_t start_time, uint32_t stop_time)
{
	uint32_t val, prev = start_time;
	int i;

	for (i = 0; i < net_pkt_stats_tick_count(pkt); i++) {
		if (!net_pkt_stats_tick(pkt)[i]) {
			break;
		}

		val = net_pkt_stats_tick(pkt)[i] - prev;
		prev = net_pkt_stats_tick(pkt)[i];
		net_pkt_stats_tick(pkt)[i] = val;
	}
}

static bool net_if_tx(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_linkaddr ll_dst = {
		.addr = NULL
	};
	struct net_linkaddr_storage ll_dst_storage;
	struct net_context *context;
	int status;

	/* Timestamp of the current network packet sent if enabled */
	struct net_ptp_time start_timestamp;
	uint32_t curr_time = 0;

	/* We collect send statistics for each socket priority if enabled */
	uint8_t pkt_priority;

	if (!pkt) {
		return false;
	}

	debug_check_packet(pkt);

	/* If there're any link callbacks, with such a callback receiving
	 * a destination address, copy that address out of packet, just in
	 * case packet is freed before callback is called.
	 */
	if (!sys_slist_is_empty(&link_callbacks)) {
		if (net_linkaddr_set(&ll_dst_storage,
				     net_pkt_lladdr_dst(pkt)->addr,
				     net_pkt_lladdr_dst(pkt)->len) == 0) {
			ll_dst.addr = ll_dst_storage.addr;
			ll_dst.len = ll_dst_storage.len;
			ll_dst.type = net_pkt_lladdr_dst(pkt)->type;
		}
	}

	context = net_pkt_context(pkt);

	if (net_if_flag_is_set(iface, NET_IF_UP)) {
		if (IS_ENABLED(CONFIG_NET_TCP) &&
		    net_pkt_family(pkt) != AF_UNSPEC) {
			net_pkt_set_queued(pkt, false);
		}

		if (IS_ENABLED(CONFIG_NET_CONTEXT_TIMESTAMP) && context) {
			if (net_context_get_timestamp(context, pkt,
						      &start_timestamp) < 0) {
				start_timestamp.nanosecond = 0;
			} else {
				pkt_priority = net_pkt_priority(pkt);
			}
		}

		if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS)) {
			memcpy(&start_timestamp, net_pkt_timestamp(pkt),
			       sizeof(start_timestamp));
			pkt_priority = net_pkt_priority(pkt);

			if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)) {
				/* Make sure the statistics information is not
				 * lost by keeping the net_pkt over L2 send.
				 */
				net_pkt_ref(pkt);
			}
		}

		status = net_if_l2(iface)->send(iface, pkt);

		if (IS_ENABLED(CONFIG_NET_CONTEXT_TIMESTAMP) && status >= 0 &&
		    context) {
			if (start_timestamp.nanosecond > 0) {
				curr_time = k_cycle_get_32();
			}
		}

		if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS)) {
			uint32_t end_tick = k_cycle_get_32();

			net_pkt_set_tx_stats_tick(pkt, end_tick);

			net_stats_update_tc_tx_time(iface,
						    pkt_priority,
						    start_timestamp.nanosecond,
						    end_tick);

			if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)) {
				update_txtime_stats_detail(
					pkt,
					start_timestamp.nanosecond,
					end_tick);

				net_stats_update_tc_tx_time_detail(
					iface, pkt_priority,
					net_pkt_stats_tick(pkt));

				/* For TCP connections, we might keep the pkt
				 * longer so that we can resend it if needed.
				 * Because of that we need to clear the
				 * statistics here.
				 */
				net_pkt_stats_tick_reset(pkt);

				net_pkt_unref(pkt);
			}
		}

	} else {
		/* Drop packet if interface is not up */
		NET_WARN("iface %p is down", iface);
		status = -ENETDOWN;
	}

	if (status < 0) {
		net_pkt_unref(pkt);
	} else {
		net_stats_update_bytes_sent(iface, status);
	}

	if (context) {
		NET_DBG("Calling context send cb %p status %d",
			context, status);

		net_context_send_cb(context, status);

		if (IS_ENABLED(CONFIG_NET_CONTEXT_TIMESTAMP) && status >= 0 &&
		    start_timestamp.nanosecond && curr_time > 0) {
			/* So we know now how long the network packet was in
			 * transit from when it was allocated to when we
			 * got information that it was sent successfully.
			 */
			net_stats_update_tc_tx_time(iface,
						    pkt_priority,
						    start_timestamp.nanosecond,
						    curr_time);
		}
	}

	if (ll_dst.addr) {
		net_if_call_link_cb(iface, &ll_dst, status);
	}

	return true;
}

static void process_tx_packet(struct k_work *work)
{
	struct net_if *iface;
	struct net_pkt *pkt;

	pkt = CONTAINER_OF(work, struct net_pkt, work);

	net_pkt_set_tx_stats_tick(pkt, k_cycle_get_32());

	iface = net_pkt_iface(pkt);

	net_if_tx(iface, pkt);

#if defined(CONFIG_NET_POWER_MANAGEMENT)
	iface->tx_pending--;
#endif
}

void net_if_queue_tx(struct net_if *iface, struct net_pkt *pkt)
{
	uint8_t prio = net_pkt_priority(pkt);
	uint8_t tc = net_tx_priority2tc(prio);

	k_work_init(net_pkt_work(pkt), process_tx_packet);

	net_stats_update_tc_sent_pkt(iface, tc);
	net_stats_update_tc_sent_bytes(iface, tc, net_pkt_get_len(pkt));
	net_stats_update_tc_sent_priority(iface, tc, prio);

#if NET_TC_TX_COUNT > 1
	NET_DBG("TC %d with prio %d pkt %p", tc, prio, pkt);
#endif

#if defined(CONFIG_NET_POWER_MANAGEMENT)
	iface->tx_pending++;
#endif

	if (!net_tc_submit_to_tx_queue(tc, pkt)) {
#if defined(CONFIG_NET_POWER_MANAGEMENT)
		iface->tx_pending--
#endif
			;
	}
}

void net_if_stats_reset(struct net_if *iface)
{
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	Z_STRUCT_SECTION_FOREACH(net_if, tmp) {
		if (iface == tmp) {
			memset(&iface->stats, 0, sizeof(iface->stats));
			return;
		}
	}
#endif
}

void net_if_stats_reset_all(void)
{
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)

	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		memset(&iface->stats, 0, sizeof(iface->stats));
	}
#endif
}

static inline void init_iface(struct net_if *iface)
{
	const struct net_if_api *api = net_if_get_device(iface)->api;

	if (!api || !api->init) {
		NET_ERR("Iface %p driver API init NULL", iface);
		return;
	}

	NET_DBG("On iface %p", iface);

#ifdef CONFIG_USERSPACE
	z_object_init(iface);
#endif

	api->init(iface);
}

enum net_verdict net_if_send_data(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_context *context = net_pkt_context(pkt);
	struct net_linkaddr *dst = net_pkt_lladdr_dst(pkt);
	enum net_verdict verdict = NET_OK;
	int status = -EIO;

	if (!net_if_flag_is_set(iface, NET_IF_UP) ||
	    net_if_flag_is_set(iface, NET_IF_SUSPENDED)) {
		/* Drop packet if interface is not up */
		NET_WARN("iface %p is down", iface);
		verdict = NET_DROP;
		status = -ENETDOWN;
		goto done;
	}

	if (IS_ENABLED(CONFIG_NET_OFFLOAD) && !net_if_l2(iface)) {
		NET_WARN("no l2 for iface %p, discard pkt", iface);
		verdict = NET_DROP;
		goto done;
	}

	/* If the ll address is not set at all, then we must set
	 * it here.
	 * Workaround Linux bug, see:
	 * https://github.com/zephyrproject-rtos/zephyr/issues/3111
	 */
	if (!net_if_flag_is_set(iface, NET_IF_POINTOPOINT) &&
	    !net_pkt_lladdr_src(pkt)->addr) {
		net_pkt_lladdr_src(pkt)->addr = net_pkt_lladdr_if(pkt)->addr;
		net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;
	}

#if defined(CONFIG_NET_LOOPBACK)
	/* If the packet is destined back to us, then there is no need to do
	 * additional checks, so let the packet through.
	 */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		goto done;
	}
#endif

	/* If the ll dst address is not set check if it is present in the nbr
	 * cache.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		verdict = net_ipv6_prepare_for_send(pkt);
	}

done:
	/*   NET_OK in which case packet has checked successfully. In this case
	 *   the net_context callback is called after successful delivery in
	 *   net_if_tx_thread().
	 *
	 *   NET_DROP in which case we call net_context callback that will
	 *   give the status to user application.
	 *
	 *   NET_CONTINUE in which case the sending of the packet is delayed.
	 *   This can happen for example if we need to do IPv6 ND to figure
	 *   out link layer address.
	 */
	if (verdict == NET_DROP) {
		if (context) {
			NET_DBG("Calling ctx send cb %p verdict %d",
				context, verdict);
			net_context_send_cb(context, status);
		}

		if (dst->addr) {
			net_if_call_link_cb(iface, dst, status);
		}
	} else if (verdict == NET_OK) {
		/* Packet is ready to be sent by L2, let's queue */
		net_if_queue_tx(iface, pkt);
	}

	return verdict;
}

struct net_if *net_if_get_by_link_addr(struct net_linkaddr *ll_addr)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		if (!memcmp(net_if_get_link_addr(iface)->addr, ll_addr->addr,
			    ll_addr->len)) {
			return iface;
		}
	}

	return NULL;
}

struct net_if *net_if_lookup_by_dev(const struct device *dev)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_get_device(iface) == dev) {
			return iface;
		}
	}

	return NULL;
}

struct net_if *net_if_get_default(void)
{
	struct net_if *iface = NULL;

	if (_net_if_list_start == _net_if_list_end) {
		return NULL;
	}

#if defined(CONFIG_NET_DEFAULT_IF_ETHERNET)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_IEEE802154)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(IEEE802154));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_BLUETOOTH)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(BLUETOOTH));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_DUMMY)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_OFFLOAD)
	iface = net_if_get_first_by_type(NULL);
#endif
#if defined(CONFIG_NET_DEFAULT_IF_CANBUS_RAW)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(CANBUS_RAW));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_CANBUS)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(CANBUS));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_PPP)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
#endif

	return iface ? iface : _net_if_list_start;
}

struct net_if *net_if_get_first_by_type(const struct net_l2 *l2)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		if (IS_ENABLED(CONFIG_NET_OFFLOAD) &&
		    !l2 && net_if_offload(iface)) {
			return iface;
		}

		if (net_if_l2(iface) == l2) {
			return iface;
		}
	}

	return NULL;
}

static enum net_l2_flags l2_flags_get(struct net_if *iface)
{
	enum net_l2_flags flags = 0;

	if (net_if_l2(iface) && net_if_l2(iface)->get_flags) {
		flags = net_if_l2(iface)->get_flags(iface);
	}

	return flags;
}

#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
/* Return how many bits are shared between two IP addresses */
static uint8_t get_ipaddr_diff(const uint8_t *src, const uint8_t *dst, int addr_len)
{
	uint8_t j, k, xor;
	uint8_t len = 0U;

	for (j = 0U; j < addr_len; j++) {
		if (src[j] == dst[j]) {
			len += 8U;
		} else {
			xor = src[j] ^ dst[j];
			for (k = 0U; k < 8; k++) {
				if (!(xor & 0x80)) {
					len++;
					xor <<= 1;
				} else {
					break;
				}
			}
			break;
		}
	}

	return len;
}

static struct net_if_router *iface_router_lookup(struct net_if *iface,
						 uint8_t family, void *addr)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (!routers[i].is_used ||
		    routers[i].address.family != family ||
		    routers[i].iface != iface) {
			continue;
		}

		if ((IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6 &&
		     net_ipv6_addr_cmp(net_if_router_ipv6(&routers[i]),
				       (struct in6_addr *)addr)) ||
		    (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET &&
		     net_ipv4_addr_cmp(net_if_router_ipv4(&routers[i]),
				       (struct in_addr *)addr))) {
			return &routers[i];
		}
	}

	return NULL;
}

static void iface_router_notify_deletion(struct net_if_router *router,
					 const char *delete_reason)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    router->address.family == AF_INET6) {
		NET_DBG("IPv6 router %s %s",
			log_strdup(net_sprint_ipv6_addr(
					   net_if_router_ipv6(router))),
			delete_reason);

		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTER_DEL,
						router->iface,
						&router->address.in6_addr,
						sizeof(struct in6_addr));
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   router->address.family == AF_INET) {
		NET_DBG("IPv4 router %s %s",
			log_strdup(net_sprint_ipv4_addr(
					   net_if_router_ipv4(router))),
			delete_reason);

		net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ROUTER_DEL,
						router->iface,
						&router->address.in_addr,
						sizeof(struct in6_addr));
	}
}

static inline int32_t iface_router_ends(const struct net_if_router *router,
					uint32_t now)
{
	uint32_t ends = router->life_start;

	ends += MSEC_PER_SEC * router->lifetime;

	/* Signed number of ms until router lifetime ends */
	return (int32_t)(ends - now);
}

static void iface_router_update_timer(uint32_t now)
{
	struct net_if_router *router, *next;
	uint32_t new_delay = UINT32_MAX;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_router_timers,
					 router, next, node) {
		int32_t ends = iface_router_ends(router, now);

		if (ends <= 0) {
			new_delay = 0;
			break;
		}

		new_delay = MIN((uint32_t)ends, new_delay);
	}

	if (new_delay == UINT32_MAX) {
		k_delayed_work_cancel(&router_timer);
	} else {
		k_delayed_work_submit(&router_timer, K_MSEC(new_delay));
	}
}

static void iface_router_expired(struct k_work *work)
{
	uint32_t current_time = k_uptime_get_32();
	struct net_if_router *router, *next;
	sys_snode_t *prev_node = NULL;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_router_timers,
					  router, next, node) {
		int32_t ends = iface_router_ends(router, current_time);

		if (ends > 0) {
			/* We have to loop on all active routers as their
			 * lifetime differ from each other.
			 */
			prev_node = &router->node;
			continue;
		}

		iface_router_notify_deletion(router, "has expired");
		sys_slist_remove(&active_router_timers,
				 prev_node, &router->node);
		router->is_used = false;
	}

	iface_router_update_timer(current_time);
}

static struct net_if_router *iface_router_add(struct net_if *iface,
					      uint8_t family, void *addr,
					      bool is_default,
					      uint16_t lifetime)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (routers[i].is_used) {
			continue;
		}

		routers[i].is_used = true;
		routers[i].iface = iface;
		routers[i].address.family = family;

		if (lifetime) {
			routers[i].is_default = true;
			routers[i].is_infinite = false;
			routers[i].lifetime = lifetime;
			routers[i].life_start = k_uptime_get_32();

			sys_slist_append(&active_router_timers,
					 &routers[i].node);

			iface_router_update_timer(routers[i].life_start);
		} else {
			routers[i].is_default = false;
			routers[i].is_infinite = true;
			routers[i].lifetime = 0;
		}

		if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
			memcpy(net_if_router_ipv6(&routers[i]), addr,
			       sizeof(struct in6_addr));
			net_mgmt_event_notify_with_info(
					NET_EVENT_IPV6_ROUTER_ADD, iface,
					&routers[i].address.in6_addr,
					sizeof(struct in6_addr));

			NET_DBG("interface %p router %s lifetime %u default %d "
				"added", iface,
				log_strdup(net_sprint_ipv6_addr(
						   (struct in6_addr *)addr)),
				lifetime, routers[i].is_default);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
			memcpy(net_if_router_ipv4(&routers[i]), addr,
			       sizeof(struct in_addr));
			routers[i].is_default = is_default;

			net_mgmt_event_notify_with_info(
					NET_EVENT_IPV4_ROUTER_ADD, iface,
					&routers[i].address.in_addr,
					sizeof(struct in_addr));

			NET_DBG("interface %p router %s lifetime %u default %d "
				"added", iface,
				log_strdup(net_sprint_ipv4_addr(
						   (struct in_addr *)addr)),
				lifetime, is_default);
		}

		return &routers[i];
	}

	return NULL;
}

static bool iface_router_rm(struct net_if_router *router)
{
	if (!router->is_used) {
		return false;
	}

	iface_router_notify_deletion(router, "has been removed");

	/* We recompute the timer if only the router was time limited */
	if (sys_slist_find_and_remove(&active_router_timers, &router->node)) {
		iface_router_update_timer(k_uptime_get_32());
	}

	router->is_used = false;

	return true;
}

static struct net_if_router *iface_router_find_default(struct net_if *iface,
						       uint8_t family, void *addr)
{
	int i;

	/* Todo: addr will need to be handled */
	ARG_UNUSED(addr);

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (!routers[i].is_used ||
		    !routers[i].is_default ||
		    routers[i].address.family != family) {
			continue;
		}

		if (iface && iface != routers[i].iface) {
			continue;
		}

		return &routers[i];
	}

	return NULL;
}

static void iface_router_init(void)
{
	k_delayed_work_init(&router_timer, iface_router_expired);
	sys_slist_init(&active_router_timers);
}
#else
#define iface_router_init(...)
#endif

#if defined(CONFIG_NET_NATIVE_IPV6)
int net_if_config_ipv6_get(struct net_if *iface, struct net_if_ipv6 **ipv6)
{
	int i;

	if (iface->config.ip.ipv6) {
		if (ipv6) {
			*ipv6 = iface->config.ip.ipv6;
		}

		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(ipv6_addresses); i++) {
		if (ipv6_addresses[i].iface) {
			continue;
		}

		iface->config.ip.ipv6 = &ipv6_addresses[i].ipv6;
		ipv6_addresses[i].iface = iface;

		if (ipv6) {
			*ipv6 = &ipv6_addresses[i].ipv6;
		}

		return 0;
	}

	return -ESRCH;
}

int net_if_config_ipv6_put(struct net_if *iface)
{
	int i;

	if (!iface->config.ip.ipv6) {
		return -EALREADY;
	}

	for (i = 0; i < ARRAY_SIZE(ipv6_addresses); i++) {
		if (ipv6_addresses[i].iface != iface) {
			continue;
		}

		iface->config.ip.ipv6 = NULL;
		ipv6_addresses[i].iface = NULL;

		return 0;
	}

	return -ESRCH;
}

#if defined(CONFIG_NET_IPV6_MLD)
static void join_mcast_allnodes(struct net_if *iface)
{
	struct in6_addr addr;
	int ret;

	net_ipv6_addr_create_ll_allnodes_mcast(&addr);

	ret = net_ipv6_mld_join(iface, &addr);
	if (ret < 0 && ret != -EALREADY) {
		NET_ERR("Cannot join all nodes address %s (%d)",
			log_strdup(net_sprint_ipv6_addr(&addr)), ret);
	}
}

static void join_mcast_solicit_node(struct net_if *iface,
				    struct in6_addr *my_addr)
{
	struct in6_addr addr;
	int ret;

	/* Join to needed multicast groups, RFC 4291 ch 2.8 */
	net_ipv6_addr_create_solicited_node(my_addr, &addr);

	ret = net_ipv6_mld_join(iface, &addr);
	if (ret < 0 && ret != -EALREADY) {
		NET_ERR("Cannot join solicit node address %s (%d)",
			log_strdup(net_sprint_ipv6_addr(&addr)), ret);
	}
}

static void leave_mcast_all(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used ||
		    !ipv6->mcast[i].is_joined) {
			continue;
		}

		net_ipv6_mld_leave(iface, &ipv6->mcast[i].address.in6_addr);
	}
}

static void join_mcast_nodes(struct net_if *iface, struct in6_addr *addr)
{
	enum net_l2_flags flags = 0;

	flags = l2_flags_get(iface);
	if (flags & NET_L2_MULTICAST) {
		join_mcast_allnodes(iface);

		if (!(flags & NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE)) {
			join_mcast_solicit_node(iface, addr);
		}
	}
}
#else
#define join_mcast_allnodes(...)
#define join_mcast_solicit_node(...)
#define leave_mcast_all(...)
#define join_mcast_nodes(...)
#endif /* CONFIG_NET_IPV6_MLD */

#if defined(CONFIG_NET_IPV6_DAD)
#define DAD_TIMEOUT 100U /* ms */

static void dad_timeout(struct k_work *work)
{
	uint32_t current_time = k_uptime_get_32();
	struct net_if_addr *ifaddr, *next;
	int32_t delay = -1;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_dad_timers,
					  ifaddr, next, dad_node) {
		struct net_if_addr *tmp;
		struct net_if *iface;

		/* DAD entries are ordered by construction.  Stop when
		 * we find one that hasn't expired.
		 */
		delay = (int32_t)(ifaddr->dad_start +
				  DAD_TIMEOUT - current_time);
		if (delay > 0) {
			break;
		}

		/* Removing the ifaddr from active_dad_timers list */
		sys_slist_remove(&active_dad_timers, NULL, &ifaddr->dad_node);

		NET_DBG("DAD succeeded for %s",
			log_strdup(net_sprint_ipv6_addr(
					   &ifaddr->address.in6_addr)));

		ifaddr->addr_state = NET_ADDR_PREFERRED;

		/* Because we do not know the interface at this point,
		 * we need to lookup for it.
		 */
		iface = NULL;
		tmp = net_if_ipv6_addr_lookup(&ifaddr->address.in6_addr,
					      &iface);
		if (tmp == ifaddr) {
			net_mgmt_event_notify_with_info(
					NET_EVENT_IPV6_DAD_SUCCEED,
					iface, &ifaddr->address.in6_addr,
					sizeof(struct in6_addr));

			/* The address gets added to neighbor cache which is not
			 * needed in this case as the address is our own one.
			 */
			net_ipv6_nbr_rm(iface, &ifaddr->address.in6_addr);
		}

		ifaddr = NULL;
	}

	if ((ifaddr != NULL) && (delay > 0)) {
		k_delayed_work_submit(&dad_timer, K_MSEC((uint32_t)delay));
	}
}

static void net_if_ipv6_start_dad(struct net_if *iface,
				  struct net_if_addr *ifaddr)
{
	ifaddr->addr_state = NET_ADDR_TENTATIVE;

	if (net_if_is_up(iface)) {
		NET_DBG("Interface %p ll addr %s tentative IPv6 addr %s",
			iface,
			log_strdup(net_sprint_ll_addr(
					   net_if_get_link_addr(iface)->addr,
					   net_if_get_link_addr(iface)->len)),
			log_strdup(net_sprint_ipv6_addr(
					   &ifaddr->address.in6_addr)));

		ifaddr->dad_count = 1U;

		if (!net_ipv6_start_dad(iface, ifaddr)) {
			ifaddr->dad_start = k_uptime_get_32();
			sys_slist_append(&active_dad_timers, &ifaddr->dad_node);

			/* FUTURE: use schedule, not reschedule. */
			if (!k_delayed_work_remaining_get(&dad_timer)) {
				k_delayed_work_submit(&dad_timer,
						      K_MSEC(DAD_TIMEOUT));
			}
		}
	} else {
		NET_DBG("Interface %p is down, starting DAD for %s later.",
			iface,
			log_strdup(net_sprint_ipv6_addr(
					   &ifaddr->address.in6_addr)));
	}
}

void net_if_start_dad(struct net_if *iface)
{
	struct net_if_addr *ifaddr;
	struct net_if_ipv6 *ipv6;
	struct in6_addr addr = { };
	int i;

	NET_DBG("Starting DAD for iface %p", iface);

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		NET_WARN("Cannot do DAD IPv6 config is not valid.");
		return;
	}

	if (!ipv6) {
		return;
	}

	net_ipv6_addr_create_iid(&addr, net_if_get_link_addr(iface));

	ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s address to interface %p, DAD fails",
			log_strdup(net_sprint_ipv6_addr(&addr)), iface);
	}

	/* Start DAD for all the addresses that were added earlier when
	 * the interface was down.
	 */
	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    &ipv6->unicast[i] == ifaddr) {
			continue;
		}

		net_if_ipv6_start_dad(iface, &ipv6->unicast[i]);
	}
}

void net_if_ipv6_dad_failed(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_if_addr *ifaddr;

	ifaddr = net_if_ipv6_addr_lookup(addr, &iface);
	if (!ifaddr) {
		NET_ERR("Cannot find %s address in interface %p",
			log_strdup(net_sprint_ipv6_addr(addr)), iface);
		return;
	}

	sys_slist_find_and_remove(&active_dad_timers, &ifaddr->dad_node);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_DAD_FAILED, iface,
					&ifaddr->address.in6_addr,
					sizeof(struct in6_addr));

	net_if_ipv6_addr_rm(iface, addr);
}

static inline void iface_ipv6_dad_init(void)
{
	k_delayed_work_init(&dad_timer, dad_timeout);
	sys_slist_init(&active_dad_timers);
}

#else
static inline void net_if_ipv6_start_dad(struct net_if *iface,
					 struct net_if_addr *ifaddr)
{
	ifaddr->addr_state = NET_ADDR_PREFERRED;
}

#define iface_ipv6_dad_init(...)
#endif /* CONFIG_NET_IPV6_DAD */

#if defined(CONFIG_NET_IPV6_ND)
#define RS_TIMEOUT (1U * MSEC_PER_SEC)
#define RS_COUNT 3

static void rs_timeout(struct k_work *work)
{
	uint32_t current_time = k_uptime_get_32();
	struct net_if_ipv6 *ipv6, *next;
	int32_t delay = -1;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_rs_timers,
					  ipv6, next, rs_node) {
		struct net_if *iface = NULL;

		/* RS entries are ordered by construction.  Stop when
		 * we find one that hasn't expired.
		 */
		delay = (int32_t)(ipv6->rs_start + RS_TIMEOUT - current_time);
		if (delay > 0) {
			break;
		}

		/* Removing the ipv6 from active_rs_timers list */
		sys_slist_remove(&active_rs_timers, NULL, &ipv6->rs_node);

		/* Did not receive RA yet. */
		ipv6->rs_count++;

		Z_STRUCT_SECTION_FOREACH(net_if, tmp) {
			if (tmp->config.ip.ipv6 == ipv6) {
				iface = tmp;
				break;
			}
		}

		if (iface) {
			NET_DBG("RS no respond iface %p count %d",
				iface, ipv6->rs_count);
			if (ipv6->rs_count < RS_COUNT) {
				net_if_start_rs(iface);
			}
		} else {
			NET_DBG("Interface IPv6 config %p not found", ipv6);
		}

		ipv6 = NULL;
	}

	if ((ipv6 != NULL) && (delay > 0)) {
		k_delayed_work_submit(&rs_timer,
				      K_MSEC(ipv6->rs_start +
					     RS_TIMEOUT - current_time));
	}
}

void net_if_start_rs(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	if (!ipv6) {
		return;
	}

	NET_DBG("Starting ND/RS for iface %p", iface);

	if (!net_ipv6_start_rs(iface)) {
		ipv6->rs_start = k_uptime_get_32();
		sys_slist_append(&active_rs_timers, &ipv6->rs_node);

		/* FUTURE: use schedule, not reschedule. */
		if (!k_delayed_work_remaining_get(&rs_timer)) {
			k_delayed_work_submit(&rs_timer, K_MSEC(RS_TIMEOUT));
		}
	}
}

void net_if_stop_rs(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	if (!ipv6) {
		return;
	}

	NET_DBG("Stopping ND/RS for iface %p", iface);

	sys_slist_find_and_remove(&active_rs_timers, &ipv6->rs_node);
}

static inline void iface_ipv6_nd_init(void)
{
	k_delayed_work_init(&rs_timer, rs_timeout);
	sys_slist_init(&active_rs_timers);
}

#else
#define net_if_start_rs(...)
#define net_if_stop_rs(...)
#define iface_ipv6_nd_init(...)
#endif /* CONFIG_NET_IPV6_ND */

struct net_if_addr *net_if_ipv6_addr_lookup(const struct in6_addr *addr,
					    struct net_if **ret)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
		int i;

		if (!ipv6) {
			continue;
		}

		for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			if (!ipv6->unicast[i].is_used ||
			    ipv6->unicast[i].address.family != AF_INET6) {
				continue;
			}

			if (net_ipv6_is_prefix(
				    addr->s6_addr,
				    ipv6->unicast[i].address.in6_addr.s6_addr,
				    128)) {

				if (ret) {
					*ret = iface;
				}

				return &ipv6->unicast[i];
			}
		}
	}

	return NULL;
}

struct net_if_addr *net_if_ipv6_addr_lookup_by_iface(struct net_if *iface,
						     struct in6_addr *addr)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		if (net_ipv6_is_prefix(
			    addr->s6_addr,
			    ipv6->unicast[i].address.in6_addr.s6_addr,
			    128)) {
			return &ipv6->unicast[i];
		}
	}

	return NULL;
}

int z_impl_net_if_ipv6_addr_lookup_by_index(const struct in6_addr *addr)
{
	struct net_if *iface = NULL;
	struct net_if_addr *if_addr;

	if_addr = net_if_ipv6_addr_lookup(addr, &iface);
	if (!if_addr) {
		return 0;
	}

	return net_if_get_by_iface(iface);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_net_if_ipv6_addr_lookup_by_index(
					  const struct in6_addr *addr)
{
	struct in6_addr addr_v6;

	Z_OOPS(z_user_from_copy(&addr_v6, (void *)addr, sizeof(addr_v6)));

	return z_impl_net_if_ipv6_addr_lookup_by_index(&addr_v6);
}
#include <syscalls/net_if_ipv6_addr_lookup_by_index_mrsh.c>
#endif

static void address_expired(struct net_if_addr *ifaddr)
{
	NET_DBG("IPv6 address %s is deprecated",
		log_strdup(net_sprint_ipv6_addr(&ifaddr->address.in6_addr)));

	ifaddr->addr_state = NET_ADDR_DEPRECATED;

	sys_slist_find_and_remove(&active_address_lifetime_timers,
				  &ifaddr->lifetime.node);

	net_timeout_set(&ifaddr->lifetime, 0, 0);
}

static void address_lifetime_timeout(struct k_work *work)
{
	uint32_t next_update = UINT32_MAX;
	uint32_t current_time = k_uptime_get_32();
	struct net_if_addr *current, *next;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_address_lifetime_timers,
					  current, next, lifetime.node) {
		struct net_timeout *timeout = &current->lifetime;
		uint32_t this_update = net_timeout_evaluate(timeout,
							     current_time);

		if (this_update == 0U) {
			address_expired(current);
			continue;
		}

		if (this_update < next_update) {
			next_update = this_update;
		}

		if (current == next) {
			break;
		}
	}

	if (next_update != UINT32_MAX) {
		NET_DBG("Waiting for %d ms", (int32_t)next_update);

		k_delayed_work_submit(&address_lifetime_timer,
				      K_MSEC(next_update));
	}
}

#if defined(CONFIG_NET_TEST)
void net_address_lifetime_timeout(void)
{
	address_lifetime_timeout(NULL);
}
#endif

static void address_start_timer(struct net_if_addr *ifaddr, uint32_t vlifetime)
{
	sys_slist_append(&active_address_lifetime_timers,
			 &ifaddr->lifetime.node);

	net_timeout_set(&ifaddr->lifetime, vlifetime, k_uptime_get_32());
	k_delayed_work_submit(&address_lifetime_timer, K_NO_WAIT);
}

void net_if_ipv6_addr_update_lifetime(struct net_if_addr *ifaddr,
				      uint32_t vlifetime)
{
	NET_DBG("Updating expire time of %s by %u secs",
		log_strdup(net_sprint_ipv6_addr(&ifaddr->address.in6_addr)),
		vlifetime);

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	address_start_timer(ifaddr, vlifetime);
}

static struct net_if_addr *ipv6_addr_find(struct net_if *iface,
					  struct in6_addr *addr)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used) {
			continue;
		}

		if (net_ipv6_addr_cmp(
			    addr, &ipv6->unicast[i].address.in6_addr)) {

			return &ipv6->unicast[i];
		}
	}

	return NULL;
}

static inline void net_if_addr_init(struct net_if_addr *ifaddr,
				    struct in6_addr *addr,
				    enum net_addr_type addr_type,
				    uint32_t vlifetime)
{
	ifaddr->is_used = true;
	ifaddr->address.family = AF_INET6;
	ifaddr->addr_type = addr_type;
	net_ipaddr_copy(&ifaddr->address.in6_addr, addr);

	/* FIXME - set the mcast addr for this node */

	if (vlifetime) {
		ifaddr->is_infinite = false;

		NET_DBG("Expiring %s in %u secs",
			log_strdup(net_sprint_ipv6_addr(addr)),
			vlifetime);

		net_if_ipv6_addr_update_lifetime(ifaddr, vlifetime);
	} else {
		ifaddr->is_infinite = true;
	}
}

struct net_if_addr *net_if_ipv6_addr_add(struct net_if *iface,
					 struct in6_addr *addr,
					 enum net_addr_type addr_type,
					 uint32_t vlifetime)
{
	struct net_if_addr *ifaddr;
	struct net_if_ipv6 *ipv6;
	int i;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return NULL;
	}

	ifaddr = ipv6_addr_find(iface, addr);
	if (ifaddr) {
		return ifaddr;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (ipv6->unicast[i].is_used) {
			continue;
		}

		net_if_addr_init(&ipv6->unicast[i], addr, addr_type,
				 vlifetime);

		NET_DBG("[%d] interface %p address %s type %s added", i,
			iface, log_strdup(net_sprint_ipv6_addr(addr)),
			net_addr_type2str(addr_type));

		if (!(l2_flags_get(iface) & NET_L2_POINT_TO_POINT)) {
			/* RFC 4862 5.4.2
			 * Before sending a Neighbor Solicitation, an interface
			 * MUST join the all-nodes multicast address and the
			 * solicited-node multicast address of the tentative
			 * address.
			 */
			/* The allnodes multicast group is only joined once as
			 * net_ipv6_mcast_join() checks if we have already
			 * joined.
			 */
			join_mcast_nodes(iface,
					 &ipv6->unicast[i].address.in6_addr);

			net_if_ipv6_start_dad(iface, &ipv6->unicast[i]);
		}

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_ADDR_ADD, iface,
			&ipv6->unicast[i].address.in6_addr,
			sizeof(struct in6_addr));

		return &ipv6->unicast[i];
	}

	return NULL;
}

bool net_if_ipv6_addr_rm(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	NET_ASSERT(addr);

	if (!ipv6) {
		return false;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		struct in6_addr maddr;

		if (!ipv6->unicast[i].is_used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(&ipv6->unicast[i].address.in6_addr,
				       addr)) {
			continue;
		}

		if (!ipv6->unicast[i].is_infinite) {
			sys_slist_find_and_remove(
				&active_address_lifetime_timers,
				&ipv6->unicast[i].lifetime.node);

			if (sys_slist_is_empty(
				    &active_address_lifetime_timers)) {
				k_delayed_work_cancel(&address_lifetime_timer);
			}
		}

		ipv6->unicast[i].is_used = false;

		net_ipv6_addr_create_solicited_node(addr, &maddr);

		net_if_ipv6_maddr_rm(iface, &maddr);

		NET_DBG("[%d] interface %p address %s type %s removed",
			i, iface, log_strdup(net_sprint_ipv6_addr(addr)),
			net_addr_type2str(ipv6->unicast[i].addr_type));

		/* Using the IPv6 address pointer here can give false
		 * info if someone adds a new IP address into this position
		 * in the address array. This is quite unlikely thou.
		 */
		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_ADDR_DEL,
			iface,
			&ipv6->unicast[i].address.in6_addr,
			sizeof(struct in6_addr));

		return true;
	}

	return false;
}

bool z_impl_net_if_ipv6_addr_add_by_index(int index,
					  struct in6_addr *addr,
					  enum net_addr_type addr_type,
					  uint32_t vlifetime)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	return net_if_ipv6_addr_add(iface, addr, addr_type, vlifetime) ?
		true : false;
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_net_if_ipv6_addr_add_by_index(int index,
					  struct in6_addr *addr,
					  enum net_addr_type addr_type,
					  uint32_t vlifetime)
{
	struct in6_addr addr_v6;
	struct net_if *iface;

	iface = z_vrfy_net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	Z_OOPS(z_user_from_copy(&addr_v6, (void *)addr, sizeof(addr_v6)));

	return z_impl_net_if_ipv6_addr_add_by_index(index,
						    &addr_v6,
						    addr_type,
						    vlifetime);
}

#include <syscalls/net_if_ipv6_addr_add_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

bool z_impl_net_if_ipv6_addr_rm_by_index(int index,
					 const struct in6_addr *addr)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	return net_if_ipv6_addr_rm(iface, addr);
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_net_if_ipv6_addr_rm_by_index(int index,
					 const struct in6_addr *addr)
{
	struct in6_addr addr_v6;
	struct net_if *iface;

	iface = z_vrfy_net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	Z_OOPS(z_user_from_copy(&addr_v6, (void *)addr, sizeof(addr_v6)));

	return z_impl_net_if_ipv6_addr_rm_by_index(index, &addr_v6);
}

#include <syscalls/net_if_ipv6_addr_rm_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

struct net_if_mcast_addr *net_if_ipv6_maddr_add(struct net_if *iface,
						const struct in6_addr *addr)
{
	struct net_if_ipv6 *ipv6;
	int i;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return NULL;
	}

	if (!net_ipv6_is_addr_mcast(addr)) {
		NET_DBG("Address %s is not a multicast address.",
			log_strdup(net_sprint_ipv6_addr(addr)));
		return NULL;
	}

	if (net_if_ipv6_maddr_lookup(addr, &iface)) {
		NET_WARN("Multicast address %s is is already registered.",
			log_strdup(net_sprint_ipv6_addr(addr)));
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (ipv6->mcast[i].is_used) {
			continue;
		}

		ipv6->mcast[i].is_used = true;
		ipv6->mcast[i].address.family = AF_INET6;
		memcpy(&ipv6->mcast[i].address.in6_addr, addr, 16);

		NET_DBG("[%d] interface %p address %s added", i, iface,
			log_strdup(net_sprint_ipv6_addr(addr)));

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_MADDR_ADD, iface,
			&ipv6->mcast[i].address.in6_addr,
			sizeof(struct in6_addr));

		return &ipv6->mcast[i];
	}

	return NULL;
}

bool net_if_ipv6_maddr_rm(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return false;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(&ipv6->mcast[i].address.in6_addr,
				       addr)) {
			continue;
		}

		ipv6->mcast[i].is_used = false;

		NET_DBG("[%d] interface %p address %s removed",
			i, iface, log_strdup(net_sprint_ipv6_addr(addr)));

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_MADDR_DEL, iface,
			&ipv6->mcast[i].address.in6_addr,
			sizeof(struct in6_addr));

		return true;
	}

	return false;
}

struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(const struct in6_addr *maddr,
						   struct net_if **ret)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
		int i;

		if (ret && *ret && iface != *ret) {
			continue;
		}

		if (!ipv6) {
			continue;
		}

		for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
			if (!ipv6->mcast[i].is_used ||
			    ipv6->mcast[i].address.family != AF_INET6) {
				continue;
			}

			if (net_ipv6_is_prefix(
				    maddr->s6_addr,
				    ipv6->mcast[i].address.in6_addr.s6_addr,
				    128)) {
				if (ret) {
					*ret = iface;
				}

				return &ipv6->mcast[i];
			}
		}
	}

	return NULL;
}

void net_if_mcast_mon_register(struct net_if_mcast_monitor *mon,
			       struct net_if *iface,
			       net_if_mcast_callback_t cb)
{
	sys_slist_find_and_remove(&mcast_monitor_callbacks, &mon->node);
	sys_slist_prepend(&mcast_monitor_callbacks, &mon->node);

	mon->iface = iface;
	mon->cb = cb;
}

void net_if_mcast_mon_unregister(struct net_if_mcast_monitor *mon)
{
	sys_slist_find_and_remove(&mcast_monitor_callbacks, &mon->node);
}

void net_if_mcast_monitor(struct net_if *iface,
			  const struct in6_addr *addr,
			  bool is_joined)
{
	struct net_if_mcast_monitor *mon, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&mcast_monitor_callbacks,
					  mon, tmp, node) {
		if (iface == mon->iface) {
			mon->cb(iface, addr, is_joined);
		}
	}
}

static void remove_prefix_addresses(struct net_if *iface,
				    struct net_if_ipv6 *ipv6,
				    struct in6_addr *addr,
				    uint8_t len)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    ipv6->unicast[i].addr_type != NET_ADDR_AUTOCONF) {
			continue;
		}

		if (net_ipv6_is_prefix(
				addr->s6_addr,
				ipv6->unicast[i].address.in6_addr.s6_addr,
				len)) {
			net_if_ipv6_addr_rm(iface,
					    &ipv6->unicast[i].address.in6_addr);
		}
	}
}

static void prefix_lifetime_expired(struct net_if_ipv6_prefix *ifprefix)
{
	struct net_if_ipv6 *ipv6;

	NET_DBG("Prefix %s/%d expired",
		log_strdup(net_sprint_ipv6_addr(&ifprefix->prefix)),
		ifprefix->len);

	ifprefix->is_used = false;

	if (net_if_config_ipv6_get(ifprefix->iface, &ipv6) < 0) {
		return;
	}

	/* Remove also all auto addresses if the they have the same prefix.
	 */
	remove_prefix_addresses(ifprefix->iface, ipv6, &ifprefix->prefix,
				ifprefix->len);

	net_mgmt_event_notify_with_info(
		NET_EVENT_IPV6_PREFIX_DEL, ifprefix->iface,
		&ifprefix->prefix, sizeof(struct in6_addr));
}

static void prefix_timer_remove(struct net_if_ipv6_prefix *ifprefix)
{
	NET_DBG("IPv6 prefix %s/%d removed",
		log_strdup(net_sprint_ipv6_addr(&ifprefix->prefix)),
		ifprefix->len);

	sys_slist_find_and_remove(&active_prefix_lifetime_timers,
				  &ifprefix->lifetime.node);

	net_timeout_set(&ifprefix->lifetime, 0, 0);
}

static void prefix_lifetime_timeout(struct k_work *work)
{
	uint32_t next_update = UINT32_MAX;
	uint32_t current_time = k_uptime_get_32();
	struct net_if_ipv6_prefix *current, *next;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_prefix_lifetime_timers,
					  current, next, lifetime.node) {
		struct net_timeout *timeout = &current->lifetime;
		uint32_t this_update = net_timeout_evaluate(timeout,
							    current_time);

		if (this_update == 0U) {
			prefix_lifetime_expired(current);
			continue;
		}

		if (this_update < next_update) {
			next_update = this_update;
		}

		if (current == next) {
			break;
		}
	}

	if (next_update != UINT32_MAX) {
		k_delayed_work_submit(&prefix_lifetime_timer,
				      K_MSEC(next_update));
	}
}

static void prefix_start_timer(struct net_if_ipv6_prefix *ifprefix,
			       uint32_t lifetime)
{
	(void)sys_slist_find_and_remove(&active_prefix_lifetime_timers,
					&ifprefix->lifetime.node);
	sys_slist_append(&active_prefix_lifetime_timers,
			 &ifprefix->lifetime.node);

	net_timeout_set(&ifprefix->lifetime, lifetime, k_uptime_get_32());
	k_delayed_work_submit(&prefix_lifetime_timer, K_NO_WAIT);
}

static struct net_if_ipv6_prefix *ipv6_prefix_find(struct net_if *iface,
						   struct in6_addr *prefix,
						   uint8_t prefix_len)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!ipv6->unicast[i].is_used) {
			continue;
		}

		if (net_ipv6_addr_cmp(prefix, &ipv6->prefix[i].prefix) &&
		    prefix_len == ipv6->prefix[i].len) {
			return &ipv6->prefix[i];
		}
	}

	return NULL;
}

static void net_if_ipv6_prefix_init(struct net_if *iface,
				    struct net_if_ipv6_prefix *ifprefix,
				    struct in6_addr *addr, uint8_t len,
				    uint32_t lifetime)
{
	ifprefix->is_used = true;
	ifprefix->len = len;
	ifprefix->iface = iface;
	net_ipaddr_copy(&ifprefix->prefix, addr);

	if (lifetime == NET_IPV6_ND_INFINITE_LIFETIME) {
		ifprefix->is_infinite = true;
	} else {
		ifprefix->is_infinite = false;
	}
}

struct net_if_ipv6_prefix *net_if_ipv6_prefix_add(struct net_if *iface,
						  struct in6_addr *prefix,
						  uint8_t len,
						  uint32_t lifetime)
{
	struct net_if_ipv6_prefix *ifprefix;
	struct net_if_ipv6 *ipv6;
	int i;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return NULL;
	}

	ifprefix = ipv6_prefix_find(iface, prefix, len);
	if (ifprefix) {
		return ifprefix;
	}

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (ipv6->prefix[i].is_used) {
			continue;
		}

		net_if_ipv6_prefix_init(iface, &ipv6->prefix[i], prefix,
					len, lifetime);

		NET_DBG("[%d] interface %p prefix %s/%d added", i, iface,
			log_strdup(net_sprint_ipv6_addr(prefix)), len);

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_PREFIX_ADD, iface,
			&ipv6->prefix[i].prefix, sizeof(struct in6_addr));

		return &ipv6->prefix[i];
	}

	return NULL;
}

bool net_if_ipv6_prefix_rm(struct net_if *iface, struct in6_addr *addr,
			   uint8_t len)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return false;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!ipv6->prefix[i].is_used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(&ipv6->prefix[i].prefix, addr) ||
		    ipv6->prefix[i].len != len) {
			continue;
		}

		net_if_ipv6_prefix_unset_timer(&ipv6->prefix[i]);

		ipv6->prefix[i].is_used = false;

		/* Remove also all auto addresses if the they have the same
		 * prefix.
		 */
		remove_prefix_addresses(iface, ipv6, addr, len);

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_PREFIX_DEL, iface,
			&ipv6->prefix[i].prefix, sizeof(struct in6_addr));

		return true;
	}

	return false;
}

struct net_if_ipv6_prefix *net_if_ipv6_prefix_get(struct net_if *iface,
						  struct in6_addr *addr)
{
	struct net_if_ipv6_prefix *prefix = NULL;
	struct net_if_ipv6 *ipv6;
	int i;

	if (!iface) {
		iface = net_if_get_default();
	}

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!ipv6->prefix[i].is_used) {
			continue;
		}

		if (net_ipv6_is_prefix(ipv6->prefix[i].prefix.s6_addr,
				       addr->s6_addr,
				       ipv6->prefix[i].len)) {
			if (!prefix || prefix->len > ipv6->prefix[i].len) {
				prefix = &ipv6->prefix[i];
			}
		}
	}

	return prefix;
}

struct net_if_ipv6_prefix *net_if_ipv6_prefix_lookup(struct net_if *iface,
						     struct in6_addr *addr,
						     uint8_t len)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!ipv6->prefix[i].is_used) {
			continue;
		}

		if (net_ipv6_is_prefix(ipv6->prefix[i].prefix.s6_addr,
				       addr->s6_addr, len)) {
			return &ipv6->prefix[i];
		}
	}

	return NULL;
}

bool net_if_ipv6_addr_onlink(struct net_if **iface, struct in6_addr *addr)
{
	Z_STRUCT_SECTION_FOREACH(net_if, tmp) {
		struct net_if_ipv6 *ipv6 = tmp->config.ip.ipv6;
		int i;

		if (iface && *iface && *iface != tmp) {
			continue;
		}

		if (!ipv6) {
			continue;
		}

		for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
			if (ipv6->prefix[i].is_used &&
			    net_ipv6_is_prefix(ipv6->prefix[i].prefix.s6_addr,
					       addr->s6_addr,
					       ipv6->prefix[i].len)) {
				if (iface) {
					*iface = tmp;
				}

				return true;
			}
		}
	}

	return false;
}

void net_if_ipv6_prefix_set_timer(struct net_if_ipv6_prefix *prefix,
				  uint32_t lifetime)
{
	/* No need to set a timer for infinite timeout */
	if (lifetime == 0xffffffff) {
		return;
	}

	NET_DBG("Prefix lifetime %u sec", lifetime);

	prefix_start_timer(prefix, lifetime);
}

void net_if_ipv6_prefix_unset_timer(struct net_if_ipv6_prefix *prefix)
{
	if (!prefix->is_used) {
		return;
	}

	prefix_timer_remove(prefix);
}

struct net_if_router *net_if_ipv6_router_lookup(struct net_if *iface,
						struct in6_addr *addr)
{
	return iface_router_lookup(iface, AF_INET6, addr);
}

struct net_if_router *net_if_ipv6_router_find_default(struct net_if *iface,
						      struct in6_addr *addr)
{
	return iface_router_find_default(iface, AF_INET6, addr);
}

void net_if_ipv6_router_update_lifetime(struct net_if_router *router,
					uint16_t lifetime)
{
	NET_DBG("Updating expire time of %s by %u secs",
		log_strdup(net_sprint_ipv6_addr(&router->address.in6_addr)),
		lifetime);

	router->life_start = k_uptime_get_32();
	router->lifetime = lifetime;

	iface_router_update_timer(router->life_start);
}

struct net_if_router *net_if_ipv6_router_add(struct net_if *iface,
					     struct in6_addr *addr,
					     uint16_t lifetime)
{
	return iface_router_add(iface, AF_INET6, addr, false, lifetime);
}

bool net_if_ipv6_router_rm(struct net_if_router *router)
{
	return iface_router_rm(router);
}

struct in6_addr *net_if_ipv6_get_ll(struct net_if *iface,
				    enum net_addr_state addr_state)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    (addr_state != NET_ADDR_ANY_STATE &&
		     ipv6->unicast[i].addr_state != addr_state) ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		if (net_ipv6_is_ll_addr(&ipv6->unicast[i].address.in6_addr)) {
			return &ipv6->unicast[i].address.in6_addr;
		}
	}

	return NULL;
}

struct in6_addr *net_if_ipv6_get_ll_addr(enum net_addr_state state,
					 struct net_if **iface)
{
	Z_STRUCT_SECTION_FOREACH(net_if, tmp) {
		struct in6_addr *addr;

		addr = net_if_ipv6_get_ll(tmp, state);
		if (addr) {
			if (iface) {
				*iface = tmp;
			}

			return addr;
		}
	}

	return NULL;
}

static inline struct in6_addr *check_global_addr(struct net_if *iface,
						 enum net_addr_state state)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    (ipv6->unicast[i].addr_state != state) ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		if (!net_ipv6_is_ll_addr(&ipv6->unicast[i].address.in6_addr)) {
			return &ipv6->unicast[i].address.in6_addr;
		}
	}

	return NULL;
}

struct in6_addr *net_if_ipv6_get_global_addr(enum net_addr_state state,
					     struct net_if **iface)
{
	Z_STRUCT_SECTION_FOREACH(net_if, tmp) {
		struct in6_addr *addr;

		if (iface && *iface && tmp != *iface) {
			continue;
		}

		addr = check_global_addr(tmp, state);
		if (addr) {
			if (iface) {
				*iface = tmp;
			}

			return addr;
		}
	}

	return NULL;
}

static uint8_t get_diff_ipv6(const struct in6_addr *src,
			  const struct in6_addr *dst)
{
	return get_ipaddr_diff((const uint8_t *)src, (const uint8_t *)dst, 16);
}

static inline bool is_proper_ipv6_address(struct net_if_addr *addr)
{
	if (addr->is_used && addr->addr_state == NET_ADDR_PREFERRED &&
	    addr->address.family == AF_INET6 &&
	    !net_ipv6_is_ll_addr(&addr->address.in6_addr)) {
		return true;
	}

	return false;
}

static struct in6_addr *net_if_ipv6_get_best_match(struct net_if *iface,
						   const struct in6_addr *dst,
						   uint8_t *best_so_far)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	struct in6_addr *src = NULL;
	uint8_t len;
	int i;

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!is_proper_ipv6_address(&ipv6->unicast[i])) {
			continue;
		}

		len = get_diff_ipv6(dst, &ipv6->unicast[i].address.in6_addr);
		if (len >= *best_so_far) {
			/* Mesh local address can only be selected for the same
			 * subnet.
			 */
			if (ipv6->unicast[i].is_mesh_local && len < 64 &&
			    !net_ipv6_is_addr_mcast_mesh(dst)) {
				continue;
			}

			*best_so_far = len;
			src = &ipv6->unicast[i].address.in6_addr;
		}
	}

	return src;
}

const struct in6_addr *net_if_ipv6_select_src_addr(struct net_if *dst_iface,
						   const struct in6_addr *dst)
{
	struct in6_addr *src = NULL;
	uint8_t best_match = 0U;

	if (!net_ipv6_is_ll_addr(dst) && (!net_ipv6_is_addr_mcast(dst) ||
	    net_ipv6_is_addr_mcast_mesh(dst))) {

		/* If caller has supplied interface, then use that */
		if (dst_iface) {
			src = net_if_ipv6_get_best_match(dst_iface, dst,
							 &best_match);
		} else {
			Z_STRUCT_SECTION_FOREACH(net_if, iface) {
				struct in6_addr *addr;

				addr = net_if_ipv6_get_best_match(iface, dst,
								  &best_match);
				if (addr) {
					src = addr;
				}
			}
		}

	} else {
		if (dst_iface) {
			src = net_if_ipv6_get_ll(dst_iface, NET_ADDR_PREFERRED);
		} else {
			Z_STRUCT_SECTION_FOREACH(net_if, iface) {
				struct in6_addr *addr;

				addr = net_if_ipv6_get_ll(iface,
							  NET_ADDR_PREFERRED);
				if (addr) {
					src = addr;
					break;
				}
			}
		}
	}

	if (!src) {
		return net_ipv6_unspecified_address();
	}

	return src;
}

struct net_if *net_if_ipv6_select_src_iface(const struct in6_addr *dst)
{
	const struct in6_addr *src;
	struct net_if *iface;

	src = net_if_ipv6_select_src_addr(NULL, dst);
	if (src == net_ipv6_unspecified_address()) {
		return net_if_get_default();
	}

	if (!net_if_ipv6_addr_lookup(src, &iface)) {
		return net_if_get_default();
	}

	return iface;
}

uint32_t net_if_ipv6_calc_reachable_time(struct net_if_ipv6 *ipv6)
{
	uint32_t min_reachable, max_reachable;

	min_reachable = (MIN_RANDOM_NUMER * ipv6->base_reachable_time)
			/ MIN_RANDOM_DENOM;
	max_reachable = (MAX_RANDOM_NUMER * ipv6->base_reachable_time)
			/ MAX_RANDOM_DENOM;

	NET_DBG("min_reachable:%u max_reachable:%u", min_reachable,
		max_reachable);

	return min_reachable +
	       sys_rand32_get() % (max_reachable - min_reachable);
}

static void iface_ipv6_start(struct net_if *iface)
{
	if (IS_ENABLED(CONFIG_NET_IPV6_DAD)) {
		net_if_start_dad(iface);
	} else {
		struct net_if_ipv6 *ipv6 __unused = iface->config.ip.ipv6;

		join_mcast_nodes(iface,
				 &ipv6->mcast[0].address.in6_addr);
	}

	net_if_start_rs(iface);
}

static void iface_ipv6_init(int if_count)
{
	int i;

	iface_ipv6_dad_init();
	iface_ipv6_nd_init();

	k_delayed_work_init(&address_lifetime_timer, address_lifetime_timeout);
	k_delayed_work_init(&prefix_lifetime_timer, prefix_lifetime_timeout);

	if (if_count > ARRAY_SIZE(ipv6_addresses)) {
		NET_WARN("You have %lu IPv6 net_if addresses but %d "
			 "network interfaces", ARRAY_SIZE(ipv6_addresses),
			 if_count);
		NET_WARN("Consider increasing CONFIG_NET_IF_MAX_IPV6_COUNT "
			 "value.");
	}

	for (i = 0; i < ARRAY_SIZE(ipv6_addresses); i++) {
		ipv6_addresses[i].ipv6.hop_limit = CONFIG_NET_INITIAL_HOP_LIMIT;
		ipv6_addresses[i].ipv6.base_reachable_time = REACHABLE_TIME;

		net_if_ipv6_set_reachable_time(&ipv6_addresses[i].ipv6);
	}
}

#else
#define join_mcast_allnodes(...)
#define join_mcast_solicit_node(...)
#define leave_mcast_all(...)
#define join_mcast_nodes(...)
#define iface_ipv6_start(...)
#define iface_ipv6_init(...)

struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(const struct in6_addr *addr,
						   struct net_if **iface)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(iface);

	return NULL;
}

struct net_if_addr *net_if_ipv6_addr_lookup(const struct in6_addr *addr,
					    struct net_if **ret)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(ret);

	return NULL;
}

struct in6_addr *net_if_ipv6_get_global_addr(enum net_addr_state state,
					     struct net_if **iface)
{
	ARG_UNUSED(state);
	ARG_UNUSED(iface);

	return NULL;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_NATIVE_IPV4)
int net_if_config_ipv4_get(struct net_if *iface, struct net_if_ipv4 **ipv4)
{
	int i;

	if (iface->config.ip.ipv4) {
		if (ipv4) {
			*ipv4 = iface->config.ip.ipv4;
		}

		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(ipv4_addresses); i++) {
		if (ipv4_addresses[i].iface) {
			continue;
		}

		iface->config.ip.ipv4 = &ipv4_addresses[i].ipv4;
		ipv4_addresses[i].iface = iface;

		if (ipv4) {
			*ipv4 = &ipv4_addresses[i].ipv4;
		}

		return 0;
	}

	return -ESRCH;
}

int net_if_config_ipv4_put(struct net_if *iface)
{
	int i;

	if (!iface->config.ip.ipv4) {
		return -EALREADY;
	}

	for (i = 0; i < ARRAY_SIZE(ipv4_addresses); i++) {
		if (ipv4_addresses[i].iface != iface) {
			continue;
		}

		iface->config.ip.ipv4 = NULL;
		ipv4_addresses[i].iface = NULL;

		return 0;
	}

	return 0;
}

struct net_if_router *net_if_ipv4_router_lookup(struct net_if *iface,
						struct in_addr *addr)
{
	return iface_router_lookup(iface, AF_INET, addr);
}

struct net_if_router *net_if_ipv4_router_find_default(struct net_if *iface,
						      struct in_addr *addr)
{
	return iface_router_find_default(iface, AF_INET, addr);
}

struct net_if_router *net_if_ipv4_router_add(struct net_if *iface,
					     struct in_addr *addr,
					     bool is_default,
					     uint16_t lifetime)
{
	return iface_router_add(iface, AF_INET, addr, is_default, lifetime);
}

bool net_if_ipv4_router_rm(struct net_if_router *router)
{
	return iface_router_rm(router);
}

bool net_if_ipv4_addr_mask_cmp(struct net_if *iface,
			       const struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	uint32_t subnet;
	int i;

	if (!ipv4) {
		return false;
	}

	subnet = UNALIGNED_GET(&addr->s_addr) & ipv4->netmask.s_addr;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!ipv4->unicast[i].is_used ||
		    ipv4->unicast[i].address.family != AF_INET) {
			continue;
		}

		if ((ipv4->unicast[i].address.in_addr.s_addr &
		     ipv4->netmask.s_addr) == subnet) {
			return true;
		}
	}

	return false;
}

static bool ipv4_is_broadcast_address(struct net_if *iface,
				      const struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;

	if (!ipv4) {
		return false;
	}

	if (!net_if_ipv4_addr_mask_cmp(iface, addr)) {
		return false;
	}

	if ((UNALIGNED_GET(&addr->s_addr) & ~ipv4->netmask.s_addr) ==
	    ~ipv4->netmask.s_addr) {
		return true;
	}

	return false;
}

bool net_if_ipv4_is_addr_bcast(struct net_if *iface,
			       const struct in_addr *addr)
{
	if (iface) {
		return ipv4_is_broadcast_address(iface, addr);
	}

	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		bool ret;

		ret = ipv4_is_broadcast_address(iface, addr);
		if (ret) {
			return ret;
		}
	}

	return false;
}

struct net_if *net_if_ipv4_select_src_iface(const struct in_addr *dst)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		bool ret;

		ret = net_if_ipv4_addr_mask_cmp(iface, dst);
		if (ret) {
			return iface;
		}
	}

	return net_if_get_default();
}

static uint8_t get_diff_ipv4(const struct in_addr *src,
			  const struct in_addr *dst)
{
	return get_ipaddr_diff((const uint8_t *)src, (const uint8_t *)dst, 4);
}

static inline bool is_proper_ipv4_address(struct net_if_addr *addr)
{
	if (addr->is_used && addr->addr_state == NET_ADDR_PREFERRED &&
	    addr->address.family == AF_INET &&
	    !net_ipv4_is_ll_addr(&addr->address.in_addr)) {
		return true;
	}

	return false;
}

static struct in_addr *net_if_ipv4_get_best_match(struct net_if *iface,
						  const struct in_addr *dst,
						  uint8_t *best_so_far)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	struct in_addr *src = NULL;
	uint8_t len;
	int i;

	if (!ipv4) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!is_proper_ipv4_address(&ipv4->unicast[i])) {
			continue;
		}

		len = get_diff_ipv4(dst, &ipv4->unicast[i].address.in_addr);
		if (len >= *best_so_far) {
			*best_so_far = len;
			src = &ipv4->unicast[i].address.in_addr;
		}
	}

	return src;
}

static struct in_addr *if_ipv4_get_addr(struct net_if *iface,
					enum net_addr_state addr_state, bool ll)
{
	struct net_if_ipv4 *ipv4;
	int i;

	if (!iface) {
		return NULL;
	}

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!ipv4->unicast[i].is_used ||
		    (addr_state != NET_ADDR_ANY_STATE &&
		     ipv4->unicast[i].addr_state != addr_state) ||
		    ipv4->unicast[i].address.family != AF_INET) {
			continue;
		}

		if (net_ipv4_is_ll_addr(&ipv4->unicast[i].address.in_addr)) {
			if (!ll) {
				continue;
			}
		} else {
			if (ll) {
				continue;
			}
		}

		return &ipv4->unicast[i].address.in_addr;
	}

	return NULL;
}

struct in_addr *net_if_ipv4_get_ll(struct net_if *iface,
				   enum net_addr_state addr_state)
{
	return if_ipv4_get_addr(iface, addr_state, true);
}

struct in_addr *net_if_ipv4_get_global_addr(struct net_if *iface,
					    enum net_addr_state addr_state)
{
	return if_ipv4_get_addr(iface, addr_state, false);
}

const struct in_addr *net_if_ipv4_select_src_addr(struct net_if *dst_iface,
						  const struct in_addr *dst)
{
	struct in_addr *src = NULL;
	uint8_t best_match = 0U;

	if (!net_ipv4_is_ll_addr(dst) && !net_ipv4_is_addr_mcast(dst)) {

		/* If caller has supplied interface, then use that */
		if (dst_iface) {
			src = net_if_ipv4_get_best_match(dst_iface, dst,
							 &best_match);
		} else {
			Z_STRUCT_SECTION_FOREACH(net_if, iface) {
				struct in_addr *addr;

				addr = net_if_ipv4_get_best_match(iface, dst,
								  &best_match);
				if (addr) {
					src = addr;
				}
			}
		}

	} else {
		if (dst_iface) {
			src = net_if_ipv4_get_ll(dst_iface, NET_ADDR_PREFERRED);
		} else {
			Z_STRUCT_SECTION_FOREACH(net_if, iface) {
				struct in_addr *addr;

				addr = net_if_ipv4_get_ll(iface,
							  NET_ADDR_PREFERRED);
				if (addr) {
					src = addr;
					break;
				}
			}
		}
	}

	if (!src) {
		src = net_if_ipv4_get_global_addr(dst_iface,
						  NET_ADDR_PREFERRED);
		if (src) {
			return src;
		}

		return net_ipv4_unspecified_address();
	}

	return src;
}

struct net_if_addr *net_if_ipv4_addr_lookup(const struct in_addr *addr,
					    struct net_if **ret)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
		int i;

		if (!ipv4) {
			continue;
		}

		for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (!ipv4->unicast[i].is_used ||
			    ipv4->unicast[i].address.family != AF_INET) {
				continue;
			}

			if (UNALIGNED_GET(&addr->s4_addr32[0]) ==
			    ipv4->unicast[i].address.in_addr.s_addr) {

				if (ret) {
					*ret = iface;
				}

				return &ipv4->unicast[i];
			}
		}
	}

	return NULL;
}

int z_impl_net_if_ipv4_addr_lookup_by_index(const struct in_addr *addr)
{
	struct net_if_addr *if_addr;
	struct net_if *iface = NULL;

	if_addr = net_if_ipv4_addr_lookup(addr, &iface);
	if (!if_addr) {
		return 0;
	}

	return net_if_get_by_iface(iface);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_net_if_ipv4_addr_lookup_by_index(
					  const struct in_addr *addr)
{
	struct in_addr addr_v4;

	Z_OOPS(z_user_from_copy(&addr_v4, (void *)addr, sizeof(addr_v4)));

	return z_impl_net_if_ipv4_addr_lookup_by_index(&addr_v4);
}
#include <syscalls/net_if_ipv4_addr_lookup_by_index_mrsh.c>
#endif

void net_if_ipv4_set_netmask(struct net_if *iface,
			     const struct in_addr *netmask)
{
	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		return;
	}

	if (!iface->config.ip.ipv4) {
		return;
	}

	net_ipaddr_copy(&iface->config.ip.ipv4->netmask, netmask);
}

bool z_impl_net_if_ipv4_set_netmask_by_index(int index,
					     const struct in_addr *netmask)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	net_if_ipv4_set_netmask(iface, netmask);

	return true;
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_net_if_ipv4_set_netmask_by_index(int index,
					     const struct in_addr *netmask)
{
	struct in_addr netmask_addr;
	struct net_if *iface;

	iface = z_vrfy_net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	Z_OOPS(z_user_from_copy(&netmask_addr, (void *)netmask,
				sizeof(netmask_addr)));

	return z_impl_net_if_ipv4_set_netmask_by_index(index, &netmask_addr);
}

#include <syscalls/net_if_ipv4_set_netmask_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

void net_if_ipv4_set_gw(struct net_if *iface, const struct in_addr *gw)
{
	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		return;
	}

	if (!iface->config.ip.ipv4) {
		return;
	}

	net_ipaddr_copy(&iface->config.ip.ipv4->gw, gw);
}

bool z_impl_net_if_ipv4_set_gw_by_index(int index,
					const struct in_addr *gw)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	net_if_ipv4_set_gw(iface, gw);

	return true;
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_net_if_ipv4_set_gw_by_index(int index,
					const struct in_addr *gw)
{
	struct in_addr gw_addr;
	struct net_if *iface;

	iface = z_vrfy_net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	Z_OOPS(z_user_from_copy(&gw_addr, (void *)gw, sizeof(gw_addr)));

	return z_impl_net_if_ipv4_set_gw_by_index(index, &gw_addr);
}

#include <syscalls/net_if_ipv4_set_gw_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

static struct net_if_addr *ipv4_addr_find(struct net_if *iface,
					  struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!ipv4->unicast[i].is_used) {
			continue;
		}

		if (net_ipv4_addr_cmp(addr,
				      &ipv4->unicast[i].address.in_addr)) {
			return &ipv4->unicast[i];
		}
	}

	return NULL;
}

struct net_if_addr *net_if_ipv4_addr_add(struct net_if *iface,
					 struct in_addr *addr,
					 enum net_addr_type addr_type,
					 uint32_t vlifetime)
{
	struct net_if_addr *ifaddr;
	struct net_if_ipv4 *ipv4;
	int i;

	if (net_if_config_ipv4_get(iface, &ipv4) < 0) {
		return NULL;
	}

	ifaddr = ipv4_addr_find(iface, addr);
	if (ifaddr) {
		/* TODO: should set addr_type/vlifetime */
		return ifaddr;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		struct net_if_addr *cur = &ipv4->unicast[i];

		if (addr_type == NET_ADDR_DHCP
		    && cur->addr_type == NET_ADDR_OVERRIDABLE) {
			ifaddr = cur;
			break;
		}

		if (!ipv4->unicast[i].is_used) {
			ifaddr = cur;
			break;
		}
	}

	if (ifaddr) {
		ifaddr->is_used = true;
		ifaddr->address.family = AF_INET;
		ifaddr->address.in_addr.s4_addr32[0] =
						addr->s4_addr32[0];
		ifaddr->addr_type = addr_type;

		/* Caller has to take care of timers and their expiry */
		if (vlifetime) {
			ifaddr->is_infinite = false;
		} else {
			ifaddr->is_infinite = true;
		}

		/**
		 *  TODO: Handle properly PREFERRED/DEPRECATED state when
		 *  address in use, expired and renewal state.
		 */
		ifaddr->addr_state = NET_ADDR_PREFERRED;

		NET_DBG("[%d] interface %p address %s type %s added", i, iface,
			log_strdup(net_sprint_ipv4_addr(addr)),
			net_addr_type2str(addr_type));

		net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ADDR_ADD, iface,
						&ifaddr->address.in_addr,
						sizeof(struct in_addr));

		return ifaddr;
	}

	return NULL;
}

bool net_if_ipv4_addr_rm(struct net_if *iface, const struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	int i;

	if (!ipv4) {
		return false;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!ipv4->unicast[i].is_used) {
			continue;
		}

		if (!net_ipv4_addr_cmp(&ipv4->unicast[i].address.in_addr,
				       addr)) {
			continue;
		}

		ipv4->unicast[i].is_used = false;

		NET_DBG("[%d] interface %p address %s removed",
			i, iface, log_strdup(net_sprint_ipv4_addr(addr)));

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV4_ADDR_DEL, iface,
			&ipv4->unicast[i].address.in_addr,
			sizeof(struct in_addr));

		return true;
	}

	return false;
}

bool z_impl_net_if_ipv4_addr_add_by_index(int index,
					  struct in_addr *addr,
					  enum net_addr_type addr_type,
					  uint32_t vlifetime)
{
	struct net_if *iface;
	struct net_if_addr *if_addr;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	if_addr = net_if_ipv4_addr_add(iface, addr, addr_type, vlifetime);
	return if_addr ? true : false;
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_net_if_ipv4_addr_add_by_index(int index,
					  struct in_addr *addr,
					  enum net_addr_type addr_type,
					  uint32_t vlifetime)
{
	struct in_addr addr_v4;
	struct net_if *iface;

	iface = z_vrfy_net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	Z_OOPS(z_user_from_copy(&addr_v4, (void *)addr, sizeof(addr_v4)));

	return z_impl_net_if_ipv4_addr_add_by_index(index,
						    &addr_v4,
						    addr_type,
						    vlifetime);
}

#include <syscalls/net_if_ipv4_addr_add_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

bool z_impl_net_if_ipv4_addr_rm_by_index(int index,
					 const struct in_addr *addr)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	return net_if_ipv4_addr_rm(iface, addr);
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_net_if_ipv4_addr_rm_by_index(int index,
					 const struct in_addr *addr)
{
	struct in_addr addr_v4;
	struct net_if *iface;

	iface = z_vrfy_net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	Z_OOPS(z_user_from_copy(&addr_v4, (void *)addr, sizeof(addr_v4)));

	return (uint32_t)z_impl_net_if_ipv4_addr_rm_by_index(index, &addr_v4);
}

#include <syscalls/net_if_ipv4_addr_rm_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

static struct net_if_mcast_addr *ipv4_maddr_find(struct net_if *iface,
						 bool is_used,
						 const struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	int i;

	if (!ipv4) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		if ((is_used && !ipv4->mcast[i].is_used) ||
		    (!is_used && ipv4->mcast[i].is_used)) {
			continue;
		}

		if (addr) {
			if (!net_ipv4_addr_cmp(&ipv4->mcast[i].address.in_addr,
					       addr)) {
				continue;
			}
		}

		return &ipv4->mcast[i];
	}

	return NULL;
}
struct net_if_mcast_addr *net_if_ipv4_maddr_add(struct net_if *iface,
						const struct in_addr *addr)
{
	struct net_if_mcast_addr *maddr;

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		return NULL;
	}

	if (!net_ipv4_is_addr_mcast(addr)) {
		NET_DBG("Address %s is not a multicast address.",
			log_strdup(net_sprint_ipv4_addr(addr)));
		return NULL;
	}

	maddr = ipv4_maddr_find(iface, false, NULL);
	if (maddr) {
		maddr->is_used = true;
		maddr->address.family = AF_INET;
		maddr->address.in_addr.s4_addr32[0] = addr->s4_addr32[0];

		NET_DBG("interface %p address %s added", iface,
			log_strdup(net_sprint_ipv4_addr(addr)));
	}

	return maddr;
}

bool net_if_ipv4_maddr_rm(struct net_if *iface, const struct in_addr *addr)
{
	struct net_if_mcast_addr *maddr;

	maddr = ipv4_maddr_find(iface, true, addr);
	if (maddr) {
		maddr->is_used = false;

		NET_DBG("interface %p address %s removed",
			iface, log_strdup(net_sprint_ipv4_addr(addr)));

		return true;
	}

	return false;
}

struct net_if_mcast_addr *net_if_ipv4_maddr_lookup(const struct in_addr *maddr,
						   struct net_if **ret)
{
	struct net_if_mcast_addr *addr;

	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		if (ret && *ret && iface != *ret) {
			continue;
		}

		addr = ipv4_maddr_find(iface, true, maddr);
		if (addr) {
			if (ret) {
				*ret = iface;
			}

			return addr;
		}
	}

	return NULL;
}

static void iface_ipv4_init(int if_count)
{
	int i;

	if (if_count > ARRAY_SIZE(ipv4_addresses)) {
		NET_WARN("You have %lu IPv4 net_if addresses but %d "
			 "network interfaces", ARRAY_SIZE(ipv4_addresses),
			 if_count);
		NET_WARN("Consider increasing CONFIG_NET_IF_MAX_IPV4_COUNT "
			 "value.");
	}

	for (i = 0; i < ARRAY_SIZE(ipv4_addresses); i++) {
		ipv4_addresses[i].ipv4.ttl = CONFIG_NET_INITIAL_TTL;
	}
}

#else
#define iface_ipv4_init(...)

struct net_if_mcast_addr *net_if_ipv4_maddr_lookup(const struct in_addr *addr,
						   struct net_if **iface)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(iface);

	return NULL;
}

struct net_if_addr *net_if_ipv4_addr_lookup(const struct in_addr *addr,
					    struct net_if **ret)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(ret);

	return NULL;
}

struct in_addr *net_if_ipv4_get_global_addr(struct net_if *iface,
					    enum net_addr_state addr_state)
{
	ARG_UNUSED(addr_state);
	ARG_UNUSED(iface);

	return NULL;
}
#endif /* CONFIG_NET_IPV4 */

struct net_if *net_if_select_src_iface(const struct sockaddr *dst)
{
	struct net_if *iface;

	if (!dst) {
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && dst->sa_family == AF_INET6) {
		iface = net_if_ipv6_select_src_iface(&net_sin6(dst)->sin6_addr);
		if (!iface) {
			goto out;
		}

		return iface;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && dst->sa_family == AF_INET) {
		iface = net_if_ipv4_select_src_iface(&net_sin(dst)->sin_addr);
		if (!iface) {
			goto out;
		}

		return iface;
	}

out:
	return net_if_get_default();
}

enum net_verdict net_if_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE) &&
	    net_if_is_promisc(iface)) {
		/* If the packet is not for us and the promiscuous
		 * mode is enabled, then increase the ref count so
		 * that net_core.c:processing_data() will not free it.
		 * The promiscuous mode handler must free the packet
		 * after it has finished working with it.
		 *
		 * If packet is for us, then NET_CONTINUE is returned.
		 * In this case we must clone the packet, as the packet
		 * could be manipulated by other part of the stack.
		 */
		enum net_verdict verdict;
		struct net_pkt *new_pkt;

		/* This protects pkt so that it will not be freed by L2 recv()
		 */
		net_pkt_ref(pkt);

		verdict = net_if_l2(iface)->recv(iface, pkt);
		if (verdict == NET_CONTINUE) {
			new_pkt = net_pkt_clone(pkt, K_NO_WAIT);
		} else {
			new_pkt = net_pkt_ref(pkt);
		}

		/* L2 has modified the buffer starting point, it is easier
		 * to re-initialize the cursor rather than updating it.
		 */
		net_pkt_cursor_init(new_pkt);

		if (net_promisc_mode_input(new_pkt) == NET_DROP) {
			net_pkt_unref(new_pkt);
		}

		net_pkt_unref(pkt);

		return verdict;
	}

	return net_if_l2(iface)->recv(iface, pkt);
}

void net_if_register_link_cb(struct net_if_link_cb *link,
			     net_if_link_callback_t cb)
{
	sys_slist_find_and_remove(&link_callbacks, &link->node);
	sys_slist_prepend(&link_callbacks, &link->node);

	link->cb = cb;
}

void net_if_unregister_link_cb(struct net_if_link_cb *link)
{
	sys_slist_find_and_remove(&link_callbacks, &link->node);
}

void net_if_call_link_cb(struct net_if *iface, struct net_linkaddr *lladdr,
			 int status)
{
	struct net_if_link_cb *link, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&link_callbacks, link, tmp, node) {
		link->cb(iface, lladdr, status);
	}
}

static bool need_calc_checksum(struct net_if *iface, enum ethernet_hw_caps caps)
{
#if defined(CONFIG_NET_L2_ETHERNET)
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return true;
	}

	return !(net_eth_get_hw_capabilities(iface) & caps);
#else
	return true;
#endif
}

bool net_if_need_calc_tx_checksum(struct net_if *iface)
{
	return need_calc_checksum(iface, ETHERNET_HW_TX_CHKSUM_OFFLOAD);
}

bool net_if_need_calc_rx_checksum(struct net_if *iface)
{
	return need_calc_checksum(iface, ETHERNET_HW_RX_CHKSUM_OFFLOAD);
}

int net_if_get_by_iface(struct net_if *iface)
{
	if (!(iface >= _net_if_list_start && iface < _net_if_list_end)) {
		return -1;
	}

	return (iface - _net_if_list_start) + 1;
}

void net_if_foreach(net_if_cb_t cb, void *user_data)
{
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		cb(iface, user_data);
	}
}

int net_if_up(struct net_if *iface)
{
	int status;

	NET_DBG("iface %p", iface);

	if (net_if_flag_is_set(iface, NET_IF_UP)) {
		return 0;
	}

	if ((IS_ENABLED(CONFIG_NET_OFFLOAD) &&
	     net_if_is_ip_offloaded(iface)) ||
	    (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD) &&
	     net_if_is_socket_offloaded(iface))) {
		net_if_flag_set(iface, NET_IF_UP);
		goto exit;
	}

	/* If the L2 does not support enable just set the flag */
	if (!net_if_l2(iface) || !net_if_l2(iface)->enable) {
		goto done;
	}

	/* Notify L2 to enable the interface */
	status = net_if_l2(iface)->enable(iface, true);
	if (status < 0) {
		return status;
	}

done:
	/* In many places it's assumed that link address was set with
	 * net_if_set_link_addr(). Better check that now.
	 */
	NET_ASSERT(net_if_get_link_addr(iface)->addr != NULL);

	net_if_flag_set(iface, NET_IF_UP);

	/* If the interface is only having point-to-point traffic then we do
	 * not need to run DAD etc for it.
	 */
	if (!(l2_flags_get(iface) & NET_L2_POINT_TO_POINT)) {
		iface_ipv6_start(iface);

		net_ipv4_autoconf_start(iface);
	}

exit:
	net_mgmt_event_notify(NET_EVENT_IF_UP, iface);

	return 0;
}

void net_if_carrier_down(struct net_if *iface)
{
	NET_DBG("iface %p", iface);

	net_if_flag_clear(iface, NET_IF_UP);

	net_ipv4_autoconf_reset(iface);

	net_mgmt_event_notify(NET_EVENT_IF_DOWN, iface);
}

int net_if_down(struct net_if *iface)
{
	int status;

	NET_DBG("iface %p", iface);

	leave_mcast_all(iface);

	if (net_if_is_ip_offloaded(iface)) {
		goto done;
	}

	/* If the L2 does not support enable just clear the flag */
	if (!net_if_l2(iface) || !net_if_l2(iface)->enable) {
		goto done;
	}

	/* Notify L2 to disable the interface */
	status = net_if_l2(iface)->enable(iface, false);
	if (status < 0) {
		return status;
	}

done:
	net_if_flag_clear(iface, NET_IF_UP);

	net_mgmt_event_notify(NET_EVENT_IF_DOWN, iface);

	return 0;
}

static int promisc_mode_set(struct net_if *iface, bool enable)
{
	enum net_l2_flags l2_flags = 0;

	NET_ASSERT(iface);

	l2_flags = l2_flags_get(iface);
	if (!(l2_flags & NET_L2_PROMISC_MODE)) {
		return -ENOTSUP;
	}

#if defined(CONFIG_NET_L2_ETHERNET)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		int ret = net_eth_promisc_mode(iface, enable);

		if (ret < 0) {
			return ret;
		}
	}
#else
	return -ENOTSUP;
#endif

	return 0;
}

int net_if_set_promisc(struct net_if *iface)
{
	int ret;

	ret = promisc_mode_set(iface, true);
	if (ret < 0) {
		return ret;
	}

	ret = net_if_flag_test_and_set(iface, NET_IF_PROMISC);
	if (ret) {
		return -EALREADY;
	}

	return 0;
}

void net_if_unset_promisc(struct net_if *iface)
{
	int ret;

	ret = promisc_mode_set(iface, false);
	if (ret < 0) {
		return;
	}

	net_if_flag_clear(iface, NET_IF_PROMISC);
}

bool net_if_is_promisc(struct net_if *iface)
{
	NET_ASSERT(iface);

	return net_if_flag_is_set(iface, NET_IF_PROMISC);
}

#ifdef CONFIG_NET_POWER_MANAGEMENT

int net_if_suspend(struct net_if *iface)
{
	if (net_if_are_pending_tx_packets(iface)) {
		return -EBUSY;
	}

	if (net_if_flag_test_and_set(iface, NET_IF_SUSPENDED)) {
		return -EALREADY;
	}

	net_stats_add_suspend_start_time(iface, k_cycle_get_32());

	return 0;
}

int net_if_resume(struct net_if *iface)
{
	if (!net_if_flag_is_set(iface, NET_IF_SUSPENDED)) {
		return -EALREADY;
	}

	net_if_flag_clear(iface, NET_IF_SUSPENDED);

	net_stats_add_suspend_end_time(iface, k_cycle_get_32());

	return 0;
}

bool net_if_is_suspended(struct net_if *iface)
{
	return net_if_flag_is_set(iface, NET_IF_SUSPENDED);
}

#endif /* CONFIG_NET_POWER_MANAGEMENT */

#if defined(CONFIG_NET_PKT_TIMESTAMP_THREAD)
static void net_tx_ts_thread(void)
{
	struct net_pkt *pkt;

	NET_DBG("Starting TX timestamp callback thread");

	while (1) {
		pkt = k_fifo_get(&tx_ts_queue, K_FOREVER);
		if (pkt) {
			net_if_call_timestamp_cb(pkt);
		}
	}
}

void net_if_register_timestamp_cb(struct net_if_timestamp_cb *handle,
				  struct net_pkt *pkt,
				  struct net_if *iface,
				  net_if_timestamp_callback_t cb)
{
	sys_slist_find_and_remove(&timestamp_callbacks, &handle->node);
	sys_slist_prepend(&timestamp_callbacks, &handle->node);

	handle->iface = iface;
	handle->cb = cb;
	handle->pkt = pkt;
}

void net_if_unregister_timestamp_cb(struct net_if_timestamp_cb *handle)
{
	sys_slist_find_and_remove(&timestamp_callbacks, &handle->node);
}

void net_if_call_timestamp_cb(struct net_pkt *pkt)
{
	sys_snode_t *sn, *sns;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&timestamp_callbacks, sn, sns) {
		struct net_if_timestamp_cb *handle =
			CONTAINER_OF(sn, struct net_if_timestamp_cb, node);

		if (((handle->iface == NULL) ||
		     (handle->iface == net_pkt_iface(pkt))) &&
		    (handle->pkt == NULL || handle->pkt == pkt)) {
			handle->cb(pkt);
		}
	}
}

void net_if_add_tx_timestamp(struct net_pkt *pkt)
{
	k_fifo_put(&tx_ts_queue, pkt);
}
#endif /* CONFIG_NET_PKT_TIMESTAMP_THREAD */

void net_if_init(void)
{
	int if_count = 0;

	NET_DBG("");

	net_tc_tx_init();

	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		init_iface(iface);
		if_count++;
	}

	if (if_count == 0) {
		NET_ERR("There is no network interface to work with!");
		return;
	}

	iface_ipv6_init(if_count);
	iface_ipv4_init(if_count);
	iface_router_init();

#if defined(CONFIG_NET_PKT_TIMESTAMP_THREAD)
	k_thread_create(&tx_thread_ts, tx_ts_stack,
			K_KERNEL_STACK_SIZEOF(tx_ts_stack),
			(k_thread_entry_t)net_tx_ts_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_thread_name_set(&tx_thread_ts, "tx_tstamp");
#endif /* CONFIG_NET_PKT_TIMESTAMP_THREAD */

#if defined(CONFIG_NET_VLAN)
	/* Make sure that we do not have too many network interfaces
	 * compared to the number of VLAN interfaces.
	 */
	if_count = 0;

	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
			if_count++;
		}
	}

	if (if_count > CONFIG_NET_VLAN_COUNT) {
		NET_WARN("You have configured only %d VLAN interfaces"
			 " but you have %d network interfaces.",
			 CONFIG_NET_VLAN_COUNT, if_count);
	}
#endif
}

void net_if_post_init(void)
{
	NET_DBG("");

	/* After TX is running, attempt to bring the interface up */
	Z_STRUCT_SECTION_FOREACH(net_if, iface) {
		if (!net_if_flag_is_set(iface, NET_IF_NO_AUTO_START)) {
			net_if_up(iface);
		}
	}
}
