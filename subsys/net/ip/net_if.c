/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_if, CONFIG_NET_IF_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/random.h>
#include <zephyr/internal/syscall_handler.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/net/igmp.h>
#include <zephyr/net/ipv4_autoconf.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ethernet.h>
#ifdef CONFIG_WIFI_NM
#include <zephyr/net/wifi_nm.h>
#endif
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/iterable_sections.h>

#include "net_private.h"
#include "ipv4.h"
#include "ipv6.h"

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

static K_MUTEX_DEFINE(lock);

/* net_if dedicated section limiters */
extern struct net_if _net_if_list_start[];
extern struct net_if _net_if_list_end[];

static struct net_if *default_iface;

#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
static struct net_if_router routers[CONFIG_NET_MAX_ROUTERS];
static struct k_work_delayable router_timer;
static sys_slist_t active_router_timers;
#endif

#if defined(CONFIG_NET_NATIVE_IPV6)
/* Timer that triggers network address renewal */
static struct k_work_delayable address_lifetime_timer;

/* Track currently active address lifetime timers */
static sys_slist_t active_address_lifetime_timers;

/* Timer that triggers IPv6 prefix lifetime */
static struct k_work_delayable prefix_lifetime_timer;

/* Track currently active IPv6 prefix lifetime timers */
static sys_slist_t active_prefix_lifetime_timers;

#if defined(CONFIG_NET_IPV6_DAD)
/** Duplicate address detection (DAD) timer */
static struct k_work_delayable dad_timer;
static sys_slist_t active_dad_timers;
#endif

#if defined(CONFIG_NET_IPV6_ND)
static struct k_work_delayable rs_timer;
static sys_slist_t active_rs_timers;
#endif
#endif /* CONFIG_NET_NATIVE_IPV6 */

#if defined(CONFIG_NET_IPV6)
static struct {
	struct net_if_ipv6 ipv6;
	struct net_if *iface;
} ipv6_addresses[CONFIG_NET_IF_MAX_IPV6_COUNT];
#endif /* CONFIG_NET_NATIVE_IPV6 */

#if defined(CONFIG_NET_IPV4)
static struct {
	struct net_if_ipv4 ipv4;
	struct net_if *iface;
} ipv4_addresses[CONFIG_NET_IF_MAX_IPV4_COUNT];
#endif /* CONFIG_NET_NATIVE_IPV4 */

/* We keep track of the link callbacks in this list.
 */
static sys_slist_t link_callbacks;

#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
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
			"iface %d (%p)",				\
			pkt, net_pkt_priority(pkt),			\
			net_if_get_by_iface(net_pkt_iface(pkt)),	\
			net_pkt_iface(pkt));				\
									\
		NET_ASSERT(pkt->frags);					\
	} while (false)
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

	iface = net_if_get_by_index(index);
	if (!iface) {
		return NULL;
	}

	if (!k_object_is_valid(iface, K_OBJ_NET_IF)) {
		return NULL;
	}

	return iface;
}

#include <zephyr/syscalls/net_if_get_by_index_mrsh.c>
#endif

#if defined(CONFIG_NET_NATIVE)
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
	    net_context_get_proto(context) == IPPROTO_UDP) {
		net_stats_update_udp_sent(net_context_get_iface(context));
	} else if (IS_ENABLED(CONFIG_NET_TCP) &&
		   net_context_get_proto(context) == IPPROTO_TCP) {
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
	struct net_linkaddr ll_dst = { 0 };
	struct net_context *context;
	uint32_t create_time;
	int status;

	/* We collect send statistics for each socket priority if enabled */
	uint8_t pkt_priority;

	if (!pkt) {
		return false;
	}

	create_time = net_pkt_create_time(pkt);

	debug_check_packet(pkt);

	/* If there're any link callbacks, with such a callback receiving
	 * a destination address, copy that address out of packet, just in
	 * case packet is freed before callback is called.
	 */
	if (!sys_slist_is_empty(&link_callbacks)) {
		if (net_linkaddr_set(&ll_dst,
				     net_pkt_lladdr_dst(pkt)->addr,
				     net_pkt_lladdr_dst(pkt)->len) < 0) {
			return false;
		}
	}

	context = net_pkt_context(pkt);

	if (net_if_flag_is_set(iface, NET_IF_LOWER_UP)) {
		if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS) ||
		    IS_ENABLED(CONFIG_TRACING_NET_CORE)) {
			pkt_priority = net_pkt_priority(pkt);

			if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)) {
				/* Make sure the statistics information is not
				 * lost by keeping the net_pkt over L2 send.
				 */
				net_pkt_ref(pkt);
			}
		}

		net_if_tx_lock(iface);
		status = net_if_l2(iface)->send(iface, pkt);
		net_if_tx_unlock(iface);

		if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS) ||
		    IS_ENABLED(CONFIG_TRACING_NET_CORE)) {
			uint32_t end_tick = k_cycle_get_32();

			net_pkt_set_tx_stats_tick(pkt, end_tick);

			net_stats_update_tc_tx_time(iface,
						    pkt_priority,
						    create_time,
						    end_tick);

			SYS_PORT_TRACING_FUNC(net, tx_time, pkt, end_tick);

			if (IS_ENABLED(CONFIG_NET_PKT_TXTIME_STATS_DETAIL)) {
				update_txtime_stats_detail(
					pkt,
					create_time,
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
	}

	if (ll_dst.len > 0) {
		net_if_call_link_cb(iface, &ll_dst, status);
	}

	return true;
}

void net_process_tx_packet(struct net_pkt *pkt)
{
	struct net_if *iface;

	net_pkt_set_tx_stats_tick(pkt, k_cycle_get_32());

	iface = net_pkt_iface(pkt);

	net_if_tx(iface, pkt);

#if defined(CONFIG_NET_POWER_MANAGEMENT)
	iface->tx_pending--;
#endif
}

void net_if_try_queue_tx(struct net_if *iface, struct net_pkt *pkt, k_timeout_t timeout)
{
	if (!net_pkt_filter_send_ok(pkt)) {
		/* silently drop the packet */
		net_pkt_unref(pkt);
		return;
	}

	size_t len = net_pkt_get_len(pkt);
	uint8_t prio = net_pkt_priority(pkt);
	uint8_t tc = net_tx_priority2tc(prio);

#if NET_TC_TX_COUNT > 1
	NET_DBG("TC %d with prio %d pkt %p", tc, prio, pkt);
#endif

	/* For highest priority packet, skip the TX queue and push directly to
	 * the driver. Also if there are no TX queue/thread, push the packet
	 * directly to the driver.
	 */
	if ((IS_ENABLED(CONFIG_NET_TC_TX_SKIP_FOR_HIGH_PRIO) &&
	     prio >= NET_PRIORITY_CA) || NET_TC_TX_COUNT == 0) {
		net_pkt_set_tx_stats_tick(pkt, k_cycle_get_32());

		net_if_tx(net_pkt_iface(pkt), pkt);
	} else {
		if (net_tc_try_submit_to_tx_queue(tc, pkt, timeout) != NET_OK) {
			goto drop;
		}
#if defined(CONFIG_NET_POWER_MANAGEMENT)
		iface->tx_pending++;
#endif
	}

	net_stats_update_tc_sent_pkt(iface, tc);
	net_stats_update_tc_sent_bytes(iface, tc, len);
	net_stats_update_tc_sent_priority(iface, tc, prio);
	return;

drop:
	net_pkt_unref(pkt);
	net_stats_update_tc_sent_dropped(iface, tc);
	return;
}
#endif /* CONFIG_NET_NATIVE */

void net_if_stats_reset(struct net_if *iface)
{
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	STRUCT_SECTION_FOREACH(net_if, tmp) {
		if (iface == tmp) {
			net_if_lock(iface);
			memset(&iface->stats, 0, sizeof(iface->stats));
			net_if_unlock(iface);
			return;
		}
	}
#else
	ARG_UNUSED(iface);
#endif
}

void net_if_stats_reset_all(void)
{
#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	STRUCT_SECTION_FOREACH(net_if, iface) {
		net_if_lock(iface);
		memset(&iface->stats, 0, sizeof(iface->stats));
		net_if_unlock(iface);
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

	/* By default IPv4 and IPv6 are enabled for a given network interface.
	 * These can be turned off later if needed.
	 */
#if defined(CONFIG_NET_NATIVE_IPV4)
	net_if_flag_set(iface, NET_IF_IPV4);
#endif
#if defined(CONFIG_NET_NATIVE_IPV6)
	net_if_flag_set(iface, NET_IF_IPV6);
#endif

	net_virtual_init(iface);

	NET_DBG("On iface %p", iface);

#ifdef CONFIG_USERSPACE
	k_object_init(iface);
#endif

	k_mutex_init(&iface->lock);
	k_mutex_init(&iface->tx_lock);

	api->init(iface);

	net_ipv6_pe_init(iface);
}

#if defined(CONFIG_NET_NATIVE)
enum net_verdict net_if_try_send_data(struct net_if *iface, struct net_pkt *pkt,
				      k_timeout_t timeout)
{
	const struct net_l2 *l2;
	struct net_context *context = net_pkt_context(pkt);
	struct net_linkaddr *dst = net_pkt_lladdr_dst(pkt);
	enum net_verdict verdict = NET_OK;
	int status = -EIO;

	if (!net_if_flag_is_set(iface, NET_IF_LOWER_UP) ||
	    net_if_flag_is_set(iface, NET_IF_SUSPENDED)) {
		/* Drop packet if interface is not up */
		NET_WARN("iface %p is down", iface);
		verdict = NET_DROP;
		status = -ENETDOWN;
		goto done;
	}

	/* The check for CONFIG_NET_*_OFFLOAD here is an optimization;
	 * This is currently the only way for net_if_l2 to be NULL or missing send().
	 */
	if (IS_ENABLED(CONFIG_NET_OFFLOAD) || IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD)) {
		l2 = net_if_l2(iface);
		if (l2 == NULL) {
			/* Offloaded ifaces may choose not to use an L2 at all. */
			NET_WARN("no l2 for iface %p, discard pkt", iface);
			verdict = NET_DROP;
			goto done;
		} else if (l2->send == NULL) {
			/* Or, their chosen L2 (for example, OFFLOADED_NETDEV_L2)
			 * might simply not implement send.
			 */
			NET_WARN("l2 for iface %p cannot send, discard pkt", iface);
			verdict = NET_DROP;
			goto done;
		}
	}

	/* If the ll address is not set at all, then we must set
	 * it here.
	 * Workaround Linux bug, see:
	 * https://github.com/zephyrproject-rtos/zephyr/issues/3111
	 */
	if (!net_if_flag_is_set(iface, NET_IF_POINTOPOINT) &&
	    net_pkt_lladdr_src(pkt)->len == 0) {
		(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
				       net_pkt_lladdr_if(pkt)->addr,
				       net_pkt_lladdr_if(pkt)->len);
	}

#if defined(CONFIG_NET_LOOPBACK)
	/* If the packet is destined back to us, then there is no need to do
	 * additional checks, so let the packet through.
	 */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		goto done;
	}
#endif

	/* Bypass the IP stack with AF_INET(6)/SOCK_RAW */
	if (context && net_context_get_type(context) == SOCK_RAW &&
	    (net_context_get_family(context) == AF_INET ||
	     net_context_get_family(context) == AF_INET6)) {
		goto done;
	}

	/* If the ll dst address is not set check if it is present in the nbr
	 * cache.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		verdict = net_ipv6_prepare_for_send(pkt);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		verdict = net_ipv4_prepare_for_send(pkt);
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

		if (dst->len > 0) {
			net_if_call_link_cb(iface, dst, status);
		}
	} else if (verdict == NET_OK) {
		/* Packet is ready to be sent by L2, let's queue */
		net_if_try_queue_tx(iface, pkt, timeout);
	}

	return verdict;
}
#endif /* CONFIG_NET_NATIVE */

int net_if_set_link_addr_locked(struct net_if *iface,
				uint8_t *addr, uint8_t len,
				enum net_link_type type)
{
	int ret;

	net_if_lock(iface);

	ret = net_if_set_link_addr_unlocked(iface, addr, len, type);

	net_if_unlock(iface);

	return ret;
}

struct net_if *net_if_get_by_link_addr(struct net_linkaddr *ll_addr)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		net_if_lock(iface);
		if (!memcmp(net_if_get_link_addr(iface)->addr, ll_addr->addr,
			    ll_addr->len)) {
			net_if_unlock(iface);
			return iface;
		}
		net_if_unlock(iface);
	}

	return NULL;
}

struct net_if *net_if_lookup_by_dev(const struct device *dev)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_get_device(iface) == dev) {
			return iface;
		}
	}

	return NULL;
}

void net_if_set_default(struct net_if *iface)
{
	default_iface = iface;
}

struct net_if *net_if_get_default(void)
{
	struct net_if *iface = NULL;

	if (&_net_if_list_start[0] == &_net_if_list_end[0]) {
		NET_WARN("No default interface found!");
		return NULL;
	}

	if (default_iface != NULL) {
		return default_iface;
	}

#if defined(CONFIG_NET_DEFAULT_IF_ETHERNET)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_IEEE802154)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(IEEE802154));
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
#if defined(CONFIG_NET_DEFAULT_IF_PPP)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_OFFLOADED_NETDEV)
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OFFLOADED_NETDEV));
#endif
#if defined(CONFIG_NET_DEFAULT_IF_UP)
	iface = net_if_get_first_up();
#endif
#if defined(CONFIG_NET_DEFAULT_IF_WIFI)
	iface = net_if_get_first_wifi();
#endif
	return iface ? iface : _net_if_list_start;
}

struct net_if *net_if_get_first_by_type(const struct net_l2 *l2)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
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

struct net_if *net_if_get_first_up(void)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_flag_is_set(iface, NET_IF_UP)) {
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

#if defined(CONFIG_NET_IP)
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
#endif /* CONFIG_NET_IP */

#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
static struct net_if_router *iface_router_lookup(struct net_if *iface,
						 uint8_t family, void *addr)
{
	struct net_if_router *router = NULL;
	int i;

	k_mutex_lock(&lock, K_FOREVER);

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
			router = &routers[i];
			goto out;
		}
	}

out:
	k_mutex_unlock(&lock);

	return router;
}

static void iface_router_notify_deletion(struct net_if_router *router,
					 const char *delete_reason)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    router->address.family == AF_INET6) {
		NET_DBG("IPv6 router %s %s",
			net_sprint_ipv6_addr(net_if_router_ipv6(router)),
			delete_reason);

		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTER_DEL,
						router->iface,
						&router->address.in6_addr,
						sizeof(struct in6_addr));
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   router->address.family == AF_INET) {
		NET_DBG("IPv4 router %s %s",
			net_sprint_ipv4_addr(net_if_router_ipv4(router)),
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

	k_mutex_lock(&lock, K_FOREVER);

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
		k_work_cancel_delayable(&router_timer);
	} else {
		k_work_reschedule(&router_timer, K_MSEC(new_delay));
	}

	k_mutex_unlock(&lock);
}

static void iface_router_expired(struct k_work *work)
{
	uint32_t current_time = k_uptime_get_32();
	struct net_if_router *router, *next;
	sys_snode_t *prev_node = NULL;

	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);

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

	k_mutex_unlock(&lock);
}

static struct net_if_router *iface_router_add(struct net_if *iface,
					      uint8_t family, void *addr,
					      bool is_default,
					      uint16_t lifetime)
{
	struct net_if_router *router = NULL;
	int i;

	k_mutex_lock(&lock, K_FOREVER);

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
				net_sprint_ipv6_addr((struct in6_addr *)addr),
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
				net_sprint_ipv4_addr((struct in_addr *)addr),
				lifetime, is_default);
		}

		router = &routers[i];
		goto out;
	}

out:
	k_mutex_unlock(&lock);

	return router;
}

static bool iface_router_rm(struct net_if_router *router)
{
	bool ret = false;

	k_mutex_lock(&lock, K_FOREVER);

	if (!router->is_used) {
		goto out;
	}

	iface_router_notify_deletion(router, "has been removed");

	/* We recompute the timer if only the router was time limited */
	if (sys_slist_find_and_remove(&active_router_timers, &router->node)) {
		iface_router_update_timer(k_uptime_get_32());
	}

	router->is_used = false;
	ret = true;

out:
	k_mutex_unlock(&lock);

	return ret;
}

void net_if_router_rm(struct net_if_router *router)
{
	k_mutex_lock(&lock, K_FOREVER);

	router->is_used = false;

	/* FIXME - remove timer */

	k_mutex_unlock(&lock);
}

