/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for Ethernet bridge FDB
 */

#ifndef ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_FDB_H_
#define ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_FDB_H_

/**
 * @brief Definitions for Ethernet bridge FDB
 * @defgroup eth_bridge_fdb Ethernet bridge FDB
 * @since 4.4
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FDB entry flags
 */
#define ETHERNET_BRIDGE_FDB_FLAG_STATIC  BIT(0) /**< Static entry (manually added) */
#define ETHERNET_BRIDGE_FDB_FLAG_DYNAMIC BIT(1) /**< Dynamic entry (learned) */

/**
 * @brief FDB entry structure
 */
struct eth_bridge_fdb_entry {
	/** MAC address */
	struct net_eth_addr mac;
	/** Interface */
	struct net_if *iface;
	/** Entry flags */
	uint8_t flags;
	/** Linked list node */
	sys_snode_t node;
};

/**
 * @brief Add a static FDB entry
 *
 * @param mac MAC address
 * @param iface Interface
 * @return 0 on success, negative errno on failure
 */
int eth_bridge_fdb_add(struct net_eth_addr *mac, struct net_if *iface);

/**
 * @brief Delete an FDB entry
 *
 * @param mac MAC address
 * @param iface Interface (NULL to delete from any interface)
 * @return 0 on success, negative errno on failure
 */
int eth_bridge_fdb_del(struct net_eth_addr *mac, struct net_if *iface);

/**
 * @brief Delete FDB entries on an interface
 *
 * @param iface Interface (NULL to delete from any interface)
 * @return 0 on success, negative errno on failure
 */
int eth_bridge_fdb_del_iface(struct net_if *iface);

/**
 * @brief Callback function type for iterating over FDB entries
 *
 * @param entry Pointer to the current FDB entry being processed
 * @param user_data User-defined data passed from the foreach function
 */
typedef void (*eth_bridge_fdb_entry_cb_t)(struct eth_bridge_fdb_entry *entry, void *user_data);

/**
 * @brief Iterate over all entries in the FDB table
 *
 * @param cb Callback function to be called for each FDB entry.
 * @param user_data User-defined data pointer passed to each callback invocation.
 */
void eth_bridge_fdb_foreach(eth_bridge_fdb_entry_cb_t cb, void *user_data);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_ETHERNET_BRIDGE_FDB_H_ */
