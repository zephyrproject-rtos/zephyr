/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_IF)
#define SYS_LOG_DOMAIN "net/if"
#define NET_LOG_ENABLED 1
#endif

#include <init.h>
#include <kernel.h>
#include <linker/sections.h>
#include <stdlib.h>
#include <string.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/ethernet.h>

#include "net_private.h"
#include "ipv6.h"
#include "rpl.h"
#include "ipv4_autoconf_internal.h"

#include "net_stats.h"

#define REACHABLE_TIME K_SECONDS(30) /* in ms */
/*
 * split the min/max random reachable factors into numerator/denominator
 * so that integer-based math works better
 */
#define MIN_RANDOM_NUMER (1)
#define MIN_RANDOM_DENOM (2)
#define MAX_RANDOM_NUMER (3)
#define MAX_RANDOM_DENOM (2)

/* net_if dedicated section limiters */
extern struct net_if __net_if_start[];
extern struct net_if __net_if_end[];

extern struct net_if_dev __net_if_dev_start[];
extern struct net_if_dev __net_if_dev_end[];

static struct net_if_router routers[CONFIG_NET_MAX_ROUTERS];

#if defined(CONFIG_NET_IPV6)
/* Timer that triggers network address renewal */
static struct k_delayed_work address_lifetime_timer;

/* Track currently active address lifetime timers */
static sys_slist_t active_address_lifetime_timers;

/* Timer that triggers IPv6 prefix lifetime */
static struct k_delayed_work prefix_lifetime_timer;

/* Track currently active IPv6 prefix lifetime timers */
static sys_slist_t active_prefix_lifetime_timers;

static struct {
	struct net_if_ipv6 ipv6;
	struct net_if *iface;
} ipv6_addresses[CONFIG_NET_IF_MAX_IPV6_COUNT];
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static struct {
	struct net_if_ipv4 ipv4;
	struct net_if *iface;
} ipv4_addresses[CONFIG_NET_IF_MAX_IPV4_COUNT];
#endif /* CONFIG_NET_IPV4 */

/* We keep track of the link callbacks in this list.
 */
static sys_slist_t link_callbacks;

#if defined(CONFIG_NET_IPV6)
/* Multicast join/leave tracking.
 */
static sys_slist_t mcast_monitor_callbacks;
#endif

#if defined(CONFIG_NET_PKT_TIMESTAMP)
#if !defined(CONFIG_NET_PKT_TIMESTAMP_STACK_SIZE)
#define CONFIG_NET_PKT_TIMESTAMP_STACK_SIZE 1024
#endif

NET_STACK_DEFINE(TIMESTAMP, tx_ts_stack,
		 CONFIG_NET_PKT_TIMESTAMP_STACK_SIZE,
		 CONFIG_NET_PKT_TIMESTAMP_STACK_SIZE);
K_FIFO_DEFINE(tx_ts_queue);

static struct k_thread tx_thread_ts;

/* We keep track of the timestamp callbacks in this list.
 */
static sys_slist_t timestamp_callbacks;
#endif /* CONFIG_NET_PKT_TIMESTAMP */

#if defined(CONFIG_NET_DEBUG_IF)
#if defined(CONFIG_NET_STATISTICS)
#define debug_check_packet(pkt)						\
	do {								\
		NET_DBG("Processing (pkt %p, data len %d, "		\
			"prio %d) network packet",			\
			pkt, pkt->total_pkt_len,			\
			net_pkt_priority(pkt));				\
									\
		NET_ASSERT(pkt->frags && pkt->total_pkt_len);		\
	} while (0)
#else /* CONFIG_NET_STATISTICS */
#define debug_check_packet(pkt)						\
	do {								\
		NET_DBG("Processing (pkt %p, prio %d) network packet",	\
			pkt, net_pkt_priority(pkt));			\
									\
		NET_ASSERT(pkt->frags);					\
	} while (0)
#endif /* CONFIG_NET_STATISTICS */
#else
#define debug_check_packet(...)
#endif /* CONFIG_NET_DEBUG_IF */

static inline void net_context_send_cb(struct net_context *context,
				       void *token, int status)
{
	if (!context) {
		return;
	}

	if (context->send_cb) {
		context->send_cb(context, status, token, context->user_data);
	}

#if defined(CONFIG_NET_UDP)
	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		net_stats_update_udp_sent(net_context_get_iface(context));
	} else
#endif
#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		net_stats_update_tcp_seg_sent(net_context_get_iface(context));
	} else
#endif
	{
	}
}

static bool net_if_tx(struct net_if *iface, struct net_pkt *pkt)
{
	const struct net_if_api *api = net_if_get_device(iface)->driver_api;
	struct net_linkaddr *dst;
	struct net_context *context;
	void *context_token;
	int status;

	if (!pkt) {
		return false;
	}

	debug_check_packet(pkt);

	dst = net_pkt_lladdr_dst(pkt);
	context = net_pkt_context(pkt);
	context_token = net_pkt_token(pkt);

	if (atomic_test_bit(iface->if_dev->flags, NET_IF_UP)) {
		if (IS_ENABLED(CONFIG_NET_TCP)) {
			net_pkt_set_sent(pkt, true);
			net_pkt_set_queued(pkt, false);
		}

		status = api->send(iface, pkt);
	} else {
		/* Drop packet if interface is not up */
		NET_WARN("iface %p is down", iface);
		status = -ENETDOWN;
	}

	if (status < 0) {
		if (IS_ENABLED(CONFIG_NET_TCP)) {
			net_pkt_set_sent(pkt, false);
		}

		net_pkt_unref(pkt);
	} else {
		net_stats_update_bytes_sent(iface, pkt->total_pkt_len);
	}

	if (context) {
		NET_DBG("Calling context send cb %p token %p status %d",
			context, context_token, status);

		net_context_send_cb(context, context_token, status);
	}

	if (dst->addr) {
		net_if_call_link_cb(iface, dst, status);
	}

	return true;
}

static void process_tx_packet(struct k_work *work)
{
	struct net_pkt *pkt;

	pkt = CONTAINER_OF(work, struct net_pkt, work);

	net_if_tx(net_pkt_iface(pkt), pkt);
}