static struct net_if_router *iface_router_find_default(struct net_if *iface,
						       uint8_t family, void *addr)
{
	struct net_if_router *router = NULL;
	int i;

	/* Todo: addr will need to be handled */
	ARG_UNUSED(addr);

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (!routers[i].is_used ||
		    !routers[i].is_default ||
		    routers[i].address.family != family) {
			continue;
		}

		if (iface && iface != routers[i].iface) {
			continue;
		}

		router = &routers[i];
		goto out;
	}

out:
	k_mutex_unlock(&lock);

	return router;
}

static void iface_router_init(void)
{
	k_work_init_delayable(&router_timer, iface_router_expired);
	sys_slist_init(&active_router_timers);
}
#else
#define iface_router_init(...)
#endif /* CONFIG_NET_NATIVE_IPV4 || CONFIG_NET_NATIVE_IPV6 */

#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
void net_if_mcast_mon_register(struct net_if_mcast_monitor *mon,
			       struct net_if *iface,
			       net_if_mcast_callback_t cb)
{
	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&mcast_monitor_callbacks, &mon->node);
	sys_slist_prepend(&mcast_monitor_callbacks, &mon->node);

	mon->iface = iface;
	mon->cb = cb;

	k_mutex_unlock(&lock);
}

void net_if_mcast_mon_unregister(struct net_if_mcast_monitor *mon)
{
	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&mcast_monitor_callbacks, &mon->node);

	k_mutex_unlock(&lock);
}

void net_if_mcast_monitor(struct net_if *iface,
			  const struct net_addr *addr,
			  bool is_joined)
{
	struct net_if_mcast_monitor *mon, *tmp;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&mcast_monitor_callbacks,
					  mon, tmp, node) {
		if (iface == mon->iface || mon->iface == NULL) {
			mon->cb(iface, addr, is_joined);
		}
	}

	k_mutex_unlock(&lock);
}
#else
#define net_if_mcast_mon_register(...)
#define net_if_mcast_mon_unregister(...)
#define net_if_mcast_monitor(...)
#endif /* CONFIG_NET_NATIVE_IPV4 || CONFIG_NET_NATIVE_IPV6 */

#if defined(CONFIG_NET_IPV6)
int net_if_config_ipv6_get(struct net_if *iface, struct net_if_ipv6 **ipv6)
{
	int ret = 0;
	int i;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV6)) {
		ret = -ENOTSUP;
		goto out;
	}

	if (iface->config.ip.ipv6) {
		if (ipv6) {
			*ipv6 = iface->config.ip.ipv6;
		}

		goto out;
	}

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(ipv6_addresses); i++) {
		if (ipv6_addresses[i].iface) {
			continue;
		}

		iface->config.ip.ipv6 = &ipv6_addresses[i].ipv6;
		ipv6_addresses[i].iface = iface;

		if (ipv6) {
			*ipv6 = &ipv6_addresses[i].ipv6;
		}

		k_mutex_unlock(&lock);
		goto out;
	}

	k_mutex_unlock(&lock);

	ret = -ESRCH;
out:
	net_if_unlock(iface);

	return ret;
}

int net_if_config_ipv6_put(struct net_if *iface)
{
	int ret = 0;
	int i;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV6)) {
		ret = -ENOTSUP;
		goto out;
	}

	if (!iface->config.ip.ipv6) {
		ret = -EALREADY;
		goto out;
	}

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(ipv6_addresses); i++) {
		if (ipv6_addresses[i].iface != iface) {
			continue;
		}

		iface->config.ip.ipv6 = NULL;
		ipv6_addresses[i].iface = NULL;

		k_mutex_unlock(&lock);
		goto out;
	}

	k_mutex_unlock(&lock);

	ret = -ESRCH;
out:
	net_if_unlock(iface);

	return ret;
}

#if defined(CONFIG_NET_NATIVE_IPV6)
#if defined(CONFIG_NET_IPV6_MLD)
static void join_mcast_allnodes(struct net_if *iface)
{
	struct in6_addr addr;
	int ret;

	if (iface->config.ip.ipv6 == NULL) {
		return;
	}

	net_ipv6_addr_create_ll_allnodes_mcast(&addr);

	ret = net_ipv6_mld_join(iface, &addr);
	if (ret < 0 && ret != -EALREADY && ret != -ENETDOWN) {
		NET_ERR("Cannot join all nodes address %s for %d (%d)",
			net_sprint_ipv6_addr(&addr),
			net_if_get_by_iface(iface), ret);
	}
}

static void join_mcast_solicit_node(struct net_if *iface,
				    struct in6_addr *my_addr)
{
	struct in6_addr addr;
	int ret;

	if (iface->config.ip.ipv6 == NULL) {
		return;
	}

	/* Join to needed multicast groups, RFC 4291 ch 2.8 */
	net_ipv6_addr_create_solicited_node(my_addr, &addr);

	ret = net_ipv6_mld_join(iface, &addr);
	if (ret < 0) {
		if (ret != -EALREADY && ret != -ENETDOWN) {
			NET_ERR("Cannot join solicit node address %s for %d (%d)",
				net_sprint_ipv6_addr(&addr),
				net_if_get_by_iface(iface), ret);
		}
	} else {
		NET_DBG("Join solicit node address %s (ifindex %d)",
			net_sprint_ipv6_addr(&addr),
			net_if_get_by_iface(iface));
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

	if (iface->config.ip.ipv6 == NULL) {
		return;
	}

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
	sys_slist_t expired_list;

	ARG_UNUSED(work);

	sys_slist_init(&expired_list);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_dad_timers,
					  ifaddr, next, dad_node) {
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
		sys_slist_append(&expired_list, &ifaddr->dad_node);

		ifaddr = NULL;
	}

	if ((ifaddr != NULL) && (delay > 0)) {
		k_work_reschedule(&dad_timer, K_MSEC((uint32_t)delay));
	}

	k_mutex_unlock(&lock);

	SYS_SLIST_FOR_EACH_CONTAINER(&expired_list, ifaddr, dad_node) {
		struct net_if *iface;

		NET_DBG("DAD succeeded for %s at interface %d",
			net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
			ifaddr->ifindex);

		ifaddr->addr_state = NET_ADDR_PREFERRED;
		iface = net_if_get_by_index(ifaddr->ifindex);

		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_DAD_SUCCEED,
						iface,
						&ifaddr->address.in6_addr,
						sizeof(struct in6_addr));

		/* The address gets added to neighbor cache which is not
		 * needed in this case as the address is our own one.
		 */
		net_ipv6_nbr_rm(iface, &ifaddr->address.in6_addr);
	}
}

void net_if_ipv6_start_dad(struct net_if *iface,
			   struct net_if_addr *ifaddr)
{
	ifaddr->addr_state = NET_ADDR_TENTATIVE;

	if (net_if_is_up(iface)) {
		NET_DBG("Interface %p ll addr %s tentative IPv6 addr %s",
			iface,
			net_sprint_ll_addr(
					   net_if_get_link_addr(iface)->addr,
					   net_if_get_link_addr(iface)->len),
			net_sprint_ipv6_addr(&ifaddr->address.in6_addr));

		ifaddr->dad_count = 1U;

		if (net_ipv6_start_dad(iface, ifaddr) != 0) {
			NET_ERR("Interface %p failed to send DAD query for %s",
				iface,
				net_sprint_ipv6_addr(&ifaddr->address.in6_addr));
		}

		ifaddr->dad_start = k_uptime_get_32();
		ifaddr->ifindex = net_if_get_by_iface(iface);

		k_mutex_lock(&lock, K_FOREVER);
		sys_slist_find_and_remove(&active_dad_timers,
					  &ifaddr->dad_node);
		sys_slist_append(&active_dad_timers, &ifaddr->dad_node);
		k_mutex_unlock(&lock);

		/* FUTURE: use schedule, not reschedule. */
		if (!k_work_delayable_remaining_get(&dad_timer)) {
			k_work_reschedule(&dad_timer,
					  K_MSEC(DAD_TIMEOUT));
		}
	} else {
		NET_DBG("Interface %p is down, starting DAD for %s later.",
			iface,
			net_sprint_ipv6_addr(&ifaddr->address.in6_addr));
	}
}

void net_if_start_dad(struct net_if *iface)
{
	struct net_if_addr *ifaddr, *next;
	struct net_if_ipv6 *ipv6;
	sys_slist_t dad_needed;
	struct in6_addr addr = { };
	int ret;

	net_if_lock(iface);

	NET_DBG("Starting DAD for iface %p", iface);

	ret = net_if_config_ipv6_get(iface, &ipv6);
	if (ret < 0) {
		if (ret != -ENOTSUP) {
			NET_WARN("Cannot do DAD IPv6 config is not valid.");
		}

		goto out;
	}

	if (!ipv6) {
		goto out;
	}

	ret = net_ipv6_addr_generate_iid(iface, NULL,
					 COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
						     ((uint8_t *)&ipv6->network_counter),
						     (NULL)),
					 COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
						     (sizeof(ipv6->network_counter)),
						     (0U)),
					 COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
						     (ipv6->iid ? ipv6->iid->dad_count : 0U),
						     (0U)),
					 &addr,
					 net_if_get_link_addr(iface));
	if (ret < 0) {
		NET_WARN("IPv6 IID generation issue (%d)", ret);
		goto out;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s address to interface %p, DAD fails",
			net_sprint_ipv6_addr(&addr), iface);
		goto out;
	}

	IF_ENABLED(CONFIG_NET_IPV6_IID_STABLE, (ipv6->iid = ifaddr));

	/* Start DAD for all the addresses that were added earlier when
	 * the interface was down.
	 */
	sys_slist_init(&dad_needed);

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    &ipv6->unicast[i] == ifaddr ||
		    net_ipv6_is_addr_loopback(
			    &ipv6->unicast[i].address.in6_addr)) {
			continue;
		}

		sys_slist_prepend(&dad_needed, &ipv6->unicast[i].dad_need_node);
	}

	net_if_unlock(iface);

	/* Start DAD for all the addresses without holding the iface lock
	 * to avoid any possible mutex deadlock issues.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dad_needed,
					  ifaddr, next, dad_need_node) {
		net_if_ipv6_start_dad(iface, ifaddr);
	}

	return;

out:
	net_if_unlock(iface);
}

void net_if_ipv6_dad_failed(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_if_addr *ifaddr;
	uint32_t timeout, preferred_lifetime;

	net_if_lock(iface);

	ifaddr = net_if_ipv6_addr_lookup(addr, &iface);
	if (!ifaddr) {
		NET_ERR("Cannot find %s address in interface %p",
			net_sprint_ipv6_addr(addr), iface);
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6_IID_STABLE) || IS_ENABLED(CONFIG_NET_IPV6_PE)) {
		ifaddr->dad_count++;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6_PE)) {
		timeout = COND_CODE_1(CONFIG_NET_IPV6_PE,
				      (ifaddr->addr_timeout), (0));
		preferred_lifetime = COND_CODE_1(CONFIG_NET_IPV6_PE,
						 (ifaddr->addr_preferred_lifetime), (0U));

		if (!net_ipv6_pe_check_dad(ifaddr->dad_count)) {
			NET_ERR("Cannot generate PE address for interface %p",
				iface);
			iface->pe_enabled = false;
			net_mgmt_event_notify(NET_EVENT_IPV6_PE_DISABLED, iface);
		}
	}

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_DAD_FAILED, iface,
					&ifaddr->address.in6_addr,
					sizeof(struct in6_addr));

	/* The old address needs to be removed from the interface before we can
	 * start new DAD for the new PE address as the amount of address slots
	 * is limited.
	 */
	net_if_ipv6_addr_rm(iface, addr);

	if (IS_ENABLED(CONFIG_NET_IPV6_PE) && iface->pe_enabled) {
		net_if_unlock(iface);

		net_ipv6_pe_start(iface, addr, timeout, preferred_lifetime);
		return;
	}

out:
	net_if_unlock(iface);
}

static inline void iface_ipv6_dad_init(void)
{
	k_work_init_delayable(&dad_timer, dad_timeout);
	sys_slist_init(&active_dad_timers);
}

#else
#define net_if_ipv6_start_dad(...)
#define iface_ipv6_dad_init(...)
#endif /* CONFIG_NET_IPV6_DAD */

#if defined(CONFIG_NET_IPV6_ND)
#define RS_TIMEOUT (CONFIG_NET_IPV6_RS_TIMEOUT * MSEC_PER_SEC)
#define RS_COUNT 3

static void rs_timeout(struct k_work *work)
{
	uint32_t current_time = k_uptime_get_32();
	struct net_if_ipv6 *ipv6, *next;
	int32_t delay = -1;
	sys_slist_t expired_list;

	ARG_UNUSED(work);

	sys_slist_init(&expired_list);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_rs_timers,
					  ipv6, next, rs_node) {
		/* RS entries are ordered by construction.  Stop when
		 * we find one that hasn't expired.
		 */
		delay = (int32_t)(ipv6->rs_start + RS_TIMEOUT - current_time);
		if (delay > 0) {
			break;
		}

		/* Removing the ipv6 from active_rs_timers list */
		sys_slist_remove(&active_rs_timers, NULL, &ipv6->rs_node);
		sys_slist_append(&expired_list, &ipv6->rs_node);

		ipv6 = NULL;
	}

	if ((ipv6 != NULL) && (delay > 0)) {
		k_work_reschedule(&rs_timer, K_MSEC(ipv6->rs_start +
						    RS_TIMEOUT - current_time));
	}

	k_mutex_unlock(&lock);

	SYS_SLIST_FOR_EACH_CONTAINER(&expired_list, ipv6, rs_node) {
		struct net_if *iface = NULL;

		/* Did not receive RA yet. */
		ipv6->rs_count++;

		STRUCT_SECTION_FOREACH(net_if, tmp) {
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
	}
}

void net_if_start_rs(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		goto out;
	}

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	net_if_unlock(iface);

	NET_DBG("Starting ND/RS for iface %p", iface);

	if (!net_ipv6_start_rs(iface)) {
		ipv6->rs_start = k_uptime_get_32();

		k_mutex_lock(&lock, K_FOREVER);
		sys_slist_append(&active_rs_timers, &ipv6->rs_node);
		k_mutex_unlock(&lock);

		/* FUTURE: use schedule, not reschedule. */
		if (!k_work_delayable_remaining_get(&rs_timer)) {
			k_work_reschedule(&rs_timer, K_MSEC(RS_TIMEOUT));
		}
	}

	return;
out:
	net_if_unlock(iface);
}

void net_if_stop_rs(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	NET_DBG("Stopping ND/RS for iface %p", iface);

	k_mutex_lock(&lock, K_FOREVER);
	sys_slist_find_and_remove(&active_rs_timers, &ipv6->rs_node);
	k_mutex_unlock(&lock);

out:
	net_if_unlock(iface);
}

static inline void iface_ipv6_nd_init(void)
{
	k_work_init_delayable(&rs_timer, rs_timeout);
	sys_slist_init(&active_rs_timers);
}

#else
#define net_if_start_rs(...)
#define net_if_stop_rs(...)
#define iface_ipv6_nd_init(...)
#endif /* CONFIG_NET_IPV6_ND */

#if defined(CONFIG_NET_IPV6_ND) && defined(CONFIG_NET_NATIVE_IPV6)

void net_if_nbr_reachability_hint(struct net_if *iface, const struct in6_addr *ipv6_addr)
{
	net_if_lock(iface);

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		goto out;
	}

	if (!iface->config.ip.ipv6) {
		goto out;
	}

	net_ipv6_nbr_reachability_hint(iface, ipv6_addr);

out:
	net_if_unlock(iface);
}

#endif

/* To be called when interface comes up so that all the non-joined multicast
 * groups are joined.
 */
static void rejoin_ipv6_mcast_groups(struct net_if *iface)
{
	struct net_if_mcast_addr *ifaddr, *next;
	struct net_if_ipv6 *ipv6;
	sys_slist_t rejoin_needed;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV6) ||
	    net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		goto out;
	}

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		goto out;
	}

	/* Rejoin solicited node multicasts. */
	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!ipv6->unicast[i].is_used) {
			continue;
		}

		join_mcast_nodes(iface, &ipv6->unicast[i].address.in6_addr);
	}

	sys_slist_init(&rejoin_needed);

	/* Rejoin any mcast address present on the interface, but marked as not joined. */
	ARRAY_FOR_EACH(ipv6->mcast, i) {
		if (!ipv6->mcast[i].is_used ||
		    net_if_ipv6_maddr_is_joined(&ipv6->mcast[i])) {
			continue;
		}

		sys_slist_prepend(&rejoin_needed, &ipv6->mcast[i].rejoin_node);
	}

	net_if_unlock(iface);

	/* Start DAD for all the addresses without holding the iface lock
	 * to avoid any possible mutex deadlock issues.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&rejoin_needed,
					  ifaddr, next, rejoin_node) {
		int ret;

		ret = net_ipv6_mld_join(iface, &ifaddr->address.in6_addr);
		if (ret < 0) {
			NET_ERR("Cannot join mcast address %s for %d (%d)",
				net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
				net_if_get_by_iface(iface), ret);
		} else {
			NET_DBG("Rejoined mcast address %s for %d",
				net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
				net_if_get_by_iface(iface));
		}
	}

	return;

out:
	net_if_unlock(iface);
}

/* To be called when interface comes operational down so that multicast
 * groups are rejoined when back up.
 */
