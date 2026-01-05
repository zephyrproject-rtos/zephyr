/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_MESH_KEY_PRIMARY 0x0000

enum bt_mesh_key_evt {
	BT_MESH_KEY_ADDED,   /* New key added */
	BT_MESH_KEY_DELETED, /* Existing key deleted */
	BT_MESH_KEY_UPDATED, /* KR phase 1, second key added */
	BT_MESH_KEY_SWAPPED, /* KR phase 2, now sending on second key */
	BT_MESH_KEY_REVOKED, /* KR phase 3, old key removed */
};

struct bt_mesh_net;

int bt_mesh_start(void);
void bt_mesh_reprovision(uint16_t addr);
void bt_mesh_dev_key_cand(const uint8_t *key);
void bt_mesh_dev_key_cand_remove(void);
void bt_mesh_dev_key_cand_activate(void);
