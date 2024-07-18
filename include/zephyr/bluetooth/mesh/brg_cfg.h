/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bt_mesh_brg_cfg Bridge Configuration common header
 * @ingroup bt_mesh
 * @{
 */

/** Subnet Bridge states */
enum bt_mesh_subnet_bridge_state {
	/** Subnet bridge functionality is disabled. */
	BT_MESH_SUBNET_BRIDGE_DISABLED,
	/** Subnet bridge state functionality is enabled. */
	BT_MESH_SUBNET_BRIDGE_ENABLED,
};

/** Bridging Table state entry corresponding to a entry in the Bridging Table. */
struct bridging_table_entry {
	/** Allowed directions for the bridged traffic (or bridged traffic not allowed) */
	uint8_t directions;
	/** NetKey Index of the first subnet */
	uint16_t net_idx1;
	/** NetKey Index of the second subnet */
	uint16_t net_idx2;
	/** Address of the node in the first subnet */
	uint16_t addr1;
	/** Address of the node in the second subnet */
	uint16_t addr2;
};

/** Bridging Table Status response */
struct bt_mesh_bridging_table_status {
	/** Status Code of the requesting message */
	uint8_t status;
	/** Requested Bridging Table entry */
	struct bridging_table_entry entry;
};

/** Used to filter set of pairs of NetKey Indexes from the Bridging Table */
struct bt_mesh_filter_netkey {
	uint16_t filter:2,     /* Filter applied to the set of pairs of NetKey Indexes */
		 prohibited:2, /* Prohibited */
		 net_idx:12;   /* NetKey Index used for filtering or ignored */
};

/** Bridged Subnets List response */
struct bt_mesh_bridged_subnets_list {
	/** Filter applied NetKey Indexes, and NetKey Index used for filtering. */
	struct bt_mesh_filter_netkey net_idx_filter;
	/** Start offset in units of bridges */
	uint8_t start_idx;
	/** Pointer to allocated buffer for storing filtered of NetKey Indexes */
	struct net_buf_simple *list;
};

/** Bridging Table List response */
struct bt_mesh_bridging_table_list {
	/** Status Code of the requesting message */
	uint8_t status;
	/** NetKey Index of the first subnet */
	uint16_t net_idx1;
	/** NetKey Index of the second subnet */
	uint16_t net_idx2;
	/** Start offset in units of bridging table state entries */
	uint16_t start_idx;
	/** Pointer to allocated buffer for storing list of bridged addresses and directions */
	struct net_buf_simple *list;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_BRG_CFG_H__ */