static void clear_joined_ipv6_mcast_groups(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV6)) {
		goto out;
	}

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->mcast, i) {
		if (!ipv6->mcast[i].is_used) {
			continue;
		}

		net_if_ipv6_maddr_leave(iface, &ipv6->mcast[i]);
	}

out:
	net_if_unlock(iface);
}

static void address_expired(struct net_if_addr *ifaddr)
{
	NET_DBG("IPv6 address %s is expired",
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr));

	sys_slist_find_and_remove(&active_address_lifetime_timers,
				  &ifaddr->lifetime.node);

	net_timeout_set(&ifaddr->lifetime, 0, 0);

	STRUCT_SECTION_FOREACH(net_if, iface) {
		ARRAY_FOR_EACH(iface->config.ip.ipv6->unicast, i) {
			if (&iface->config.ip.ipv6->unicast[i] == ifaddr) {
				net_if_ipv6_addr_rm(iface,
					&iface->config.ip.ipv6->unicast[i].address.in6_addr);
				return;
			}
		}
	}
}

static void address_lifetime_timeout(struct k_work *work)
{
	uint32_t next_update = UINT32_MAX;
	uint32_t current_time = k_uptime_get_32();
	struct net_if_addr *current, *next;

	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);

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

		k_work_reschedule(&address_lifetime_timer, K_MSEC(next_update));
	}

	k_mutex_unlock(&lock);
}

#if defined(CONFIG_NET_TEST)
void net_address_lifetime_timeout(void)
{
	address_lifetime_timeout(NULL);
}
#endif

static void address_start_timer(struct net_if_addr *ifaddr, uint32_t vlifetime)
{
	/* Make sure that we do not insert the address twice to
	 * the lifetime timer list.
	 */
	sys_slist_find_and_remove(&active_address_lifetime_timers,
				  &ifaddr->lifetime.node);

	sys_slist_append(&active_address_lifetime_timers,
			 &ifaddr->lifetime.node);

	net_timeout_set(&ifaddr->lifetime, vlifetime, k_uptime_get_32());
	k_work_reschedule(&address_lifetime_timer, K_NO_WAIT);
}
#else /* CONFIG_NET_NATIVE_IPV6 */
#define address_start_timer(...)
#define net_if_ipv6_start_dad(...)
#define join_mcast_nodes(...)
#endif /* CONFIG_NET_NATIVE_IPV6 */

struct net_if_addr *net_if_ipv6_addr_lookup(const struct in6_addr *addr,
					    struct net_if **ret)
{
	struct net_if_addr *ifaddr = NULL;

	STRUCT_SECTION_FOREACH(net_if, iface) {
		struct net_if_ipv6 *ipv6;

		net_if_lock(iface);

		ipv6 = iface->config.ip.ipv6;
		if (!ipv6) {
			net_if_unlock(iface);
			continue;
		}

		ARRAY_FOR_EACH(ipv6->unicast, i) {
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

				ifaddr = &ipv6->unicast[i];
				net_if_unlock(iface);
				goto out;
			}
		}

		net_if_unlock(iface);
	}

out:
	return ifaddr;
}

struct net_if_addr *net_if_ipv6_addr_lookup_by_iface(struct net_if *iface,
						     struct in6_addr *addr)
{
	struct net_if_addr *ifaddr = NULL;
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		if (net_ipv6_is_prefix(
			    addr->s6_addr,
			    ipv6->unicast[i].address.in6_addr.s6_addr,
			    128)) {
			ifaddr = &ipv6->unicast[i];
			goto out;
		}
	}

out:
	net_if_unlock(iface);

	return ifaddr;
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

	K_OOPS(k_usermode_from_copy(&addr_v6, (void *)addr, sizeof(addr_v6)));

	return z_impl_net_if_ipv6_addr_lookup_by_index(&addr_v6);
}
#include <zephyr/syscalls/net_if_ipv6_addr_lookup_by_index_mrsh.c>
#endif

void net_if_ipv6_addr_update_lifetime(struct net_if_addr *ifaddr,
				      uint32_t vlifetime)
{
	k_mutex_lock(&lock, K_FOREVER);

	NET_DBG("Updating expire time of %s by %u secs",
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
		vlifetime);

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	address_start_timer(ifaddr, vlifetime);

	k_mutex_unlock(&lock);
}

static struct net_if_addr *ipv6_addr_find(struct net_if *iface,
					  struct in6_addr *addr)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	ARRAY_FOR_EACH(ipv6->unicast, i) {
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
	ifaddr->is_added = true;
	ifaddr->is_temporary = false;
	ifaddr->address.family = AF_INET6;
	ifaddr->addr_type = addr_type;
	ifaddr->atomic_ref = ATOMIC_INIT(1);

	net_ipaddr_copy(&ifaddr->address.in6_addr, addr);

	/* FIXME - set the mcast addr for this node */

	if (vlifetime) {
		ifaddr->is_infinite = false;

		NET_DBG("Expiring %s in %u secs",
			net_sprint_ipv6_addr(addr),
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
	struct net_if_addr *ifaddr = NULL;
	struct net_if_ipv6 *ipv6;
	bool do_dad = false;

	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		goto out;
	}

	ifaddr = ipv6_addr_find(iface, addr);
	if (ifaddr) {
		/* Address already exists, just return it but update ref count
		 * if it was not updated. This could happen if the address was
		 * added and then removed but for example an active connection
		 * was still using it. In this case we must update the ref count
		 * so that the address is not removed if the connection is closed.
		 */
		if (!ifaddr->is_added) {
			atomic_inc(&ifaddr->atomic_ref);
			ifaddr->is_added = true;
		}

		goto out;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (ipv6->unicast[i].is_used) {
			continue;
		}

		net_if_addr_init(&ipv6->unicast[i], addr, addr_type,
				 vlifetime);

		NET_DBG("[%zu] interface %d (%p) address %s type %s added", i,
			net_if_get_by_iface(iface), iface,
			net_sprint_ipv6_addr(addr),
			net_addr_type2str(addr_type));

		if (IS_ENABLED(CONFIG_NET_IPV6_DAD) &&
		    !(l2_flags_get(iface) & NET_L2_POINT_TO_POINT) &&
		    !net_ipv6_is_addr_loopback(addr) &&
		    !net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
			/* The groups are joined without locks held */
			do_dad = true;
		} else {
			/* If DAD is not done for point-to-point links, then
			 * the address is usable immediately.
			 */
			ipv6->unicast[i].addr_state = NET_ADDR_PREFERRED;
		}

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_ADDR_ADD, iface,
			&ipv6->unicast[i].address.in6_addr,
			sizeof(struct in6_addr));

		ifaddr = &ipv6->unicast[i];
		break;
	}

	net_if_unlock(iface);

	if (ifaddr != NULL && do_dad) {
		/* RFC 4862 5.4.2
		 * Before sending a Neighbor Solicitation, an interface
		 * MUST join the all-nodes multicast address and the
		 * solicited-node multicast address of the tentative
		 * address.
		 */
		/* The allnodes multicast group is only joined once as
		 * net_ipv6_mld_join() checks if we have already
		 * joined.
		 */
		join_mcast_nodes(iface, &ifaddr->address.in6_addr);

		net_if_ipv6_start_dad(iface, ifaddr);
	}

	return ifaddr;

out:
	net_if_unlock(iface);

	return ifaddr;
}

bool net_if_ipv6_addr_rm(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_if_addr *ifaddr;
	struct net_if_ipv6 *ipv6;
	bool result = true;
	int ret;

	if (iface == NULL || addr == NULL) {
		return false;
	}

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		result = false;
		goto out;
	}

	ret = net_if_addr_unref(iface, AF_INET6, addr, &ifaddr);
	if (ret > 0) {
		NET_DBG("Address %s still in use (ref %d)",
			net_sprint_ipv6_addr(addr), ret);
		result = false;
		ifaddr->is_added = false;
		goto out;
	} else if (ret < 0) {
		NET_DBG("Address %s not found (%d)",
			net_sprint_ipv6_addr(addr), ret);
	}

out:
	net_if_unlock(iface);

	return result;
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

	K_OOPS(k_usermode_from_copy(&addr_v6, (void *)addr, sizeof(addr_v6)));

	return z_impl_net_if_ipv6_addr_add_by_index(index,
						    &addr_v6,
						    addr_type,
						    vlifetime);
}

#include <zephyr/syscalls/net_if_ipv6_addr_add_by_index_mrsh.c>
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

	K_OOPS(k_usermode_from_copy(&addr_v6, (void *)addr, sizeof(addr_v6)));

	return z_impl_net_if_ipv6_addr_rm_by_index(index, &addr_v6);
}

#include <zephyr/syscalls/net_if_ipv6_addr_rm_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

void net_if_ipv6_addr_foreach(struct net_if *iface, net_if_ip_addr_cb_t cb,
			      void *user_data)
{
	struct net_if_ipv6 *ipv6;

	if (iface == NULL) {
		return;
	}

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (ipv6 == NULL) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		struct net_if_addr *if_addr = &ipv6->unicast[i];

		if (!if_addr->is_used) {
			continue;
		}

		cb(iface, if_addr, user_data);
	}

out:
	net_if_unlock(iface);
}

struct net_if_mcast_addr *net_if_ipv6_maddr_add(struct net_if *iface,
						const struct in6_addr *addr)
{
	struct net_if_mcast_addr *ifmaddr = NULL;
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		goto out;
	}

	if (!net_ipv6_is_addr_mcast(addr)) {
		NET_DBG("Address %s is not a multicast address.",
			net_sprint_ipv6_addr(addr));
		goto out;
	}

	if (net_if_ipv6_maddr_lookup(addr, &iface)) {
		NET_WARN("Multicast address %s is already registered.",
			net_sprint_ipv6_addr(addr));
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->mcast, i) {
		if (ipv6->mcast[i].is_used) {
			continue;
		}

		ipv6->mcast[i].is_used = true;
		ipv6->mcast[i].address.family = AF_INET6;
		memcpy(&ipv6->mcast[i].address.in6_addr, addr, 16);

		NET_DBG("[%zu] interface %d (%p) address %s added", i,
			net_if_get_by_iface(iface), iface,
			net_sprint_ipv6_addr(addr));

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_MADDR_ADD, iface,
			&ipv6->mcast[i].address.in6_addr,
			sizeof(struct in6_addr));

		ifmaddr = &ipv6->mcast[i];
		goto out;
	}

out:
	net_if_unlock(iface);

	return ifmaddr;
}

bool net_if_ipv6_maddr_rm(struct net_if *iface, const struct in6_addr *addr)
{
	bool ret = false;
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->mcast, i) {
		if (!ipv6->mcast[i].is_used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(&ipv6->mcast[i].address.in6_addr,
				       addr)) {
			continue;
		}

		ipv6->mcast[i].is_used = false;

		NET_DBG("[%zu] interface %d (%p) address %s removed",
			i, net_if_get_by_iface(iface), iface,
			net_sprint_ipv6_addr(addr));

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV6_MADDR_DEL, iface,
			&ipv6->mcast[i].address.in6_addr,
			sizeof(struct in6_addr));

		ret = true;
		goto out;
	}

out:
	net_if_unlock(iface);

	return ret;
}

void net_if_ipv6_maddr_foreach(struct net_if *iface, net_if_ip_maddr_cb_t cb,
			       void *user_data)
{
	struct net_if_ipv6 *ipv6;

	if (iface == NULL || cb == NULL) {
		return;
	}

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	for (int i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used) {
			continue;
		}

		cb(iface, &ipv6->mcast[i], user_data);
	}

out:
	net_if_unlock(iface);
}

struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(const struct in6_addr *maddr,
						   struct net_if **ret)
{
	struct net_if_mcast_addr *ifmaddr = NULL;

	STRUCT_SECTION_FOREACH(net_if, iface) {
		struct net_if_ipv6 *ipv6;

		if (ret && *ret && iface != *ret) {
			continue;
		}

		net_if_lock(iface);

		ipv6 = iface->config.ip.ipv6;
		if (!ipv6) {
			net_if_unlock(iface);
			continue;
		}

		ARRAY_FOR_EACH(ipv6->mcast, i) {
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

				ifmaddr = &ipv6->mcast[i];
				net_if_unlock(iface);
				goto out;
			}
		}

		net_if_unlock(iface);
	}

out:
	return ifmaddr;
}

void net_if_ipv6_maddr_leave(struct net_if *iface, struct net_if_mcast_addr *addr)
{
	if (iface == NULL || addr == NULL) {
		return;
	}

	net_if_lock(iface);
	addr->is_joined = false;
	net_if_unlock(iface);
}

void net_if_ipv6_maddr_join(struct net_if *iface, struct net_if_mcast_addr *addr)
{
	if (iface == NULL || addr == NULL) {
		return;
	}

	net_if_lock(iface);
	addr->is_joined = true;
	net_if_unlock(iface);
}

struct in6_addr *net_if_ipv6_get_ll(struct net_if *iface,
				    enum net_addr_state addr_state)
{
	struct in6_addr *addr = NULL;
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!ipv6->unicast[i].is_used ||
		    (addr_state != NET_ADDR_ANY_STATE &&
		     ipv6->unicast[i].addr_state != addr_state) ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		if (net_ipv6_is_ll_addr(&ipv6->unicast[i].address.in6_addr)) {
			addr = &ipv6->unicast[i].address.in6_addr;
			goto out;
		}
	}

out:
	net_if_unlock(iface);

	return addr;
}

struct in6_addr *net_if_ipv6_get_ll_addr(enum net_addr_state state,
					 struct net_if **iface)
{
	struct in6_addr *addr = NULL;

	STRUCT_SECTION_FOREACH(net_if, tmp) {
		net_if_lock(tmp);

		addr = net_if_ipv6_get_ll(tmp, state);
		if (addr) {
			if (iface) {
				*iface = tmp;
			}

			net_if_unlock(tmp);
			goto out;
		}

		net_if_unlock(tmp);
	}

out:
	return addr;
}

static inline struct in6_addr *check_global_addr(struct net_if *iface,
						 enum net_addr_state state)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	if (!ipv6) {
		return NULL;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
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
	struct in6_addr *addr = NULL;

	STRUCT_SECTION_FOREACH(net_if, tmp) {
		if (iface && *iface && tmp != *iface) {
			continue;
		}

		net_if_lock(tmp);
		addr = check_global_addr(tmp, state);
		if (addr) {
			if (iface) {
				*iface = tmp;
			}

			net_if_unlock(tmp);
			goto out;
		}

		net_if_unlock(tmp);
	}

out:

	return addr;
}

#if defined(CONFIG_NET_NATIVE_IPV6)
static void remove_prefix_addresses(struct net_if *iface,
				    struct net_if_ipv6 *ipv6,
				    struct in6_addr *addr,
				    uint8_t len)
{
	ARRAY_FOR_EACH(ipv6->unicast, i) {
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

	net_if_lock(ifprefix->iface);

	NET_DBG("Prefix %s/%d expired",
		net_sprint_ipv6_addr(&ifprefix->prefix),
		ifprefix->len);

	ifprefix->is_used = false;

	if (net_if_config_ipv6_get(ifprefix->iface, &ipv6) < 0) {
		return;
	}

	/* Remove also all auto addresses if the they have the same prefix.
	 */
	remove_prefix_addresses(ifprefix->iface, ipv6, &ifprefix->prefix,
				ifprefix->len);

	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)) {
		struct net_event_ipv6_prefix info;

		net_ipaddr_copy(&info.addr, &ifprefix->prefix);
		info.len = ifprefix->len;
		info.lifetime = 0;

		net_mgmt_event_notify_with_info(NET_EVENT_IPV6_PREFIX_DEL,
						ifprefix->iface,
						(const void *) &info,
						sizeof(struct net_event_ipv6_prefix));
	} else {
		net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_DEL, ifprefix->iface);
	}

	net_if_unlock(ifprefix->iface);
}

