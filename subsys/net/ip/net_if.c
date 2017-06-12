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
#include <sections.h>
#include <string.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/arp.h>
#include <net/net_mgmt.h>

#include "net_private.h"
#include "ipv6.h"
#include "rpl.h"

#include "net_stats.h"

#define REACHABLE_TIME (30 * MSEC_PER_SEC) /* in ms */
#define MIN_RANDOM_FACTOR (1/2)
#define MAX_RANDOM_FACTOR (3/2)

/* net_if dedicated section limiters */
extern struct net_if __net_if_start[];
extern struct net_if __net_if_end[];

extern struct k_poll_event __net_if_event_start[];
extern struct k_poll_event __net_if_event_stop[];

static struct net_if_router routers[CONFIG_NET_MAX_ROUTERS];

/* We keep track of the link callbacks in this list.
 */
static sys_slist_t link_callbacks;

NET_STACK_DEFINE(TX, tx_stack, CONFIG_NET_TX_STACK_SIZE,
		 CONFIG_NET_TX_STACK_SIZE);
static struct k_thread tx_thread_data;

#if defined(CONFIG_NET_DEBUG_IF)
#define debug_check_packet(pkt)						    \
	{								    \
		size_t len = net_pkt_get_len(pkt);			    \
									    \
		NET_DBG("Processing (pkt %p, data len %zu) network packet", \
			pkt, len);					    \
									    \
		NET_ASSERT(pkt->frags && len);				    \
	} while (0)
#else
#define debug_check_packet(...)
#endif /* CONFIG_NET_DEBUG_IF */

static inline void net_context_send_cb(struct net_context *context,
				       void *token, int status)
{
	if (context->send_cb) {
		context->send_cb(context, status, token, context->user_data);
	}

#if defined(CONFIG_NET_UDP)
	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		net_stats_update_udp_sent();
	} else
#endif
#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		net_stats_update_tcp_seg_sent();
	} else
#endif
	{
	}
}

