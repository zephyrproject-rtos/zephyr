/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Implementation for states of Subnet Bridge feature in Bluetooth Mesh Protocol v1.1
 * specification
 */
#include <errno.h>
#include <zephyr/bluetooth/mesh.h>

#include "mesh.h"
#include "net.h"
#include "settings.h"
#include "brg_cfg.h"

#define LOG_LEVEL CONFIG_BT_MESH_BRG_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_brg_cfg);

/* Bridging table state and counter */
static struct bt_mesh_brg_cfg_row brg_tbl[CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX];
static uint32_t bt_mesh_brg_cfg_row_cnt;
/* Bridging enabled state */
static bool brg_enabled;

static void brg_tbl_compact(void)
{
	int j = 0;

	for (int k = 0; k < bt_mesh_brg_cfg_row_cnt; k++) {
		if (brg_tbl[k].direction != 0) {
			brg_tbl[j] = brg_tbl[k];
			j++;
		}
	}
	memset(&brg_tbl[j], 0, sizeof(brg_tbl[j]));
	bt_mesh_brg_cfg_row_cnt--;
}

#if IS_ENABLED(CONFIG_BT_SETTINGS)
/* Set function for initializing bridging enable state from value stored in settings. */
static int brg_en_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	int err;

	if (len_rd == 0) {
		brg_enabled = 0;
		LOG_DBG("Cleared bridge enable state");
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &brg_enabled, sizeof(brg_enabled));
	if (err) {
		LOG_ERR("Failed to set bridge enable state");
		return err;
	}

	LOG_DBG("Restored bridge enable state");

	return 0;
}

/* Define a setting for storing enable state */
BT_MESH_SETTINGS_DEFINE(brg_en, "brg_en", brg_en_set);

/* Set function for initializing bridging table rows from values stored in settings. */
static int brg_tbl_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	if (len_rd == 0) {
		memset(brg_tbl, 0, sizeof(brg_tbl));
		bt_mesh_brg_cfg_row_cnt = 0;
		LOG_DBG("Cleared bridging table entries");
		return 0;
	}

	int err = bt_mesh_settings_set(read_cb, cb_arg, brg_tbl, sizeof(brg_tbl));

	if (err) {
		LOG_ERR("Failed to set bridging table entries");
		return err;
	}

	LOG_DBG("Restored bridging table");

	return 0;
}

/* Define a setting for storing briging table rows */
BT_MESH_SETTINGS_DEFINE(brg_tbl, "brg_tbl", brg_tbl_set);
#endif

bool bt_mesh_brg_cfg_enable_get(void)
{
	return brg_enabled;
}

int bt_mesh_brg_cfg_enable_set(bool enable)
{
	if (brg_enabled == enable) {
		return 0;
	}

	brg_enabled = enable;
#if IS_ENABLED(CONFIG_BT_SETTINGS)
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_BRG_PENDING);
#endif
	return 0;
}

void bt_mesh_brg_cfg_pending_store(void)
{
#if CONFIG_BT_SETTINGS
	char *path_en = "bt/mesh/brg_en";
	char *path_tbl = "bt/mesh/brg_tbl";
	int err;

	if (brg_enabled) {
		err = settings_save_one(path_en, &brg_enabled, sizeof(brg_enabled));
	} else {
		err = settings_delete(path_en);
	}

	if (err) {
		LOG_ERR("Failed to store %s value", path_en);
	}


	if (bt_mesh_brg_cfg_row_cnt) {
		err = settings_save_one(path_tbl, &brg_tbl,
					bt_mesh_brg_cfg_row_cnt * sizeof(brg_tbl[0]));
	} else {
		err = settings_delete(path_tbl);
	}

	if (err) {
		LOG_ERR("Failed to store %s value", path_tbl);
	}
#endif
}

/* Remove the entry from the bridging table that corresponds with the NetKey Index of the removed
 * subnet.
 */
static void brg_tbl_netkey_removed_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	if (evt != BT_MESH_KEY_DELETED) {
		return;
	}

	for (int i = 0; i < CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX; i++) {
		if (brg_tbl[i].direction && (
		    brg_tbl[i].net_idx1 == sub->net_idx ||
		    brg_tbl[i].net_idx2 == sub->net_idx)) {
			memset(&brg_tbl[i], 0, sizeof(brg_tbl[i]));
			brg_tbl_compact();
		}
	}

#if IS_ENABLED(CONFIG_BT_SETTINGS)
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_BRG_PENDING);
#endif
}

/* Add event hook for key deletion event */
BT_MESH_SUBNET_CB_DEFINE(sbr) = {
	.evt_handler = brg_tbl_netkey_removed_evt,
};

int bt_mesh_brg_cfg_tbl_reset(void)
{
	int err = 0;

	brg_enabled = false;
	bt_mesh_brg_cfg_row_cnt = 0;
	memset(brg_tbl, 0, sizeof(brg_tbl));

#if CONFIG_BT_SETTINGS
	err = settings_delete("bt/mesh/brg_en");

	if (err) {
		return err;
	}

	err = settings_delete("bt/mesh/brg_tbl");
#endif
	return err;
}

int bt_mesh_brg_cfg_tbl_get(const struct bt_mesh_brg_cfg_row **rows)
{
	*rows = brg_tbl;
	return bt_mesh_brg_cfg_row_cnt;
}