static void prefix_timer_remove(struct net_if_ipv6_prefix *ifprefix)
{
	k_mutex_lock(&lock, K_FOREVER);

	NET_DBG("IPv6 prefix %s/%d removed",
		net_sprint_ipv6_addr(&ifprefix->prefix),
		ifprefix->len);

	sys_slist_find_and_remove(&active_prefix_lifetime_timers,
				  &ifprefix->lifetime.node);

	net_timeout_set(&ifprefix->lifetime, 0, 0);

	k_mutex_unlock(&lock);
}

static void prefix_lifetime_timeout(struct k_work *work)
{
	uint32_t next_update = UINT32_MAX;
	uint32_t current_time = k_uptime_get_32();
	struct net_if_ipv6_prefix *current, *next;
	sys_slist_t expired_list;

	ARG_UNUSED(work);

	sys_slist_init(&expired_list);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_prefix_lifetime_timers,
					  current, next, lifetime.node) {
		struct net_timeout *timeout = &current->lifetime;
		uint32_t this_update = net_timeout_evaluate(timeout,
							    current_time);

		if (this_update == 0U) {
			sys_slist_find_and_remove(
				&active_prefix_lifetime_timers,
				&current->lifetime.node);
			sys_slist_append(&expired_list,
					 &current->lifetime.node);
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
		k_work_reschedule(&prefix_lifetime_timer, K_MSEC(next_update));
	}

	k_mutex_unlock(&lock);

	SYS_SLIST_FOR_EACH_CONTAINER(&expired_list, current, lifetime.node) {
		prefix_lifetime_expired(current);
	}
}

static void prefix_start_timer(struct net_if_ipv6_prefix *ifprefix,
			       uint32_t lifetime)
{
	k_mutex_lock(&lock, K_FOREVER);

	(void)sys_slist_find_and_remove(&active_prefix_lifetime_timers,
					&ifprefix->lifetime.node);
	sys_slist_append(&active_prefix_lifetime_timers,
			 &ifprefix->lifetime.node);

	net_timeout_set(&ifprefix->lifetime, lifetime, k_uptime_get_32());
	k_work_reschedule(&prefix_lifetime_timer, K_NO_WAIT);

	k_mutex_unlock(&lock);
}

static struct net_if_ipv6_prefix *ipv6_prefix_find(struct net_if *iface,
						   struct in6_addr *prefix,
						   uint8_t prefix_len)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	if (!ipv6) {
		return NULL;
	}

	ARRAY_FOR_EACH(ipv6->prefix, i) {
		if (!ipv6->prefix[i].is_used) {
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
	struct net_if_ipv6_prefix *ifprefix = NULL;
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		goto out;
	}

	ifprefix = ipv6_prefix_find(iface, prefix, len);
	if (ifprefix) {
		goto out;
	}

	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->prefix, i) {
		if (ipv6->prefix[i].is_used) {
			continue;
		}

		net_if_ipv6_prefix_init(iface, &ipv6->prefix[i], prefix,
					len, lifetime);

		NET_DBG("[%zu] interface %p prefix %s/%d added", i, iface,
			net_sprint_ipv6_addr(prefix), len);

		if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)) {
			struct net_event_ipv6_prefix info;

			net_ipaddr_copy(&info.addr, prefix);
			info.len = len;
			info.lifetime = lifetime;

			net_mgmt_event_notify_with_info(NET_EVENT_IPV6_PREFIX_ADD,
							iface, (const void *) &info,
							sizeof(struct net_event_ipv6_prefix));
		} else {
			net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_ADD, iface);
		}

		ifprefix = &ipv6->prefix[i];
		goto out;
	}

out:
	net_if_unlock(iface);

	return ifprefix;
}

bool net_if_ipv6_prefix_rm(struct net_if *iface, struct in6_addr *addr,
			   uint8_t len)
{
	bool ret = false;
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->prefix, i) {
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

		if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)) {
			struct net_event_ipv6_prefix info;

			net_ipaddr_copy(&info.addr, addr);
			info.len = len;
			info.lifetime = 0;

			net_mgmt_event_notify_with_info(NET_EVENT_IPV6_PREFIX_DEL,
							iface, (const void *) &info,
							sizeof(struct net_event_ipv6_prefix));
		} else {
			net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_DEL, iface);
		}

		ret = true;
		goto out;
	}

out:
	net_if_unlock(iface);

	return ret;
}

struct net_if_ipv6_prefix *net_if_ipv6_prefix_get(struct net_if *iface,
						  const struct in6_addr *addr)
{
	struct net_if_ipv6_prefix *prefix = NULL;
	struct net_if_ipv6 *ipv6;

	if (!iface) {
		iface = net_if_get_default();
	}

	if (!iface) {
		return NULL;
	}

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->prefix, i) {
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

out:
	net_if_unlock(iface);

	return prefix;
}

struct net_if_ipv6_prefix *net_if_ipv6_prefix_lookup(struct net_if *iface,
						     struct in6_addr *addr,
						     uint8_t len)
{
	struct net_if_ipv6_prefix *prefix = NULL;
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->prefix, i) {
		if (!ipv6->prefix[i].is_used) {
			continue;
		}

		if (net_ipv6_is_prefix(ipv6->prefix[i].prefix.s6_addr,
				       addr->s6_addr, len)) {
			prefix = &ipv6->prefix[i];
			goto out;
		}
	}

out:
	net_if_unlock(iface);

	return prefix;
}

bool net_if_ipv6_addr_onlink(struct net_if **iface, struct in6_addr *addr)
{
	bool ret = false;

	STRUCT_SECTION_FOREACH(net_if, tmp) {
		struct net_if_ipv6 *ipv6;

		if (iface && *iface && *iface != tmp) {
			continue;
		}

		net_if_lock(tmp);

		ipv6 = tmp->config.ip.ipv6;
		if (!ipv6) {
			net_if_unlock(tmp);
			continue;
		}

		ARRAY_FOR_EACH(ipv6->prefix, i) {
			if (ipv6->prefix[i].is_used &&
			    net_ipv6_is_prefix(ipv6->prefix[i].prefix.s6_addr,
					       addr->s6_addr,
					       ipv6->prefix[i].len)) {
				if (iface) {
					*iface = tmp;
				}

				ret = true;
				net_if_unlock(tmp);
				goto out;
			}
		}

		net_if_unlock(tmp);
	}

out:
	return ret;
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
		net_sprint_ipv6_addr(&router->address.in6_addr),
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

uint8_t net_if_ipv6_get_mcast_hop_limit(struct net_if *iface)
{
	int ret = 0;

	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv6) {
		goto out;
	}

	ret = iface->config.ip.ipv6->mcast_hop_limit;
out:
	net_if_unlock(iface);

	return ret;
}

void net_if_ipv6_set_mcast_hop_limit(struct net_if *iface, uint8_t hop_limit)
{
	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv6) {
		goto out;
	}

	iface->config.ip.ipv6->mcast_hop_limit = hop_limit;
out:
	net_if_unlock(iface);
}

uint8_t net_if_ipv6_get_hop_limit(struct net_if *iface)
{
	int ret = 0;

	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv6) {
		goto out;
	}

	ret = iface->config.ip.ipv6->hop_limit;
out:
	net_if_unlock(iface);

	return ret;
}

void net_if_ipv6_set_hop_limit(struct net_if *iface, uint8_t hop_limit)
{
	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv6) {
		goto out;
	}

	iface->config.ip.ipv6->hop_limit = hop_limit;
out:
	net_if_unlock(iface);
}

#endif /* CONFIG_NET_NATIVE_IPV6 */

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

static bool use_public_address(bool prefer_public, bool is_temporary,
			       int flags)
{
	if (IS_ENABLED(CONFIG_NET_IPV6_PE)) {
		if (!prefer_public && is_temporary) {

			/* Allow socket to override the kconfig option */
			if (flags & IPV6_PREFER_SRC_PUBLIC) {
				return true;
			}

			return false;
		}
	}

	if (flags & IPV6_PREFER_SRC_TMP) {
		return false;
	}

	return true;
}

static struct in6_addr *net_if_ipv6_get_best_match(struct net_if *iface,
						   const struct in6_addr *dst,
						   uint8_t prefix_len,
						   uint8_t *best_so_far,
						   int flags)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	struct net_if_addr *public_addr = NULL;
	struct in6_addr *src = NULL;
	uint8_t public_addr_len = 0;
	struct in6_addr *temp_addr = NULL;
	uint8_t len, temp_addr_len = 0;
	bool ret;

	net_if_lock(iface);

	ipv6 = iface->config.ip.ipv6;
	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!is_proper_ipv6_address(&ipv6->unicast[i])) {
			continue;
		}

		/* This is a dirty hack until we have proper IPv6 routing.
		 * Without this the IPv6 packets might go to VPN interface for
		 * subnets that are not on the same subnet as the VPN interface
		 * which typically is not desired.
		 * TODO: Implement IPv6 routing support and remove this hack.
		 */
		if (IS_ENABLED(CONFIG_NET_VPN)) {
			/* For the VPN interface, we need to check if
			 * address matches exactly the address of the interface.
			 */
			if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
			    net_virtual_get_iface_capabilities(iface) == VIRTUAL_INTERFACE_VPN) {
				/* FIXME: Do not hard code the prefix length */
				if (!net_ipv6_is_prefix(
					    (const uint8_t *)&ipv6->unicast[i].address.in6_addr,
					    (const uint8_t *)dst,
					    64)) {
					/* Skip this address as it is no match */
					continue;
				}
			}
		}

		len = get_diff_ipv6(dst, &ipv6->unicast[i].address.in6_addr);
		if (len >= prefix_len) {
			len = prefix_len;
		}

		if (len >= *best_so_far) {
			/* Mesh local address can only be selected for the same
			 * subnet.
			 */
			if (ipv6->unicast[i].is_mesh_local && len < 64 &&
			    !net_ipv6_is_addr_mcast_mesh(dst)) {
				continue;
			}

			ret = use_public_address(iface->pe_prefer_public,
						 ipv6->unicast[i].is_temporary,
						 flags);
			if (!ret) {
				temp_addr = &ipv6->unicast[i].address.in6_addr;
				temp_addr_len = len;

				*best_so_far = len;
				src = &ipv6->unicast[i].address.in6_addr;
				continue;
			}

			if (!ipv6->unicast[i].is_temporary) {
				public_addr = &ipv6->unicast[i];
				public_addr_len = len;
			}

			*best_so_far = len;
			src = &ipv6->unicast[i].address.in6_addr;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6_PE) && !iface->pe_prefer_public && temp_addr) {
		if (temp_addr_len >= *best_so_far) {
			*best_so_far = temp_addr_len;
			src = temp_addr;
		}
	} else {
		/* By default prefer always public address if found */
		if (flags & IPV6_PREFER_SRC_PUBLIC) {
use_public:
			if (public_addr &&
			    !net_ipv6_addr_cmp(&public_addr->address.in6_addr, src)) {
				src = &public_addr->address.in6_addr;
				*best_so_far = public_addr_len;
			}
		} else if (flags & IPV6_PREFER_SRC_TMP) {
			if (temp_addr && !net_ipv6_addr_cmp(temp_addr, src)) {
				src = temp_addr;
				*best_so_far = temp_addr_len;
			}
		} else if (flags & IPV6_PREFER_SRC_PUBTMP_DEFAULT) {
			goto use_public;
		}
	}

out:
	net_if_unlock(iface);

	return src;
}

const struct in6_addr *net_if_ipv6_select_src_addr_hint(struct net_if *dst_iface,
							const struct in6_addr *dst,
							int flags)
{
	const struct in6_addr *src = NULL;
	uint8_t best_match = 0U;

	if (dst == NULL) {
		return NULL;
	}

	if (!net_ipv6_is_ll_addr(dst) && !net_ipv6_is_addr_mcast_link(dst)) {
		struct net_if_ipv6_prefix *prefix;
		uint8_t prefix_len = 128;

		prefix = net_if_ipv6_prefix_get(dst_iface, dst);
		if (prefix) {
			prefix_len = prefix->len;
		}

		/* If caller has supplied interface, then use that */
		if (dst_iface) {
			src = net_if_ipv6_get_best_match(dst_iface, dst,
							 prefix_len,
							 &best_match,
							 flags);
		} else {
			STRUCT_SECTION_FOREACH(net_if, iface) {
				struct in6_addr *addr;

				addr = net_if_ipv6_get_best_match(iface, dst,
								  prefix_len,
								  &best_match,
								  flags);
				if (addr) {
					src = addr;
				}
			}
		}

	} else {
		if (dst_iface) {
			src = net_if_ipv6_get_ll(dst_iface, NET_ADDR_PREFERRED);
		} else {
			struct in6_addr *addr;

			addr = net_if_ipv6_get_ll(net_if_get_default(), NET_ADDR_PREFERRED);
			if (addr) {
				src = addr;
				goto out;
			}

			STRUCT_SECTION_FOREACH(net_if, iface) {
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
		src = net_ipv6_unspecified_address();
	}

out:
	return src;
}

const struct in6_addr *net_if_ipv6_select_src_addr(struct net_if *dst_iface,
						   const struct in6_addr *dst)
{
	return net_if_ipv6_select_src_addr_hint(dst_iface,
						dst,
						IPV6_PREFER_SRC_PUBTMP_DEFAULT);
}

struct net_if *net_if_ipv6_select_src_iface_addr(const struct in6_addr *dst,
						 const struct in6_addr **src_addr)
{
	struct net_if *iface = NULL;
	const struct in6_addr *src;

	src = net_if_ipv6_select_src_addr(NULL, dst);
	if (src != net_ipv6_unspecified_address()) {
		net_if_ipv6_addr_lookup(src, &iface);
	}

	if (src_addr != NULL) {
		*src_addr = src;
	}

	if (iface == NULL) {
		iface = net_if_get_default();
	}

	return iface;
}

struct net_if *net_if_ipv6_select_src_iface(const struct in6_addr *dst)
{
	return net_if_ipv6_select_src_iface_addr(dst, NULL);
}

#if defined(CONFIG_NET_NATIVE_IPV6)

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
	if (!net_if_flag_is_set(iface, NET_IF_IPV6) ||
	    net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6_DAD)) {
		net_if_start_dad(iface);
	} else {
		struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

		if (ipv6 != NULL) {
			join_mcast_nodes(iface,
					 &ipv6->mcast[0].address.in6_addr);
		}
	}

	net_if_start_rs(iface);
}

static void iface_ipv6_stop(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	if (!net_if_flag_is_set(iface, NET_IF_IPV6) ||
	    net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		return;
	}

	if (ipv6 == NULL) {
		return;
	}

	IF_ENABLED(CONFIG_NET_IPV6_IID_STABLE, (ipv6->network_counter++));
	IF_ENABLED(CONFIG_NET_IPV6_IID_STABLE, (ipv6->iid = NULL));

	/* Remove all autoconf addresses */
	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (ipv6->unicast[i].is_used &&
		    ipv6->unicast[i].address.family == AF_INET6 &&
		    ipv6->unicast[i].addr_type == NET_ADDR_AUTOCONF) {
			(void)net_if_ipv6_addr_rm(iface,
						  &ipv6->unicast[i].address.in6_addr);
		}
	}
}

static void iface_ipv6_init(int if_count)
{
	iface_ipv6_dad_init();
	iface_ipv6_nd_init();

	k_work_init_delayable(&address_lifetime_timer,
			      address_lifetime_timeout);
	k_work_init_delayable(&prefix_lifetime_timer, prefix_lifetime_timeout);

	if (if_count > ARRAY_SIZE(ipv6_addresses)) {
		NET_WARN("You have %zu IPv6 net_if addresses but %d "
			 "network interfaces", ARRAY_SIZE(ipv6_addresses),
			 if_count);
		NET_WARN("Consider increasing CONFIG_NET_IF_MAX_IPV6_COUNT "
			 "value.");
	}

	ARRAY_FOR_EACH(ipv6_addresses, i) {
		ipv6_addresses[i].ipv6.hop_limit = CONFIG_NET_INITIAL_HOP_LIMIT;
		ipv6_addresses[i].ipv6.mcast_hop_limit = CONFIG_NET_INITIAL_MCAST_HOP_LIMIT;
		ipv6_addresses[i].ipv6.base_reachable_time = REACHABLE_TIME;

		net_if_ipv6_set_reachable_time(&ipv6_addresses[i].ipv6);
	}
}
#endif /* CONFIG_NET_NATIVE_IPV6 */
#else /* CONFIG_NET_IPV6 */
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

#if !defined(CONFIG_NET_NATIVE_IPV6)
#define join_mcast_allnodes(...)
#define leave_mcast_all(...)
#define clear_joined_ipv6_mcast_groups(...)
#define iface_ipv6_start(...)
#define iface_ipv6_stop(...)
#define iface_ipv6_init(...)
#endif /* !CONFIG_NET_NATIVE_IPV4 */

