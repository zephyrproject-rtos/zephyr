/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONN_MGR_H_
#define ZEPHYR_INCLUDE_CONN_MGR_H_

#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_pkt.h>

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
 * @typedef net_conn_mgr_online_checker_t
 * @brief Handler function that is called when network stack needs
 * to do a HTTPS request and needs to ask application to setup TLS
 * credentials and return them to the caller.
 *
 * @param iface Network interface where the online check is sent.
 * @param sec_tag_list Array of TLS security tags returned to the connection manager.
 * @param sec_tag_size Size of the returned tags array.
 * @param tls_hostname User needs to set this and it is returned to the connection
 *        manager. The value is used to set TLS_HOSTNAME socket option. If the value
 *        is empty, the TLS connection most like will not work. The returned pointer
 *        should point to a NULL terminated buffer that is valid when the callback
 *        returns.
 * @param url URL where the HTTPS request is sent.
 * @param host Hostname where the HTTPS request is sent.
 * @param port Port where the HTTPS request is sent.
 * @param dst Destination address where the HTTPS request is sent.
 * @param user_data A valid pointer to user data or NULL
 *
 * @retval <0 if this online check request is cancelled and not done.
 * @retval  0 the online check is done by the connection manager.
 */
typedef int (*net_conn_mgr_online_checker_t)(struct net_if *iface,
					     const sec_tag_t **sec_tag_list,
					     size_t *sec_tag_size,
					     const char **tls_hostname,
					     const char *url,
					     const char *host,
					     const char *port,
					     struct sockaddr *dst,
					     void *user_data);

/**
 * @brief Register online checker socket configuration callback.
 *
 * Application wishing to use Online Checker should register a setup
 * callback function. The connection manager will call this function
 * if HTTPS checker needs to be setup. Typically application would need
 * to setup TLS credentials etc. for the checker socket.
 *
 * @param cb Callback function to be called.
 * @param user_data Application specific user data is returned in callback.
 *
 * @return Return 0 if checker registration succeed, <0 otherwise.
 */
int conn_mgr_register_online_checker_cb(net_conn_mgr_online_checker_t cb,
					void *user_data);

/**
 * @}
 */

#else

#define conn_mgr_mon_resend_status(...)
#define conn_mgr_ignore_iface(...)
#define conn_mgr_watch_iface(...)
#define conn_mgr_ignore_l2(...)
#define conn_mgr_watch_l2(...)
#define conn_mgr_register_online_checker_cb(...)

#endif /* CONFIG_NET_CONNECTION_MANAGER */

/** @cond INTERNAL_HIDDEN */
#if defined(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CONNECTIVITY_CHECK)
/* Let online checker to decide if this network packet can be considered
 * when deciding if we are online or not.
 */
void conn_mgr_online_checker_update(struct net_pkt *pkt,
				    void *hdr,
				    sa_family_t family,
				    bool is_loopback);

/* Used when triggering online connectivity check manually from net-shell
 * and "net conn check" command. This is mainly for debugging purposes.
 */
bool conn_mgr_trigger_online_connectivity_check(void);
#else
static inline void conn_mgr_online_checker_update(struct net_pkt *pkt,
						  void *hdr,
						  sa_family_t family,
						  bool is_loopback)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(family);
	ARG_UNUSED(is_loopback);
}

static inline bool conn_mgr_trigger_online_connectivity_check(void)
{
	return false;
}

#endif
/** @endcond */

/** Possible choices for the online connectivity check. */
enum net_conn_mgr_online_check_type {
	/** Use ICMP Echo-Request (ping). This is the default */
	NET_CONN_MGR_ONLINE_CHECK_PING = 0,
	/** Use HTTP(S) GET request. */
	NET_CONN_MGR_ONLINE_CHECK_HTTP,
};

/**
 * @brief Set how the online connectivity check should be done.
 * Default is ICMP Echo-Request i.e., ping.
 *
 * @param type Online connectivity check strategy (ping or http(s))
 *
 */
#if defined(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CONNECTIVITY_CHECK)
void conn_mgr_set_online_check_strategy(enum net_conn_mgr_online_check_type type);
#else
static inline void conn_mgr_set_online_check_strategy(enum net_conn_mgr_online_check_type type)
{
	ARG_UNUSED(type);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONN_MGR_H_ */