void net_if_queue_tx(struct net_if *iface, struct net_pkt *pkt)
{
	u8_t prio = net_pkt_priority(pkt);
	u8_t tc = net_tx_priority2tc(prio);

	k_work_init(net_pkt_work(pkt), process_tx_packet);

#if defined(CONFIG_NET_STATISTICS)
	pkt->total_pkt_len = net_pkt_get_len(pkt);

	net_stats_update_tc_sent_pkt(iface, tc);
	net_stats_update_tc_sent_bytes(iface, tc, pkt->total_pkt_len);
	net_stats_update_tc_sent_priority(iface, tc, prio);
#endif

#if NET_TC_TX_COUNT > 1
	NET_DBG("TC %d with prio %d pkt %p", tc, prio, pkt);
#endif

	net_tc_submit_to_tx_queue(tc, pkt);
}

static inline void init_iface(struct net_if *iface)
{
	const struct net_if_api *api = net_if_get_device(iface)->driver_api;

	NET_ASSERT(api && api->init && api->send);

	NET_DBG("On iface %p", iface);

	api->init(iface);
}

enum net_verdict net_if_send_data(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_context *context = net_pkt_context(pkt);
	struct net_linkaddr *dst = net_pkt_lladdr_dst(pkt);
	void *token = net_pkt_token(pkt);
	enum net_verdict verdict;
	int status = -EIO;

	if (!atomic_test_bit(iface->if_dev->flags, NET_IF_UP)) {
		/* Drop packet if interface is not up */
		NET_WARN("iface %p is down", iface);
		verdict = NET_DROP;
		status = -ENETDOWN;
		goto done;
	}

	/* If the ll address is not set at all, then we must set
	 * it here.
	 * Workaround Linux bug, see:
	 * https://github.com/zephyrproject-rtos/zephyr/issues/3111
	 */
	if (!atomic_test_bit(iface->if_dev->flags, NET_IF_POINTOPOINT) &&
	    !net_pkt_lladdr_src(pkt)->addr) {
		net_pkt_lladdr_src(pkt)->addr = net_pkt_lladdr_if(pkt)->addr;
		net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;
	}

#if defined(CONFIG_NET_LOOPBACK)
	/* If the packet is destined back to us, then there is no need to do
	 * additional checks, so let the packet through.
	 */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		goto send;
	}
#endif

#if defined(CONFIG_NET_IPV6)
	/* If the ll dst address is not set check if it is present in the nbr
	 * cache.
	 */
	if (net_pkt_family(pkt) == AF_INET6) {
		pkt = net_ipv6_prepare_for_send(pkt);
		if (!pkt) {
			verdict = NET_CONTINUE;
			goto done;
		}
	}
#endif

#if defined(CONFIG_NET_LOOPBACK)
send:
#endif
	verdict = net_if_l2(iface)->send(iface, pkt);

done:
	/* The L2 send() function can return
	 *   NET_OK in which case packet was sent successfully. In this case
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
	if (context && verdict == NET_DROP) {
		NET_DBG("Calling context send cb %p token %p verdict %d",
			context, token, verdict);

		net_context_send_cb(context, token, status);
	}

	if (verdict == NET_DROP && dst->addr) {
		net_if_call_link_cb(iface, dst, status);
	}

	return verdict;
}

struct net_if *net_if_get_by_link_addr(struct net_linkaddr *ll_addr)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		if (!memcmp(net_if_get_link_addr(iface)->addr, ll_addr->addr,
			    ll_addr->len)) {
			return iface;
		}
	}

	return NULL;
}

struct net_if *net_if_lookup_by_dev(struct device *dev)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		if (net_if_get_device(iface) == dev) {
			return iface;
		}
	}

	return NULL;
}

struct net_if *net_if_get_default(void)
{
	struct net_if *iface = NULL;

	if (__net_if_start == __net_if_end) {
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

	return iface ? iface : __net_if_start;
}

struct net_if *net_if_get_first_by_type(const struct net_l2 *l2)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
#if defined(CONFIG_NET_OFFLOAD)
		if (!l2 && iface->if_dev->offload) {
			return iface;
		}
#endif

		if (net_if_l2(iface) == l2) {
			return iface;
		}
	}

	return NULL;
}

/* Return how many bits are shared between two IP addresses */
static u8_t get_ipaddr_diff(const u8_t *src, const u8_t *dst, int addr_len)
{
	u8_t j, k, xor;
	u8_t len = 0;

	for (j = 0; j < addr_len; j++) {
		if (src[j] == dst[j]) {
			len += 8;
		} else {
			xor = src[j] ^ dst[j];
			for (k = 0; k < 8; k++) {
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

#if defined(CONFIG_NET_IPV6)
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
			net_sprint_ipv6_addr(&addr), ret);
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
			net_sprint_ipv6_addr(&addr), ret);
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
#else
#define join_mcast_allnodes(...)
#define join_mcast_solicit_node(...)
#define leave_mcast_all(...)
#endif /* CONFIG_NET_IPV6_MLD */

#if defined(CONFIG_NET_IPV6_DAD)
#define DAD_TIMEOUT K_MSEC(100)

static void dad_timeout(struct k_work *work)
{
	/* This means that the DAD succeed. */
	struct net_if_addr *tmp, *ifaddr = CONTAINER_OF(work,
							struct net_if_addr,
							dad_timer);
	struct net_if *iface = NULL;

	NET_DBG("DAD succeeded for %s",
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr));

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	/* Because we do not know the interface at this point, we need to
	 * lookup for it.
	 */
	tmp = net_if_ipv6_addr_lookup(&ifaddr->address.in6_addr, &iface);
	if (tmp == ifaddr) {
		net_mgmt_event_notify(NET_EVENT_IPV6_DAD_SUCCEED, iface);

		/* The address gets added to neighbor cache which is not needed
		 * in this case as the address is our own one.
		 */
		net_ipv6_nbr_rm(iface, &ifaddr->address.in6_addr);
	}
}

static void net_if_ipv6_start_dad(struct net_if *iface,
				  struct net_if_addr *ifaddr)
{
	ifaddr->addr_state = NET_ADDR_TENTATIVE;

	if (net_if_is_up(iface)) {
		NET_DBG("Interface %p ll addr %s tentative IPv6 addr %s",
			iface,
			net_sprint_ll_addr(net_if_get_link_addr(iface)->addr,
					   net_if_get_link_addr(iface)->len),
			net_sprint_ipv6_addr(&ifaddr->address.in6_addr));

		ifaddr->dad_count = 1;

		if (!net_ipv6_start_dad(iface, ifaddr)) {
			k_delayed_work_submit(&ifaddr->dad_timer, DAD_TIMEOUT);
		}
	} else {
		NET_DBG("Interface %p is down, starting DAD for %s later.",
			iface,
			net_sprint_ipv6_addr(&ifaddr->address.in6_addr));
	}
}

void net_if_start_dad(struct net_if *iface)
{
	struct net_if_addr *ifaddr;
	struct net_if_ipv6 *ipv6;
	struct in6_addr addr = { };
	int i;

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
			net_sprint_ipv6_addr(&addr), iface);
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
			net_sprint_ipv6_addr(addr), iface);
		return;
	}

	k_delayed_work_cancel(&ifaddr->dad_timer);

	net_mgmt_event_notify(NET_EVENT_IPV6_DAD_FAILED, iface);

	net_if_ipv6_addr_rm(iface, addr);
}
#else
static inline void net_if_ipv6_start_dad(struct net_if *iface,
					 struct net_if_addr *ifaddr)
{
	ifaddr->addr_state = NET_ADDR_PREFERRED;
}
#endif /* CONFIG_NET_IPV6_DAD */