#if defined(CONFIG_NET_IPV4)
int net_if_config_ipv4_get(struct net_if *iface, struct net_if_ipv4 **ipv4)
{
	int ret = 0;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV4)) {
		ret = -ENOTSUP;
		goto out;
	}

	if (iface->config.ip.ipv4) {
		if (ipv4) {
			*ipv4 = iface->config.ip.ipv4;
		}

		goto out;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(ipv4_addresses, i) {
		if (ipv4_addresses[i].iface) {
			continue;
		}

		iface->config.ip.ipv4 = &ipv4_addresses[i].ipv4;
		ipv4_addresses[i].iface = iface;

		if (ipv4) {
			*ipv4 = &ipv4_addresses[i].ipv4;
		}

		k_mutex_unlock(&lock);
		goto out;
	}

	k_mutex_unlock(&lock);

	ret = -ESRCH;
out:
	net_if_unlock(iface);

	return ret;
}

int net_if_config_ipv4_put(struct net_if *iface)
{
	int ret = 0;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV4)) {
		ret = -ENOTSUP;
		goto out;
	}

	if (!iface->config.ip.ipv4) {
		ret = -EALREADY;
		goto out;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(ipv4_addresses, i) {
		if (ipv4_addresses[i].iface != iface) {
			continue;
		}

		iface->config.ip.ipv4 = NULL;
		ipv4_addresses[i].iface = NULL;

		k_mutex_unlock(&lock);
		goto out;
	}

	k_mutex_unlock(&lock);

	ret = -ESRCH;
out:
	net_if_unlock(iface);

	return ret;
}

bool net_if_ipv4_addr_mask_cmp(struct net_if *iface,
			       const struct in_addr *addr)
{
	bool ret = false;
	struct net_if_ipv4 *ipv4;
	uint32_t subnet;

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		subnet = UNALIGNED_GET(&addr->s_addr) &
			 ipv4->unicast[i].netmask.s_addr;

		if ((ipv4->unicast[i].ipv4.address.in_addr.s_addr &
		     ipv4->unicast[i].netmask.s_addr) == subnet) {
			ret = true;
			goto out;
		}
	}

out:
	net_if_unlock(iface);

	return ret;
}

static bool ipv4_is_broadcast_address(struct net_if *iface,
				      const struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4;
	bool ret = false;
	struct in_addr bcast;

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		ret = false;
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		bcast.s_addr = ipv4->unicast[i].ipv4.address.in_addr.s_addr |
			       ~ipv4->unicast[i].netmask.s_addr;

		if (bcast.s_addr == UNALIGNED_GET(&addr->s_addr)) {
			ret = true;
			goto out;
		}
	}

out:
	net_if_unlock(iface);
	return ret;
}

bool net_if_ipv4_is_addr_bcast(struct net_if *iface,
			       const struct in_addr *addr)
{
	bool ret = false;

	if (iface) {
		ret = ipv4_is_broadcast_address(iface, addr);
		goto out;
	}

	STRUCT_SECTION_FOREACH(net_if, one_iface) {
		ret = ipv4_is_broadcast_address(one_iface, addr);
		if (ret) {
			goto out;
		}
	}

out:
	return ret;
}

struct net_if *net_if_ipv4_select_src_iface_addr(const struct in_addr *dst,
						 const struct in_addr **src_addr)
{
	struct net_if *selected = NULL;
	const struct in_addr *src;

	src = net_if_ipv4_select_src_addr(NULL, dst);
	if (src != net_ipv4_unspecified_address()) {
		net_if_ipv4_addr_lookup(src, &selected);
	}

	if (selected == NULL) {
		selected = net_if_get_default();
	}

	if (src_addr != NULL) {
		*src_addr = src;
	}

	return selected;
}

struct net_if *net_if_ipv4_select_src_iface(const struct in_addr *dst)
{
	return net_if_ipv4_select_src_iface_addr(dst, NULL);
}

static uint8_t get_diff_ipv4(const struct in_addr *src,
			  const struct in_addr *dst)
{
	return get_ipaddr_diff((const uint8_t *)src, (const uint8_t *)dst, 4);
}

static inline bool is_proper_ipv4_address(struct net_if_addr *addr)
{
	if (addr->is_used && addr->addr_state == NET_ADDR_PREFERRED &&
	    addr->address.family == AF_INET) {
		return true;
	}

	return false;
}

static struct in_addr *net_if_ipv4_get_best_match(struct net_if *iface,
						  const struct in_addr *dst,
						  uint8_t *best_so_far, bool ll)
{
	struct net_if_ipv4 *ipv4;
	struct in_addr *src = NULL;
	uint8_t len;

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		struct in_addr subnet;

		if (!is_proper_ipv4_address(&ipv4->unicast[i].ipv4)) {
			continue;
		}

		if (net_ipv4_is_ll_addr(&ipv4->unicast[i].ipv4.address.in_addr) != ll) {
			continue;
		}

		/* This is a dirty hack until we have proper IPv4 routing.
		 * Without this the IPv4 packets might go to VPN interface for
		 * subnets that are not on the same subnet as the VPN interface
		 * which typically is not desired.
		 * TODO: Implement IPv4 routing support and remove this hack.
		 */
		if (IS_ENABLED(CONFIG_NET_VPN)) {
			/* For the VPN interface, we need to check if
			 * address matches exactly the address of the interface.
			 */
			if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
			    net_virtual_get_iface_capabilities(iface) == VIRTUAL_INTERFACE_VPN) {
				subnet.s_addr = ipv4->unicast[i].ipv4.address.in_addr.s_addr &
					ipv4->unicast[i].netmask.s_addr;

				if (subnet.s_addr !=
				    (dst->s_addr & ipv4->unicast[i].netmask.s_addr)) {
					/* Skip this address as it is no match */
					continue;
				}
			}
		}

		subnet.s_addr = ipv4->unicast[i].ipv4.address.in_addr.s_addr &
				ipv4->unicast[i].netmask.s_addr;
		len = get_diff_ipv4(dst, &subnet);
		if (len >= *best_so_far) {
			*best_so_far = len;
			src = &ipv4->unicast[i].ipv4.address.in_addr;
		}
	}

out:
	net_if_unlock(iface);

	return src;
}

static struct in_addr *if_ipv4_get_addr(struct net_if *iface,
					enum net_addr_state addr_state, bool ll)
{
	struct in_addr *addr = NULL;
	struct net_if_ipv4 *ipv4;

	if (!iface) {
		return NULL;
	}

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    (addr_state != NET_ADDR_ANY_STATE &&
		     ipv4->unicast[i].ipv4.addr_state != addr_state) ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		if (net_ipv4_is_ll_addr(&ipv4->unicast[i].ipv4.address.in_addr)) {
			if (!ll) {
				continue;
			}
		} else {
			if (ll) {
				continue;
			}
		}

		addr = &ipv4->unicast[i].ipv4.address.in_addr;
		goto out;
	}

out:
	net_if_unlock(iface);

	return addr;
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
	const struct in_addr *src = NULL;
	uint8_t best_match = 0U;

	if (dst == NULL) {
		return NULL;
	}

	if (!net_ipv4_is_ll_addr(dst)) {

		/* If caller has supplied interface, then use that */
		if (dst_iface) {
			src = net_if_ipv4_get_best_match(dst_iface, dst,
							 &best_match, false);
		} else {
			STRUCT_SECTION_FOREACH(net_if, iface) {
				struct in_addr *addr;

				addr = net_if_ipv4_get_best_match(iface, dst,
								  &best_match,
								  false);
				if (addr) {
					src = addr;
				}
			}
		}

	} else {
		if (dst_iface) {
			src = net_if_ipv4_get_ll(dst_iface, NET_ADDR_PREFERRED);
		} else {
			struct in_addr *addr;

			STRUCT_SECTION_FOREACH(net_if, iface) {
				addr = net_if_ipv4_get_best_match(iface, dst,
								  &best_match,
								  true);
				if (addr) {
					src = addr;
				}
			}

			/* Check the default interface again. It will only
			 * be used if it has a valid LL address, and there was
			 * no better match on any other interface.
			 */
			addr = net_if_ipv4_get_best_match(net_if_get_default(),
							  dst, &best_match,
							  true);
			if (addr) {
				src = addr;
			}
		}
	}

	if (!src) {
		src = net_if_ipv4_get_global_addr(dst_iface,
						  NET_ADDR_PREFERRED);

		if (IS_ENABLED(CONFIG_NET_IPV4_AUTO) && !src) {
			/* Try to use LL address if there's really no other
			 * address available.
			 */
			src = net_if_ipv4_get_ll(dst_iface, NET_ADDR_PREFERRED);
		}

		if (!src) {
			src = net_ipv4_unspecified_address();
		}
	}

	return src;
}

/* Internal function to get the first IPv4 address of the interface */
struct net_if_addr *net_if_ipv4_addr_get_first_by_index(int ifindex)
{
	struct net_if *iface = net_if_get_by_index(ifindex);
	struct net_if_addr *ifaddr = NULL;
	struct net_if_ipv4 *ipv4;

	if (!iface) {
		return NULL;
	}

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		ifaddr = &ipv4->unicast[i].ipv4;
		break;
	}

out:
	net_if_unlock(iface);

	return ifaddr;
}

struct net_if_addr *net_if_ipv4_addr_lookup(const struct in_addr *addr,
					    struct net_if **ret)
{
	struct net_if_addr *ifaddr = NULL;

	STRUCT_SECTION_FOREACH(net_if, iface) {
		struct net_if_ipv4 *ipv4;

		net_if_lock(iface);

		ipv4 = iface->config.ip.ipv4;
		if (!ipv4) {
			net_if_unlock(iface);
			continue;
		}

		ARRAY_FOR_EACH(ipv4->unicast, i) {
			if (!ipv4->unicast[i].ipv4.is_used ||
			    ipv4->unicast[i].ipv4.address.family != AF_INET) {
				continue;
			}

			if (UNALIGNED_GET(&addr->s4_addr32[0]) ==
			    ipv4->unicast[i].ipv4.address.in_addr.s_addr) {

				if (ret) {
					*ret = iface;
				}

				ifaddr = &ipv4->unicast[i].ipv4;
				net_if_unlock(iface);
				goto out;
			}
		}

		net_if_unlock(iface);
	}

out:
	return ifaddr;
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

	K_OOPS(k_usermode_from_copy(&addr_v4, (void *)addr, sizeof(addr_v4)));

	return z_impl_net_if_ipv4_addr_lookup_by_index(&addr_v4);
}
#include <zephyr/syscalls/net_if_ipv4_addr_lookup_by_index_mrsh.c>
#endif

struct in_addr net_if_ipv4_get_netmask_by_addr(struct net_if *iface,
					       const struct in_addr *addr)
{
	struct in_addr netmask = { 0 };
	struct net_if_ipv4 *ipv4;
	uint32_t subnet;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	ipv4 = iface->config.ip.ipv4;
	if (ipv4 == NULL) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		subnet = UNALIGNED_GET(&addr->s_addr) &
			 ipv4->unicast[i].netmask.s_addr;

		if ((ipv4->unicast[i].ipv4.address.in_addr.s_addr &
		     ipv4->unicast[i].netmask.s_addr) == subnet) {
			netmask = ipv4->unicast[i].netmask;
			goto out;
		}
	}

out:
	net_if_unlock(iface);

	return netmask;
}

bool net_if_ipv4_set_netmask_by_addr(struct net_if *iface,
				     const struct in_addr *addr,
				     const struct in_addr *netmask)
{
	struct net_if_ipv4 *ipv4;
	uint32_t subnet;
	bool ret = false;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	ipv4 = iface->config.ip.ipv4;
	if (ipv4 == NULL) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		subnet = UNALIGNED_GET(&addr->s_addr) &
			 ipv4->unicast[i].netmask.s_addr;

		if ((ipv4->unicast[i].ipv4.address.in_addr.s_addr &
		     ipv4->unicast[i].netmask.s_addr) == subnet) {
			ipv4->unicast[i].netmask = *netmask;
			ret = true;
			goto out;
		}
	}

out:
	net_if_unlock(iface);

	return ret;
}

/* Using this function is problematic as if we have multiple
 * addresses configured, which one to return. Use heuristic
 * in this case and return the first one found. Please use
 * net_if_ipv4_get_netmask_by_addr() instead.
 */
struct in_addr net_if_ipv4_get_netmask(struct net_if *iface)
{
	struct in_addr netmask = { 0 };
	struct net_if_ipv4 *ipv4;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	ipv4 = iface->config.ip.ipv4;
	if (ipv4 == NULL) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		netmask = iface->config.ip.ipv4->unicast[i].netmask;
		break;
	}

out:
	net_if_unlock(iface);

	return netmask;
}

/* Using this function is problematic as if we have multiple
 * addresses configured, which one to set. Use heuristic
 * in this case and set the first one found. Please use
 * net_if_ipv4_set_netmask_by_addr() instead.
 */
static void net_if_ipv4_set_netmask_deprecated(struct net_if *iface,
					       const struct in_addr *netmask)
{
	struct net_if_ipv4 *ipv4;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	ipv4 = iface->config.ip.ipv4;
	if (ipv4 == NULL) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET) {
			continue;
		}

		net_ipaddr_copy(&ipv4->unicast[i].netmask, netmask);
		break;
	}

out:
	net_if_unlock(iface);
}

void net_if_ipv4_set_netmask(struct net_if *iface,
			     const struct in_addr *netmask)
{
	net_if_ipv4_set_netmask_deprecated(iface, netmask);
}

bool z_impl_net_if_ipv4_set_netmask_by_index(int index,
					     const struct in_addr *netmask)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	net_if_ipv4_set_netmask_deprecated(iface, netmask);

	return true;
}

bool z_impl_net_if_ipv4_set_netmask_by_addr_by_index(int index,
						     const struct in_addr *addr,
						     const struct in_addr *netmask)
{
	struct net_if *iface;

	iface = net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	net_if_ipv4_set_netmask_by_addr(iface, addr, netmask);

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

	K_OOPS(k_usermode_from_copy(&netmask_addr, (void *)netmask,
				sizeof(netmask_addr)));

	return z_impl_net_if_ipv4_set_netmask_by_index(index, &netmask_addr);
}

#include <zephyr/syscalls/net_if_ipv4_set_netmask_by_index_mrsh.c>

bool z_vrfy_net_if_ipv4_set_netmask_by_addr_by_index(int index,
						     const struct in_addr *addr,
						     const struct in_addr *netmask)
{
	struct in_addr ipv4_addr, netmask_addr;
	struct net_if *iface;

	iface = z_vrfy_net_if_get_by_index(index);
	if (!iface) {
		return false;
	}

	K_OOPS(k_usermode_from_copy(&ipv4_addr, (void *)addr,
				    sizeof(ipv4_addr)));
	K_OOPS(k_usermode_from_copy(&netmask_addr, (void *)netmask,
				    sizeof(netmask_addr)));

	return z_impl_net_if_ipv4_set_netmask_by_addr_by_index(index,
							       &ipv4_addr,
							       &netmask_addr);
}

#include <zephyr/syscalls/net_if_ipv4_set_netmask_by_addr_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

struct in_addr net_if_ipv4_get_gw(struct net_if *iface)
{
	struct in_addr gw = { 0 };

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv4) {
		goto out;
	}

	gw = iface->config.ip.ipv4->gw;
out:
	net_if_unlock(iface);

	return gw;
}

void net_if_ipv4_set_gw(struct net_if *iface, const struct in_addr *gw)
{
	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv4) {
		goto out;
	}

	net_ipaddr_copy(&iface->config.ip.ipv4->gw, gw);
out:
	net_if_unlock(iface);
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

	K_OOPS(k_usermode_from_copy(&gw_addr, (void *)gw, sizeof(gw_addr)));

	return z_impl_net_if_ipv4_set_gw_by_index(index, &gw_addr);
}

#include <zephyr/syscalls/net_if_ipv4_set_gw_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

static struct net_if_addr *ipv4_addr_find(struct net_if *iface,
					  struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used) {
			continue;
		}

		if (net_ipv4_addr_cmp(addr,
				      &ipv4->unicast[i].ipv4.address.in_addr)) {
			return &ipv4->unicast[i].ipv4;
		}
	}

	return NULL;
}

