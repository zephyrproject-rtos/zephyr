/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * SNMP transport adapter: one Zephyr socket-service descriptor per agent instance slot.
 *
 * Design contract section 4. Zephyr's net_socket_service_register() wipes every event
 * slot of a descriptor and assigns a single user_data to all file descriptors passed in
 * one call (subsys/net/lib/sockets/sockets_service.c), so a process-wide shared
 * descriptor cannot give each agent its own identity, and descriptors must be link-time
 * objects inside the net_socket_service_desc section. The consequence is a bounded
 * ladder of pre-allocated descriptors, one per slot, and the owner is handed to
 * register() as user_data so the callback recovers it from the event copy — no fd scan,
 * no default singleton.
 */

#include <zephyr/net/snmp/snmp_core.h>
#include "snmp_internal.h"
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)

LOG_MODULE_REGISTER(snmp_service, CONFIG_SNMP_LOG_LEVEL);

/* ---------------------------------------------------------------------------
 * Slot inventory (contract 4.2)
 * ------------------------------------------------------------------------ */

#define SNMP_SVC_MAX_SLOTS 8

#ifndef CONFIG_SNMP_MAX_INSTANCES
#define CONFIG_SNMP_MAX_INSTANCES 1
#endif

BUILD_ASSERT(CONFIG_SNMP_MAX_INSTANCES >= 1 && CONFIG_SNMP_MAX_INSTANCES <= SNMP_SVC_MAX_SLOTS);

#ifdef CONFIG_NET_IPV6
#define SNMP_SVC_FDS 2 /* pev[0] = IPv4, pev[1] = IPv6 */
#else
#define SNMP_SVC_FDS 1
#endif

/* Every descriptor present in the section costs SNMP_SVC_FDS poll entries in the
 * socket-service thread's budget even when never registered, and the thread reserves
 * one extra entry for its eventfd (sockets_service.c). Hence the #if ladder below: the
 * SNMP share is exactly CONFIG_SNMP_MAX_INSTANCES * SNMP_SVC_FDS (contract 4.6).
 */
BUILD_ASSERT(CONFIG_ZVFS_POLL_MAX >= CONFIG_SNMP_MAX_INSTANCES * SNMP_SVC_FDS + 1);

static void snmp_service_cb(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_0, snmp_service_cb, SNMP_SVC_FDS);
#if CONFIG_SNMP_MAX_INSTANCES >= 2
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_1, snmp_service_cb, SNMP_SVC_FDS);
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 3
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_2, snmp_service_cb, SNMP_SVC_FDS);
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 4
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_3, snmp_service_cb, SNMP_SVC_FDS);
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 5
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_4, snmp_service_cb, SNMP_SVC_FDS);
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 6
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_5, snmp_service_cb, SNMP_SVC_FDS);
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 7
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_6, snmp_service_cb, SNMP_SVC_FDS);
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 8
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(snmp_svc_7, snmp_service_cb, SNMP_SVC_FDS);
#endif

static const struct net_socket_service_desc *const s_slot_desc[CONFIG_SNMP_MAX_INSTANCES] = {
	&snmp_svc_0,
#if CONFIG_SNMP_MAX_INSTANCES >= 2
	&snmp_svc_1,
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 3
	&snmp_svc_2,
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 4
	&snmp_svc_3,
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 5
	&snmp_svc_4,
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 6
	&snmp_svc_5,
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 7
	&snmp_svc_6,
#endif
#if CONFIG_SNMP_MAX_INSTANCES >= 8
	&snmp_svc_7,
#endif
};

/*
 * Slot allocator. Justified exception to the no-file-scope-state invariant (contract
 * 4.3 / 9.3): it holds no SNMP protocol, configuration or counter state, starts empty,
 * is guarded by its own mutex, holds pointers only (no agent payload) and each entry is
 * owned by exactly the instance that acquired it. The descriptors above are
 * Zephyr-mandated static infrastructure; s_slot_lock guards s_slot_owner and nothing
 * else, and is held for a slot find/set/clear only — never across a syscall.
 */
static struct snmp_agent *s_slot_owner[CONFIG_SNMP_MAX_INSTANCES];
static K_MUTEX_DEFINE(s_slot_lock);

/* ---------------------------------------------------------------------------
 * Per-instance rate limiter (contract 5)
 * ------------------------------------------------------------------------ */

#ifdef CONFIG_SNMP_RATE_LIMIT
#ifndef CONFIG_SNMP_RATE_LIMIT_RPS
#define CONFIG_SNMP_RATE_LIMIT_RPS 20
#endif
#ifndef CONFIG_SNMP_RATE_LIMIT_BURST
#define CONFIG_SNMP_RATE_LIMIT_BURST 5
#endif