#if defined(CONFIG_NET_IPV6_ND)
#define RS_TIMEOUT K_SECONDS(1)
#define RS_COUNT 3

static void rs_timeout(struct k_work *work)
{
	/* Did not receive RA yet. */
	struct net_if_ipv6 *ipv6 = CONTAINER_OF(work,
						struct net_if_ipv6,
						rs_timer);
	struct net_if *iface;

	ipv6->rs_count++;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		if (iface->config.ip.ipv6 == ipv6) {
			goto found;
		}
	}

	NET_DBG("Interface IPv6 config %p not found", ipv6);
	return;

found:
	NET_DBG("RS no respond iface %p count %d", iface,
		ipv6->rs_count);

	if (ipv6->rs_count < RS_COUNT) {
		net_if_start_rs(iface);
	}
}

void net_if_start_rs(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	if (!ipv6) {
		return;
	}

	NET_DBG("Interface %p", iface);

	if (!net_ipv6_start_rs(iface)) {
		k_delayed_work_submit(&ipv6->rs_timer, RS_TIMEOUT);
	}
}
#endif /* CONFIG_NET_IPV6_ND */

struct net_if_addr *net_if_ipv6_addr_lookup(const struct in6_addr *addr,
					    struct net_if **ret)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
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

			if (net_is_ipv6_prefix(
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

		if (net_is_ipv6_prefix(
			    addr->s6_addr,
			    ipv6->unicast[i].address.in6_addr.s6_addr,
			    128)) {
			return &ipv6->unicast[i];
		}
	}

	return NULL;
}

static bool check_timeout(u32_t start, s32_t timeout, u32_t counter,
			  u32_t current_time)
{
	if (counter > 0) {
		return false;
	}

	if ((s32_t)((start + (u32_t)timeout) - current_time) > 0) {
		return false;
	}

	return true;
}

static void address_expired(struct net_if_addr *ifaddr)
{
	NET_DBG("IPv6 address %s is deprecated",
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr));

	ifaddr->addr_state = NET_ADDR_DEPRECATED;
	ifaddr->lifetime.timer_timeout = 0;
	ifaddr->lifetime.wrap_counter = 0;

	sys_slist_find_and_remove(&active_address_lifetime_timers,
				  &ifaddr->lifetime.node);
}

static bool address_manage_timeout(struct net_if_addr *ifaddr,
				   u32_t current_time, u32_t *next_wakeup)
{
	if (check_timeout(ifaddr->lifetime.timer_start,
			  ifaddr->lifetime.timer_timeout,
			  ifaddr->lifetime.wrap_counter,
			  current_time)) {
		address_expired(ifaddr);
		return true;
	}

	if (current_time == NET_TIMEOUT_MAX_VALUE) {
		ifaddr->lifetime.timer_start = k_uptime_get_32();
		ifaddr->lifetime.wrap_counter--;
	}

	if (ifaddr->lifetime.wrap_counter > 0) {
		*next_wakeup = NET_TIMEOUT_MAX_VALUE;
	} else {
		*next_wakeup = ifaddr->lifetime.timer_timeout;
	}

	return false;
}

static void address_lifetime_timeout(struct k_work *work)
{
	u64_t timeout_update = UINT64_MAX;
	u32_t current_time = k_uptime_get_32();
	bool found = false;
	struct net_if_addr *current, *next;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_address_lifetime_timers,
					  current, next, lifetime.node) {
		u32_t next_timeout;
		bool is_timeout;

		is_timeout = address_manage_timeout(current, current_time,
						    &next_timeout);
		if (!is_timeout) {
			if (next_timeout < timeout_update) {
				timeout_update = next_timeout;
				found = true;
			}
		}

		if (current == next) {
			break;
		}
	}

	if (found) {
		/* If we are near upper limit of s32_t timeout, then lower it
		 * a bit so that kernel timeout variable will not overflow.
		 */
		if (timeout_update >= NET_TIMEOUT_MAX_VALUE) {
			timeout_update = NET_TIMEOUT_MAX_VALUE;
		}

		NET_DBG("Waiting for %d ms", (s32_t)timeout_update);

		k_delayed_work_submit(&address_lifetime_timer, timeout_update);
	}
}

#if defined(CONFIG_NET_TEST)
void net_address_lifetime_timeout(void)
{
	address_lifetime_timeout(NULL);
}
#endif

static void address_submit_work(struct net_if_addr *ifaddr)
{
	s32_t remaining;

	remaining = k_delayed_work_remaining_get(&address_lifetime_timer);
	if (!remaining || (ifaddr->lifetime.wrap_counter == 0 &&
			   ifaddr->lifetime.timer_timeout < remaining)) {
		k_delayed_work_cancel(&address_lifetime_timer);

		if (ifaddr->lifetime.wrap_counter > 0 && remaining == 0) {
			k_delayed_work_submit(&address_lifetime_timer,
					      NET_TIMEOUT_MAX_VALUE);
		} else {
			k_delayed_work_submit(&address_lifetime_timer,
					      ifaddr->lifetime.timer_timeout);
		}

		NET_DBG("Next wakeup in %d ms",
			k_delayed_work_remaining_get(&address_lifetime_timer));
	}
}

