/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for controlling generic network association routines on network devices that
 * support it.
 */

#ifndef ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_H_
#define ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/net_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connection Manager Connectivity API
 * @defgroup conn_mgr_connectivity Connection Manager Connectivity API
 * @since 3.4
 * @version 0.1.0
 * @ingroup networking
 * @{
 */


/** @cond INTERNAL_HIDDEN */

/* Connectivity Events */
#define _NET_MGMT_CONN_LAYER			NET_MGMT_LAYER(NET_MGMT_LAYER_L2)
#define _NET_MGMT_CONN_CODE			NET_MGMT_LAYER_CODE(0x207)
#define _NET_MGMT_CONN_BASE			(_NET_MGMT_CONN_LAYER | _NET_MGMT_CONN_CODE | \
						 NET_MGMT_EVENT_BIT)
#define _NET_MGMT_CONN_IF_EVENT			(NET_MGMT_IFACE_BIT | _NET_MGMT_CONN_BASE)

enum net_event_conn_cmd {
	NET_EVENT_CONN_CMD_IF_TIMEOUT = 1,
	NET_EVENT_CONN_CMD_IF_FATAL_ERROR,
};

/** @endcond */

/**
 * @brief net_mgmt event raised when a connection attempt times out
 */
#define NET_EVENT_CONN_IF_TIMEOUT					\
	(_NET_MGMT_CONN_IF_EVENT | NET_EVENT_CONN_CMD_IF_TIMEOUT)

/**
 * @brief net_mgmt event raised when a non-recoverable connectivity error occurs on an iface
 */
#define NET_EVENT_CONN_IF_FATAL_ERROR					\
	(_NET_MGMT_CONN_IF_EVENT | NET_EVENT_CONN_CMD_IF_FATAL_ERROR)


/**
 * @brief Per-iface connectivity flags
 */
enum conn_mgr_if_flag {
	/**
	 * Persistent
	 *
	 * When set, indicates that the connectivity implementation bound to this iface should
	 * attempt to persist connectivity by automatically reconnecting after connection loss.
	 */
	CONN_MGR_IF_PERSISTENT,

	/**
	 * No auto-connect
	 *
	 * When set, conn_mgr will not automatically attempt to connect this iface when it reaches
	 * admin-up.
	 */
	CONN_MGR_IF_NO_AUTO_CONNECT,

	/**
	 * No auto-down
	 *
	 * When set, conn_mgr will not automatically take the iface admin-down when it stops
	 * trying to connect, even if CONFIG_NET_CONNECTION_MANAGER_AUTO_IF_DOWN is enabled.
	 */
	CONN_MGR_IF_NO_AUTO_DOWN,

/** @cond INTERNAL_HIDDEN */
	/* Total number of flags - must be at the end of the enum */
	CONN_MGR_NUM_IF_FLAGS,
/** @endcond */
};

/** Value to use with @ref conn_mgr_if_set_timeout and @ref conn_mgr_conn_binding.timeout to
 * indicate no timeout
 */
#define CONN_MGR_IF_NO_TIMEOUT 0

/**
 * @brief Connect interface
 *
 * If the provided iface has been bound to a connectivity implementation, initiate
 * network connect/association.
 *
 * Automatically takes the iface admin-up (by calling @ref net_if_up) if it isn't already.
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
 * If the provided iface has been bound to a connectivity implementation, disconnect/disassociate
 * it from the network, and cancel any pending attempts to connect/associate.
 *
 * Does nothing if the iface is currently admin-down.
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
 *		    Pass @ref CONN_MGR_IF_NO_TIMEOUT to disable the timeout.
 * @retval 0 on success.
 * @retval -ENOTSUP if the provided iface is not bound to a connectivity implementation.
 */
int conn_mgr_if_set_timeout(struct net_if *iface, int timeout);

/**
 * @}
 */

/**
 * @brief Connection Manager Bulk API
 * @defgroup conn_mgr_connectivity_bulk Connection Manager Connectivity Bulk API
 * @ingroup networking
 * @{
 */

/**
 * @brief Convenience function that takes all available ifaces into the admin-up state.
 *
 * Essentially a wrapper for @ref net_if_up.
 *
 * @param skip_ignored - If true, only affect ifaces that aren't ignored by conn_mgr.
 *			 Otherwise, affect all ifaces.
 * @return 0 if all net_if_up calls returned 0, otherwise the first nonzero value
 *         returned by a net_if_up call.
 */
int conn_mgr_all_if_up(bool skip_ignored);


/**
 * @brief Convenience function that takes all available ifaces into the admin-down state.
 *
 * Essentially a wrapper for @ref net_if_down.
 *
 * @param skip_ignored - If true, only affect ifaces that aren't ignored by conn_mgr.
 *			 Otherwise, affect all ifaces.
 * @return 0 if all net_if_down calls returned 0, otherwise the first nonzero value
 *         returned by a net_if_down call.
 */
int conn_mgr_all_if_down(bool skip_ignored);

/**
 * @brief Convenience function that takes all available ifaces into the admin-up state, and
 * connects those that support connectivity.
 *
 * Essentially a wrapper for @ref net_if_up and @ref conn_mgr_if_connect.
 *
 * @param skip_ignored - If true, only affect ifaces that aren't ignored by conn_mgr.
 *			 Otherwise, affect all ifaces.
 * @return 0 if all net_if_up and conn_mgr_if_connect calls returned 0, otherwise the first nonzero
 *	   value returned by either net_if_up or conn_mgr_if_connect.
 */
int conn_mgr_all_if_connect(bool skip_ignored);

/**
 * @brief Convenience function that disconnects all available ifaces that support connectivity
 *        without putting them into admin-down state (unless auto-down is enabled for the iface).
 *
 * Essentially a wrapper for @ref net_if_down.
 *
 * @param skip_ignored - If true, only affect ifaces that aren't ignored by conn_mgr.
 *			 Otherwise, affect all ifaces.
 * @return 0 if all net_if_up and conn_mgr_if_connect calls returned 0, otherwise the first nonzero
 *	   value returned by either net_if_up or conn_mgr_if_connect.
 */
int conn_mgr_all_if_disconnect(bool skip_ignored);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_H_ */
