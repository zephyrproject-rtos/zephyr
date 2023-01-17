/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for managing L2 layers that have separated post-admin-up connect
 *	  or associate phases
 */

#ifndef ZEPHYR_INCLUDE_NET_L2_CONNECTIVITY_H_
#define ZEPHYR_INCLUDE_NET_L2_CONNECTIVITY_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network Layer 2 connectivity management abstraction layer
 * @defgroup net_l2_connectivity Network L2 Connectivity Abstraction Layer
 * @ingroup networking
 * @{
 */

/**
 * @brief Network L2 connectivity API structure
 *
 * Used to provide an interface to connect parameters and procedures
 */
struct net_if_conn;
struct net_l2_conn_api {
	/**
	 * @brief When called, the L2 connectivity implementation should start attempting to
	 * establish connectivity for the bound iface pointed to by if_conn->iface.
	 *
	 * Must be non-blocking.
	 *
	 * Called by net_if_connect.
	 */
	int (*connect)(const struct net_if_conn *if_conn);

	/**
	 * @brief When called, the L2 connectivity implementation should stop disconnect and stop
	 * any in-progress attempts to establish connectivity for the bound iface pointed to by
	 * if_conn->iface.
	 *
	 * Must be non-blocking.
	 *
	 * Called by net_if_disconnect.
	 */
	int (*disconnect)(const struct net_if_conn *if_conn);

	/**
	 * @brief Called once for each iface that has been bound to this L2
	 * connectivity implementation.
	 *
	 * L2 connectivity implementations should use this callback to perform any required
	 * per-bound-iface initialization.
	 *
	 * Called during net_if_init, immediately after all ifaces have been initialized.
	 *
	 * Implementations may choose to gracefully handle invalid buffer lengths with partial
	 * writes, rather than raise errors, if deemed appropriate.
	 */
	void (*init)(const struct net_if_conn *if_conn);

	/**
	 * @brief Implementation callback for net_if_set_conn_opt.
	 *
	 * Calls to net_if_set_conn_opt on an iface will result in calls to this callback with
	 * the net_if_conn struct bound to that iface.
	 *
	 * It is up to the L2 connectivity implementation to interpret optname. Options can be
	 * specific to the bound iface (pointed to by if_conn->iface), or can apply to the whole
	 * L2 connectivity implementation.
	 *
	 * See its description in net_if.h for more details. set_opt implementations should conform
	 * to that description.
	 *
	 * Implementations may choose to gracefully handle invalid buffer lengths with partial
	 * reads, rather than raise errors, if deemed appropriate.
	 */
	int (*set_opt)(const struct net_if_conn *if_conn, int optname,
							  const void *optval,
							  size_t optlen);

	/**
	 * @brief Implementation callback for net_if_get_conn_opt.
	 *
	 * Calls to net_if_get_conn_opt on an iface will result in calls to this callback with
	 * the net_if_conn struct bound to that iface.
	 *
	 * It is up to the L2 connectivity implementation to interpret optname. Options can be
	 * specific to the bound iface (pointed to by if_conn->iface), or can apply to the whole
	 * L2 connectivity implementation.
	 *
	 * See its description in net_if.h for specifications. get_opt implementations should
	 * conform to that description.
	 */
	int (*get_opt)(const struct net_if_conn *if_conn, int optname,
							  void *optval,
							  size_t *optlen);
};

/**
 * @brief L2 Connectivity Implementation struct
 *	  Binds a connectivity implementation API to a defined L2 connectivity implementation.
 */
struct net_l2_conn {
	/* The L2 connectivity implementation associated with the network device */
	struct net_l2_conn_api *api;
};

/**
 * @brief Network interface connectivity binding structure
 *
 * Binds L2 connectivity implementation to an iface / network device.
 * Stores per-iface state for the connectivity implementation.
 */
struct net_if_conn {
	/* The network interface the connectivity implementation is bound to */
	struct net_if *iface;

	/* The connectivity implementation the network device is bound to */
	const struct net_l2_conn *impl;

	/* Pointer to private, per-iface connectivity context */
	void *ctx;

	/* Generic connectivity state - Persistence
	 * Indicates whether the L2 connectivity implementation should attempt to persist
	 * connectivity by automatically reconnecting after connection loss.
	 */
	bool persistence;

	/* Generic connectivity state - Timeout (seconds)
	 * Indicates to the L2 connectivity implementation how long it should attempt to
	 * establish for during a connection attempt before giving up and raising
	 * NET_EVENT_IF_CONNECTIVITY_TIMEOUT.
	 *
	 * The L2 connectivity should give up on establishing connectivity after this timeout,
	 * even if persistence is enabled.
	 */
	int timeout;
};

/** @cond INTERNAL_HIDDEN */
#define NET_IF_CONN_GET_NAME(dev_id, sfx)	__net_if_conn_##dev_id##_##sfx
#define NET_L2_CONN_GET_NAME(conn_id)		__net_l2_conn_##conn_id
#define NET_L2_CONN_GET_CTX_TYPE(conn_id)	conn_id##_CTX_TYPE
#define NET_L2_CONN_GET_DATA(dev_id, sfx)	__net_if_conn_data_##dev_id##_##sfx
/** @endcond */


/**
 * @brief Define an L2 connectivity implementation that can be bound to network devices.
 *
 * @param conn_id The name of the new L2 connectivity implementation
 * @param conn_api A pointer to a net_l2_conn_api struct
 */
#define NET_L2_CONNECTIVITY_DEFINE(conn_id, conn_api)				\
	const struct net_l2_conn NET_L2_CONN_GET_NAME(conn_id) = {		\
		.api = conn_api,						\
	};

/**
 * @brief Helper macro to make an L2 connectivity implementation publicly available.
 */
#define NET_L2_CONNECTIVITY_DECLARE_PUBLIC(conn_id)				\
	extern const struct net_l2_conn NET_L2_CONN_GET_NAME(conn_id)

/**
 * @brief Associate an L2 connectivity implementation with an existing network device instance
 *
 * @param dev_id Network device id.
 * @param inst Network device instance.
 * @param conn_id Name of the L2 connectivity implementation to associate.
 */
#define NET_DEVICE_INSTANCE_BIND_CONNECTIVITY(dev_id, inst, conn_id)				\
	static NET_L2_CONN_GET_CTX_TYPE(conn_id) NET_L2_CONN_GET_DATA(dev_id, inst);		\
	static STRUCT_SECTION_ITERABLE(net_if_conn,						\
				       NET_IF_CONN_GET_NAME(dev_id, inst)) = {			\
		.iface = NET_IF_GET(dev_id, inst),						\
		.impl = &(NET_L2_CONN_GET_NAME(conn_id)),					\
		.ctx = &(NET_L2_CONN_GET_DATA(dev_id, inst))					\
	};

/**
 * @brief Associate an L2 connectivity implementation with an existing network device
 *
 * @param dev_id Network device id.
 * @param conn_id Name of the L2 connectivity implementation to associate.
 */
#define NET_DEVICE_BIND_CONNECTIVITY(dev_id, conn_id) \
	NET_DEVICE_INSTANCE_BIND_CONNECTIVITY(dev_id, 0, conn_id)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_L2_CONNECTIVITY_H_ */