static void address_start_timer(struct net_if_addr *ifaddr, u32_t vlifetime)
{
	u64_t expire_timeout = K_SECONDS((u64_t)vlifetime);

	sys_slist_append(&active_address_lifetime_timers,
			 &ifaddr->lifetime.node);

	ifaddr->lifetime.timer_start = k_uptime_get_32();
	ifaddr->lifetime.wrap_counter = expire_timeout /
		(u64_t)NET_TIMEOUT_MAX_VALUE;
	ifaddr->lifetime.timer_timeout = expire_timeout -
		(u64_t)NET_TIMEOUT_MAX_VALUE *
		(u64_t)ifaddr->lifetime.wrap_counter;

	address_submit_work(ifaddr);
}

void net_if_ipv6_addr_update_lifetime(struct net_if_addr *ifaddr,
				      u32_t vlifetime)
{
	NET_DBG("Updating expire time of %s by %u secs",
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
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
				    u32_t vlifetime)
{
	ifaddr->is_used = true;
	ifaddr->address.family = AF_INET6;
	ifaddr->addr_type = addr_type;
	net_ipaddr_copy(&ifaddr->address.in6_addr, addr);

#if defined(CONFIG_NET_IPV6_DAD)
	k_delayed_work_init(&ifaddr->dad_timer, dad_timeout);
#endif

	/* FIXME - set the mcast addr for this node */

	if (vlifetime) {
		ifaddr->is_infinite = false;

		NET_DBG("Expiring %s in %u secs", net_sprint_ipv6_addr(addr),
			vlifetime);

		net_if_ipv6_addr_update_lifetime(ifaddr, vlifetime);
	} else {
		ifaddr->is_infinite = true;
	}
}

static inline struct in6_addr *check_global_addr(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return NULL;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    (ipv6->unicast[i].addr_state != NET_ADDR_TENTATIVE &&
		     ipv6->unicast[i].addr_state != NET_ADDR_PREFERRED) ||
		    ipv6->unicast[i].address.family != AF_INET6) {
			continue;
		}

		if (!net_is_ipv6_ll_addr(&ipv6->unicast[i].address.in6_addr)) {
			return &ipv6->unicast[i].address.in6_addr;
		}
	}

	return NULL;
}

static void join_mcast_nodes(struct net_if *iface, struct in6_addr *addr)
{
	enum net_l2_flags flags = 0;

	if (net_if_l2(iface)->get_flags) {
		flags = net_if_l2(iface)->get_flags(iface);
	}

	if (flags & NET_L2_MULTICAST) {
		join_mcast_allnodes(iface);

		if (!(flags & NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE)) {
			join_mcast_solicit_node(iface, addr);
		}
	}
}

struct net_if_addr *net_if_ipv6_addr_add(struct net_if *iface,
					 struct in6_addr *addr,
					 enum net_addr_type addr_type,
					 u32_t vlifetime)
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
#if defined(CONFIG_NET_RPL)
		struct in6_addr *global;
#endif
		if (ipv6->unicast[i].is_used) {
			continue;
		}

		net_if_addr_init(&ipv6->unicast[i], addr, addr_type,
				 vlifetime);

		NET_DBG("[%d] interface %p address %s type %s added", i,
			iface, net_sprint_ipv6_addr(addr),
			net_addr_type2str(addr_type));

		/* RFC 4862 5.4.2
		 * "Before sending a Neighbor Solicitation, an interface
		 * MUST join the all-nodes multicast address and the
		 * solicited-node multicast address of the tentative address."
		 */
		/* The allnodes multicast group is only joined once as
		 * net_ipv6_mcast_join() checks if we have already joined.
		 */
		join_mcast_nodes(iface, &ipv6->unicast[i].address.in6_addr);

#if defined(CONFIG_NET_RPL)
		/* Do not send DAD for global addresses */
		global = check_global_addr(iface);
		if (!net_ipv6_addr_cmp(global, addr)) {
			net_if_ipv6_start_dad(iface, &ipv6->unicast[i]);
		}
#else
		net_if_ipv6_start_dad(iface, &ipv6->unicast[i]);
#endif

		net_mgmt_event_notify(NET_EVENT_IPV6_ADDR_ADD, iface);

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
			i, iface, net_sprint_ipv6_addr(addr),
			net_addr_type2str(ipv6->unicast[i].addr_type));

		net_mgmt_event_notify(NET_EVENT_IPV6_ADDR_DEL, iface);

		return true;
	}

	return false;
}

struct net_if_mcast_addr *net_if_ipv6_maddr_add(struct net_if *iface,
						const struct in6_addr *addr)
{
	struct net_if_ipv6 *ipv6;
	int i;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return NULL;
	}

	if (!net_is_ipv6_addr_mcast(addr)) {
		NET_DBG("Address %s is not a multicast address.",
			net_sprint_ipv6_addr(addr));
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
			net_sprint_ipv6_addr(addr));

		net_mgmt_event_notify(NET_EVENT_IPV6_MADDR_ADD, iface);

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
			i, iface, net_sprint_ipv6_addr(addr));

		net_mgmt_event_notify(NET_EVENT_IPV6_MADDR_DEL, iface);

		return true;
	}

	return false;
}

struct net_if_mcast_addr *net_if_ipv6_maddr_lookup(const struct in6_addr *maddr,
						   struct net_if **ret)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
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

			if (net_is_ipv6_prefix(
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
				    u8_t len)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    ipv6->unicast[i].addr_type != NET_ADDR_AUTOCONF) {
			continue;
		}

		if (net_is_ipv6_prefix(
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
		net_sprint_ipv6_addr(&ifprefix->prefix), ifprefix->len);

	ifprefix->is_used = false;

	if (net_if_config_ipv6_get(ifprefix->iface, &ipv6) < 0) {
		return;
	}

	/* Remove also all auto addresses if the they have the same prefix.
	 */
	remove_prefix_addresses(ifprefix->iface, ipv6, &ifprefix->prefix,
				ifprefix->len);

	net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_DEL, ifprefix->iface);
}

