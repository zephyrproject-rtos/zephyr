/** @file
 * @brief Ethernet Bridge public header file
 *
 * Ethernet Bridges connect two or more Ethernet networks together and
 * transparently forward packets from one network to the others as if
 * they were part of the same network.
 */

/*
 * Copyright (c) 2021 BayLibre SAS
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_H_
#define ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_H_

#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ethernet Bridging API
 * @defgroup eth_bridge Ethernet Bridging API
 * @since 2.7
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_NET_ETHERNET_BRIDGE)
#define NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT CONFIG_NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT
#else
#define NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT 1
#endif

struct eth_bridge_iface_context {
	/* Lock to protect access to interface array below */
	struct k_mutex lock;

	/* The actual bridge virtual interface  */
	struct net_if *iface;

	/* What Ethernet interfaces are bridged together */
	struct net_if *eth_iface[NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT];

	/* How many interfaces are bridged atm */
	size_t count;

	/* Bridge instance id */
	int id;

	/* Is the bridge interface initialized */
	bool is_init : 1;

	/* Has user configured the bridge */
	bool is_setup : 1;

	/* Is the interface enabled or not */
	bool status : 1;
};

/** @endcond */

/**
 * @brief Add an Ethernet network interface to a bridge
 *
 * This adds a network interface to a bridge. The interface is then put
 * into promiscuous mode. After more than one Ethernet interfaces are
 * added to the bridge interface, the bridge interface is setup.
 * After the setup is done, the bridge interface can be brought up so
 * that it can start bridging L2 traffic.
 *
 * @param br A pointer to a bridge interface
 * @param iface Interface to add
 *
 * @return 0 if OK, negative error code otherwise.
 */
int eth_bridge_iface_add(struct net_if *br, struct net_if *iface);

/**
 * @brief Remove an Ethernet network interface from a bridge.
 *
 * If the bridge interface setup has only one Ethernet interface left
 * after this function call, the bridge is disabled as it cannot bridge
 * the L2 traffic any more. The bridge interface is left in UP state
 * if this case.
 *
 * @param br A pointer to a bridge interface
 * @param iface Interface to remove
 *
 * @return 0 if OK, negative error code otherwise.
 */
int eth_bridge_iface_remove(struct net_if *br, struct net_if *iface);

/**
 * @brief Get bridge index according to pointer
 *
 * @param br Pointer to bridge instance
 *
 * @return Bridge index
 */
int eth_bridge_get_index(struct net_if *br);

/**
 * @brief Get bridge instance according to index
 *
 * @param index Bridge instance index
 *
 * @return Pointer to bridge interface or NULL if not found.
 */
struct net_if *eth_bridge_get_by_index(int index);

/**
 * @typedef eth_bridge_cb_t
 * @brief Callback used while iterating over bridge instances
 *
 * @param br Pointer to bridge context instance
 * @param user_data User supplied data
 */
typedef void (*eth_bridge_cb_t)(struct eth_bridge_iface_context *br, void *user_data);

/**
 * @brief Go through all the bridge context instances in order to get
 *        information about them. This is mainly useful in
 *        net-shell to print data about currently active bridges.
 *
 * @param cb Callback to call for each bridge instance
 * @param user_data User supplied data
 */
void net_eth_bridge_foreach(eth_bridge_cb_t cb, void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_H_ */
