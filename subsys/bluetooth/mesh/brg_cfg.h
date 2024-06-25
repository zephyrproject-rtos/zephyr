/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_BRG_CFG_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_BRG_CFG_H_

/** These are internal APIs. They do not sanitize input params. */
enum bt_mesh_brg_cfg_dir {
	/* Value is prohibited. */
	BT_MESH_BRG_CFG_DIR_PROHIBITED = 0,
	/* Briging from Addr1 to Addr2. */
	BT_MESH_BRG_CFG_DIR_ONEWAY = 1,
	/* Briging to/from Addr1 from/to Addr2. */
	BT_MESH_BRG_CFG_DIR_TWOWAY = 2,
	/* Values above these are prohibited. */
	BT_MESH_BRG_CFG_DIR_MAX = 3,
};

#define BT_MESH_BRG_CFG_NETIDX_NOMATCH 0xFFFF

/* One row of the bridging table */
struct bt_mesh_brg_cfg_row {
	/* Direction of the entry in the bridging table
	 * 0 - no entry,
	 * 1 - bridge messages with src as addr1 and dst as addr2
	 * 2 - bridge messages with src as addr1 and dst as addr2 and vice-versa
	 */
	uint32_t direction:8;
	uint32_t net_idx1:12;
	uint32_t net_idx2:12;
	uint16_t addr1;
	uint16_t addr2;
};

bool bt_mesh_brg_cfg_enable_get(void);

int bt_mesh_brg_cfg_enable_set(bool enable);

void bt_mesh_brg_cfg_pending_store(void);

int bt_mesh_brg_cfg_tbl_reset(void);

int bt_mesh_brg_cfg_tbl_get(const struct bt_mesh_brg_cfg_row **rows);

int bt_mesh_brg_cfg_tbl_add(enum bt_mesh_brg_cfg_dir direction, uint16_t net_idx1,
			    uint16_t net_idx2, uint16_t addr1, uint16_t addr2);

void bt_mesh_brg_cfg_tbl_remove(uint16_t net_idx1, uint16_t net_idx2, uint16_t addr1,
				uint16_t addr2);

typedef void (*bt_mesh_brg_cfg_cb_t)(uint16_t new_netidx, void *user_data);

/**
 * @brief Iterate over the bridging table to find a matching entry for the given SRC, DST, and
 * NetKey Index.
 *
 * This function iterates over the bridging table and checks if there is a match for the provided
 * parameters. If a match is found, the callback function specified by the 'cb' parameter is
 * invoked with the NetKey Index of each matching entry (there can be several). Relaying operation
 * can then happen inside this callback.
 *
 * @param src The source address to match.
 * @param dst The destination address to match.
 * @param net_idx The NetKey Index to match.
 * @param cb The callback function to be invoked for each matching entry.
 * @param user_data User data to be passed to the callback function.
 */
void bt_mesh_brg_cfg_tbl_foreach_subnet(uint16_t src, uint16_t dst, uint16_t net_idx,
					bt_mesh_brg_cfg_cb_t cb, void *user_data);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_BRG_CFG_H_ */
