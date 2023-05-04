/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for defining generic interfaces for configuring and firing network association
 *	  routines on network devices that support it.
 */

#ifndef ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_H_
#define ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connection Manager Connectivity API
 * @defgroup conn_mgr_connectivity Connection Manager Connectivity API
 * @ingroup networking
 * @{
 */


/** @cond INTERNAL_HIDDEN */

/* Connectivity Events */
#define _NET_MGMT_CONN_LAYER			NET_MGMT_LAYER(NET_MGMT_LAYER_L2)
#define _NET_MGMT_CONN_CODE			NET_MGMT_LAYER_CODE(0x207)
#define _NET_MGMT_CONN_BASE			(_NET_MGMT_CONN_LAYER | _NET_MGMT_CONN_CODE)
#define _NET_MGMT_CONN_IF_EVENT			(NET_MGMT_IFACE_BIT | _NET_MGMT_CONN_BASE)

enum net_event_ethernet_cmd {
	NET_EVENT_CONN_CMD_IF_TIMEOUT = 1,
	NET_EVENT_CONN_CMD_IF_FATAL_ERROR,
};

#define NET_EVENT_CONN_IF_TIMEOUT					\
	(_NET_MGMT_CONN_IF_EVENT | NET_EVENT_CONN_CMD_IF_TIMEOUT)

#define NET_EVENT_CONN_IF_FATAL_ERROR					\
	(_NET_MGMT_CONN_IF_EVENT | NET_EVENT_CONN_CMD_IF_FATAL_ERROR)

/** @endcond */

/**
 * @brief Connectivity Manager Connectivity API structure
 *
 * Used to provide generic access to network association parameters and procedures
 */
struct conn_mgr_conn_binding;
struct conn_mgr_conn_api {
	/**
	 * @brief When called, the connectivity implementation should start attempting to
	 * establish connectivity for (associate with a network) the bound iface pointed
	 * to by if_conn->iface.
	 *
	 * Must be non-blocking.
	 *
	 * Called by conn_mgr_if_connect.
	 */
	int (*connect)(struct conn_mgr_conn_binding *const binding);

	/**
	 * @brief When called, the connectivity implementation should disconnect (dissasociate), or
	 * stop any in-progress attempts to associate to a network, the bound iface pointed to by
	 * if_conn->iface.
	 *
	 * Must be non-blocking.
	 *
	 * Called by conn_mgr_if_disconnect.
	 */
	int (*disconnect)(struct conn_mgr_conn_binding *const binding);

	/**
	 * @brief Called once for each iface that has been bound to a connectivity implementation
	 * using this API.
	 *
	 * Connectivity implementations should use this callback to perform any required
	 * per-bound-iface initialization.
	 *
	 * Implementations may choose to gracefully handle invalid buffer lengths with partial
	 * writes, rather than raise errors, if deemed appropriate.
	 */
	void (*init)(struct conn_mgr_conn_binding *const binding);

	/**
	 * @brief Implementation callback for conn_mgr_if_set_opt.
	 *
	 * Used to set implementation-specific connectivity settings.
	 *
	 * Calls to conn_mgr_if_set_opt on an iface will result in calls to this callback with
	 * the conn_mgr_conn_binding struct bound to that iface.
	 *
	 * It is up to the connectivity implementation to interpret optname. Options can be
	 * specific to the bound iface (pointed to by if_conn->iface), or can apply to the whole
	 * connectivity implementation.
	 *
	 * See the description of conn_mgr_if_set_opt for more details.
	 * set_opt implementations should conform to that description.
	 *
	 * Implementations may choose to gracefully handle invalid buffer lengths with partial
	 * reads, rather than raise errors, if deemed appropriate.
	 */
	int (*set_opt)(struct conn_mgr_conn_binding *const binding,
		       int optname, const void *optval, size_t optlen);

	/**
	 * @brief Implementation callback for conn_mgr_if_get_opt.
	 *
	 * Used to retrieve implementation-specific connectivity settings.
	 *
	 * Calls to conn_mgr_if_get_opt on an iface will result in calls to this callback with
	 * the conn_mgr_conn_binding struct bound to that iface.
	 *
	 * It is up to the connectivity implementation to interpret optname. Options can be
	 * specific to the bound iface (pointed to by if_conn->iface), or can apply to the whole
	 * connectivity implementation.
	 *
	 * See the description of conn_mgr_if_get_opt for more details.
	 * get_opt implementations should conform to that description.
	 */
	int (*get_opt)(struct conn_mgr_conn_binding *const binding,
		       int optname, void *optval, size_t *optlen);
};

/** @cond INTERNAL_HIDDEN */
#define CONN_MGR_CONN_IMPL_GET_NAME(conn_id)		__conn_mgr_conn_##conn_id
#define CONN_MGR_CONN_IMPL_GET_CTX_TYPE(conn_id)	conn_id##_CTX_TYPE
/** @endcond */

/**
 * @brief conn_mgr Connectivity Implementation struct
 *	  Declares a conn_mgr connectivity layer implementation with the provided API
 */
struct conn_mgr_conn_impl {
	/* The connectivity API used by the implementation */
	struct conn_mgr_conn_api *api;
};

/**
 * @brief Define a conn_mgr connectivity implementation that can be bound to network devices.
 *
 * @param conn_id The name of the new connectivity implementation
 * @param conn_api A pointer to a conn_mgr_conn_api struct
 */
#define CONN_MGR_CONN_DEFINE(conn_id, conn_api)						\
	const struct conn_mgr_conn_impl CONN_MGR_CONN_IMPL_GET_NAME(conn_id) = {	\
		.api = conn_api,							\
	};

/**
 * @brief Helper macro to make a conn_mgr connectivity implementation publicly available.
 */
#define CONN_MGR_CONN_DECLARE_PUBLIC(conn_id)						\
	extern const struct conn_mgr_conn_impl CONN_MGR_CONN_IMPL_GET_NAME(conn_id)

/** @cond INTERNAL_HIDDEN */
#define CONN_MGR_CONN_BINDING_GET_NAME(dev_id, sfx)	__conn_mgr_bndg_##dev_id##_##sfx
#define CONN_MGR_CONN_BINDING_GET_DATA(dev_id, sfx)	__conn_mgr_bndg_data_##dev_id##_##sfx
#define CONN_MGR_CONN_BINDING_GET_MUTEX(dev_id, sfx)	__conn_mgr_bndg_mutex_##dev_id##_##sfx
/** @endcond */

/**
 * @brief Per-iface connectivity flags
 */
enum conn_mgr_if_flag {
	/* Persistent
	 * When set, indicates that the connectivity implementation bound to this iface should
	 * attempt to persist connectivity by automatically reconnecting after connection loss.
	 */
	CONN_MGR_IF_PERSISTENT,

/** @cond INTERNAL_HIDDEN */
	/* Total number of flags - must be at the end of the enum */
	CONN_MGR_NUM_IF_FLAGS,
/** @endcond */
};

/* Value to use with conn_mgr_conn_binding->timeout to indicate no timeout */
#define CONN_MGR_IF_NO_TIMEOUT 0

/**
 * @brief Connectivity Manager network interface binding structure
 *
 * Binds a conn_mgr connectivity implementation to an iface / network device.
 * Stores per-iface state for the connectivity implementation.
 */
struct conn_mgr_conn_binding {
	/* The network interface the connectivity implementation is bound to */
	struct net_if *iface;

	/* The connectivity implementation the network device is bound to */
	const struct conn_mgr_conn_impl *impl;

	/* Pointer to private, per-iface connectivity context */
	void *ctx;

	/* Generic connectivity state - Flags
	 * Public boolean state and configuration values supported by all bindings.
	 * See conn_mgr_if_flag for options.
	 */
	uint32_t flags;

	/* Generic connectivity state - Timeout (seconds)
	 * Indicates to the connectivity implementation how long it should attempt to
	 * establish connectivity for during a connection attempt before giving up.
	 *
	 * The connectivity implementation should give up on establishing connectivity after this
	 * timeout, even if persistence is enabled.
	 *
	 * Set to CONN_MGR_IF_NO_TIMEOUT to indicate that no timeout should be used.
	 */
	int timeout;

/** @cond INTERNAL_HIDDEN */
	/* Internal-use mutex for protecting access to the binding and API functions. */
	struct k_mutex *mutex;
/** @endcond */
};

/**
 * @brief Associate a connectivity implementation with an existing network device instance
 *
 * @param dev_id Network device id.
 * @param inst Network device instance.
 * @param conn_id Name of the connectivity implementation to associate.
 */
#define CONN_MGR_BIND_CONN_INST(dev_id, inst, conn_id)						\
	K_MUTEX_DEFINE(CONN_MGR_CONN_BINDING_GET_MUTEX(dev_id, inst));				\
	static CONN_MGR_CONN_IMPL_GET_CTX_TYPE(conn_id)						\
					CONN_MGR_CONN_BINDING_GET_DATA(dev_id, inst);		\
	static STRUCT_SECTION_ITERABLE(conn_mgr_conn_binding,					\
					CONN_MGR_CONN_BINDING_GET_NAME(dev_id, inst)) = {	\
		.iface = NET_IF_GET(dev_id, inst),						\
		.impl = &(CONN_MGR_CONN_IMPL_GET_NAME(conn_id)),				\
		.ctx = &(CONN_MGR_CONN_BINDING_GET_DATA(dev_id, inst)),				\
		.mutex = &(CONN_MGR_CONN_BINDING_GET_MUTEX(dev_id, inst))			\
	};

/**
 * @brief Associate a connectivity implementation with an existing network device
 *
 * @param dev_id Network device id.
 * @param conn_id Name of the connectivity implementation to associate.
 */
#define CONN_MGR_BIND_CONN(dev_id, conn_id)		\
	CONN_MGR_BIND_CONN_INST(dev_id, 0, conn_id)

/**
 * @brief Connect interface
 *
 * If the provided iface has been bound to a connectivity implementation, initiate
 * network connect/association.
 *
 * Non-Blocking.
 *
 * @param iface Pointer to network interface
 * @retval 0 on success.
 * @retval -ESHUTDOWN if the iface is not admin-up.
 * @retval -ENOTSUP if the iface does not have a connectivity implementation.
 * @retval implementation-specific status code otherwise.
 */
int conn_mgr_if_connect(struct net_if *iface);

/**
 * @brief Disconnect interface
 *
 * If the provided iface has been bound to a connectivity implementation, disconnect/dissassociate
 * it from the network, and cancel any pending attempts to connect/associate.
 *
 * @param iface Pointer to network interface
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the iface does not have a connectivity implementation.
 * @retval implementation-specific status code otherwise.
 */
int conn_mgr_if_disconnect(struct net_if *iface);

/**
 * @brief Check whether the provided network interface supports connectivity / has been bound
 *	  to a connectivity implementation.
 *
 * @param iface Pointer to the iface to check.
 * @retval true if connectivity is supported (a connectivity implementation has been bound).
 * @retval false otherwise.
 */
bool conn_mgr_if_is_bound(struct net_if *iface);

/**
 * @brief Set implementation-specific connectivity options.
 *
 * If the provided iface has been bound to a connectivity implementation that supports it,
 * implementation-specific connectivity options related to the iface.
 *
 * @param iface Pointer to the network interface.
 * @param optname Integer value representing the option to set.
 *		  The meaning of values is up to the conn_mgr_conn_api implementation.
 *		  Some settings may affect multiple ifaces.
 * @param optval Pointer to the value to be assigned to the option.
 * @param optlen Length (in bytes) of the value to be assigned to the option.
 * @retval 0 if successful.
 * @retval -ENOTSUP if conn_mgr_if_set_opt not implemented by the iface.
 * @retval -ENOBUFS if optlen is too long.
 * @retval -EINVAL if NULL optval pointer provided.
 * @retval -ENOPROTOOPT if the optname is not recognized.
 * @retval implementation-specific error code otherwise.
 */
int conn_mgr_if_set_opt(struct net_if *iface, int optname, const void *optval, size_t optlen);

/**
 * @brief Get implementation-specific connectivity options.
 *
 * If the provided iface has been bound to a connectivity implementation that supports it,
 * retrieves implementation-specific connectivity options related to the iface.
 *
 * @param iface Pointer to the network interface.
 * @param optname Integer value representing the option to set.
 *		  The meaning of values is up to the conn_mgr_conn_api implementation.
 *		  Some settings may be shared by multiple ifaces.
 * @param optval Pointer to where the retrieved value should be stored.
 * @param optlen Pointer to length (in bytes) of the destination buffer available for storing the
 *		 retrieved value. If the available space is less than what is needed, -ENOBUFS
 *		 is returned. If the available space is invalid, -EINVAL is returned.
 *
 *		 optlen will always be set to the total number of bytes written, regardless of
 *		 whether an error is returned, even if zero bytes were written.
 *
 * @retval 0 if successful.
 * @retval -ENOTSUP if conn_mgr_if_get_opt is not implemented by the iface.
 * @retval -ENOBUFS if retrieval buffer is too small.
 * @retval -EINVAL if invalid retrieval buffer length is provided, or if NULL optval or
 *		   optlen pointer provided.
 * @retval -ENOPROTOOPT if the optname is not recognized.
 * @retval implementation-specific error code otherwise.
 */
int conn_mgr_if_get_opt(struct net_if *iface, int optname, void *optval, size_t *optlen);

/**
 * @brief Check the value of connectivity flags
 *
 * If the provided iface is bound to a connectivity implementation, retrieves the value of the
 * specified connectivity flag associated with that iface.
 *
 * @param iface - Pointer to the network interface to check.
 * @param flag - The flag to check.
 * @return True if the flag is set, otherwise False.
 *	   Also returns False if the provided iface is not bound to a connectivity implementation,
 *	   or the requested flag doesn't exist.
 */
bool conn_mgr_if_get_flag(struct net_if *iface, enum conn_mgr_if_flag flag);

/**
 * @brief Set the value of a connectivity flags
 *
 * If the provided iface is bound to a connectivity implementation, sets the value of the
 * specified connectivity flag associated with that iface.
 *
 * @param iface - Pointer to the network interface to modify.
 * @param flag - The flag to set.
 * @param value - Whether the flag should be enabled or disabled.
 * @retval 0 on success.
 * @retval -EINVAL if the flag does not exist.
 * @retval -ENOTSUP if the provided iface is not bound to a connectivity implementation.
 */
int conn_mgr_if_set_flag(struct net_if *iface, enum conn_mgr_if_flag flag, bool value);

/**
 * @brief Get the connectivity timeout for an iface
 *
 * If the provided iface is bound to a connectivity implementation, retrieves the timeout setting
 * in seconds for it.
 *
 * @param iface - Pointer to the iface to check.
 * @return int - The connectivity timeout value (in seconds) if it could be retrieved, otherwise
 *		 CONN_MGR_IF_NO_TIMEOUT.
 */
int conn_mgr_if_get_timeout(struct net_if *iface);

/**
 * @brief Set the connectivity timeout for an iface.
 *
 * If the provided iface is bound to a connectivity implementation, sets the timeout setting in
 * seconds for it.
 *
 * @param iface - Pointer to the network interface to modify.
 * @param timeout - The timeout value to set (in seconds).
 *		    Pass CONN_MGR_IF_NO_TIMEOUT to disable the timeout.
 * @retval 0 on success.
 * @retval -ENOTSUP if the provided iface is not bound to a connectivity implementation.
 */
int conn_mgr_if_set_timeout(struct net_if *iface, int timeout);

/**
 * @brief  Initialize all connectivity implementation bindings
 *
 *
 */
void conn_mgr_conn_init(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_H_ */
