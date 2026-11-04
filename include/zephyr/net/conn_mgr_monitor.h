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
#define CONN_MGR_READY_IPV4   BIT(0)
/** An iface is ready with IPv6, see @ref NET_EVENT_L4_IPV6_CONNECTED. */
#define CONN_MGR_READY_IPV6   BIT(1)
/** @ref CONN_MGR_READY_IPV4 or @ref CONN_MGR_READY_IPV6 is set. */
#define CONN_MGR_READY_IP_ANY BIT(2)
/**
 * A unicast DNS server is configured for a ready IP family, or name resolution is offloaded,
 * see @ref conn_mgr_dns_is_ready.
 *
 * mDNS and LLMNR do not use a unicast DNS server. Names resolved with them can work
 * while this flag is cleared, so do not wait for it when only such names are used.
 */
#define CONN_MGR_READY_DNS    BIT(3)
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
 * @brief Get the current readiness flags.
 *
 * Without @kconfig{CONFIG_NET_CONNECTION_MANAGER}, readiness is not tracked and all flags are set.
 *
 * The function takes an internal lock and can block for a short time on it.
 *
 * @return Bitmask of @ref CONN_MGR_READY_FLAGS "readiness flags" currently set.
 */
uint32_t conn_mgr_get_readiness(void);

/**
 * @brief Wait until it is time to try to connect.
 *
 * Returns 0, meaning "try to connect now", when all flags in @p mask are ready and one of
 * the following is true:
 * - any flag in @p mask was gained after @p last_seen,
 * - @p timeout expired.
 *
 * A flag is gained when an iface becomes ready for an IP family, or when name resolution
 * becomes available.
 *
 * Set @p last_seen to 0 before the first call and do not change it afterwards.
 * A retry loop then waits for @p timeout or a gain between attempts.
 * A NULL @p last_seen acts like 0 and is not written back. Use it only for a one-time wait.
 *
 * Without @kconfig{CONFIG_NET_CONNECTION_MANAGER}, the network is considered ready.
 *
 * In a net_mgmt event callback or on the work queue delivering net_mgmt events, @p timeout
 * must be K_NO_WAIT.
 *
 * @note @p timeout limits only the wait for readiness. The function always takes an
 *       internal lock and can block for a short time on it, also with K_NO_WAIT.
 *       For this reason it cannot be called from an ISR, even with K_NO_WAIT.
 *
 * @note In a loop that retries on -EAGAIN, do not use K_NO_WAIT or a zero timeout.
 *       Otherwise, the loop busy-spins while the network is not ready.
 *
 * @param mask Bitwise OR of @ref CONN_MGR_READY_FLAGS "readiness flags" to wait for.
 * @param last_seen Last readiness change seen by the caller, or NULL.
 *                  Updated on return, unless -EINVAL or -EDEADLK is returned.
 * @param timeout Maximum time to wait.
 *
 * @retval 0 All flags in @p mask are ready. Try to connect now.
 * @retval -EAGAIN A flag in @p mask was not ready when @p timeout expired.
 * @retval -EINVAL @p mask is 0 or contains an unknown flag.
 * @retval -EDEADLK Called with a non-zero @p timeout from net_mgmt event context.
 */
int conn_mgr_wait_ready(uint32_t mask, uint32_t *last_seen, k_timeout_t timeout);

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
static inline uint32_t conn_mgr_get_readiness(void)
{
	return CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6 | CONN_MGR_READY_DNS |
	       CONN_MGR_READY_IP_ANY;
}

static inline int conn_mgr_wait_ready(uint32_t mask, uint32_t *last_seen, k_timeout_t timeout)
{
	if ((mask == 0U) || ((mask & ~(CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6 |
				       CONN_MGR_READY_DNS | CONN_MGR_READY_IP_ANY)) != 0U)) {
		return -EINVAL;
	}

	if (last_seen == NULL) {
		return 0;
	}

	/* Ready since boot: the first call returns at once, later ones after timeout */
	if (*last_seen == 0U) {
		*last_seen = 1U;
		return 0;
	}

	k_sleep(timeout);

	return 0;
}

#endif /* CONFIG_NET_CONNECTION_MANAGER */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_CONN_MGR_MONITOR_H_ */