static bool net_if_tx(struct net_if *iface)
{
	const struct net_if_api *api = iface->dev->driver_api;
	struct net_linkaddr *dst;
	struct net_context *context;
	struct net_pkt *pkt;
	void *context_token;
	int status;
#if defined(CONFIG_NET_STATISTICS)
	size_t pkt_len;
#endif

	pkt = k_fifo_get(&iface->tx_queue, K_NO_WAIT);
	if (!pkt) {
		return false;
	}

	debug_check_packet(pkt);

	dst = net_pkt_ll_dst(pkt);
	context = net_pkt_context(pkt);
	context_token = net_pkt_token(pkt);

	if (atomic_test_bit(iface->flags, NET_IF_UP)) {
#if defined(CONFIG_NET_STATISTICS)
		pkt_len = net_pkt_get_len(pkt);
#endif
		status = api->send(iface, pkt);
	} else {
		/* Drop packet if interface is not up */
		NET_WARN("iface %p is down", iface);
		status = -ENETDOWN;
	}

	if (status < 0) {
		net_pkt_unref(pkt);
	} else {
		net_stats_update_bytes_sent(pkt_len);
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

static void net_if_flush_tx(struct net_if *iface)
{
	if (k_fifo_is_empty(&iface->tx_queue)) {
		return;
	}

	/* Without this, the k_fifo_get() can return a pkt which
	 * has pkt->frags set to NULL. This is not allowed as we
	 * cannot send a packet that has no data in it.
	 * The k_yield() fixes the issue and packets are flushed
	 * correctly.
	 */
	k_yield();

	while (1) {
		if (!net_if_tx(iface)) {
			break;
		}
	}
}

static void net_if_process_events(struct k_poll_event *event, int ev_count)
{
	for (; ev_count; event++, ev_count--) {
		switch (event->state) {
		case K_POLL_STATE_SIGNALED:
			break;
		case K_POLL_STATE_FIFO_DATA_AVAILABLE:
		{
			struct net_if *iface;

			iface = CONTAINER_OF(event->fifo, struct net_if,
					     tx_queue);
			net_if_tx(iface);

			break;
		}
		case K_POLL_STATE_NOT_READY:
			break;
		default:
			break;
		}
	}
}

static int net_if_prepare_events(void)
{
	struct net_if *iface;
	int ev_count = 0;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		k_poll_event_init(&__net_if_event_start[ev_count],
				  K_POLL_TYPE_FIFO_DATA_AVAILABLE,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &iface->tx_queue);
		ev_count++;
	}

	return ev_count;
}

static void net_if_tx_thread(struct k_sem *startup_sync)
{
	NET_DBG("Starting TX thread (stack %d bytes)",
		CONFIG_NET_TX_STACK_SIZE);

	/* This will allow RX thread to start to receive data. */
	k_sem_give(startup_sync);

	while (1) {
		int ev_count;

		ev_count = net_if_prepare_events();

		k_poll(__net_if_event_start, ev_count, K_FOREVER);

		net_if_process_events(__net_if_event_start, ev_count);

		k_yield();
	}
}

static inline void init_iface(struct net_if *iface)
{
	const struct net_if_api *api = iface->dev->driver_api;

	NET_ASSERT(api && api->init && api->send);

	NET_DBG("On iface %p", iface);

	k_fifo_init(&iface->tx_queue);

	api->init(iface);
}

enum net_verdict net_if_send_data(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_context *context = net_pkt_context(pkt);
	struct net_linkaddr *dst = net_pkt_ll_dst(pkt);
	void *token = net_pkt_token(pkt);
	enum net_verdict verdict;
	int status = -EIO;

	if (!atomic_test_bit(iface->flags, NET_IF_UP)) {
		/* Drop packet if interface is not up */
		NET_WARN("iface %p is down", iface);
		verdict = NET_DROP;
		status = -ENETDOWN;
		goto done;
	}

	/* If the ll address is not set at all, then we must set
	 * it here.
	 * Workaround Linux bug, see:
	 * https://jira.zephyrproject.org/browse/ZEP-1656
	 */
	if (!atomic_test_bit(iface->flags, NET_IF_POINTOPOINT) &&
	    !net_pkt_ll_src(pkt)->addr) {
		net_pkt_ll_src(pkt)->addr = net_pkt_ll_if(pkt)->addr;
		net_pkt_ll_src(pkt)->len = net_pkt_ll_if(pkt)->len;
	}

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

	verdict = iface->l2->send(iface, pkt);

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
		if (!memcmp(iface->link_addr.addr, ll_addr->addr,
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
		if (iface->dev == dev) {
			return iface;
		}
	}

	return NULL;
}

struct net_if *net_if_get_default(void)
{
	if (__net_if_start == __net_if_end) {
		return NULL;
	}

	return __net_if_start;
}

#if defined(CONFIG_NET_IPV6)

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
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!iface->ipv6.mcast[i].is_used ||
		    !iface->ipv6.mcast[i].is_joined) {
			continue;
		}

		net_ipv6_mld_leave(iface,
				   &iface->ipv6.mcast[i].address.in6_addr);
	}
}
#else
#define join_mcast_allnodes(...)
#define join_mcast_solicit_node(...)
#define leave_mcast_all(...)
#endif /* CONFIG_NET_IPV6_MLD */

#if defined(CONFIG_NET_IPV6_DAD)
#define DAD_TIMEOUT (MSEC_PER_SEC / 10)

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

		/* We join the allnodes group from here so that we have
		 * a link local address defined. If done from ifup(),
		 * we would only get unspecified address as a source
		 * address. The allnodes multicast group is only joined
		 * once in this case as the net_ipv6_mcast_join() checks
		 * if we have already joined.
		 */
		join_mcast_allnodes(iface);

		join_mcast_solicit_node(iface, &ifaddr->address.in6_addr);
	}
}

static void net_if_ipv6_start_dad(struct net_if *iface,
				  struct net_if_addr *ifaddr)
{
	ifaddr->addr_state = NET_ADDR_TENTATIVE;
	ifaddr->dad_count = 1;