static void prefix_timer_remove(struct net_if_ipv6_prefix *ifprefix)
{
	NET_DBG("IPv6 prefix %s/%d removed",
		net_sprint_ipv6_addr(&ifprefix->prefix),
		ifprefix->len);

	ifprefix->lifetime.timer_timeout = 0;
	ifprefix->lifetime.wrap_counter = 0;

	sys_slist_find_and_remove(&active_prefix_lifetime_timers,
				  &ifprefix->lifetime.node);
}

static bool prefix_manage_timeout(struct net_if_ipv6_prefix *ifprefix,
				  u32_t current_time, u32_t *next_wakeup)
{
	if (check_timeout(ifprefix->lifetime.timer_start,
			  ifprefix->lifetime.timer_timeout,
			  ifprefix->lifetime.wrap_counter,
			  current_time)) {
		prefix_lifetime_expired(ifprefix);
		return true;
	}

	if (current_time == NET_TIMEOUT_MAX_VALUE) {
		ifprefix->lifetime.wrap_counter--;
	}

	if (ifprefix->lifetime.wrap_counter > 0) {
		*next_wakeup = NET_TIMEOUT_MAX_VALUE;
	} else {
		*next_wakeup = ifprefix->lifetime.timer_timeout;
	}

	return false;
}

static void prefix_lifetime_timeout(struct k_work *work)
{
	u64_t timeout_update = UINT64_MAX;
	u32_t current_time = k_uptime_get_32();
	bool found = false;
	struct net_if_ipv6_prefix *current, *next;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_prefix_lifetime_timers,
					  current, next, lifetime.node) {
		u32_t next_timeout;
		bool is_timeout;

		is_timeout = prefix_manage_timeout(current, current_time,
						   &next_timeout);
		if (!is_timeout) {
			if (next_timeout < timeout_update) {
				timeout_update = next_timeout;
				found = true;
			}
		}

		if (current == next) {
			break;
		}
	}

	if (found) {
		/* If we are near upper limit of s32_t timeout, then lower it
		 * a bit so that kernel timeout will not overflow.
		 */
		if (timeout_update >= NET_TIMEOUT_MAX_VALUE) {
			timeout_update = NET_TIMEOUT_MAX_VALUE;
		}

		NET_DBG("Waiting for %d ms", (u32_t)timeout_update);

		k_delayed_work_submit(&prefix_lifetime_timer, timeout_update);
	}
}

static void prefix_submit_work(struct net_if_ipv6_prefix *ifprefix)
{
	s32_t remaining;

	remaining = k_delayed_work_remaining_get(&prefix_lifetime_timer);
	if (!remaining || (ifprefix->lifetime.wrap_counter == 0 &&
			   ifprefix->lifetime.timer_timeout < remaining)) {
		k_delayed_work_cancel(&prefix_lifetime_timer);

		if (ifprefix->lifetime.wrap_counter > 0 && remaining == 0) {
			k_delayed_work_submit(&prefix_lifetime_timer,
					      NET_TIMEOUT_MAX_VALUE);
		} else {
			k_delayed_work_submit(&prefix_lifetime_timer,
					      ifprefix->lifetime.timer_timeout);
		}

		NET_DBG("Next wakeup in %d ms",
			k_delayed_work_remaining_get(&prefix_lifetime_timer));
	}
}

static void prefix_start_timer(struct net_if_ipv6_prefix *ifprefix,
			       u32_t lifetime)
{
	u64_t expire_timeout = K_SECONDS((u64_t)lifetime);

	sys_slist_append(&active_prefix_lifetime_timers,
			 &ifprefix->lifetime.node);

	ifprefix->lifetime.timer_start = k_uptime_get_32();
	ifprefix->lifetime.wrap_counter = expire_timeout /
		(u64_t)NET_TIMEOUT_MAX_VALUE;
	ifprefix->lifetime.timer_timeout = expire_timeout -
		(u64_t)NET_TIMEOUT_MAX_VALUE *
		(u64_t)ifprefix->lifetime.wrap_counter;

	prefix_submit_work(ifprefix);
}

static struct net_if_ipv6_prefix *ipv6_prefix_find(struct net_if *iface,
						   struct in6_addr *prefix,
						   u8_t prefix_len)
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
				    struct in6_addr *addr, u8_t len,
				    u32_t lifetime)
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
						  u8_t len,
						  u32_t lifetime)
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
			net_sprint_ipv6_addr(prefix), len);

		net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_ADD, iface);

		return &ipv6->prefix[i];
	}

	return NULL;
}

bool net_if_ipv6_prefix_rm(struct net_if *iface, struct in6_addr *addr,
			   u8_t len)
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

		net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_DEL, iface);

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

		if (net_is_ipv6_prefix(ipv6->prefix[i].prefix.s6_addr,
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
						     u8_t len)
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

		if (net_is_ipv6_prefix(ipv6->prefix[i].prefix.s6_addr,
				       addr->s6_addr, len)) {
			return &ipv6->prefix[i];
		}
	}

	return NULL;
}

bool net_if_ipv6_addr_onlink(struct net_if **iface, struct in6_addr *addr)
{
	struct net_if *tmp;

	for (tmp = __net_if_start; tmp != __net_if_end; tmp++) {
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
			    net_is_ipv6_prefix(ipv6->prefix[i].prefix.s6_addr,
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
				  u32_t lifetime)
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
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (!routers[i].is_used ||
		    routers[i].address.family != AF_INET6 ||
		    routers[i].iface != iface) {
			continue;
		}

		if (net_ipv6_addr_cmp(&routers[i].address.in6_addr, addr)) {
			return &routers[i];
		}
	}

	return NULL;
}

struct net_if_router *net_if_ipv6_router_find_default(struct net_if *iface,
						      struct in6_addr *addr)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (!routers[i].is_used ||
		    !routers[i].is_default ||
		    routers[i].address.family != AF_INET6) {
			continue;
		}

		if (iface && iface != routers[i].iface) {
			continue;
		}

		return &routers[i];
	}

	return NULL;
}

