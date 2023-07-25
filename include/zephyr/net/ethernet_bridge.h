/** @file
 * @brief Ethernet Bridge public header file
 *
 * Ethernet Bridges connect two or more Ethernet networks together and
 * transparently forward packets from one network to the others as if
 * they were part of the same network.
 */

/*
 * Copyright (c) 2021 BayLibre SAS
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
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct eth_bridge {
	struct k_mutex lock;
	sys_slist_t interfaces;
	sys_slist_t listeners;
	bool initialized;
};

#define ETH_BRIDGE_INITIALIZER(obj) \
	{ \
		.lock		= { }, \
		.interfaces	= SYS_SLIST_STATIC_INIT(&obj.interfaces), \
		.listeners	= SYS_SLIST_STATIC_INIT(&obj.listeners), \
	}

/** @endcond */

/**
 * @brief Statically define and initialize a bridge instance.
 *
 * @param name Name of the bridge object
 */
#define ETH_BRIDGE_INIT(name) \
	STRUCT_SECTION_ITERABLE(eth_bridge, name) = \
		ETH_BRIDGE_INITIALIZER(name)

struct eth_bridge_iface_context {
	sys_snode_t node;
	struct eth_bridge *instance;
	bool allow_tx;
};

struct eth_bridge_listener {
	sys_snode_t node;
	struct k_fifo pkt_queue;
};

/**
 * @brief Add an Ethernet network interface to a bridge
 *
 * This adds a network interface to a bridge. The interface is then put
 * into promiscuous mode, all packets received by this interface are sent
 * to the bridge, and any other packets sent to the bridge (with some
 * exceptions) are transmitted via this interface.
 *
 * For transmission from the bridge to occur via this interface, it is
 * necessary to enable TX mode with eth_bridge_iface_tx(). TX mode is
 * initially disabled.
 *
 * Once an interface is added to a bridge, all its incoming traffic is
 * diverted to the bridge. However, packets sent out with net_if_queue_tx()
 * via this interface are not subjected to the bridge.
 *
 * @param br A pointer to an initialized bridge object
 * @param iface Interface to add
 *
 * @return 0 if OK, negative error code otherwise.
 */
int eth_bridge_iface_add(struct eth_bridge *br, struct net_if *iface);

/**
 * @brief Remove an Ethernet network interface from a bridge
 *
 * @param br A pointer to an initialized bridge object
 * @param iface Interface to remove
 *
 * @return 0 if OK, negative error code otherwise.
 */
int eth_bridge_iface_remove(struct eth_bridge *br, struct net_if *iface);

/**
 * @brief Enable/disable transmission mode for a bridged interface
 *
 * When TX mode is off, the interface may receive packets and send them to
 * the bridge but no packets coming from the bridge will be sent through this
 * interface. When TX mode is on, both incoming and outgoing packets are
 * allowed.
 *
 * @param iface Interface to configure
 * @param allow true to activate TX mode, false otherwise
 *
 * @return 0 if OK, negative error code otherwise.
 */
int eth_bridge_iface_allow_tx(struct net_if *iface, bool allow);

/**
 * @brief Add (register) a listener to the bridge
 *
 * This lets a software listener register a pointer to a provided FIFO for
 * receiving packets sent to the bridge. The listener is responsible for
 * emptying the FIFO with k_fifo_get() which will return a struct net_pkt
 * pointer, and releasing the packet with net_pkt_unref() when done with it.
 *
 * The listener wishing not to receive any more packets should simply
 * unregister itself with eth_bridge_listener_remove().
 *
 * @param br A pointer to an initialized bridge object
 * @param l A pointer to an initialized listener instance.
 *
 * @return 0 if OK, negative error code otherwise.
 */
int eth_bridge_listener_add(struct eth_bridge *br, struct eth_bridge_listener *l);

/**
 * @brief Remove (unregister) a listener from the bridge
 *
 * @param br A pointer to an initialized bridge object
 * @param l A pointer to the listener instance to be removed.
 *
 * @return 0 if OK, negative error code otherwise.
 */
int eth_bridge_listener_remove(struct eth_bridge *br, struct eth_bridge_listener *l);

/**
 * @brief Get bridge index according to pointer
 *
 * @param br Pointer to bridge instance
 *
 * @return Bridge index
 */
int eth_bridge_get_index(struct eth_bridge *br);

/**
 * @brief Get bridge instance according to index
 *
 * @param index Bridge instance index
 *
 * @return Pointer to bridge instance or NULL if not found.
 */
struct eth_bridge *eth_bridge_get_by_index(int index);

/**
 * @typedef eth_bridge_cb_t
 * @brief Callback used while iterating over bridge instances
 *
 * @param br Pointer to bridge instance
 * @param user_data User supplied data
 */
typedef void (*eth_bridge_cb_t)(struct eth_bridge *br, void *user_data);

/**
 * @brief Go through all the bridge instances in order to get
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
