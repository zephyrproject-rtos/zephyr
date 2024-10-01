/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for monitoring network connections and interfaces.
 */

#ifndef ZEPHYR_INCLUDE_CONN_MGR_H_
#define ZEPHYR_INCLUDE_CONN_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_CONNECTION_MANAGER) || defined(__DOXYGEN__)

/**
 * @brief Connection Manager API
 * @defgroup conn_mgr Connection Manager API
 * @ingroup networking
 * @{
 */

struct net_if;
struct net_l2;

/**
 * @brief Resend either NET_L4_CONNECTED or NET_L4_DISCONNECTED depending on whether connectivity
 * is currently available.
 */
void conn_mgr_mon_resend_status(void);

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

/**
 * @}
 */

#else

#define conn_mgr_mon_resend_status(...)
#define conn_mgr_ignore_iface(...)
#define conn_mgr_watch_iface(...)
#define conn_mgr_ignore_l2(...)
#define conn_mgr_watch_l2(...)

#endif /* CONFIG_NET_CONNECTION_MANAGER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONN_MGR_H_ */
