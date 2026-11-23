/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(conn_mgr, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/dns_resolve.h>
#include "conn_mgr_private.h"
#include "net_private.h"

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(7)
#endif

static K_THREAD_STACK_DEFINE(conn_mgr_mon_stack,
			     CONFIG_NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE);
static struct k_thread conn_mgr_mon_thread;

/* Internal state array tracking readiness, flags, and other state information for all available
 * ifaces. Note that indexing starts at 0, whereas Zephyr iface indices start at 1.
 * conn_mgr_mon_get_if_by_index and conn_mgr_get_index_for_if are used to go back and forth between
 * iface_states indices and Zephyr iface pointers.
 */
static uint16_t iface_states[CONN_MGR_IFACE_MAX];

/* Tracks the most recent total quantity of L4-ready ifaces (any, IPv4, IPv6) */
static uint16_t last_ready_count;
static uint16_t last_ready_count_ipv4;
static uint16_t last_ready_count_ipv6;

/* Tracks the last ifaces to cause a major state change (any, IPv4, IPv6) */
static struct net_if *last_blame;
static struct net_if *last_blame_ipv4;
static struct net_if *last_blame_ipv6;

/* Last reported DNS readiness */
static bool last_dns_ready;

/* Readiness gain counter, 0 until the first gain and skipping 0 on wrap, and its value at
 * the last gain of each flag, indexed by flag bit. 0 in flag_gain_seq means never gained.
 */
static uint32_t gain_seq;
static uint32_t flag_gain_seq[LOG2(CONN_MGR_READY_DNS) + 1];

/* Address families of the configured unicast DNS servers: unbound and per interface */
static uint32_t dns_unbound_families;
static uint32_t dns_bound_families[CONN_MGR_IFACE_MAX];

/* Used to signal when modifications have been made that need to be responded to */
K_SEM_DEFINE(conn_mgr_mon_updated, 1, 1);

/* Used to protect conn_mgr_monitor state */
K_MUTEX_DEFINE(conn_mgr_mon_lock);

/* Signaled when a flag is gained */
static K_CONDVAR_DEFINE(conn_mgr_mon_ready_gained);

uint16_t *conn_mgr_if_state_internal(void)
{
	return iface_states;
}

/**
 * @brief Retrieves pointer to an iface by the index that corresponds to it in iface_states
 *
 * @param index - The index in iface_states to find the corresponding iface for.
 * @return net_if* - The corresponding iface.
 */
static struct net_if *conn_mgr_mon_get_if_by_index(int index)
{
	return net_if_get_by_index(index + 1);
}

/**
 * @brief Gets the index in iface_states for the state corresponding to a provided iface.
 *
 * @param iface - iface to find the index of.
 * @return int - The index found.
 */
static int conn_mgr_get_index_for_if(struct net_if *iface)
{
	return net_if_get_by_iface(iface) - 1;
}

/**
 * @brief Conveniently update iface readiness state
 *
 * @param idx - index (in iface_states) of the iface to mark ready or unready
 * @param ready - true if the iface should be considered ready, otherwise false
 * @param ready_ipv4 - true if the iface is ready with IPv4, otherwise false
 * @param ready_ipv6 - true if the iface is ready with IPv6, otherwise false
 */
static void conn_mgr_mon_set_ready(int idx, bool ready, bool ready_ipv4, bool ready_ipv6)
{
	/* Clear and then update the L4-readiness bit */
	iface_states[idx] &= ~CONN_MGR_IF_READY;
	iface_states[idx] &= ~CONN_MGR_IF_READY_IPV4;
	iface_states[idx] &= ~CONN_MGR_IF_READY_IPV6;

	if (ready) {
		iface_states[idx] |= CONN_MGR_IF_READY;
	}

	if (ready_ipv4) {
		iface_states[idx] |= CONN_MGR_IF_READY_IPV4;
	}

	if (ready_ipv6) {
		iface_states[idx] |= CONN_MGR_IF_READY_IPV6;
	}
}

/**
 * @brief Refresh dns_unbound_families and dns_bound_families from the default resolver context.
 */
static void conn_mgr_mon_dns_snapshot(void)
{
	struct dns_resolve_context *ctx;

	if (!IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		return;
	}

	ctx = dns_resolve_get_default();
	dns_unbound_families = dns_resolve_get_server_families(ctx, 0);

	ARRAY_FOR_EACH(dns_bound_families, i) {
		dns_bound_families[i] = dns_resolve_get_server_families(ctx, i + 1);
	}
}

/**
 * @brief Check whether an interface provides name resolution.
 *
 * @param idx - index (in iface_states) of the iface to check.
 * @retval true if a DNS server can be reached through the iface.
 */
static bool conn_mgr_mon_iface_has_dns(int idx)
{
	uint32_t families;

	if ((iface_states[idx] & CONN_MGR_IF_READY) && socket_offload_dns_is_enabled()) {
		/* Name resolution is offloaded and its servers are not visible to us */
		return true;
	}

	if (!IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		return false;
	}

	families = dns_unbound_families | dns_bound_families[idx];

	return ((iface_states[idx] & CONN_MGR_IF_READY_IPV4) && (families & BIT(NET_AF_INET))) ||
	       ((iface_states[idx] & CONN_MGR_IF_READY_IPV6) && (families & BIT(NET_AF_INET6)));
}

/* Records a gain of flag. Must be called with conn_mgr_mon_lock held */
static void conn_mgr_mon_record_gain(uint32_t flag)
{
	/* Skip 0 on wrap: it means "nothing seen yet" */
	if (++gain_seq == 0U) {
		gain_seq = 1U;
	}
	flag_gain_seq[find_lsb_set(flag) - 1] = gain_seq;

	/* A gain of either IP family is a gain of any IP family */
	if ((flag & (CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6)) != 0U) {
		flag_gain_seq[find_lsb_set(CONN_MGR_READY_IP_ANY) - 1] = gain_seq;
	}
}

/* True if a flag in mask was gained since last_seen. Must be called with conn_mgr_mon_lock held */
static bool conn_mgr_mon_gained_since(uint32_t mask, uint32_t last_seen)
{
	while (mask != 0U) {
		int i = find_lsb_set(mask) - 1;

		/* Signed difference stays correct across counter wrap */
		if ((flag_gain_seq[i] != 0U) &&
		    ((last_seen == 0U) || ((int32_t)(flag_gain_seq[i] - last_seen) > 0))) {
			return true;
		}

		mask &= ~BIT(i);
	}

	return false;
}

/* Must be called with conn_mgr_mon_lock held */
static uint32_t conn_mgr_mon_ready_flags(void)
{
	uint32_t flags = 0U;

	if (last_ready_count_ipv4 > 0U) {
		flags |= CONN_MGR_READY_IPV4;
	}

	if (last_ready_count_ipv6 > 0U) {
		flags |= CONN_MGR_READY_IPV6;
	}

	if ((flags & (CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6)) != 0U) {
		flags |= CONN_MGR_READY_IP_ANY;
	}

	if (last_dns_ready) {
		flags |= CONN_MGR_READY_DNS;
	}

	return flags;
}

static void conn_mgr_mon_handle_update(void)
{
	int idx;
	bool has_ip;
	bool has_ipv6;
	bool has_ipv4;
	bool is_l4_ready;
	bool is_ipv6_ready;
	bool is_ipv4_ready;
	bool is_oper_up;
	bool was_l4_ready;
	bool was_ipv6_ready;
	bool was_ipv4_ready;
	bool is_ignored;
	uint64_t mgmt_event;
	int ready_count = 0;
	int ready_count_ipv4 = 0;
	int ready_count_ipv6 = 0;
	uint64_t l4_event = 0U;
	uint64_t ipv4_event = 0U;
	uint64_t ipv6_event = 0U;
	uint64_t dns_event = 0U;
	struct net_if *blame = NULL;
	struct net_if *blame_ipv4 = NULL;
	struct net_if *blame_ipv6 = NULL;
	bool dns_ready = false;
	uint32_t prev_gain_seq;

	conn_mgr_mon_dns_snapshot();

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	prev_gain_seq = gain_seq;

	for (idx = 0; idx < ARRAY_SIZE(iface_states); idx++) {
		if (iface_states[idx] == 0) {
			/* This interface is not used */
			continue;
		}

		/* Detect whether iface was previously considered ready */
		was_l4_ready	= iface_states[idx] & CONN_MGR_IF_READY;
		was_ipv6_ready	= iface_states[idx] & CONN_MGR_IF_READY_IPV6;
		was_ipv4_ready	= iface_states[idx] & CONN_MGR_IF_READY_IPV4;

		/* Collect iface readiness requirements */
		has_ipv6	= iface_states[idx] & CONN_MGR_IF_IPV6_SET;
		has_ipv4	= iface_states[idx] & CONN_MGR_IF_IPV4_SET;
		has_ip		= has_ipv6 || has_ipv4;
		is_oper_up	= iface_states[idx] & CONN_MGR_IF_UP;
		is_ignored	= iface_states[idx] & CONN_MGR_IF_IGNORED;

		/* Determine whether iface is currently considered ready */
		is_l4_ready	= is_oper_up && has_ip   && !is_ignored;
		is_ipv6_ready	= is_oper_up && has_ipv6 && !is_ignored;
		is_ipv4_ready	= is_oper_up && has_ipv4 && !is_ignored;

		/* Track ready iface count */
		if (is_l4_ready) {
			ready_count += 1;
		}
		if (is_ipv6_ready) {
			ready_count_ipv6 += 1;
		}
		if (is_ipv4_ready) {
			ready_count_ipv4 += 1;
		}

		/* If any states changed, track blame for possibly triggered events */
		if (was_l4_ready != is_l4_ready) {
			blame = conn_mgr_mon_get_if_by_index(idx);

			/* Per-interface L4 events */
			mgmt_event = is_l4_ready ? NET_EVENT_L4_IF_CONNECTED
						 : NET_EVENT_L4_IF_DISCONNECTED;
			net_mgmt_event_notify(mgmt_event, blame);
		}
		if (was_ipv6_ready != is_ipv6_ready) {
			blame_ipv6 = conn_mgr_mon_get_if_by_index(idx);
		}
		if (was_ipv4_ready != is_ipv4_ready) {
			blame_ipv4 = conn_mgr_mon_get_if_by_index(idx);
		}

		/* Every iface becoming ready is a gain, also when the family already was */
		if (is_ipv4_ready && !was_ipv4_ready) {
			conn_mgr_mon_record_gain(CONN_MGR_READY_IPV4);
		}
		if (is_ipv6_ready && !was_ipv6_ready) {
			conn_mgr_mon_record_gain(CONN_MGR_READY_IPV6);
		}

		/* Update readiness state flags with the (possibly) new values */
		conn_mgr_mon_set_ready(idx, is_l4_ready, is_ipv4_ready, is_ipv6_ready);

		if (conn_mgr_mon_iface_has_dns(idx)) {
			dns_ready = true;
		}
	}

	/* If the total number of ready ifaces changed, possibly send an event */
	if (ready_count != last_ready_count) {
		if (ready_count == 0) {
			/* We just lost connectivity */
			l4_event = NET_EVENT_L4_DISCONNECTED;
		} else if (last_ready_count == 0) {
			/* We just gained connectivity */
			l4_event = NET_EVENT_L4_CONNECTED;
		}
		last_ready_count = ready_count;
		last_blame = blame;
	}

	/* Same, but specifically for IPv4 */
	if (ready_count_ipv4 != last_ready_count_ipv4) {
		if (ready_count_ipv4 == 0) {
			/* We just lost IPv4 connectivity */
			ipv4_event = NET_EVENT_L4_IPV4_DISCONNECTED;
		} else if (last_ready_count_ipv4 == 0) {
			/* We just gained IPv4 connectivity */
			ipv4_event = NET_EVENT_L4_IPV4_CONNECTED;
		}
		last_ready_count_ipv4 = ready_count_ipv4;
		last_blame_ipv4 = blame_ipv4;
	}

	/* Same, but specifically for IPv6 */
	if (ready_count_ipv6 != last_ready_count_ipv6) {
		if (ready_count_ipv6 == 0) {
			/* We just lost IPv6 connectivity */
			ipv6_event = NET_EVENT_L4_IPV6_DISCONNECTED;
		} else if (last_ready_count_ipv6 == 0) {
			/* We just gained IPv6 connectivity */
			ipv6_event = NET_EVENT_L4_IPV6_CONNECTED;
		}
		last_ready_count_ipv6 = ready_count_ipv6;
		last_blame_ipv6 = blame_ipv6;
	}

	/* Notify DNS availability */
	if (dns_ready != last_dns_ready) {
		last_dns_ready = dns_ready;
		if (dns_ready) {
			conn_mgr_mon_record_gain(CONN_MGR_READY_DNS);
		}
		dns_event = dns_ready ? NET_EVENT_L4_DNS_READY : NET_EVENT_L4_DNS_LOST;
	}

	/* Notify only after all last_* state is updated,
	 * allowing direct callbacks to read up-to-date readiness
	 */
	if (l4_event != 0U) {
		net_mgmt_event_notify(l4_event, blame);
	}
	if (ipv4_event != 0U) {
		net_mgmt_event_notify(ipv4_event, blame_ipv4);
	}
	if (ipv6_event != 0U) {
		net_mgmt_event_notify(ipv6_event, blame_ipv6);
	}
	if (dns_event != 0U) {
		net_mgmt_event_notify(dns_event, NULL);
	}

	/* Only gains can satisfy a waiter */
	if (gain_seq != prev_gain_seq) {
		k_condvar_broadcast(&conn_mgr_mon_ready_gained);
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

static void conn_mgr_mon_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	conn_mgr_conn_init();

	k_mutex_unlock(&conn_mgr_mon_lock);

	NET_DBG("Connection Manager started");

	while (true) {
		/* Wait for changes */
		k_sem_take(&conn_mgr_mon_updated, K_FOREVER);

		/* Respond to changes */
		conn_mgr_mon_handle_update();
	}
}

void conn_mgr_mon_resend_status(void)
{
	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	if (last_ready_count == 0) {
		net_mgmt_event_notify(NET_EVENT_L4_DISCONNECTED, last_blame);
	} else {
		net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, last_blame);
	}

	if (last_ready_count_ipv6 == 0) {
		net_mgmt_event_notify(NET_EVENT_L4_IPV6_DISCONNECTED, last_blame_ipv6);
	} else {
		net_mgmt_event_notify(NET_EVENT_L4_IPV6_CONNECTED, last_blame_ipv6);
	}

	if (last_ready_count_ipv4 == 0) {
		net_mgmt_event_notify(NET_EVENT_L4_IPV4_DISCONNECTED, last_blame_ipv4);
	} else {
		net_mgmt_event_notify(NET_EVENT_L4_IPV4_CONNECTED, last_blame_ipv4);
	}

	net_mgmt_event_notify(last_dns_ready ? NET_EVENT_L4_DNS_READY : NET_EVENT_L4_DNS_LOST,
			      NULL);

	k_mutex_unlock(&conn_mgr_mon_lock);
}

bool conn_mgr_dns_is_ready(void)
{
	bool ret;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);
	/* Return the last reported value */
	ret = last_dns_ready;
	k_mutex_unlock(&conn_mgr_mon_lock);

	return ret;
}