/* Called with agent->lock held: the bucket needs no atomics, all RX is serialised on the
 * socket-service thread and the lock also serialises against API callers.
 */
static bool snmp_rate_check(struct snmp_agent *agent)
{
	int64_t now = k_uptime_get();
	int64_t elapsed_ms = now - agent->rate_last_refill;
	uint32_t cap;
	uint32_t refill;

	if (elapsed_ms < 0) {
		elapsed_ms = 0; /* uptime wrap: treat as no elapsed time */
	}

	refill = (uint32_t)(elapsed_ms * CONFIG_SNMP_RATE_LIMIT_RPS / 1000);
	if (refill > 0) {
		cap = CONFIG_SNMP_RATE_LIMIT_BURST + CONFIG_SNMP_RATE_LIMIT_RPS;
		agent->rate_tokens += refill;
		if (agent->rate_tokens > cap) {
			agent->rate_tokens = cap;
		}
		agent->rate_last_refill = now;
	}

	if (agent->rate_tokens > 0) {
		agent->rate_tokens--;
		return true;
	}

	return false;
}
#endif /* CONFIG_SNMP_RATE_LIMIT */

/* ---------------------------------------------------------------------------
 * Binding helpers (contract 4.5)
 * ------------------------------------------------------------------------ */

/* Shared with the trap engine (declared in src/snmp_internal.h). */
int snmp_sock_bind_to_iface(int sock, struct net_if *iface)
{
	struct net_ifreq ifr;

	if (sock < 0) {
		return -EINVAL;
	}
	if (iface == NULL) {
		return 0;
	}

	memset(&ifr, 0, sizeof(ifr));
	/* net_if_get_device() can be NULL for virtual interfaces; the name accessor
	 * is the portable one and also fixes the old IP-only code path. */
	int name_ret = net_if_get_name(iface, ifr.ifr_name, sizeof(ifr.ifr_name));

	if (name_ret < 0 || ifr.ifr_name[0] == '\0') {
		return -ENODEV;
	}

	return zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
}

#if defined(CONFIG_NET_IPV4)
/* Fallback for configurations where SO_BINDTODEVICE is not supported: bind to the
 * interface's first assigned unicast address instead of INADDR_ANY.
 * Returns true when an address was produced.
 */
static bool snmp_bind_addr_from_iface(const struct snmp_agent *agent, struct sockaddr_in *addr)
{
	int i;

	if (agent->bound_iface == NULL || agent->bound_iface->config.ip.ipv4 == NULL) {
		return false;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (agent->bound_iface->config.ip.ipv4->unicast[i].ipv4.is_used &&
		    agent->bound_iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr.s_addr !=
			    0) {
			addr->sin_addr =
				agent->bound_iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr;
			return true;
		}
	}

	return false;
}

static int snmp_open_bind_v4(struct snmp_agent *agent)
{
	struct sockaddr_in bind_addr;
	int sock;
	int err;

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		return -errno;
	}

	if (agent->bound_iface != NULL) {
		int bind_ret = snmp_sock_bind_to_iface(sock, agent->bound_iface);

		/* A requested interface is an isolation requirement, not a hint. */
		if (bind_ret < 0) {
			LOG_ERR("IPv4 interface binding failed (%d)", bind_ret);
			zsock_close(sock);
			return bind_ret;
		}
	}

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(agent->port); /* per-instance port, not Kconfig */

	if (!agent->bound_iface) {
		bind_addr.sin_addr.s_addr = INADDR_ANY;
	} else if (!snmp_bind_addr_from_iface(agent, &bind_addr)) {
		/* SO_BINDTODEVICE already constrains this socket. This is safe while
		 * waiting for DHCP to assign the selected interface an address. */
		bind_addr.sin_addr.s_addr = INADDR_ANY;
	}

	if (zsock_bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		err = -errno;
		zsock_close(sock);
		return err;
	}

	agent->sock_v4 = sock;
	return 0;
}
#endif /* CONFIG_NET_IPV4 */

#ifdef CONFIG_NET_IPV6
static int snmp_open_bind_v6(struct snmp_agent *agent)
{
	struct sockaddr_in6 bind_addr;
	int sock;
	int err;

	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		return -errno;
	}

	if (agent->bound_iface != NULL) {
		int bind_ret = snmp_sock_bind_to_iface(sock, agent->bound_iface);

		if (bind_ret < 0) {
			LOG_ERR("IPv6 interface binding failed (%d)", bind_ret);
			zsock_close(sock);
			return bind_ret;
		}
	}

	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin6_family = AF_INET6;
	bind_addr.sin6_port = htons(agent->port);

	if (zsock_bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		err = -errno;
		zsock_close(sock);
		return err;
	}

	agent->sock_v6 = sock;
	return 0;
}
#endif /* CONFIG_NET_IPV6 */