	NET_DBG("Interface %p ll addr %s tentative IPv6 addr %s", iface,
		net_sprint_ll_addr(iface->link_addr.addr,
				   iface->link_addr.len),
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr));

	if (!net_ipv6_start_dad(iface, ifaddr)) {
		k_delayed_work_submit(&ifaddr->dad_timer, DAD_TIMEOUT);
	}
}

void net_if_start_dad(struct net_if *iface)
{
	struct net_if_addr *ifaddr;
	struct in6_addr addr = { };

	net_ipv6_addr_create_iid(&addr, &iface->link_addr);

	ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s address to interface %p, DAD fails",
			net_sprint_ipv6_addr(&addr), iface);
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
#define RS_TIMEOUT MSEC_PER_SEC
#define RS_COUNT 3

static void rs_timeout(struct k_work *work)
{
	/* Did not receive RA yet. */
	struct net_if *iface = CONTAINER_OF(work, struct net_if, ipv6.rs_timer);

	iface->ipv6.rs_count++;

	NET_DBG("RS no respond iface %p count %d", iface, iface->ipv6.rs_count);

	if (iface->ipv6.rs_count < RS_COUNT) {
		net_if_start_rs(iface);
	}
}

void net_if_start_rs(struct net_if *iface)
{
	NET_DBG("Interface %p", iface);

	if (!net_ipv6_start_rs(iface)) {
		k_delayed_work_submit(&iface->ipv6.rs_timer, RS_TIMEOUT);
	}
}
#endif /* CONFIG_NET_IPV6_ND */

struct net_if_addr *net_if_ipv6_addr_lookup(const struct in6_addr *addr,
					    struct net_if **ret)
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

				if (ret) {
					*ret = iface;
				}

				return &iface->ipv6.unicast[i];
			}
		}
	}

	return NULL;
}

static void ipv6_addr_expired(struct k_work *work)
{
	struct net_if_addr *ifaddr = CONTAINER_OF(work,
						  struct net_if_addr,
						  lifetime);

	NET_DBG("IPv6 address %s is deprecated",
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr));

	ifaddr->addr_state = NET_ADDR_DEPRECATED;
}

void net_if_ipv6_addr_update_lifetime(struct net_if_addr *ifaddr,
				      u32_t vlifetime)
{
	NET_DBG("Updating expire time of %s by %u secs",
		net_sprint_ipv6_addr(&ifaddr->address.in6_addr),
		vlifetime);

	k_delayed_work_submit(&ifaddr->lifetime,
			      vlifetime * MSEC_PER_SEC);
}

static struct net_if_addr *ipv6_addr_find(struct net_if *iface,
					  struct in6_addr *addr)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!iface->ipv6.unicast[i].is_used) {
			continue;
		}

		if (net_ipv6_addr_cmp(addr,
				&iface->ipv6.unicast[i].address.in6_addr)) {
			return &iface->ipv6.unicast[i];
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

		k_delayed_work_init(&ifaddr->lifetime, ipv6_addr_expired);

		NET_DBG("Expiring %s in %u secs", net_sprint_ipv6_addr(addr),
			vlifetime);

		net_if_ipv6_addr_update_lifetime(ifaddr, vlifetime);
	} else {
		ifaddr->is_infinite = true;
	}
}

struct net_if_addr *net_if_ipv6_addr_add(struct net_if *iface,
					 struct in6_addr *addr,
					 enum net_addr_type addr_type,
					 u32_t vlifetime)
{
	struct net_if_addr *ifaddr;
	int i;

	ifaddr = ipv6_addr_find(iface, addr);
	if (ifaddr) {
		return ifaddr;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (iface->ipv6.unicast[i].is_used) {
			continue;
		}

		net_if_addr_init(&iface->ipv6.unicast[i], addr, addr_type,
				 vlifetime);

		NET_DBG("[%d] interface %p address %s type %s added", i, iface,
			net_sprint_ipv6_addr(addr),
			net_addr_type2str(addr_type));

		net_if_ipv6_start_dad(iface, &iface->ipv6.unicast[i]);

		net_mgmt_event_notify(NET_EVENT_IPV6_ADDR_ADD, iface);

		return &iface->ipv6.unicast[i];
	}

	return NULL;
}