static void ipv6_router_expired(struct k_work *work)
{
	struct net_if_router *router = CONTAINER_OF(work,
						    struct net_if_router,
						    lifetime);

	NET_DBG("IPv6 router %s is expired",
		net_sprint_ipv6_addr(&router->address.in6_addr));

	router->is_used = false;
}

void net_if_ipv6_router_update_lifetime(struct net_if_router *router,
					u32_t lifetime)
{
	NET_DBG("Updating expire time of %s by %u secs",
		net_sprint_ipv6_addr(&router->address.in6_addr),
		lifetime);

	k_delayed_work_submit(&router->lifetime, K_SECONDS(lifetime));
}

static inline void net_if_router_init(struct net_if_router *router,
				      struct net_if *iface,
				      struct in6_addr *addr, u16_t lifetime)
{
	router->is_used = true;
	router->iface = iface;
	router->address.family = AF_INET6;
	net_ipaddr_copy(&router->address.in6_addr, addr);

	if (lifetime) {
		/* This is a default router. RFC 4861 page 43
		 * AdvDefaultLifetime variable
		 */
		router->is_default = true;
		router->is_infinite = false;

		k_delayed_work_init(&router->lifetime, ipv6_router_expired);
		k_delayed_work_submit(&router->lifetime, K_SECONDS(lifetime));

		NET_DBG("Expiring %s in %u secs", net_sprint_ipv6_addr(addr),
			lifetime);
	} else {
		router->is_default = false;
		router->is_infinite = true;
	}
}

struct net_if_router *net_if_ipv6_router_add(struct net_if *iface,
					     struct in6_addr *addr,
					     u16_t lifetime)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (routers[i].is_used) {
			continue;
		}

		net_if_router_init(&routers[i], iface, addr, lifetime);

		NET_DBG("[%d] interface %p router %s lifetime %u default %d "
			"added",
			i, iface, net_sprint_ipv6_addr(addr), lifetime,
			routers[i].is_default);

		net_mgmt_event_notify(NET_EVENT_IPV6_ROUTER_ADD, iface);

		return &routers[i];
	}

	return NULL;
}

bool net_if_ipv6_router_rm(struct net_if_router *router)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (!routers[i].is_used) {
			continue;
		}

		if (&routers[i] != router) {
			continue;
		}

		k_delayed_work_cancel(&routers[i].lifetime);

		routers[i].is_used = false;

		net_mgmt_event_notify(NET_EVENT_IPV6_ROUTER_DEL,
				      routers[i].iface);

		NET_DBG("[%d] router %s removed",
			i, net_sprint_ipv6_addr(&routers[i].address.in6_addr));

		return true;
	}

	return false;
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

		if (net_is_ipv6_ll_addr(&ipv6->unicast[i].address.in6_addr)) {
			return &ipv6->unicast[i].address.in6_addr;
		}
	}

	return NULL;
}

struct in6_addr *net_if_ipv6_get_ll_addr(enum net_addr_state state,
					 struct net_if **iface)
{
	struct net_if *tmp;

	for (tmp = __net_if_start; tmp != __net_if_end; tmp++) {
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

struct in6_addr *net_if_ipv6_get_global_addr(struct net_if **iface)
{
	struct net_if *tmp;

	for (tmp = __net_if_start; tmp != __net_if_end; tmp++) {
		struct in6_addr *addr;

		if (iface && *iface && tmp != *iface) {
			continue;
		}

		addr = check_global_addr(tmp);
		if (addr) {
			if (iface) {
				*iface = tmp;
			}

			return addr;
		}
	}

	return NULL;
}

static u8_t get_diff_ipv6(const struct in6_addr *src,
			  const struct in6_addr *dst)
{
	return get_ipaddr_diff((const u8_t *)src, (const u8_t *)dst, 16);
}

static inline bool is_proper_ipv6_address(struct net_if_addr *addr)
{
	if (addr->is_used && addr->addr_state == NET_ADDR_PREFERRED &&
	    addr->address.family == AF_INET6 &&
	    !net_is_ipv6_ll_addr(&addr->address.in6_addr)) {
		return true;
	}

	return false;
}

static inline struct in6_addr *net_if_ipv6_get_best_match(struct net_if *iface,
							  struct in6_addr *dst,
							  u8_t *best_so_far)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	struct in6_addr *src = NULL;
	u8_t len;
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
			*best_so_far = len;
			src = &ipv6->unicast[i].address.in6_addr;
		}
	}

	return src;
}

const struct in6_addr *net_if_ipv6_select_src_addr(struct net_if *dst_iface,
						   struct in6_addr *dst)
{
	struct in6_addr *src = NULL;
	u8_t best_match = 0;
	struct net_if *iface;

	if (!net_is_ipv6_ll_addr(dst) && !net_is_ipv6_addr_mcast(dst)) {

		for (iface = __net_if_start;
		     !dst_iface && iface != __net_if_end;
		     iface++) {
			struct in6_addr *addr;

			addr = net_if_ipv6_get_best_match(iface, dst,
							  &best_match);
			if (addr) {
				src = addr;
			}
		}

		/* If caller has supplied interface, then use that */
		if (dst_iface) {
			src = net_if_ipv6_get_best_match(dst_iface, dst,
							 &best_match);
		}

	} else {
		for (iface = __net_if_start;
		     !dst_iface && iface != __net_if_end;
		     iface++) {
			struct in6_addr *addr;

			addr = net_if_ipv6_get_ll(iface, NET_ADDR_PREFERRED);
			if (addr) {
				src = addr;
				break;
			}
		}

		if (dst_iface) {
			src = net_if_ipv6_get_ll(dst_iface, NET_ADDR_PREFERRED);
		}
	}

	if (!src) {
		return net_ipv6_unspecified_address();
	}

	return src;
}

struct net_if *net_if_ipv6_select_src_iface(struct in6_addr *dst)
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

u32_t net_if_ipv6_calc_reachable_time(struct net_if_ipv6 *ipv6)
{
	u32_t min_reachable, max_reachable;

	min_reachable = (MIN_RANDOM_NUMER * ipv6->base_reachable_time)
			/ MIN_RANDOM_DENOM;
	max_reachable = (MAX_RANDOM_NUMER * ipv6->base_reachable_time)
			/ MAX_RANDOM_DENOM;

	NET_DBG("min_reachable:%u max_reachable:%u", min_reachable,
		max_reachable);

	return min_reachable +
	       sys_rand32_get() % (max_reachable - min_reachable);
}