/* ---------------------------------------------------------------------------
 * Service callback (contract 4.3 / 4.4)
 * ------------------------------------------------------------------------ */

static void snmp_service_cb(struct net_socket_service_event *pev)
{
	struct snmp_agent *agent;
	struct sockaddr_storage client_addr;
	socklen_t addr_len = sizeof(client_addr);
	size_t resp_len = 0;
	ssize_t len;

	if (!pev || pev->event.fd < 0 || !(pev->event.revents & ZSOCK_POLLIN)) {
		return;
	}

	/* Exact owner: register() wrote it and the service thread copied the event.
	 * No fd scan, no default singleton.
	 */
	agent = pev->user_data;
	if (!agent) {
		return;
	}

	k_mutex_lock(&agent->lock, K_FOREVER);

	/* Re-check after the barrier: snmp_agent_stop() publishes STOPPING before
	 * deregistering and serialises with us on this same lock, so a stale event that
	 * raced in is dropped here rather than touching closed sockets or freed scratch.
	 */
	if (agent->state != SNMP_AGENT_RUNNING) {
		k_mutex_unlock(&agent->lock);
		return;
	}

	/* Defense in depth: an event must arrive through this agent's own descriptor
	 * (contract 4.3). Skipped if the instance holds no slot, which is already the
	 * case for a stale event that raced stop().
	 */
	if (agent->svc_slot < CONFIG_SNMP_MAX_INSTANCES) {
		__ASSERT(pev->svc == s_slot_desc[agent->svc_slot],
			 "slot %p event routed to agent %p", (void *)pev->svc, (void *)agent);
	}

#ifdef CONFIG_SNMP_RATE_LIMIT
	if (!snmp_rate_check(agent)) {
		/* Exactly one increment, on the receiving agent (contract 6.2). */
		snmp_counter_inc(&agent->counters.snmpSilentDrops);
		k_mutex_unlock(&agent->lock);
		return; /* Silently drop — don't respond */
	}
#endif

	len = zsock_recvfrom(pev->event.fd, agent->rx_buf, sizeof(agent->rx_buf), 0,
			     (struct sockaddr *)&client_addr, &addr_len);
	if (len > 0) {
		int ret = snmp_agent_process_pdu(agent, agent->rx_buf, (size_t)len, agent->tx_buf,
						 sizeof(agent->tx_buf), &resp_len);
		if (ret == 0 && resp_len > 0) {
			/* Lock held across recv/process/send: required to protect this
			 * instance's rx_buf/tx_buf, and the service thread is serial anyway.
			 */
			ssize_t sent =
				zsock_sendto(pev->event.fd, agent->tx_buf, resp_len, 0,
					     (const struct sockaddr *)&client_addr, addr_len);
			if (sent < 0) {
				LOG_WRN("sendto failed (%d)", -errno);
			}
		}
	}

	k_mutex_unlock(&agent->lock);
}

/* ---------------------------------------------------------------------------
 * Attach / detach (contract 4.3 / 4.4)
 * ------------------------------------------------------------------------ */

static int snmp_reserve_slot(struct snmp_agent *agent)
{
	int slot = -EBUSY;
	int i;

	k_mutex_lock(&s_slot_lock, K_FOREVER);
	for (i = 0; i < CONFIG_SNMP_MAX_INSTANCES; i++) {
		if (s_slot_owner[i] == NULL) {
			s_slot_owner[i] = agent;
			slot = i;
			break;
		}
	}
	k_mutex_unlock(&s_slot_lock);

	if (slot < 0) {
		LOG_ERR("No free SNMP socket-service slot (CONFIG_SNMP_MAX_INSTANCES=%d)",
			CONFIG_SNMP_MAX_INSTANCES);
		return slot;
	}

	agent->svc_slot = (uint8_t)slot;
	return slot;
}

static void snmp_release_slot(struct snmp_agent *agent)
{
	uint8_t slot = agent->svc_slot;

	if (slot >= CONFIG_SNMP_MAX_INSTANCES) {
		return;
	}

	k_mutex_lock(&s_slot_lock, K_FOREVER);
	if (s_slot_owner[slot] == agent) {
		s_slot_owner[slot] = NULL;
	}
	k_mutex_unlock(&s_slot_lock);

	agent->svc_slot = SNMP_SLOT_NONE;
}

static void snmp_close_sockets(struct snmp_agent *agent)
{
	if (agent->sock_v4 >= 0) {
		zsock_close(agent->sock_v4);
		agent->sock_v4 = -1;
	}
#ifdef CONFIG_NET_IPV6
	if (agent->sock_v6 >= 0) {
		zsock_close(agent->sock_v6);
		agent->sock_v6 = -1;
	}
#endif
}