#if defined(CONFIG_NET_IPV4_ACD)
void net_if_ipv4_acd_succeeded(struct net_if *iface, struct net_if_addr *ifaddr)
{
	net_if_lock(iface);

	NET_DBG("ACD succeeded for %s at interface %d",
		net_sprint_ipv4_addr(&ifaddr->address.in_addr),
		ifaddr->ifindex);

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ACD_SUCCEED, iface,
					&ifaddr->address.in_addr,
					sizeof(struct in_addr));

	net_if_unlock(iface);
}

void net_if_ipv4_acd_failed(struct net_if *iface, struct net_if_addr *ifaddr)
{
	net_if_lock(iface);

	NET_DBG("ACD failed for %s at interface %d",
		net_sprint_ipv4_addr(&ifaddr->address.in_addr),
		ifaddr->ifindex);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ACD_FAILED, iface,
					&ifaddr->address.in_addr,
					sizeof(struct in_addr));

	net_if_ipv4_addr_rm(iface, &ifaddr->address.in_addr);

	net_if_unlock(iface);
}

void net_if_ipv4_start_acd(struct net_if *iface, struct net_if_addr *ifaddr)
{
	ifaddr->addr_state = NET_ADDR_TENTATIVE;

	if (net_if_is_up(iface)) {
		NET_DBG("Interface %p ll addr %s tentative IPv4 addr %s",
			iface,
			net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
					   net_if_get_link_addr(iface)->len),
			net_sprint_ipv4_addr(&ifaddr->address.in_addr));

		if (net_ipv4_acd_start(iface, ifaddr) != 0) {
			NET_DBG("Failed to start ACD for %s on iface %p.",
				net_sprint_ipv4_addr(&ifaddr->address.in_addr),
				iface);

			/* Just act as if no conflict was detected. */
			net_if_ipv4_acd_succeeded(iface, ifaddr);
		}
	} else {
		NET_DBG("Interface %p is down, starting ACD for %s later.",
			iface, net_sprint_ipv4_addr(&ifaddr->address.in_addr));
	}
}

void net_if_start_acd(struct net_if *iface)
{
	struct net_if_addr *ifaddr, *next;
	struct net_if_ipv4 *ipv4;
	sys_slist_t acd_needed;
	int ret;

	net_if_lock(iface);

	NET_DBG("Starting ACD for iface %p", iface);

	ret = net_if_config_ipv4_get(iface, &ipv4);
	if (ret < 0) {
		if (ret != -ENOTSUP) {
			NET_WARN("Cannot do ACD IPv4 config is not valid.");
		}

		goto out;
	}

	if (!ipv4) {
		goto out;
	}

	ipv4->conflict_cnt = 0;

	/* Start ACD for all the addresses that were added earlier when
	 * the interface was down.
	 */
	sys_slist_init(&acd_needed);

	/* Start ACD for all the addresses that were added earlier when
	 * the interface was down.
	 */
	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (!ipv4->unicast[i].ipv4.is_used ||
		    ipv4->unicast[i].ipv4.address.family != AF_INET ||
		    net_ipv4_is_addr_loopback(
			    &ipv4->unicast[i].ipv4.address.in_addr)) {
			continue;
		}

		sys_slist_prepend(&acd_needed, &ipv4->unicast[i].ipv4.acd_need_node);
	}

	net_if_unlock(iface);

	/* Start ACD for all the addresses without holding the iface lock
	 * to avoid any possible mutex deadlock issues.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&acd_needed,
					  ifaddr, next, acd_need_node) {
		net_if_ipv4_start_acd(iface, ifaddr);
	}

	return;

out:
	net_if_unlock(iface);
}
#else
#define net_if_ipv4_start_acd(...)
#define net_if_start_acd(...)
#endif /* CONFIG_NET_IPV4_ACD */

struct net_if_addr *net_if_ipv4_addr_add(struct net_if *iface,
					 struct in_addr *addr,
					 enum net_addr_type addr_type,
					 uint32_t vlifetime)
{
	uint32_t default_netmask = UINT32_MAX << (32 - CONFIG_NET_IPV4_DEFAULT_NETMASK);
	struct net_if_addr *ifaddr = NULL;
	struct net_if_addr_ipv4 *cur;
	struct net_if_ipv4 *ipv4;
	int idx;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, &ipv4) < 0) {
		goto out;
	}

	ifaddr = ipv4_addr_find(iface, addr);
	if (ifaddr) {
		/* TODO: should set addr_type/vlifetime */
		/* Address already exists, just return it but update ref count
		 * if it was not updated. This could happen if the address was
		 * added and then removed but for example an active connection
		 * was still using it. In this case we must update the ref count
		 * so that the address is not removed if the connection is closed.
		 */
		if (!ifaddr->is_added) {
			atomic_inc(&ifaddr->atomic_ref);
			ifaddr->is_added = true;
		}

		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		cur = &ipv4->unicast[i];

		if (addr_type == NET_ADDR_DHCP
		    && cur->ipv4.addr_type == NET_ADDR_OVERRIDABLE) {
			ifaddr = &cur->ipv4;
			idx = i;
			break;
		}

		if (!ipv4->unicast[i].ipv4.is_used) {
			ifaddr = &cur->ipv4;
			idx = i;
			break;
		}
	}

	if (ifaddr) {
		ifaddr->is_used = true;
		ifaddr->is_added = true;
		ifaddr->address.family = AF_INET;
		ifaddr->address.in_addr.s4_addr32[0] =
						addr->s4_addr32[0];
		ifaddr->addr_type = addr_type;
		ifaddr->atomic_ref = ATOMIC_INIT(1);

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

		NET_DBG("[%d] interface %d (%p) address %s type %s added",
			idx, net_if_get_by_iface(iface), iface,
			net_sprint_ipv4_addr(addr),
			net_addr_type2str(addr_type));

		if (IS_ENABLED(CONFIG_NET_IPV4_ACD) &&
		    !(l2_flags_get(iface) & NET_L2_POINT_TO_POINT) &&
		    !net_ipv4_is_addr_loopback(addr)) {
			/* ACD is started after the lock is released. */
			;
		} else {
			ifaddr->addr_state = NET_ADDR_PREFERRED;
		}

		cur->netmask.s_addr = htonl(default_netmask);

		net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ADDR_ADD, iface,
						&ifaddr->address.in_addr,
						sizeof(struct in_addr));

		net_if_unlock(iface);

		net_if_ipv4_start_acd(iface, ifaddr);

		return ifaddr;
	}

out:
	net_if_unlock(iface);

	return ifaddr;
}

bool net_if_ipv4_addr_rm(struct net_if *iface, const struct in_addr *addr)
{
	struct net_if_addr *ifaddr;
	struct net_if_ipv4 *ipv4;
	bool result = true;
	int ret;

	if (iface == NULL || addr == NULL) {
		return false;
	}

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		result = false;
		goto out;
	}

	ret = net_if_addr_unref(iface, AF_INET, addr, &ifaddr);
	if (ret > 0) {
		NET_DBG("Address %s still in use (ref %d)",
			net_sprint_ipv4_addr(addr), ret);
		result = false;
		ifaddr->is_added = false;
		goto out;
	} else if (ret < 0) {
		NET_DBG("Address %s not found (%d)",
			net_sprint_ipv4_addr(addr), ret);
	}

out:
	net_if_unlock(iface);

	return result;
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

	K_OOPS(k_usermode_from_copy(&addr_v4, (void *)addr, sizeof(addr_v4)));

	return z_impl_net_if_ipv4_addr_add_by_index(index,
						    &addr_v4,
						    addr_type,
						    vlifetime);
}

#include <zephyr/syscalls/net_if_ipv4_addr_add_by_index_mrsh.c>
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

	K_OOPS(k_usermode_from_copy(&addr_v4, (void *)addr, sizeof(addr_v4)));

	return (uint32_t)z_impl_net_if_ipv4_addr_rm_by_index(index, &addr_v4);
}

#include <zephyr/syscalls/net_if_ipv4_addr_rm_by_index_mrsh.c>
#endif /* CONFIG_USERSPACE */

void net_if_ipv4_addr_foreach(struct net_if *iface, net_if_ip_addr_cb_t cb,
			      void *user_data)
{
	struct net_if_ipv4 *ipv4;

	if (iface == NULL) {
		return;
	}

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (ipv4 == NULL) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		struct net_if_addr *if_addr = &ipv4->unicast[i].ipv4;

		if (!if_addr->is_used) {
			continue;
		}

		cb(iface, if_addr, user_data);
	}

out:
	net_if_unlock(iface);
}

static struct net_if_mcast_addr *ipv4_maddr_find(struct net_if *iface,
						 bool is_used,
						 const struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;

	if (!ipv4) {
		return NULL;
	}

	ARRAY_FOR_EACH(ipv4->mcast, i) {
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
	struct net_if_mcast_addr *maddr = NULL;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	if (!net_ipv4_is_addr_mcast(addr)) {
		NET_DBG("Address %s is not a multicast address.",
			net_sprint_ipv4_addr(addr));
		goto out;
	}

	maddr = ipv4_maddr_find(iface, false, NULL);
	if (maddr) {
		maddr->is_used = true;
		maddr->address.family = AF_INET;
		maddr->address.in_addr.s4_addr32[0] = addr->s4_addr32[0];

		NET_DBG("interface %d (%p) address %s added",
			net_if_get_by_iface(iface), iface,
			net_sprint_ipv4_addr(addr));

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV4_MADDR_ADD, iface,
			&maddr->address.in_addr,
			sizeof(struct in_addr));
	}

out:
	net_if_unlock(iface);

	return maddr;
}

bool net_if_ipv4_maddr_rm(struct net_if *iface, const struct in_addr *addr)
{
	struct net_if_mcast_addr *maddr;
	bool ret = false;

	net_if_lock(iface);

	maddr = ipv4_maddr_find(iface, true, addr);
	if (maddr) {
		maddr->is_used = false;

		NET_DBG("interface %d (%p) address %s removed",
			net_if_get_by_iface(iface), iface,
			net_sprint_ipv4_addr(addr));

		net_mgmt_event_notify_with_info(
			NET_EVENT_IPV4_MADDR_DEL, iface,
			&maddr->address.in_addr,
			sizeof(struct in_addr));

		ret = true;
	}

	net_if_unlock(iface);

	return ret;
}

void net_if_ipv4_maddr_foreach(struct net_if *iface, net_if_ip_maddr_cb_t cb,
			       void *user_data)
{
	struct net_if_ipv4 *ipv4;

	if (iface == NULL || cb == NULL) {
		return;
	}

	net_if_lock(iface);

	ipv4 = iface->config.ip.ipv4;
	if (!ipv4) {
		goto out;
	}

	for (int i = 0; i < NET_IF_MAX_IPV4_MADDR; i++) {
		if (!ipv4->mcast[i].is_used) {
			continue;
		}

		cb(iface, &ipv4->mcast[i], user_data);
	}

out:
	net_if_unlock(iface);
}

struct net_if_mcast_addr *net_if_ipv4_maddr_lookup(const struct in_addr *maddr,
						   struct net_if **ret)
{
	struct net_if_mcast_addr *addr = NULL;

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (ret && *ret && iface != *ret) {
			continue;
		}

		net_if_lock(iface);

		addr = ipv4_maddr_find(iface, true, maddr);
		if (addr) {
			if (ret) {
				*ret = iface;
			}

			net_if_unlock(iface);
			goto out;
		}

		net_if_unlock(iface);
	}

out:
	return addr;
}

void net_if_ipv4_maddr_leave(struct net_if *iface, struct net_if_mcast_addr *addr)
{
	if (iface == NULL || addr == NULL) {
		return;
	}

	net_if_lock(iface);
	addr->is_joined = false;
	net_if_unlock(iface);
}

void net_if_ipv4_maddr_join(struct net_if *iface, struct net_if_mcast_addr *addr)
{
	if (iface == NULL || addr == NULL) {
		return;
	}

	net_if_lock(iface);
	addr->is_joined = true;
	net_if_unlock(iface);
}

#if defined(CONFIG_NET_NATIVE_IPV4)
uint8_t net_if_ipv4_get_ttl(struct net_if *iface)
{
	int ret = 0;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv4) {
		goto out;
	}

	ret = iface->config.ip.ipv4->ttl;
out:
	net_if_unlock(iface);

	return ret;
}

void net_if_ipv4_set_ttl(struct net_if *iface, uint8_t ttl)
{
	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv4) {
		goto out;
	}

	iface->config.ip.ipv4->ttl = ttl;
out:
	net_if_unlock(iface);
}

uint8_t net_if_ipv4_get_mcast_ttl(struct net_if *iface)
{
	int ret = 0;

	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv4) {
		goto out;
	}

	ret = iface->config.ip.ipv4->mcast_ttl;
out:
	net_if_unlock(iface);

	return ret;
}

void net_if_ipv4_set_mcast_ttl(struct net_if *iface, uint8_t ttl)
{
	net_if_lock(iface);

	if (net_if_config_ipv4_get(iface, NULL) < 0) {
		goto out;
	}

	if (!iface->config.ip.ipv4) {
		goto out;
	}

	iface->config.ip.ipv4->mcast_ttl = ttl;
out:
	net_if_unlock(iface);
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


static void iface_ipv4_init(int if_count)
{
	int i;

	if (if_count > ARRAY_SIZE(ipv4_addresses)) {
		NET_WARN("You have %zu IPv4 net_if addresses but %d "
			 "network interfaces", ARRAY_SIZE(ipv4_addresses),
			 if_count);
		NET_WARN("Consider increasing CONFIG_NET_IF_MAX_IPV4_COUNT "
			 "value.");
	}

	for (i = 0; i < ARRAY_SIZE(ipv4_addresses); i++) {
		ipv4_addresses[i].ipv4.ttl = CONFIG_NET_INITIAL_TTL;
		ipv4_addresses[i].ipv4.mcast_ttl = CONFIG_NET_INITIAL_MCAST_TTL;
	}
}

static void leave_ipv4_mcast_all(struct net_if *iface)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;

	if (!ipv4) {
		return;
	}

	ARRAY_FOR_EACH(ipv4->mcast, i) {
		if (!ipv4->mcast[i].is_used ||
		    !ipv4->mcast[i].is_joined) {
			continue;
		}

		net_ipv4_igmp_leave(iface, &ipv4->mcast[i].address.in_addr);
	}
}

static void iface_ipv4_start(struct net_if *iface)
{
	if (!net_if_flag_is_set(iface, NET_IF_IPV4)) {
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_ACD)) {
		net_if_start_acd(iface);
	}
}

/* To be called when interface comes up so that all the non-joined multicast
 * groups are joined.
 */
static void rejoin_ipv4_mcast_groups(struct net_if *iface)
{
	struct net_if_mcast_addr *ifaddr, *next;
	struct net_if_ipv4 *ipv4;
	sys_slist_t rejoin_needed;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV4)) {
		goto out;
	}

	if (net_if_config_ipv4_get(iface, &ipv4) < 0) {
		goto out;
	}

	sys_slist_init(&rejoin_needed);

	/* Rejoin any mcast address present on the interface, but marked as not joined. */
	ARRAY_FOR_EACH(ipv4->mcast, i) {
		if (!ipv4->mcast[i].is_used ||
		    net_if_ipv4_maddr_is_joined(&ipv4->mcast[i])) {
			continue;
		}

		sys_slist_prepend(&rejoin_needed, &ipv4->mcast[i].rejoin_node);
	}

	net_if_unlock(iface);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&rejoin_needed, ifaddr, next, rejoin_node) {
		int ret;

		ret = net_ipv4_igmp_join(iface, &ifaddr->address.in_addr, NULL);
		if (ret < 0) {
			NET_ERR("Cannot join mcast address %s for %d (%d)",
				net_sprint_ipv4_addr(&ifaddr->address.in_addr),
				net_if_get_by_iface(iface), ret);
		} else {
			NET_DBG("Rejoined mcast address %s for %d",
				net_sprint_ipv4_addr(&ifaddr->address.in_addr),
				net_if_get_by_iface(iface));
		}
	}

	return;

out:
	net_if_unlock(iface);
}

/* To be called when interface comes operational down so that multicast
 * groups are rejoined when back up.
 */
static void clear_joined_ipv4_mcast_groups(struct net_if *iface)
{
	struct net_if_ipv4 *ipv4;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_IPV4)) {
		goto out;
	}

	if (net_if_config_ipv4_get(iface, &ipv4) < 0) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv4->mcast, i) {
		if (!ipv4->mcast[i].is_used) {
			continue;
		}

		net_if_ipv4_maddr_leave(iface, &ipv4->mcast[i]);
	}

out:
	net_if_unlock(iface);
}

#endif /* CONFIG_NET_NATIVE_IPV4 */
#else  /* CONFIG_NET_IPV4 */
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