bool net_if_ipv6_addr_rm(struct net_if *iface, const struct in6_addr *addr)
{
	int i;

	NET_ASSERT(addr);

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		struct in6_addr maddr;

		if (!iface->ipv6.unicast[i].is_used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(
			    &iface->ipv6.unicast[i].address.in6_addr,
			    addr)) {
			continue;
		}

		k_delayed_work_cancel(&iface->ipv6.unicast[i].lifetime);

		iface->ipv6.unicast[i].is_used = false;

		net_ipv6_addr_create_solicited_node(addr, &maddr);
		net_if_ipv6_maddr_rm(iface, &maddr);

		NET_DBG("[%d] interface %p address %s type %s removed",
			i, iface, net_sprint_ipv6_addr(addr),
			net_addr_type2str(iface->ipv6.unicast[i].addr_type));

		net_mgmt_event_notify(NET_EVENT_IPV6_ADDR_DEL, iface);

		return true;
	}

	return false;
}

struct net_if_mcast_addr *net_if_ipv6_maddr_add(struct net_if *iface,
						const struct in6_addr *addr)
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
		iface->ipv6.mcast[i].address.family = AF_INET6;
		memcpy(&iface->ipv6.mcast[i].address.in6_addr, addr, 16);

		NET_DBG("[%d] interface %p address %s added", i, iface,
			net_sprint_ipv6_addr(addr));

		net_mgmt_event_notify(NET_EVENT_IPV6_MADDR_ADD, iface);

		return &iface->ipv6.mcast[i];
	}

	return NULL;
}

bool net_if_ipv6_maddr_rm(struct net_if *iface, const struct in6_addr *addr)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!iface->ipv6.mcast[i].is_used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(
			    &iface->ipv6.mcast[i].address.in6_addr,
			    addr)) {
			continue;
		}

		iface->ipv6.mcast[i].is_used = false;

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
		int i;

		if (ret && *ret && iface != *ret) {
			continue;
		}

		for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
			if (!iface->ipv6.mcast[i].is_used ||
			    iface->ipv6.mcast[i].address.family != AF_INET6) {
				continue;
			}

			if (net_is_ipv6_prefix(maddr->s6_addr,
				iface->ipv6.mcast[i].address.in6_addr.s6_addr,
					       128)) {

				if (ret) {
					*ret = iface;
				}

				return &iface->ipv6.mcast[i];
			}
		}
	}

	return NULL;
}

static struct net_if_ipv6_prefix *ipv6_prefix_find(struct net_if *iface,
						   struct in6_addr *prefix,
						   u8_t prefix_len)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!iface->ipv6.unicast[i].is_used) {
			continue;
		}

		if (net_ipv6_addr_cmp(prefix, &iface->ipv6.prefix[i].prefix) &&
		    prefix_len == iface->ipv6.prefix[i].len) {
			return &iface->ipv6.prefix[i];
		}
	}

	return NULL;
}

static inline void prefix_lf_timeout(struct k_work *work)
{
	struct net_if_ipv6_prefix *prefix =
		CONTAINER_OF(work, struct net_if_ipv6_prefix, lifetime);

	NET_DBG("Prefix %s/%d expired",
		net_sprint_ipv6_addr(&prefix->prefix), prefix->len);

	prefix->is_used = false;
}

static void net_if_ipv6_prefix_init(struct net_if_ipv6_prefix *prefix,
				    struct in6_addr *addr, u8_t len,
				    u32_t lifetime)
{
	prefix->is_used = true;
	prefix->len = len;
	net_ipaddr_copy(&prefix->prefix, addr);
	k_delayed_work_init(&prefix->lifetime, prefix_lf_timeout);

	if (lifetime == NET_IPV6_ND_INFINITE_LIFETIME) {
		prefix->is_infinite = true;
	} else {
		prefix->is_infinite = false;
	}
}