uint32_t conn_mgr_get_readiness(void)
{
	uint32_t flags;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);
	flags = conn_mgr_mon_ready_flags();
	k_mutex_unlock(&conn_mgr_mon_lock);

	return flags;
}

net_sa_family_t conn_mgr_get_ready_addr_family(void)
{
	uint32_t ip = conn_mgr_get_readiness() & (CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6);

	if (ip == CONN_MGR_READY_IPV4) {
		return NET_AF_INET;
	}

	if (ip == CONN_MGR_READY_IPV6) {
		return NET_AF_INET6;
	}

	return NET_AF_UNSPEC;
}

int conn_mgr_wait_ready(uint32_t mask, uint32_t *last_seen, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	uint32_t none_seen = 0U;
	bool waited = false;
	int ret;

	if ((mask == 0U) || ((mask & ~(CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6 |
				       CONN_MGR_READY_DNS | CONN_MGR_READY_IP_ANY)) != 0U)) {
		return -EINVAL;
	}

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) && net_mgmt_is_event_context()) {
		LOG_ERR("Cannot wait for readiness from net_mgmt event context");
		return -EDEADLK;
	}

	/* NULL behaves like a pointer to 0, without write-back */
	if (last_seen == NULL) {
		last_seen = &none_seen;
	}

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	/* Recheck after every wake-up, including the one at timeout */
	while (true) {
		bool expired = sys_timepoint_expired(end);
		bool ready = (conn_mgr_mon_ready_flags() & mask) == mask;

		if (ready && (expired || conn_mgr_mon_gained_since(mask, *last_seen))) {
			ret = 0;
			break;
		}

		if (expired) {
			ret = -EAGAIN;
			break;
		}

		/* Log only when blocking on a mask not met, not for a retry delay while ready */
		if (!ready && !waited) {
			LOG_DBG("Waiting for network...");
			waited = true;
		}

		(void)k_condvar_wait(&conn_mgr_mon_ready_gained, &conn_mgr_mon_lock,
				     sys_timepoint_timeout(end));
	}

	*last_seen = gain_seq;

	k_mutex_unlock(&conn_mgr_mon_lock);

	if (waited && (ret == 0)) {
		LOG_DBG("Network ready");
	}

	return ret;
}