#if !defined(CONFIG_NET_NATIVE_IPV4)
#define leave_ipv4_mcast_all(...)
#define clear_joined_ipv4_mcast_groups(...)
#define iface_ipv4_init(...)
#define iface_ipv4_start(...)
#endif /* !CONFIG_NET_NATIVE_IPV4 */

struct net_if *net_if_select_src_iface(const struct sockaddr *dst)
{
	struct net_if *iface = NULL;

	if (!dst) {
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && dst->sa_family == AF_INET6) {
		iface = net_if_ipv6_select_src_iface(&net_sin6(dst)->sin6_addr);
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && dst->sa_family == AF_INET) {
		iface = net_if_ipv4_select_src_iface(&net_sin(dst)->sin_addr);
		goto out;
	}

out:
	if (iface == NULL) {
		iface = net_if_get_default();
	}

	return iface;
}

static struct net_if_addr *get_ifaddr(struct net_if *iface,
				      sa_family_t family,
				      const void *addr,
				      unsigned int *mcast_addr_count)
{
	struct net_if_addr *ifaddr = NULL;

	net_if_lock(iface);

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		struct net_if_ipv6 *ipv6 =
			COND_CODE_1(CONFIG_NET_IPV6, (iface->config.ip.ipv6), (NULL));
		struct in6_addr maddr;
		unsigned int maddr_count = 0;
		int found = -1;

		if (ipv6 == NULL) {
			goto out;
		}

		net_ipv6_addr_create_solicited_node((struct in6_addr *)addr,
						    &maddr);

		ARRAY_FOR_EACH(ipv6->unicast, i) {
			struct in6_addr unicast_maddr;

			if (!ipv6->unicast[i].is_used) {
				continue;
			}

			/* Count how many times this solicited-node multicast address is identical
			 * for all the used unicast addresses
			 */
			net_ipv6_addr_create_solicited_node(
				&ipv6->unicast[i].address.in6_addr,
				&unicast_maddr);

			if (net_ipv6_addr_cmp(&maddr, &unicast_maddr)) {
				maddr_count++;
			}

			if (!net_ipv6_addr_cmp(&ipv6->unicast[i].address.in6_addr, addr)) {
				continue;
			}

			found = i;
		}

		if (found >= 0) {
			ifaddr = &ipv6->unicast[found];

			if (mcast_addr_count != NULL) {
				*mcast_addr_count = maddr_count;
			}
		}

		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		struct net_if_ipv4 *ipv4 =
			COND_CODE_1(CONFIG_NET_IPV4, (iface->config.ip.ipv4), (NULL));

		if (ipv4 == NULL) {
			goto out;
		}

		ARRAY_FOR_EACH(ipv4->unicast, i) {
			if (!ipv4->unicast[i].ipv4.is_used) {
				continue;
			}

			if (!net_ipv4_addr_cmp(&ipv4->unicast[i].ipv4.address.in_addr,
					       addr)) {
				continue;
			}

			ifaddr = &ipv4->unicast[i].ipv4;

			goto out;
		}
	}

out:
	net_if_unlock(iface);

	return ifaddr;
}

static void remove_ipv6_ifaddr(struct net_if *iface,
			       struct net_if_addr *ifaddr,
			       unsigned int maddr_count)
{
	struct net_if_ipv6 *ipv6;

	net_if_lock(iface);

	ipv6 = COND_CODE_1(CONFIG_NET_IPV6, (iface->config.ip.ipv6), (NULL));
	if (!ipv6) {
		goto out;
	}

	if (!ifaddr->is_infinite) {
		k_mutex_lock(&lock, K_FOREVER);

#if defined(CONFIG_NET_NATIVE_IPV6)
		sys_slist_find_and_remove(&active_address_lifetime_timers,
					  &ifaddr->lifetime.node);

		if (sys_slist_is_empty(&active_address_lifetime_timers)) {
			k_work_cancel_delayable(&address_lifetime_timer);
		}
#endif
		k_mutex_unlock(&lock);
	}

#if defined(CONFIG_NET_IPV6_DAD)
	if (!net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		k_mutex_lock(&lock, K_FOREVER);
		if (sys_slist_find_and_remove(&active_dad_timers,
					      &ifaddr->dad_node)) {
			/* Addreess with active DAD timer would still have
			 * stale entry in the neighbor cache.
			 */
			net_ipv6_nbr_rm(iface, &ifaddr->address.in6_addr);
		}
		k_mutex_unlock(&lock);
	}
#endif

	if (maddr_count == 1) {
		/* Remove the solicited-node multicast address only if no other
		 * unicast address is also using it
		 */
		struct in6_addr maddr;

		net_ipv6_addr_create_solicited_node(&ifaddr->address.in6_addr,
						    &maddr);
		net_if_ipv6_maddr_rm(iface, &maddr);
	}

	/* Using the IPv6 address pointer here can give false
	 * info if someone adds a new IP address into this position
	 * in the address array. This is quite unlikely thou.
	 */
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ADDR_DEL,
					iface,
					&ifaddr->address.in6_addr,
					sizeof(struct in6_addr));
out:
	net_if_unlock(iface);
}

static void remove_ipv4_ifaddr(struct net_if *iface,
			       struct net_if_addr *ifaddr)
{
	struct net_if_ipv4 *ipv4;

	net_if_lock(iface);

	ipv4 = COND_CODE_1(CONFIG_NET_IPV4, (iface->config.ip.ipv4), (NULL));
	if (!ipv4) {
		goto out;
	}

#if defined(CONFIG_NET_IPV4_ACD)
	net_ipv4_acd_cancel(iface, ifaddr);
#endif

	net_mgmt_event_notify_with_info(NET_EVENT_IPV4_ADDR_DEL,
					iface,
					&ifaddr->address.in_addr,
					sizeof(struct in_addr));
out:
	net_if_unlock(iface);
}

#if defined(CONFIG_NET_IF_LOG_LEVEL)
#define NET_LOG_LEVEL CONFIG_NET_IF_LOG_LEVEL
#else
#define NET_LOG_LEVEL 0
#endif

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
struct net_if_addr *net_if_addr_ref_debug(struct net_if *iface,
					  sa_family_t family,
					  const void *addr,
					  const char *caller,
					  int line)
#else
struct net_if_addr *net_if_addr_ref(struct net_if *iface,
				    sa_family_t family,
				    const void *addr)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	struct net_if_addr *ifaddr;
	atomic_val_t ref;

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	char addr_str[IS_ENABLED(CONFIG_NET_IPV6) ?
		      INET6_ADDRSTRLEN : INET_ADDRSTRLEN];

	__ASSERT(iface, "iface is NULL (%s():%d)", caller, line);
#endif

	ifaddr = get_ifaddr(iface, family, addr, NULL);

	do {
		ref = ifaddr ? atomic_get(&ifaddr->atomic_ref) : 0;
		if (!ref) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
			NET_ERR("iface %d addr %s (%s():%d)",
				net_if_get_by_iface(iface),
				net_addr_ntop(family,
					      addr,
					      addr_str, sizeof(addr_str)),
				caller, line);
#endif
			return NULL;
		}
	} while (!atomic_cas(&ifaddr->atomic_ref, ref, ref + 1));

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("[%d] ifaddr %s state %d ref %ld (%s():%d)",
		net_if_get_by_iface(iface),
		net_addr_ntop(ifaddr->address.family,
			      (void *)&ifaddr->address.in_addr,
			      addr_str, sizeof(addr_str)),
		ifaddr->addr_state,
		ref + 1,
		caller, line);
#endif

	return ifaddr;
}

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
int net_if_addr_unref_debug(struct net_if *iface,
			    sa_family_t family,
			    const void *addr,
			    struct net_if_addr **ret_ifaddr,
			    const char *caller, int line)
#else
int net_if_addr_unref(struct net_if *iface,
		      sa_family_t family,
		      const void *addr,
		      struct net_if_addr **ret_ifaddr)
#endif /* NET_LOG_LEVEL >= LOG_LEVEL_DBG */
{
	struct net_if_addr *ifaddr;
	unsigned int maddr_count = 0;
	atomic_val_t ref;

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	char addr_str[IS_ENABLED(CONFIG_NET_IPV6) ?
		      INET6_ADDRSTRLEN : INET_ADDRSTRLEN];

	__ASSERT(iface, "iface is NULL (%s():%d)", caller, line);
#endif

	ifaddr = get_ifaddr(iface, family, addr, &maddr_count);

	if (!ifaddr) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		NET_ERR("iface %d addr %s (%s():%d)",
			net_if_get_by_iface(iface),
			net_addr_ntop(family,
				      addr,
				      addr_str, sizeof(addr_str)),
			caller, line);
#endif
		return -EINVAL;
	}

	do {
		ref = atomic_get(&ifaddr->atomic_ref);
		if (!ref) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
			NET_ERR("*** ERROR *** iface %d ifaddr %p "
				"is freed already (%s():%d)",
				net_if_get_by_iface(iface),
				ifaddr,
				caller, line);
#endif
			return -EINVAL;
		}

	} while (!atomic_cas(&ifaddr->atomic_ref, ref, ref - 1));

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("[%d] ifaddr %s state %d ref %ld (%s():%d)",
		net_if_get_by_iface(iface),
		net_addr_ntop(ifaddr->address.family,
			      (void *)&ifaddr->address.in_addr,
			      addr_str, sizeof(addr_str)),
		ifaddr->addr_state,
		ref - 1, caller, line);
#endif

	if (ref > 1) {
		if (ret_ifaddr) {
			*ret_ifaddr = ifaddr;
		}

		return ref - 1;
	}

	ifaddr->is_used = false;

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6 && addr != NULL) {
		remove_ipv6_ifaddr(iface, ifaddr, maddr_count);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET && addr != NULL) {
		remove_ipv4_ifaddr(iface, ifaddr);
	}

	return 0;
}

enum net_verdict net_if_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE) &&
	    net_if_is_promisc(iface)) {
		struct net_pkt *new_pkt;

		new_pkt = net_pkt_clone(pkt, K_NO_WAIT);

		if (net_promisc_mode_input(new_pkt) == NET_DROP) {
			net_pkt_unref(new_pkt);
		}
	}

	return net_if_l2(iface)->recv(iface, pkt);
}

void net_if_register_link_cb(struct net_if_link_cb *link,
			     net_if_link_callback_t cb)
{
	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&link_callbacks, &link->node);
	sys_slist_prepend(&link_callbacks, &link->node);

	link->cb = cb;

	k_mutex_unlock(&lock);
}

void net_if_unregister_link_cb(struct net_if_link_cb *link)
{
	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&link_callbacks, &link->node);

	k_mutex_unlock(&lock);
}

void net_if_call_link_cb(struct net_if *iface, struct net_linkaddr *lladdr,
			 int status)
{
	struct net_if_link_cb *link, *tmp;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&link_callbacks, link, tmp, node) {
		link->cb(iface, lladdr, status);
	}

	k_mutex_unlock(&lock);
}

static bool need_calc_checksum(struct net_if *iface, enum ethernet_hw_caps caps,
			      enum net_if_checksum_type chksum_type)
{
#if defined(CONFIG_NET_L2_ETHERNET)
	struct ethernet_config config;
	enum ethernet_config_type config_type;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		/* For VLANs, figure out the main Ethernet interface and
		 * get the offloading capabilities from it.
		 */
		if (IS_ENABLED(CONFIG_NET_VLAN) && net_eth_is_vlan_interface(iface)) {
			iface = net_eth_get_vlan_main(iface);
			if (iface == NULL) {
				return true;
			}

			NET_ASSERT(net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET));
		} else {
			return true;
		}
	}

	if (!(net_eth_get_hw_capabilities(iface) & caps)) {
		return true; /* No checksum offload*/
	}

	if (caps == ETHERNET_HW_RX_CHKSUM_OFFLOAD) {
		config_type = ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT;
	} else {
		config_type = ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT;
	}

	if (net_eth_get_hw_config(iface, config_type, &config) != 0) {
		return false; /* No extra info, assume all offloaded. */
	}

	/* bitmaps are encoded such that this works */
	return !((config.chksum_support & chksum_type) == chksum_type);
#else
	ARG_UNUSED(iface);
	ARG_UNUSED(caps);

	return true;
#endif
}

bool net_if_need_calc_tx_checksum(struct net_if *iface, enum net_if_checksum_type chksum_type)
{
	return need_calc_checksum(iface, ETHERNET_HW_TX_CHKSUM_OFFLOAD, chksum_type);
}

bool net_if_need_calc_rx_checksum(struct net_if *iface, enum net_if_checksum_type chksum_type)
{
	return need_calc_checksum(iface, ETHERNET_HW_RX_CHKSUM_OFFLOAD, chksum_type);
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
	STRUCT_SECTION_FOREACH(net_if, iface) {
		cb(iface, user_data);
	}
}

bool net_if_is_offloaded(struct net_if *iface)
{
	return (IS_ENABLED(CONFIG_NET_OFFLOAD) &&
		net_if_is_ip_offloaded(iface)) ||
	       (IS_ENABLED(CONFIG_NET_SOCKETS_OFFLOAD) &&
		net_if_is_socket_offloaded(iface));
}

static void rejoin_multicast_groups(struct net_if *iface)
{
#if defined(CONFIG_NET_NATIVE_IPV6)
	rejoin_ipv6_mcast_groups(iface);
	if (l2_flags_get(iface) & NET_L2_MULTICAST) {
		join_mcast_allnodes(iface);
	}
#endif
#if defined(CONFIG_NET_NATIVE_IPV4)
	rejoin_ipv4_mcast_groups(iface);
#endif
#if !defined(CONFIG_NET_NATIVE_IPV6) && !defined(CONFIG_NET_NATIVE_IPV4)
	ARG_UNUSED(iface);
#endif
}

static void notify_iface_up(struct net_if *iface)
{
	/* In many places it's assumed that link address was set with
	 * net_if_set_link_addr(). Better check that now.
	 */
	if (IS_ENABLED(CONFIG_NET_L2_CANBUS_RAW) &&
	    IS_ENABLED(CONFIG_NET_SOCKETS_CAN) &&
	    (net_if_l2(iface) == &NET_L2_GET_NAME(CANBUS_RAW)))	{
		/* CAN does not require link address. */
	} else {
		if (!net_if_is_offloaded(iface)) {
			NET_ASSERT(net_if_get_link_addr(iface)->addr != NULL);
		}
	}

	net_if_flag_set(iface, NET_IF_RUNNING);
	net_mgmt_event_notify(NET_EVENT_IF_UP, iface);
	net_virtual_enable(iface);

	/* If the interface is only having point-to-point traffic then we do
	 * not need to run DAD etc for it.
	 */
	if (!net_if_is_offloaded(iface) &&
	    !(l2_flags_get(iface) & NET_L2_POINT_TO_POINT)) {
		/* Make sure that we update the IPv6 addresses and join the
		 * multicast groups.
		 */
		rejoin_multicast_groups(iface);
		iface_ipv6_start(iface);
		iface_ipv4_start(iface);
		net_ipv4_autoconf_start(iface);
	}
}

static void notify_iface_down(struct net_if *iface)
{
	net_if_flag_clear(iface, NET_IF_RUNNING);
	net_mgmt_event_notify(NET_EVENT_IF_DOWN, iface);
	net_virtual_disable(iface);

	if (!net_if_is_offloaded(iface) &&
	    !(l2_flags_get(iface) & NET_L2_POINT_TO_POINT)) {
		iface_ipv6_stop(iface);
		clear_joined_ipv6_mcast_groups(iface);
		clear_joined_ipv4_mcast_groups(iface);
		net_ipv4_autoconf_reset(iface);
	}
}

const char *net_if_oper_state2str(enum net_if_oper_state state)
{
	switch (state) {
	case NET_IF_OPER_UNKNOWN:
		return "UNKNOWN";
	case NET_IF_OPER_NOTPRESENT:
		return "NOTPRESENT";
	case NET_IF_OPER_DOWN:
		return "DOWN";
	case NET_IF_OPER_LOWERLAYERDOWN:
		return "LOWERLAYERDOWN";
	case NET_IF_OPER_TESTING:
		return "TESTING";
	case NET_IF_OPER_DORMANT:
		return "DORMANT";
	case NET_IF_OPER_UP:
		return "UP";
	default:
		break;
	}

	return "<invalid>";
}