struct net_if_ipv6_prefix *net_if_ipv6_prefix_add(struct net_if *iface,
						  struct in6_addr *prefix,
						  u8_t len,
						  u32_t lifetime)
{
	struct net_if_ipv6_prefix *if_prefix;
	int i;

	if_prefix = ipv6_prefix_find(iface, prefix, len);
	if (if_prefix) {
		return if_prefix;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (iface->ipv6.prefix[i].is_used) {
			continue;
		}

		net_if_ipv6_prefix_init(&iface->ipv6.prefix[i], prefix, len,
					lifetime);

		NET_DBG("[%d] interface %p prefix %s/%d added", i, iface,
			net_sprint_ipv6_addr(prefix), len);

		net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_ADD, iface);

		return &iface->ipv6.prefix[i];
	}

	return NULL;
}

bool net_if_ipv6_prefix_rm(struct net_if *iface, struct in6_addr *addr,
			   u8_t len)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!iface->ipv6.prefix[i].is_used) {
			continue;
		}

		if (!net_ipv6_addr_cmp(&iface->ipv6.prefix[i].prefix, addr) ||
		    iface->ipv6.prefix[i].len != len) {
			continue;
		}

		net_if_ipv6_prefix_unset_timer(&iface->ipv6.prefix[i]);

		iface->ipv6.prefix[i].is_used = false;

		net_mgmt_event_notify(NET_EVENT_IPV6_PREFIX_DEL, iface);

		return true;
	}

	return false;
}

struct net_if_ipv6_prefix *net_if_ipv6_prefix_lookup(struct net_if *iface,
						     struct in6_addr *addr,
						     u8_t len)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!iface->ipv6.prefix[i].is_used) {
			continue;
		}

		if (net_is_ipv6_prefix(iface->ipv6.prefix[i].prefix.s6_addr,
				       addr->s6_addr, len)) {
			return &iface->ipv6.prefix[i];
		}
	}

	return NULL;
}

