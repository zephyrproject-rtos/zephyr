/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for monitoring network connections and interfaces.
 */

#ifndef ZEPHYR_INCLUDE_NET_CONN_MGR_MONITOR_H_
#define ZEPHYR_INCLUDE_NET_CONN_MGR_MONITOR_H_

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connection Manager API
 * @defgroup conn_mgr Connection Manager API
 * @since 2.0
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/**
 * @name Readiness flags
 * @anchor CONN_MGR_READY_FLAGS
 * @{
 */
/** An iface is ready with IPv4, see @ref NET_EVENT_L4_IPV4_CONNECTED. */
#define CONN_MGR_READY_IPV4 BIT(0)
/** An iface is ready with IPv6, see @ref NET_EVENT_L4_IPV6_CONNECTED. */
#define CONN_MGR_READY_IPV6 BIT(1)
/** Name resolution is available, see @ref conn_mgr_dns_is_ready. */
#define CONN_MGR_READY_DNS  BIT(2)
/** @} */

#if defined(CONFIG_NET_CONNECTION_MANAGER) || defined(__DOXYGEN__)

struct net_if;
struct net_l2;

/**
 * @brief Resend L4 connectivity and DNS readiness events depending on current status.
 *
 * Events covered:
 * - NET_EVENT_L4_CONNECTED / NET_EVENT_L4_DISCONNECTED
 * - NET_EVENT_L4_IPV4_CONNECTED / NET_EVENT_L4_IPV4_DISCONNECTED
 * - NET_EVENT_L4_IPV6_CONNECTED / NET_EVENT_L4_IPV6_DISCONNECTED
 * - NET_EVENT_L4_DNS_READY / NET_EVENT_L4_DNS_LOST
 */
void conn_mgr_mon_resend_status(void);

/**
 * @brief Check whether name resolution is currently available.
 *
 * A unicast DNS server being configured does not mean that it can be reached.
 * Requires CONFIG_DNS_RESOLVER or offloaded name resolution.
 *
 * @retval true A unicast DNS server is configured for a ready iface and address family,
 *              or name resolution is offloaded and an iface is ready.
 * @retval false Name resolution is not available.
 */
bool conn_mgr_dns_is_ready(void);

/**
 * @brief Get the current network readiness.
 *
 * @param gen If not NULL, set to an opaque snapshot of the readiness changes, to pass to
 *            @ref conn_mgr_wait_ready_change.
 *
 * Without CONFIG_NET_CONNECTION_MANAGER, readiness is not tracked and all flags are set.
 *
 * Must not be called from an ISR.
 *
 * @return Bitmask of @ref CONN_MGR_READY_FLAGS "readiness flags" currently set.
 */
uint32_t conn_mgr_get_readiness(uint32_t *gen);

/**
 * @brief Wait until the network is ready.
 *
 * The network is ready when at least one of the IP families in @p mask is ready and, if
 * @ref CONN_MGR_READY_DNS is in @p mask, name resolution is available. The current state is
 * checked, so this returns immediately if the network is already ready.
 * Without CONFIG_NET_CONNECTION_MANAGER, the network is considered ready.
 *
 * Must not be called from an ISR. Must not be called with a non-zero timeout from a net_mgmt
 * event callback or from the work queue delivering net_mgmt events, which is the system work
 * queue with CONFIG_NET_MGMT_EVENT_SYSTEM_WORKQUEUE.
 *
 * @param mask Bitmask of @ref CONN_MGR_READY_FLAGS "readiness flags" to wait for.
 * @param timeout Maximum time to wait. Use K_NO_WAIT to only check the current state.
 *
 * @retval 0 The network is ready.
 * @retval -EAGAIN The network did not become ready within @p timeout.
 * @retval -EINVAL @p mask contains no IP family.
 */
int conn_mgr_wait_ready(uint32_t mask, k_timeout_t timeout);