void conn_mgr_ignore_iface(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	if (!(iface_states[idx] & CONN_MGR_IF_IGNORED)) {
		/* Set ignored flag and mark state as changed */
		iface_states[idx] |= CONN_MGR_IF_IGNORED;
		k_sem_give(&conn_mgr_mon_updated);
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

void conn_mgr_watch_iface(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	if (iface_states[idx] & CONN_MGR_IF_IGNORED) {
		/* Clear ignored flag and mark state as changed */
		iface_states[idx] &= ~CONN_MGR_IF_IGNORED;
		k_sem_give(&conn_mgr_mon_updated);
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

bool conn_mgr_is_iface_ignored(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);

	bool ret = false;

	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	ret = iface_states[idx] & CONN_MGR_IF_IGNORED;

	k_mutex_unlock(&conn_mgr_mon_lock);

	return ret;
}

/**
 * @brief Check whether a provided iface uses the provided L2.
 *
 * @param iface - iface to check.
 * @param l2 - L2 to check. NULL will match offloaded ifaces.
 * @retval true if the iface uses the provided L2.
 * @retval false otherwise.
 */
static bool iface_uses_l2(struct net_if *iface, const struct net_l2 *l2)
{
	return	(!l2 && net_if_offload(iface)) ||
		(net_if_l2(iface) == l2);
}

void conn_mgr_ignore_l2(const struct net_l2 *l2)
{
	/* conn_mgr_ignore_iface already locks the mutex, but we lock it here too
	 * so that all matching ifaces are updated simultaneously.
	 */
	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (iface_uses_l2(iface, l2)) {
			conn_mgr_ignore_iface(iface);
		}
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

void conn_mgr_watch_l2(const struct net_l2 *l2)
{
	/* conn_mgr_watch_iface already locks the mutex, but we lock it here too
	 * so that all matching ifaces are updated simultaneously.
	 */
	k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (iface_uses_l2(iface, l2)) {
			conn_mgr_watch_iface(iface);
		}
	}

	k_mutex_unlock(&conn_mgr_mon_lock);
}

static int conn_mgr_mon_init(void)
{
	k_thread_create(&conn_mgr_mon_thread, conn_mgr_mon_stack,
			CONFIG_NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE,
			conn_mgr_mon_thread_fn,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&conn_mgr_mon_thread, "conn_mgr_monitor");

	return 0;
}

uint16_t conn_mgr_if_state(struct net_if *iface)
{
	int idx = conn_mgr_get_index_for_if(iface);
	uint16_t state = CONN_MGR_IF_STATE_INVALID;

	if (idx < CONN_MGR_IFACE_MAX) {
		k_mutex_lock(&conn_mgr_mon_lock, K_FOREVER);
		state = iface_states[idx];
		k_mutex_unlock(&conn_mgr_mon_lock);
	}

	return state;
}

SYS_INIT(conn_mgr_mon_init, APPLICATION, CONFIG_NET_CONNECTION_MANAGER_MONITOR_PRIORITY);