bool net_if_ipv6_addr_onlink(struct net_if **iface, struct in6_addr *addr)
{
	struct net_if *tmp;

	for (tmp = __net_if_start; tmp != __net_if_end; tmp++) {
		int i;

		if (iface && *iface && *iface != tmp) {
			continue;
		}

		for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
			if (tmp->ipv6.prefix[i].is_used &&
			    net_is_ipv6_prefix(tmp->ipv6.prefix[i].prefix.
					       s6_addr,
					       addr->s6_addr,
					       tmp->ipv6.prefix[i].len)) {
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
	/* The maximum lifetime might be shorter than expected
	 * because we have only 32-bit int to store the value and
	 * the timer API uses ms value. The lifetime value with
	 * all bits set means infinite and that value is never set
	 * to timer.
	 */
	u32_t timeout = lifetime * MSEC_PER_SEC;

	NET_ASSERT(lifetime != 0xffffffff);

	if (lifetime > (0xfffffffe / MSEC_PER_SEC)) {
		timeout = 0xfffffffe;

		NET_ERR("Prefix %s/%d lifetime %u overflow, "
			"setting it to %u secs",
			net_sprint_ipv6_addr(&prefix->prefix),
			prefix->len,
			lifetime, timeout / MSEC_PER_SEC);
	}

	NET_DBG("Prefix lifetime %u ms", timeout);

	k_delayed_work_submit(&prefix->lifetime, timeout);
}

void net_if_ipv6_prefix_unset_timer(struct net_if_ipv6_prefix *prefix)
{
	if (!prefix->is_used) {
		return;
	}

	k_delayed_work_cancel(&prefix->lifetime);
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

	k_delayed_work_submit(&router->lifetime,
			      lifetime * MSEC_PER_SEC);
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
		k_delayed_work_submit(&router->lifetime,
				      lifetime * MSEC_PER_SEC);

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
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!iface->ipv6.unicast[i].is_used ||
		    (addr_state != NET_ADDR_ANY_STATE &&
		     iface->ipv6.unicast[i].addr_state != addr_state) ||
		    iface->ipv6.unicast[i].address.family != AF_INET6) {
			continue;
		}
		if (net_is_ipv6_ll_addr(&iface->ipv6.unicast[i].address.in6_addr)) {
			return &iface->ipv6.unicast[i].address.in6_addr;
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

static inline struct in6_addr *check_global_addr(struct net_if *iface)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!iface->ipv6.unicast[i].is_used ||
		    (iface->ipv6.unicast[i].addr_state != NET_ADDR_TENTATIVE &&
		     iface->ipv6.unicast[i].addr_state != NET_ADDR_PREFERRED) ||
		    iface->ipv6.unicast[i].address.family != AF_INET6) {
			continue;
		}

		if (!net_is_ipv6_ll_addr(
			    &iface->ipv6.unicast[i].address.in6_addr)) {
			return &iface->ipv6.unicast[i].address.in6_addr;
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

static inline u8_t get_length(struct in6_addr *src, struct in6_addr *dst)
{
	u8_t j, k, xor;
	u8_t len = 0;

	for (j = 0; j < 16; j++) {
		if (src->s6_addr[j] == dst->s6_addr[j]) {
			len += 8;
		} else {
			xor = src->s6_addr[j] ^ dst->s6_addr[j];
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
	struct in6_addr *src = NULL;
	u8_t i, len;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!is_proper_ipv6_address(&iface->ipv6.unicast[i])) {
			continue;
		}

		len = get_length(dst,
				 &iface->ipv6.unicast[i].address.in6_addr);
		if (len >= *best_so_far) {
			*best_so_far = len;
			src = &iface->ipv6.unicast[i].address.in6_addr;
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

u32_t net_if_ipv6_calc_reachable_time(struct net_if *iface)
{
	return MIN_RANDOM_FACTOR * iface->ipv6.base_reachable_time +
		sys_rand32_get() %
		(MAX_RANDOM_FACTOR * iface->ipv6.base_reachable_time -
		 MIN_RANDOM_FACTOR * iface->ipv6.base_reachable_time);
}

#else /* CONFIG_NET_IPV6 */
#define join_mcast_allnodes(...)
#define join_mcast_solicit_node(...)
#define leave_mcast_all(...)
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
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
	u32_t subnet = ntohl(addr->s_addr) &
			ntohl(iface->ipv4.netmask.s_addr);
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!iface->ipv4.unicast[i].is_used ||
		    iface->ipv4.unicast[i].address.family != AF_INET) {
				continue;
		}
		if ((ntohl(iface->ipv4.unicast[i].address.in_addr.s_addr) &
		     ntohl(iface->ipv4.netmask.s_addr)) == subnet) {
			return true;
		}
	}

	return false;
}

struct net_if_addr *net_if_ipv4_addr_lookup(const struct in_addr *addr,
					    struct net_if **ret)
{
	struct net_if *iface;

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		int i;

		for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (!iface->ipv4.unicast[i].is_used ||
			    iface->ipv4.unicast[i].address.family != AF_INET) {
				continue;
			}

			if (UNALIGNED_GET(&addr->s4_addr32[0]) ==
			    iface->ipv4.unicast[i].address.in_addr.s_addr) {

				if (ret) {
					*ret = iface;
				}

				return &iface->ipv4.unicast[i];
			}
		}
	}

	return NULL;
}

static struct net_if_addr *ipv4_addr_find(struct net_if *iface,
					  struct in_addr *addr)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!iface->ipv4.unicast[i].is_used) {
			continue;
		}

		if (net_ipv4_addr_cmp(addr,
				&iface->ipv4.unicast[i].address.in_addr)) {
			return &iface->ipv4.unicast[i];
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
	int i;

	ifaddr = ipv4_addr_find(iface, addr);
	if (ifaddr) {
		return ifaddr;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->ipv4.unicast[i].is_used) {
			continue;
		}

		iface->ipv4.unicast[i].is_used = true;
		iface->ipv4.unicast[i].address.family = AF_INET;
		iface->ipv4.unicast[i].address.in_addr.s4_addr32[0] =
						addr->s4_addr32[0];
		iface->ipv4.unicast[i].addr_type = addr_type;

		/* Caller has to take care of timers and their expiry */
		if (vlifetime) {
			iface->ipv4.unicast[i].is_infinite = false;
		} else {
			iface->ipv4.unicast[i].is_infinite = true;
		}

		/**
		 *  TODO: Handle properly PREFERRED/DEPRECATED state when
		 *  address in use, expired and renewal state.
		 */
		iface->ipv4.unicast[i].addr_state = NET_ADDR_PREFERRED;

		NET_DBG("[%d] interface %p address %s type %s added", i, iface,
			net_sprint_ipv4_addr(addr),
			net_addr_type2str(addr_type));

		net_mgmt_event_notify(NET_EVENT_IPV4_ADDR_ADD, iface);

		return &iface->ipv4.unicast[i];
	}

	return NULL;
}

