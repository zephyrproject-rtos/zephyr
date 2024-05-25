/** @file
 * @brief Offloaded network device iface API
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_OFFLOADED_NETDEV_H_
#define ZEPHYR_INCLUDE_OFFLOADED_NETDEV_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Offloaded Net Devices
 * @defgroup offloaded_netdev Offloaded Net Devices
 * @ingroup networking
 * @{
 */

/** Types of offloaded netdev L2 */
enum offloaded_net_if_types {
	/** Unknown, device hasn't register a type */
	L2_OFFLOADED_NET_IF_TYPE_UNKNOWN,

	/** Ethernet devices */
	L2_OFFLOADED_NET_IF_TYPE_ETHERNET,

	/** Modem */
	L2_OFFLOADED_NET_IF_TYPE_MODEM,

	/** IEEE 802.11 Wi-Fi */
	L2_OFFLOADED_NET_IF_TYPE_WIFI,
};

/**
 * @brief Extended net_if_api for offloaded ifaces/network devices, allowing handling of
 *	  admin up/down state changes
 */
struct offloaded_if_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Enable or disable the device (in response to admin state change) */
	int (*enable)(const struct net_if *iface, bool state);

	/** Types of offloaded net device */
	enum offloaded_net_if_types (*get_type)(void);
};

/* Ensure offloaded_if_api is compatible with net_if_api */
BUILD_ASSERT(offsetof(struct offloaded_if_api, iface_api) == 0);

/**
 * @brief Check if the offloaded network interface supports Wi-Fi.
 *
 * @param iface Pointer to network interface
 *
 * @return True if interface supports Wi-Fi, False otherwise.
 */
static inline bool net_off_is_wifi_offloaded(struct net_if *iface)
{
	const struct offloaded_if_api *api = (const struct offloaded_if_api *)
		net_if_get_device(iface)->api;

	return api->get_type && api->get_type() == L2_OFFLOADED_NET_IF_TYPE_WIFI;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_OFFLOADED_NETDEV_H_ */