#else /* CONFIG_NET_IPV6 */
#define join_mcast_allnodes(...)
#define join_mcast_solicit_node(...)
#define leave_mcast_all(...)
#define join_mcast_nodes(...)
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
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
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (!routers[i].is_used ||
		    routers[i].address.family != AF_INET) {
			continue;
		}

		if (net_ipv4_addr_cmp(&routers[i].address.in_addr, addr)) {
			return &routers[i];
		}
	}

	return NULL;
}

struct net_if_router *net_if_ipv4_router_add(struct net_if *iface,
					     struct in_addr *addr,
					     bool is_default,
					     u16_t lifetime)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_ROUTERS; i++) {
		if (routers[i].is_used) {
			continue;
		}

		routers[i].is_used = true;
		routers[i].iface = iface;
		routers[i].address.family = AF_INET;
		routers[i].is_default = is_default;

		if (lifetime) {
			routers[i].is_infinite = false;

			/* FIXME - add timer */
		} else {
			routers[i].is_infinite = true;
		}

		net_ipaddr_copy(&routers[i].address.in_addr, addr);

		NET_DBG("[%d] interface %p router %s lifetime %u default %d "
			"added",
			i, iface, net_sprint_ipv4_addr(addr), lifetime,
			is_default);

		net_mgmt_event_notify(NET_EVENT_IPV4_ROUTER_ADD, iface);

		return &routers[i];
	}

	return NULL;
}

bool net_if_ipv4_addr_mask_cmp(struct net_if *iface,
			       struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	u32_t subnet;
	int i;

	if (!ipv4) {
		return false;
	}

	subnet = ntohl(UNALIGNED_GET(&addr->s_addr)) &
		ntohl(ipv4->netmask.s_addr);

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!ipv4->unicast[i].is_used ||
		    ipv4->unicast[i].address.family != AF_INET) {
			continue;
		}

		if ((ntohl(ipv4->unicast[i].address.in_addr.s_addr) &
		     ntohl(ipv4->netmask.s_addr)) == subnet) {
			return true;
		}
	}

	return false;
}

struct net_if *net_if_ipv4_select_src_iface(struct in_addr *dst)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		bool ret;

		ret = net_if_ipv4_addr_mask_cmp(iface, dst);
		if (ret) {
			return iface;
		}
	}

	return net_if_get_default();
}

static u8_t get_diff_ipv4(const struct in_addr *src,
			  const struct in_addr *dst)
{
	return get_ipaddr_diff((const u8_t *)src, (const u8_t *)dst, 4);
}

static inline bool is_proper_ipv4_address(struct net_if_addr *addr)
{
	if (addr->is_used && addr->addr_state == NET_ADDR_PREFERRED &&
	    addr->address.family == AF_INET &&
	    !net_is_ipv4_ll_addr(&addr->address.in_addr)) {
		return true;
	}

	return false;
}

static struct in_addr *net_if_ipv4_get_best_match(struct net_if *iface,
						  struct in_addr *dst,
						  u8_t *best_so_far)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	struct in_addr *src = NULL;
	u8_t len;
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

struct in_addr *net_if_ipv4_get_ll(struct net_if *iface,
				   enum net_addr_state addr_state)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	int i;

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

		if (net_is_ipv4_ll_addr(&ipv4->unicast[i].address.in_addr)) {
			return &ipv4->unicast[i].address.in_addr;
		}
	}

	return NULL;
}

const struct in_addr *net_if_ipv4_select_src_addr(struct net_if *dst_iface,
						  struct in_addr *dst)
{
	struct in_addr *src = NULL;
	u8_t best_match = 0;
	struct net_if *iface;

	if (!net_is_ipv4_ll_addr(dst) && !net_is_ipv4_addr_mcast(dst)) {

		for (iface = __net_if_start;
		     !dst_iface && iface != __net_if_end;
		     iface++) {
			struct in_addr *addr;

			addr = net_if_ipv4_get_best_match(iface, dst,
							  &best_match);
			if (addr) {
				src = addr;
			}
		}

		/* If caller has supplied interface, then use that */
		if (dst_iface) {
			src = net_if_ipv4_get_best_match(dst_iface, dst,
							 &best_match);
		}

	} else {
		for (iface = __net_if_start;
		     !dst_iface && iface != __net_if_end;
		     iface++) {
			struct in_addr *addr;

			addr = net_if_ipv4_get_ll(iface, NET_ADDR_PREFERRED);
			if (addr) {
				src = addr;
				break;
			}
		}

		if (dst_iface) {
			src = net_if_ipv4_get_ll(dst_iface, NET_ADDR_PREFERRED);
		}
	}

	if (!src) {
		return net_ipv4_unspecified_address();
	}

	return src;
}

struct net_if_addr *net_if_ipv4_addr_lookup(const struct in_addr *addr,
					    struct net_if **ret)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
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
					 u32_t vlifetime)
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
			net_sprint_ipv4_addr(addr),
			net_addr_type2str(addr_type));

		net_mgmt_event_notify(NET_EVENT_IPV4_ADDR_ADD, iface);

		return ifaddr;
	}

	return NULL;
}

bool net_if_ipv4_addr_rm(struct net_if *iface, struct in_addr *addr)
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
			i, iface, net_sprint_ipv4_addr(addr));

		net_mgmt_event_notify(NET_EVENT_IPV4_ADDR_DEL, iface);

		return true;
	}

	return false;
}

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

	if (!net_is_ipv4_addr_mcast(addr)) {
		NET_DBG("Address %s is not a multicast address.",
			net_sprint_ipv4_addr(addr));
		return NULL;
	}

	maddr = ipv4_maddr_find(iface, false, NULL);
	if (maddr) {
		maddr->is_used = true;
		maddr->address.family = AF_INET;
		maddr->address.in_addr.s4_addr32[0] = addr->s4_addr32[0];

		NET_DBG("interface %p address %s added", iface,
			net_sprint_ipv4_addr(addr));
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
			iface, net_sprint_ipv4_addr(addr));

		return true;
	}

	return false;
}