bool net_if_ipv4_addr_rm(struct net_if *iface, struct in_addr *addr)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (!iface->ipv4.unicast[i].is_used) {
			continue;
		}

		if (!net_ipv4_addr_cmp(
			    &iface->ipv4.unicast[i].address.in_addr,
			    addr)) {
			continue;
		}

		iface->ipv4.unicast[i].is_used = false;

		NET_DBG("[%d] interface %p address %s removed",
			i, iface, net_sprint_ipv4_addr(addr));

		net_mgmt_event_notify(NET_EVENT_IPV4_ADDR_DEL, iface);

		return true;
	}

	return false;
}
#endif /* CONFIG_NET_IPV4 */

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

	if (atomic_test_bit(iface->flags, NET_IF_UP)) {
		return 0;
	}

	/* If the L2 does not support enable just set the flag */
	if (!iface->l2->enable) {
		goto done;
	}

	/* Notify L2 to enable the interface */
	status = iface->l2->enable(iface, true);
	if (status < 0) {
		return status;
	}

done:
	atomic_set_bit(iface->flags, NET_IF_UP);

#if defined(CONFIG_NET_IPV6_DAD)
	NET_DBG("Starting DAD for iface %p", iface);
	net_if_start_dad(iface);
#else
	/* Join the IPv6 multicast groups even if DAD is disabled */
	join_mcast_allnodes(iface);
	join_mcast_solicit_node(iface, &iface->ipv6.mcast[0].address.in6_addr);
#endif

#if defined(CONFIG_NET_IPV6_ND)
	NET_DBG("Starting ND/RS for iface %p", iface);
	net_if_start_rs(iface);
#endif

	net_mgmt_event_notify(NET_EVENT_IF_UP, iface);

	return 0;
}

int net_if_down(struct net_if *iface)
{
	int status;

	NET_DBG("iface %p", iface);

	leave_mcast_all(iface);

	net_if_flush_tx(iface);

	/* If the L2 does not support enable just clear the flag */
	if (!iface->l2->enable) {
		goto done;
	}

	/* Notify L2 to disable the interface */
	status = iface->l2->enable(iface, false);
	if (status < 0) {
		return status;
	}

done:
	atomic_clear_bit(iface->flags, NET_IF_UP);

	net_mgmt_event_notify(NET_EVENT_IF_DOWN, iface);

	return 0;
}

void net_if_init(struct k_sem *startup_sync)
{
	struct net_if *iface;

	NET_DBG("");

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		init_iface(iface);

#if defined(CONFIG_NET_IPV4)
		iface->ipv4.ttl = CONFIG_NET_INITIAL_TTL;
#endif

#if defined(CONFIG_NET_IPV6)
		iface->ipv6.hop_limit = CONFIG_NET_INITIAL_HOP_LIMIT;
		iface->ipv6.base_reachable_time = REACHABLE_TIME;

		net_if_ipv6_set_reachable_time(iface);

#if defined(CONFIG_NET_IPV6_ND)
		k_delayed_work_init(&iface->ipv6.rs_timer, rs_timeout);
#endif
#endif
	}

	if (iface == __net_if_start) {
		NET_WARN("There is no network interface to work with!");
		return;
	}

	k_thread_create(&tx_thread_data, tx_stack,
			K_THREAD_STACK_SIZEOF(tx_stack),
			(k_thread_entry_t)net_if_tx_thread,
			startup_sync, NULL, NULL, K_PRIO_COOP(7),
			K_ESSENTIAL, K_NO_WAIT);
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