int bt_mesh_brg_cfg_tbl_add(enum bt_mesh_brg_cfg_dir direction, uint16_t net_idx1,
			    uint16_t net_idx2, uint16_t addr1, uint16_t addr2)
{
	/* Sanity checks */
	if (!BT_MESH_ADDR_IS_UNICAST(addr1) || net_idx1 == net_idx2 || addr1 == addr2 ||
	    net_idx1 > 0x03FF || net_idx2 > 0x03FF) {
		return -EINVAL;
	}

	if (direction != BT_MESH_BRG_CFG_DIR_ONEWAY && direction != BT_MESH_BRG_CFG_DIR_TWOWAY) {
		return -EINVAL;
	}

	if ((direction == BT_MESH_BRG_CFG_DIR_ONEWAY &&
		(addr2 == BT_MESH_ADDR_UNASSIGNED || addr2 == BT_MESH_ADDR_ALL_NODES)) ||
	    (direction == BT_MESH_BRG_CFG_DIR_TWOWAY &&
		!BT_MESH_ADDR_IS_UNICAST(addr2))) {
		return -EINVAL;
	}

	/* Check if entry already exists, if yes, then, update the direction field and it is a
	 * success.
	 * "If a Bridging Table state entry corresponding to the received message exists, the
	 * element shall set the Directions field in the entry to the value of the Directions field
	 * in the received message."
	 */
	for (int i = 0; i < bt_mesh_brg_cfg_row_cnt; i++) {
		if (brg_tbl[i].net_idx1 == net_idx1 &&
		    brg_tbl[i].net_idx2 == net_idx2 && brg_tbl[i].addr1 == addr1 &&
		    brg_tbl[i].addr2 == addr2) {
			brg_tbl[i].direction = direction;
			goto store;
		}
	}

	/* Empty element, is the current table row counter */
	if (bt_mesh_brg_cfg_row_cnt == CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX) {
		return -ENOMEM;
	}

	/* Update the row */
	brg_tbl[bt_mesh_brg_cfg_row_cnt].direction = direction;
	brg_tbl[bt_mesh_brg_cfg_row_cnt].net_idx1 = net_idx1;
	brg_tbl[bt_mesh_brg_cfg_row_cnt].net_idx2 = net_idx2;
	brg_tbl[bt_mesh_brg_cfg_row_cnt].addr1 = addr1;
	brg_tbl[bt_mesh_brg_cfg_row_cnt].addr2 = addr2;
	bt_mesh_brg_cfg_row_cnt++;

store:
#if IS_ENABLED(CONFIG_BT_SETTINGS)
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_BRG_PENDING);
#endif

	return 0;
}

void bt_mesh_brg_cfg_tbl_foreach_subnet(uint16_t src, uint16_t dst, uint16_t net_idx,
					bt_mesh_brg_cfg_cb_t cb, void *user_data)
{
	for (int i = 0; i < bt_mesh_brg_cfg_row_cnt; i++) {
		if ((brg_tbl[i].direction ==  BT_MESH_BRG_CFG_DIR_ONEWAY ||
		     brg_tbl[i].direction == BT_MESH_BRG_CFG_DIR_TWOWAY) &&
		     brg_tbl[i].net_idx1 == net_idx && brg_tbl[i].addr1 == src &&
		     brg_tbl[i].addr2 == dst) {
			cb(brg_tbl[i].net_idx2, user_data);
		} else if ((brg_tbl[i].direction == BT_MESH_BRG_CFG_DIR_TWOWAY &&
		     brg_tbl[i].net_idx2 == net_idx && brg_tbl[i].addr2 == src &&
		     brg_tbl[i].addr1 == dst)) {
			cb(brg_tbl[i].net_idx1, user_data);
		}
	}
}

void bt_mesh_brg_cfg_tbl_remove(uint16_t net_idx1, uint16_t net_idx2, uint16_t addr1,
				uint16_t addr2)
{
#if IS_ENABLED(CONFIG_BT_SETTINGS)
	bool store = false;
#endif

	/* Iterate over items and set matching row to 0, if nothing exist, or nothing matches, then
	 * it is success (similar to add)
	 */
	if (bt_mesh_brg_cfg_row_cnt == 0) {
		return;
	}

	for (int i = 0; i < bt_mesh_brg_cfg_row_cnt; i++) {
		/* Match according to remove behavior in Section 4.4.9.2.2 of MshPRT_v1.1 */
		if (brg_tbl[i].direction) {
			if (!(brg_tbl[i].net_idx1 == net_idx1 && brg_tbl[i].net_idx2 == net_idx2)) {
				continue;
			}

			if ((brg_tbl[i].addr1 == addr1 && brg_tbl[i].addr2 == addr2) ||
			    (addr2 == BT_MESH_ADDR_UNASSIGNED && brg_tbl[i].addr1 == addr1) ||
			    (addr1 == BT_MESH_ADDR_UNASSIGNED && brg_tbl[i].addr2 == addr2)) {
				memset(&brg_tbl[i], 0, sizeof(brg_tbl[i]));
				brg_tbl_compact();
#if IS_ENABLED(CONFIG_BT_SETTINGS)
				store = true;
#endif
			}
		}
	}
#if IS_ENABLED(CONFIG_BT_SETTINGS)
	if (store) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_BRG_PENDING);
	}
#endif
}