struct net_if_mcast_addr *net_if_ipv4_maddr_lookup(const struct in_addr *maddr,
						   struct net_if **ret)
{
	struct net_if_mcast_addr *addr;
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
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

struct net_if *net_if_get_by_index(u8_t index)
{
	if (&__net_if_start[index] >= __net_if_end) {
		NET_DBG("Index %d is too large", index);
		return NULL;
	}

	return &__net_if_start[index];
}

u8_t net_if_get_by_iface(struct net_if *iface)
{
	NET_ASSERT(iface >= __net_if_start && iface < __net_if_end);

	return iface - __net_if_start;
}

void net_if_foreach(net_if_cb_t cb, void *user_data)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		cb(iface, user_data);
	}
}

int net_if_up(struct net_if *iface)
{
	int status;

	NET_DBG("iface %p", iface);

	if (atomic_test_bit(iface->if_dev->flags, NET_IF_UP)) {
		return 0;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(iface)) {
		goto done;
	}
#endif

	/* If the L2 does not support enable just set the flag */
	if (!net_if_l2(iface)->enable) {
		goto done;
	}

	/* Notify L2 to enable the interface */
	status = net_if_l2(iface)->enable(iface, true);
	if (status < 0) {
		return status;
	}

done:
	atomic_set_bit(iface->if_dev->flags, NET_IF_UP);

#if defined(CONFIG_NET_IPV6_DAD)
	NET_DBG("Starting DAD for iface %p", iface);
	net_if_start_dad(iface);
#else
	join_mcast_nodes(iface,
			 &iface->config.ip.ipv6->mcast[0].address.in6_addr);
#endif /* CONFIG_NET_IPV6_DAD */

#if defined(CONFIG_NET_IPV6_ND)
	NET_DBG("Starting ND/RS for iface %p", iface);
	net_if_start_rs(iface);
#endif

#if defined(CONFIG_NET_IPV4_AUTO)
	net_ipv4_autoconf_start(iface);
#endif

	net_mgmt_event_notify(NET_EVENT_IF_UP, iface);

	return 0;
}

void net_if_carrier_down(struct net_if *iface)
{
	NET_DBG("iface %p", iface);

	atomic_clear_bit(iface->if_dev->flags, NET_IF_UP);

#if defined(CONFIG_NET_IPV4_AUTO)
	net_ipv4_autoconf_reset(iface);
#endif

	net_mgmt_event_notify(NET_EVENT_IF_DOWN, iface);
}

int net_if_down(struct net_if *iface)
{
	int status;

	NET_DBG("iface %p", iface);

	leave_mcast_all(iface);

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(iface)) {
		goto done;
	}
#endif

	/* If the L2 does not support enable just clear the flag */
	if (!net_if_l2(iface)->enable) {
		goto done;
	}

	/* Notify L2 to disable the interface */
	status = net_if_l2(iface)->enable(iface, false);
	if (status < 0) {
		return status;
	}

done:
	atomic_clear_bit(iface->if_dev->flags, NET_IF_UP);

	net_mgmt_event_notify(NET_EVENT_IF_DOWN, iface);

	return 0;
}

int net_if_set_promisc(struct net_if *iface)
{
	enum net_l2_flags l2_flags = 0;
	int ret;

	NET_ASSERT(iface);

	if (net_if_l2(iface)->get_flags) {
		l2_flags = net_if_l2(iface)->get_flags(iface);
	}

	if (!(l2_flags & NET_L2_PROMISC_MODE)) {
		return -ENOTSUP;
	}

#if defined(CONFIG_NET_L2_ETHERNET)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		ret = net_eth_promisc_mode(iface, true);
		if (ret < 0) {
			return ret;
		}
	}
#else
	return -ENOTSUP;
#endif

	ret = atomic_test_and_set_bit(iface->if_dev->flags, NET_IF_PROMISC);
	if (ret) {
		return -EALREADY;
	}

	return 0;
}

void net_if_unset_promisc(struct net_if *iface)
{
	NET_ASSERT(iface);

	atomic_clear_bit(iface->if_dev->flags, NET_IF_PROMISC);
}

bool net_if_is_promisc(struct net_if *iface)
{
	NET_ASSERT(iface);

	return atomic_test_bit(iface->if_dev->flags, NET_IF_PROMISC);
}

#if defined(CONFIG_NET_PKT_TIMESTAMP)
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
#endif /* CONFIG_NET_PKT_TIMESTAMP */

void net_if_init(void)
{
	struct net_if *iface;
	int i, if_count;

	NET_DBG("");

	net_tc_tx_init();

#if defined(CONFIG_NET_IPV6)
	k_delayed_work_init(&address_lifetime_timer, address_lifetime_timeout);
	k_delayed_work_init(&prefix_lifetime_timer, prefix_lifetime_timeout);
#endif

	for (iface = __net_if_start, if_count = 0; iface != __net_if_end;
	     iface++, if_count++) {
		init_iface(iface);
	}

	if (iface == __net_if_start) {
		NET_ERR("There is no network interface to work with!");
		return;
	}

#if defined(CONFIG_NET_IPV4)
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
#endif

#if defined(CONFIG_NET_IPV6)
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

#if defined(CONFIG_NET_IPV6_ND)
		k_delayed_work_init(&ipv6_addresses[i].ipv6.rs_timer,
				    rs_timeout);
#endif
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_PKT_TIMESTAMP)
	k_thread_create(&tx_thread_ts, tx_ts_stack,
			K_THREAD_STACK_SIZEOF(tx_ts_stack),
			(k_thread_entry_t)net_tx_ts_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);
#endif /* CONFIG_NET_PKT_TIMESTAMP */

#if defined(CONFIG_NET_VLAN)
	/* Make sure that we do not have too many network interfaces
	 * compared to the number of VLAN interfaces.
	 */
	for (iface = __net_if_start, if_count = 0;
	     iface != __net_if_end; iface++) {
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
	struct net_if *iface;

	NET_DBG("");

	/* After TX is running, attempt to bring the interface up */
	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		net_if_up(iface);
	}

	/* RPL init must be done after the network interface is up
	 * as the RPL code wants to add multicast address to interface.
	 */
	net_rpl_init();
}
