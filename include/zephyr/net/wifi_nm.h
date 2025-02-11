/** @file
 * @brief Wi-Fi Network manager API
 *
 * This file contains the Wi-Fi network manager API. These APIs are used by the
 * any network management application to register as a Wi-Fi network manager.
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_NET_WIFI_NM_H_
#define ZEPHYR_INCLUDE_ZEPHYR_NET_WIFI_NM_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
/**
 * @brief Wi-Fi Network manager API
 * @defgroup wifi_nm Wi-Fi Network Manager API
 * @ingroup networking
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Types of Wi-Fi interface */
enum wifi_nm_iface_type {
	/** IEEE 802.11 Wi-Fi Station */
	WIFI_TYPE_STA = 0,
	/** IEEE 802.11 Wi-Fi Soft AP */
	WIFI_TYPE_SAP,
};

/**
 * @brief WiFi Network Managed interfaces
 */
struct wifi_nm_mgd_iface {
	/** Wi-Fi interface type */
	unsigned char type;
	/** Managed net interfaces */
	struct net_if *iface;
};

/**
 * @brief WiFi Network manager instance
 */
struct wifi_nm_instance {
	/** Name of the Network manager instance */
	const char *name;
	/** Wi-Fi Management operations */
	const struct wifi_mgmt_ops *ops;
	/** List of Managed interfaces */
	struct wifi_nm_mgd_iface mgd_ifaces[CONFIG_WIFI_NM_MAX_MANAGED_INTERFACES];
};

/** @cond INTERNAL_HIDDEN */

#define WIFI_NM_NAME(name) wifi_nm_##name

#define DEFINE_WIFI_NM_INSTANCE(_name, _ops)		\
	static STRUCT_SECTION_ITERABLE(wifi_nm_instance, WIFI_NM_NAME(_name)) = { \
		.name = STRINGIFY(_name),		\
		.ops = _ops,				\
		.mgd_ifaces = {},		\
	}

/** @endcond */

/**
 * @brief Get a Network manager instance for a given name
 *
 * @param name Name of the Network manager instance
 *
 */
struct wifi_nm_instance *wifi_nm_get_instance(const char *name);

/**
 * @brief Get a Network manager instance for a given interface
 *
 * @param iface Interface
 *
 */
struct wifi_nm_instance *wifi_nm_get_instance_iface(struct net_if *iface);

/**
 * @brief Get a Wi-Fi type for a given interface
 *
 * @param iface Interface
 *
 */
unsigned char wifi_nm_get_type_iface(struct net_if *iface);

/**
 * @brief Check if the interface is a Wi-Fi station interface
 *
 * @param iface Interface
 *
 * @retval true If the interface is a Wi-Fi station interface.
 *
 */
bool wifi_nm_iface_is_sta(struct net_if *iface);

/**
 * @brief Check if the interface is a Wi-Fi Soft AP interface
 *
 * @param iface Interface
 *
 * @retval true If the interface is a Wi-Fi Soft AP interface.
 *
 */
bool wifi_nm_iface_is_sap(struct net_if *iface);

/**
 * @brief Register a managed interface
 *
 * @param nm Pointer to Network manager instance
 * @param iface Managed interface
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid parameters were passed.
 * @retval -ENOTSUP If the interface is not a Wi-Fi interface.
 * @retval -ENOMEM If the maximum number of managed interfaces has been reached.
 */
int wifi_nm_register_mgd_iface(struct wifi_nm_instance *nm, struct net_if *iface);

/**
 * @brief Register a managed interface
 *
 * @param nm Pointer to Network manager instance
 * @param type Wi-Fi type
 * @param iface Managed interface
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid parameters were passed.
 * @retval -ENOTSUP If the interface is not a Wi-Fi interface.
 * @retval -ENOMEM If the maximum number of managed interfaces has been reached.
 */
int wifi_nm_register_mgd_type_iface(struct wifi_nm_instance *nm,
		enum wifi_nm_iface_type type, struct net_if *iface);

/**
 * @brief Unregister managed interface
 *
 * @param nm Pointer to Network manager instance
 * @param iface Interface
 * @return int 0 for OK; -EINVAL for invalid parameters; -ENOENT if interface is not registered
 *	   with the Network manager.
 */
int wifi_nm_unregister_mgd_iface(struct wifi_nm_instance *nm, struct net_if *iface);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_ZEPHYR_NET_WIFI_NM_H_ */