int snmp_service_attach(struct snmp_agent *agent)
{
	struct zsock_pollfd pfds[SNMP_SVC_FDS];
	int slot;
	int n = 0;
	int ret;

	if (!agent) {
		return -EINVAL;
	}

	slot = snmp_reserve_slot(agent);
	if (slot < 0) {
		return slot;
	}

	agent->sock_v4 = -1;
	agent->sock_v6 = -1;

#if defined(CONFIG_NET_IPV4)
	ret = snmp_open_bind_v4(agent);
	if (ret < 0) {
		LOG_WRN("IPv4 listener bind failed (%d) on port %u", ret, agent->port);
	}
#endif
#ifdef CONFIG_NET_IPV6
	ret = snmp_open_bind_v6(agent);
	if (ret < 0) {
		/* Tolerant, as before: keep running IPv4-only (contract 4.5). */
		LOG_WRN("IPv6 listener bind failed (%d), continuing IPv4-only", ret);
	}
#endif

	if (agent->sock_v4 >= 0) {
		pfds[n].fd = agent->sock_v4;
		pfds[n].events = ZSOCK_POLLIN;
		n++;
	}
#ifdef CONFIG_NET_IPV6
	if (agent->sock_v6 >= 0) {
		pfds[n].fd = agent->sock_v6;
		pfds[n].events = ZSOCK_POLLIN;
		n++;
	}
#endif

	if (n == 0) {
		/* Nothing to listen on: report why instead of pretending success. */
		snmp_release_slot(agent);
#if !defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
		return -EAFNOSUPPORT;
#else
		return -ENETDOWN;
#endif
	}

	/* pev_len stays SNMP_SVC_FDS; an unused entry simply keeps fd == -1 and is never
	 * polled (contract 4.5).
	 */
	ret = net_socket_service_register(s_slot_desc[slot], pfds, n, agent);
	if (ret < 0) {
		LOG_ERR("net_socket_service_register failed (%d)", ret);
		snmp_close_sockets(agent);
		snmp_release_slot(agent);
		return ret;
	}

	char if_name[16] = "any";

	if (agent->bound_iface != NULL) {
		if (net_if_get_name(agent->bound_iface, if_name, sizeof(if_name)) < 0) {
			strncpy(if_name, "?", sizeof(if_name) - 1);
		}
	}

	LOG_INF("SNMP agent listening on UDP port %u (%d socket%s, iface %s)", agent->port, n,
		n > 1 ? "s" : "", if_name);
	return 0;
}

int snmp_service_detach(struct snmp_agent *agent)
{
	uint8_t slot;

	if (!agent) {
		return -EINVAL;
	}

	slot = agent->svc_slot;

	/* Ordering matters (contract 4.4):
	 *  1. state is already STOPPING (plain store by snmp_agent_stop before this call);
	 *  2. deregister wipes every pev slot of this descriptor under the service mutex
	 *     and kicks its eventfd, so no NEW dispatch can resolve this fd again;
	 *  3. take the agent lock as the QUIESCE BARRIER — the service thread polls with
	 *     its own mutex released, so an in-flight callback for this agent is still
	 *     running; waiting for the lock drains it, and its post-lock state check
	 *     discards a stale event;
	 *  4. close the sockets;
	 *  5. release the slot and publish STOPPED;
	 *  6. release the barrier.
	 * Caller-owned static storage means a late callback can never dangle: the struct
	 * outlives everything.
	 */
	if (slot < CONFIG_SNMP_MAX_INSTANCES) {
		int uret = net_socket_service_unregister(s_slot_desc[slot]);

		if (uret < 0 && uret != -ENOENT) {
			LOG_WRN("net_socket_service_unregister failed (%d)", uret);
		}
	}

	k_mutex_lock(&agent->lock, K_FOREVER);

	snmp_close_sockets(agent);

	if (slot < CONFIG_SNMP_MAX_INSTANCES) {
		agent->svc_slot = SNMP_SLOT_NONE;
		agent->state = SNMP_AGENT_STOPPED;
		k_mutex_lock(&s_slot_lock, K_FOREVER);
		if (s_slot_owner[slot] == agent) {
			s_slot_owner[slot] = NULL;
		}
		k_mutex_unlock(&s_slot_lock);
	} else {
		agent->svc_slot = SNMP_SLOT_NONE;
		agent->state = SNMP_AGENT_STOPPED;
	}

	k_mutex_unlock(&agent->lock);
	return 0;
}

#endif /* CONFIG_SNMP_AGENT */