static void update_operational_state(struct net_if *iface)
{
	enum net_if_oper_state prev_state = iface->if_dev->oper_state;
	enum net_if_oper_state new_state = NET_IF_OPER_UNKNOWN;

	if (!net_if_is_admin_up(iface)) {
		new_state = NET_IF_OPER_DOWN;
		goto exit;
	}

	if (!device_is_ready(net_if_get_device(iface))) {
		new_state = NET_IF_OPER_LOWERLAYERDOWN;
		goto exit;
	}

	if (!net_if_is_carrier_ok(iface)) {
#if defined(CONFIG_NET_L2_VIRTUAL)
		if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
			new_state = NET_IF_OPER_LOWERLAYERDOWN;
		} else
#endif /* CONFIG_NET_L2_VIRTUAL */
		{
			new_state = NET_IF_OPER_DOWN;
		}

		goto exit;
	}

	if (net_if_is_dormant(iface)) {
		new_state = NET_IF_OPER_DORMANT;
		goto exit;
	}

	new_state = NET_IF_OPER_UP;

exit:
	if (net_if_oper_state_set(iface, new_state) != new_state) {
		NET_ERR("Failed to update oper state to %d", new_state);
		return;
	}

	NET_DBG("iface %d (%p), oper state %s admin %s carrier %s dormant %s",
		net_if_get_by_iface(iface), iface,
		net_if_oper_state2str(net_if_oper_state(iface)),
		net_if_is_admin_up(iface) ? "UP" : "DOWN",
		net_if_is_carrier_ok(iface) ? "ON" : "OFF",
		net_if_is_dormant(iface) ? "ON" : "OFF");

	if (net_if_oper_state(iface) == NET_IF_OPER_UP) {
		if (prev_state != NET_IF_OPER_UP) {
			notify_iface_up(iface);
		}
	} else {
		if (prev_state == NET_IF_OPER_UP) {
			notify_iface_down(iface);
		}
	}
}

static void init_igmp(struct net_if *iface)
{
#if defined(CONFIG_NET_IPV4_IGMP)
	/* Ensure IPv4 is enabled for this interface. */
	if (net_if_config_ipv4_get(iface, NULL)) {
		return;
	}

	net_ipv4_igmp_init(iface);
#else
	ARG_UNUSED(iface);
	return;
#endif
}

int net_if_up(struct net_if *iface)
{
	int status = 0;

	NET_DBG("iface %d (%p)", net_if_get_by_iface(iface), iface);

	net_if_lock(iface);

	if (net_if_flag_is_set(iface, NET_IF_UP)) {
		status = -EALREADY;
		goto out;
	}

	/* If the L2 does not support enable just set the flag */
	if (!net_if_l2(iface) || !net_if_l2(iface)->enable) {
		goto done;
	} else {
		/* If the L2 does not implement enable(), then the network
		 * device driver cannot implement start(), in which case
		 * we can do simple check here and not try to bring interface
		 * up as the device is not ready.
		 *
		 * If the network device driver does implement start(), then
		 * it could bring the interface up when the enable() is called
		 * few lines below.
		 */
		const struct device *dev;

		dev = net_if_get_device(iface);
		NET_ASSERT(dev);

		/* If the device is not ready it is pointless trying to take it up. */
		if (!device_is_ready(dev)) {
			NET_DBG("Device %s (%p) is not ready", dev->name, dev);
			status = -ENXIO;
			goto out;
		}
	}

	/* Notify L2 to enable the interface. Note that the interface is still down
	 * at this point from network interface point of view i.e., the NET_IF_UP
	 * flag has not been set yet.
	 */
	status = net_if_l2(iface)->enable(iface, true);
	if (status < 0) {
		NET_DBG("Cannot take interface %d up (%d)",
			net_if_get_by_iface(iface), status);
		goto out;
	}

	init_igmp(iface);

done:
	net_if_flag_set(iface, NET_IF_UP);
	net_mgmt_event_notify(NET_EVENT_IF_ADMIN_UP, iface);
	update_operational_state(iface);

out:
	net_if_unlock(iface);

	return status;
}

int net_if_down(struct net_if *iface)
{
	int status = 0;

	NET_DBG("iface %p", iface);

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_UP)) {
		status = -EALREADY;
		goto out;
	}

	leave_mcast_all(iface);
	leave_ipv4_mcast_all(iface);

	/* If the L2 does not support enable just clear the flag */
	if (!net_if_l2(iface) || !net_if_l2(iface)->enable) {
		goto done;
	}

	/* Notify L2 to disable the interface */
	status = net_if_l2(iface)->enable(iface, false);
	if (status < 0) {
		goto out;
	}

done:
	net_if_flag_clear(iface, NET_IF_UP);
	net_mgmt_event_notify(NET_EVENT_IF_ADMIN_DOWN, iface);
	update_operational_state(iface);

out:
	net_if_unlock(iface);

	return status;
}

void net_if_carrier_on(struct net_if *iface)
{
	if (iface == NULL) {
		return;
	}

	net_if_lock(iface);

	if (!net_if_flag_test_and_set(iface, NET_IF_LOWER_UP)) {
		update_operational_state(iface);
	}

	net_if_unlock(iface);
}

void net_if_carrier_off(struct net_if *iface)
{
	if (iface == NULL) {
		return;
	}

	net_if_lock(iface);

	if (net_if_flag_test_and_clear(iface, NET_IF_LOWER_UP)) {
		update_operational_state(iface);
	}

	net_if_unlock(iface);
}

void net_if_dormant_on(struct net_if *iface)
{
	if (iface == NULL) {
		return;
	}

	net_if_lock(iface);

	if (!net_if_flag_test_and_set(iface, NET_IF_DORMANT)) {
		update_operational_state(iface);
	}

	net_if_unlock(iface);
}

void net_if_dormant_off(struct net_if *iface)
{
	if (iface == NULL) {
		return;
	}

	net_if_lock(iface);

	if (net_if_flag_test_and_clear(iface, NET_IF_DORMANT)) {
		update_operational_state(iface);
	}

	net_if_unlock(iface);
}

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
static int promisc_mode_set(struct net_if *iface, bool enable)
{
	enum net_l2_flags l2_flags = 0;

	if (iface == NULL) {
		return -EINVAL;
	}

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
	ARG_UNUSED(enable);

	return -ENOTSUP;
#endif

	return 0;
}

int net_if_set_promisc(struct net_if *iface)
{
	int ret;

	net_if_lock(iface);

	ret = promisc_mode_set(iface, true);
	if (ret < 0 && ret != -EALREADY) {
		goto out;
	}

	ret = net_if_flag_test_and_set(iface, NET_IF_PROMISC);
	if (ret) {
		ret = -EALREADY;
		goto out;
	}

out:
	net_if_unlock(iface);

	return ret;
}

void net_if_unset_promisc(struct net_if *iface)
{
	int ret;

	net_if_lock(iface);

	ret = promisc_mode_set(iface, false);
	if (ret < 0) {
		goto out;
	}

	net_if_flag_clear(iface, NET_IF_PROMISC);

out:
	net_if_unlock(iface);
}

bool net_if_is_promisc(struct net_if *iface)
{
	if (iface == NULL) {
		return false;
	}

	return net_if_flag_is_set(iface, NET_IF_PROMISC);
}
#endif /* CONFIG_NET_PROMISCUOUS_MODE */

#ifdef CONFIG_NET_POWER_MANAGEMENT

int net_if_suspend(struct net_if *iface)
{
	int ret = 0;

	net_if_lock(iface);

	if (net_if_are_pending_tx_packets(iface)) {
		ret = -EBUSY;
		goto out;
	}

	if (net_if_flag_test_and_set(iface, NET_IF_SUSPENDED)) {
		ret = -EALREADY;
		goto out;
	}

	net_stats_add_suspend_start_time(iface, k_cycle_get_32());

out:
	net_if_unlock(iface);

	return ret;
}

int net_if_resume(struct net_if *iface)
{
	int ret = 0;

	net_if_lock(iface);

	if (!net_if_flag_is_set(iface, NET_IF_SUSPENDED)) {
		ret = -EALREADY;
		goto out;
	}

	net_if_flag_clear(iface, NET_IF_SUSPENDED);

	net_stats_add_suspend_end_time(iface, k_cycle_get_32());

out:
	net_if_unlock(iface);

	return ret;
}

bool net_if_is_suspended(struct net_if *iface)
{
	return net_if_flag_is_set(iface, NET_IF_SUSPENDED);
}

#endif /* CONFIG_NET_POWER_MANAGEMENT */

#if defined(CONFIG_NET_PKT_TIMESTAMP_THREAD)
static void net_tx_ts_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct net_pkt *pkt;

	NET_DBG("Starting TX timestamp callback thread");

	while (1) {
		pkt = k_fifo_get(&tx_ts_queue, K_FOREVER);
		if (pkt) {
			net_if_call_timestamp_cb(pkt);
		}
		net_pkt_unref(pkt);
	}
}

void net_if_register_timestamp_cb(struct net_if_timestamp_cb *handle,
				  struct net_pkt *pkt,
				  struct net_if *iface,
				  net_if_timestamp_callback_t cb)
{
	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&timestamp_callbacks, &handle->node);
	sys_slist_prepend(&timestamp_callbacks, &handle->node);

	handle->iface = iface;
	handle->cb = cb;
	handle->pkt = pkt;

	k_mutex_unlock(&lock);
}

void net_if_unregister_timestamp_cb(struct net_if_timestamp_cb *handle)
{
	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&timestamp_callbacks, &handle->node);

	k_mutex_unlock(&lock);
}

void net_if_call_timestamp_cb(struct net_pkt *pkt)
{
	sys_snode_t *sn, *sns;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&timestamp_callbacks, sn, sns) {
		struct net_if_timestamp_cb *handle =
			CONTAINER_OF(sn, struct net_if_timestamp_cb, node);

		if (((handle->iface == NULL) ||
		     (handle->iface == net_pkt_iface(pkt))) &&
		    (handle->pkt == NULL || handle->pkt == pkt)) {
			handle->cb(pkt);
		}
	}

	k_mutex_unlock(&lock);
}

void net_if_add_tx_timestamp(struct net_pkt *pkt)
{
	k_fifo_put(&tx_ts_queue, pkt);
	net_pkt_ref(pkt);
}
#endif /* CONFIG_NET_PKT_TIMESTAMP_THREAD */

bool net_if_is_wifi(struct net_if *iface)
{
	if (net_if_is_offloaded(iface)) {
		return net_off_is_wifi_offloaded(iface);
	}

	if (IS_ENABLED(CONFIG_NET_L2_ETHERNET)) {
		return net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) &&
			net_eth_type_is_wifi(iface);
	}

	return false;
}

struct net_if *net_if_get_first_wifi(void)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_is_wifi(iface)) {
			return iface;
		}
	}
	return NULL;
}

struct net_if *net_if_get_wifi_sta(void)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_is_wifi(iface)
#ifdef CONFIG_WIFI_NM
			&& wifi_nm_iface_is_sta(iface)
#endif
			) {
			return iface;
		}
	}

	/* If no STA interface is found, return the first WiFi interface */
	return net_if_get_first_wifi();
}

struct net_if *net_if_get_wifi_sap(void)
{
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_is_wifi(iface)
#ifdef CONFIG_WIFI_NM
			&& wifi_nm_iface_is_sap(iface)
#endif
			) {
			return iface;
		}
	}

	/* If no STA interface is found, return the first WiFi interface */
	return net_if_get_first_wifi();
}

int net_if_get_name(struct net_if *iface, char *buf, int len)
{
#if defined(CONFIG_NET_INTERFACE_NAME)
	int name_len;

	if (iface == NULL || buf == NULL || len <= 0) {
		return -EINVAL;
	}

	name_len = strlen(net_if_get_config(iface)->name);
	if (name_len >= len) {
		return -ERANGE;
	}

	/* Copy string and null terminator */
	memcpy(buf, net_if_get_config(iface)->name, name_len + 1);

	return name_len;
#else
	return -ENOTSUP;
#endif
}

int net_if_set_name(struct net_if *iface, const char *buf)
{
#if defined(CONFIG_NET_INTERFACE_NAME)
	int name_len;

	if (iface == NULL || buf == NULL) {
		return -EINVAL;
	}

	name_len = strlen(buf);
	if (name_len >= sizeof(iface->config.name)) {
		return -ENAMETOOLONG;
	}

	STRUCT_SECTION_FOREACH(net_if, iface_check) {
		if (iface_check == iface) {
			continue;
		}

		if (memcmp(net_if_get_config(iface_check)->name,
			   buf,
			   name_len + 1) == 0) {
			return -EALREADY;
		}
	}

	/* Copy string and null terminator */
	memcpy(net_if_get_config(iface)->name, buf, name_len + 1);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int net_if_get_by_name(const char *name)
{
#if defined(CONFIG_NET_INTERFACE_NAME)
	if (name == NULL) {
		return -EINVAL;
	}

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (strncmp(net_if_get_config(iface)->name, name, strlen(name)) == 0) {
			return net_if_get_by_iface(iface);
		}
	}

	return -ENOENT;
#else
	return -ENOTSUP;
#endif
}

#if defined(CONFIG_NET_INTERFACE_NAME)
static void set_default_name(struct net_if *iface)
{
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	int ret;

	if (net_if_is_wifi(iface)) {
		static int count;

		snprintk(name, sizeof(name), "wlan%d", count++);

	} else if (IS_ENABLED(CONFIG_NET_L2_ETHERNET) &&
		   (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET))) {
		static int count;

		snprintk(name, sizeof(name), "eth%d", count++);
	} else if (IS_ENABLED(CONFIG_NET_L2_IEEE802154) &&
		   (net_if_l2(iface) == &NET_L2_GET_NAME(IEEE802154))) {
		static int count;

		snprintk(name, sizeof(name), "ieee%d", count++);
	} else if (IS_ENABLED(CONFIG_NET_L2_DUMMY) &&
		   (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY))) {
		static int count;

		snprintk(name, sizeof(name), "dummy%d", count++);
	} else if (IS_ENABLED(CONFIG_NET_L2_CANBUS_RAW) &&
		   (net_if_l2(iface) == &NET_L2_GET_NAME(CANBUS_RAW))) {
		static int count;

		snprintk(name, sizeof(name), "can%d", count++);
	} else if (IS_ENABLED(CONFIG_NET_L2_PPP) &&
		   (net_if_l2(iface) == &NET_L2_GET_NAME(PPP))) {
		static int count;

		snprintk(name, sizeof(name) - 1, "ppp%d", count++);
	} else if (IS_ENABLED(CONFIG_NET_L2_OPENTHREAD) &&
		   (net_if_l2(iface) == &NET_L2_GET_NAME(OPENTHREAD))) {
		static int count;

		snprintk(name, sizeof(name), "thread%d", count++);
	} else {
		static int count;

		snprintk(name, sizeof(name), "net%d", count++);
	}

	ret = net_if_set_name(iface, name);
	if (ret < 0) {
		NET_WARN("Cannot set default name for interface %d (%p) (%d)",
			 net_if_get_by_iface(iface), iface, ret);
	}
}
#endif /* CONFIG_NET_INTERFACE_NAME */

void net_if_init(void)
{
	int if_count = 0;

	NET_DBG("");

	k_mutex_lock(&lock, K_FOREVER);

	net_tc_tx_init();

	STRUCT_SECTION_FOREACH(net_if, iface) {
#if defined(CONFIG_NET_INTERFACE_NAME)
		memset(net_if_get_config(iface)->name, 0,
		       sizeof(iface->config.name));
#endif

		init_iface(iface);

#if defined(CONFIG_NET_INTERFACE_NAME)
		/* If the driver did not set the name, then set
		 * a default name for the network interface.
		 */
		if (net_if_get_config(iface)->name[0] == '\0') {
			set_default_name(iface);
		}
#endif

		net_stats_prometheus_init(iface);

		if_count++;
	}

	if (if_count == 0) {
		NET_ERR("There is no network interface to work with!");
		goto out;
	}

#if defined(CONFIG_ASSERT)
	/* Do extra check that verifies that interface count is properly
	 * done.
	 */
	int count_if;

	NET_IFACE_COUNT(&count_if);
	NET_ASSERT(count_if == if_count);
#endif

	iface_ipv6_init(if_count);
	iface_ipv4_init(if_count);
	iface_router_init();

#if defined(CONFIG_NET_PKT_TIMESTAMP_THREAD)
	k_thread_create(&tx_thread_ts, tx_ts_stack,
			K_KERNEL_STACK_SIZEOF(tx_ts_stack),
			net_tx_ts_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_thread_name_set(&tx_thread_ts, "tx_tstamp");
#endif /* CONFIG_NET_PKT_TIMESTAMP_THREAD */

out:
	k_mutex_unlock(&lock);
}

void net_if_post_init(void)
{
	NET_DBG("");

	/* After TX is running, attempt to bring the interface up */
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (!net_if_flag_is_set(iface, NET_IF_NO_AUTO_START)) {
			net_if_up(iface);
		}
	}
}
