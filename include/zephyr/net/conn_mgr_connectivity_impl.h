/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for defining conn_mgr connectivity implementations (allowing ifaces to be used with
 * conn_mgr_connectivity).
 */

#ifndef ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_IMPL_H_
#define ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_IMPL_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connection Manager Connectivity Implementation API
 * @defgroup conn_mgr_connectivity_impl Connection Manager Connectivity Implementation API
 * @since 3.4
 * @version 0.1.0
 * @ingroup conn_mgr_connectivity
 * @{
 */

/* Forward declaration */
struct conn_mgr_conn_binding;

/**
 * @brief Connectivity Manager Connectivity API structure
 *
 * Used to provide generic access to network association parameters and procedures
 */
struct conn_mgr_conn_api {
	/**
	 * @brief When called, the connectivity implementation should start attempting to
	 * establish connectivity (association with a network) for the bound iface pointed
	 * to by if_conn->iface.
	 *
	 * Must be non-blocking.
	 *
	 * Called by @ref conn_mgr_if_connect.
	 */
	int (*connect)(struct conn_mgr_conn_binding *const binding);

	/**
	 * @brief When called, the connectivity implementation should disconnect (disassociate), or
	 * stop any in-progress attempts to associate to a network, the bound iface pointed to by
	 * if_conn->iface.
	 *
	 * Must be non-blocking.
	 *
	 * Called by @ref conn_mgr_if_disconnect.
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
 * @brief Connectivity Implementation struct
 *
 * Declares a conn_mgr connectivity layer implementation with the provided API
 */
struct conn_mgr_conn_impl {
	/** The connectivity API used by the implementation */
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
 * @brief Connectivity Manager network interface binding structure
 *
 * Binds a conn_mgr connectivity implementation to an iface / network device.
 * Stores per-iface state for the connectivity implementation.
 */
struct conn_mgr_conn_binding {
	/** The network interface the connectivity implementation is bound to */
	struct net_if *iface;

	/** The connectivity implementation the network device is bound to */
	const struct conn_mgr_conn_impl *impl;

	/** Pointer to private, per-iface connectivity context */
	void *ctx;

	/**
	 * @name Generic connectivity state
	 * @{
	 */

	/**
	 * Connectivity flags
	 *
	 * Public boolean state and configuration values supported by all bindings.
	 * See conn_mgr_if_flag for options.
	 */
	uint32_t flags;

	/**
	 * Timeout (seconds)
	 *
	 * Indicates to the connectivity implementation how long it should attempt to
	 * establish connectivity for during a connection attempt before giving up.
	 *
	 * The connectivity implementation should give up on establishing connectivity after this
	 * timeout, even if persistence is enabled.
	 *
	 * Set to @ref CONN_MGR_IF_NO_TIMEOUT to indicate that no timeout should be used.
	 */
	int timeout;

	/** @} */

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
 * @brief Retrieves the conn_mgr binding struct for a provided iface if it exists.
 *
 * Bindings for connectivity implementations with missing API structs are ignored.
 *
 * For use only by connectivity implementations.
 *
 * @param iface - bound network interface to obtain the binding struct for.
 * @return struct conn_mgr_conn_binding* Pointer to the retrieved binding struct if it exists,
 *	   NULL otherwise.
 */
static inline struct conn_mgr_conn_binding *conn_mgr_if_get_binding(struct net_if *iface)
{
	STRUCT_SECTION_FOREACH(conn_mgr_conn_binding, binding) {
		if (iface == binding->iface) {
			if (binding->impl->api) {
				return binding;
			}
			return NULL;
		}
	}
	return NULL;
}

/**
 * @brief Lock the passed-in binding, making it safe to access.
 *
 * Call this whenever accessing binding data, unless inside a conn_mgr_conn_api callback, where it
 * is called automatically by conn_mgr.
 *
 * Reentrant.
 *
 * For use only by connectivity implementations.
 *
 * @param binding - Binding to lock
 */
static inline void conn_mgr_binding_lock(struct conn_mgr_conn_binding *binding)
{
	(void)k_mutex_lock(binding->mutex, K_FOREVER);
}

/**
 * @brief Unlocks the passed-in binding.
 *
 * Call this after any call to @ref conn_mgr_binding_lock once done accessing binding data.
 *
 * Reentrant.
 *
 * For use only by connectivity implementations.
 *
 * @param binding - Binding to unlock
 */
static inline void conn_mgr_binding_unlock(struct conn_mgr_conn_binding *binding)
{
	(void)k_mutex_unlock(binding->mutex);
}

/**
 * @brief Set the value of the specified connectivity flag for the provided binding
 *
 * Can be used from any thread or callback without calling @ref conn_mgr_binding_lock.
 *
 * For use only by connectivity implementations
 *
 * @param binding The binding to check
 * @param flag The flag to check
 * @param value New value for the specified flag
 */
static inline void conn_mgr_binding_set_flag(struct conn_mgr_conn_binding *binding,
					     enum conn_mgr_if_flag flag, bool value)
{
	conn_mgr_binding_lock(binding);

	binding->flags &= ~BIT(flag);
	if (value) {
		binding->flags |= BIT(flag);
	}

	conn_mgr_binding_unlock(binding);
}

/**
 * @brief Check the value of the specified connectivity flag for the provided binding
 *
 * Can be used from any thread or callback without calling @ref conn_mgr_binding_lock.
 *
 * For use only by connectivity implementations
 *
 * @param binding The binding to check
 * @param flag The flag to check
 * @return bool The value of the specified flag
 */
static inline bool conn_mgr_binding_get_flag(struct conn_mgr_conn_binding *binding,
					     enum conn_mgr_if_flag flag)
{
	bool value = false;

	conn_mgr_binding_lock(binding);

	value = !!(binding->flags & BIT(flag));

	conn_mgr_binding_unlock(binding);

	return value;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_IMPL_H_ */