/**
 * @brief Wait until the network is ready and has changed since a snapshot.
 *
 * Like @ref conn_mgr_wait_ready, but additionally waits until a flag in @p mask became ready
 * after @p gen was read with @ref conn_mgr_get_readiness. This includes an interface becoming
 * ready for a family that was already ready, and a family or name resolution that was lost
 * and regained.
 *
 * Use it as the delay between connection attempts: the wait lasts for @p timeout, and ends
 * early on a readiness change, with @p gen read before the attempt.
 * Without CONFIG_NET_CONNECTION_MANAGER, no change is seen and this waits for @p timeout.
 *
 * Must not be called from an ISR. Must not be called with a non-zero timeout from a net_mgmt
 * event callback or from the work queue delivering net_mgmt events, which is the system work
 * queue with CONFIG_NET_MGMT_EVENT_SYSTEM_WORKQUEUE.
 *
 * @param mask Bitmask of @ref CONN_MGR_READY_FLAGS "readiness flags" to wait for.
 * @param gen Snapshot read with @ref conn_mgr_get_readiness.
 * @param timeout Maximum time to wait. Use K_NO_WAIT to only check the current state.
 *
 * @retval 0 The network is ready and changed.
 * @retval -EAGAIN The network did not become ready or change within @p timeout.
 * @retval -EINVAL @p mask contains no IP family.
 */
int conn_mgr_wait_ready_change(uint32_t mask, uint32_t gen, k_timeout_t timeout);

/**
 * @brief Mark an iface to be ignored by conn_mgr.
 *
 * Ignoring an iface forces conn_mgr to consider it unready/disconnected.
 *
 * This means that events related to the iface connecting/disconnecting will not be fired,
 * and if the iface was connected before being ignored, events will be fired as though it
 * disconnected at that moment.
 *
 * @param iface iface to be ignored.
 */
void conn_mgr_ignore_iface(struct net_if *iface);

/**
 * @brief Watch (stop ignoring) an iface.
 *
 * conn_mgr will no longer be forced to consider the iface unreadly/disconnected.
 *
 * Events related to the iface connecting/disconnecting will no longer be blocked,
 * and if the iface was connected before being watched, events will be fired as though
 * it connected in that moment.
 *
 * All ifaces default to watched at boot.
 *
 * @param iface iface to no longer ignore.
 */
void conn_mgr_watch_iface(struct net_if *iface);

/**
 * @brief Check whether the provided iface is currently ignored.
 *
 * @param iface The iface to check.
 * @retval true if the iface is being ignored by conn_mgr.
 * @retval false if the iface is being watched by conn_mgr.
 */
bool conn_mgr_is_iface_ignored(struct net_if *iface);

/**
 * @brief Mark an L2 to be ignored by conn_mgr.
 *
 * This is a wrapper for conn_mgr_ignore_iface that ignores all ifaces that use the L2.
 *
 * @param l2 L2 to be ignored.
 */
void conn_mgr_ignore_l2(const struct net_l2 *l2);

/**
 * @brief Watch (stop ignoring) an L2.
 *
 *  This is a wrapper for conn_mgr_watch_iface that watches all ifaces that use the L2.
 *
 * @param l2 L2 to watch.
 */
void conn_mgr_watch_l2(const struct net_l2 *l2);

#else

#define conn_mgr_mon_resend_status(...)
#define conn_mgr_ignore_iface(...)
#define conn_mgr_watch_iface(...)
#define conn_mgr_ignore_l2(...)
#define conn_mgr_watch_l2(...)

/* Readiness is not tracked: everything is assumed ready */
static inline uint32_t conn_mgr_get_readiness(uint32_t *gen)
{
	if (gen != NULL) {
		*gen = 0U;
	}

	return CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6 | CONN_MGR_READY_DNS;
}

static inline int conn_mgr_wait_ready(uint32_t mask, k_timeout_t timeout)
{
	ARG_UNUSED(timeout);

	if ((mask & (CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6)) == 0U) {
		return -EINVAL;
	}

	return 0;
}

static inline int conn_mgr_wait_ready_change(uint32_t mask, uint32_t gen, k_timeout_t timeout)
{
	ARG_UNUSED(gen);

	if ((mask & (CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6)) == 0U) {
		return -EINVAL;
	}

	k_sleep(timeout);

	return -EAGAIN;
}

#endif /* CONFIG_NET_CONNECTION_MANAGER */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_CONN_MGR_MONITOR_H_ */
