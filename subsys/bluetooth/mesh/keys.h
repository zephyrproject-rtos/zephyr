/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum bt_mesh_key_type {
	BT_MESH_KEY_TYPE_ECB,
	BT_MESH_KEY_TYPE_CCM,
	BT_MESH_KEY_TYPE_CMAC,
	BT_MESH_KEY_TYPE_NET,
	BT_MESH_KEY_TYPE_APP,
	BT_MESH_KEY_TYPE_DEV
};

int bt_mesh_key_import(enum bt_mesh_key_type type, const uint8_t in[16], struct bt_mesh_key *out);
int bt_mesh_key_export(uint8_t out[16], const struct bt_mesh_key *in);
void bt_mesh_key_assign(struct bt_mesh_key *dst, const struct bt_mesh_key *src);
int bt_mesh_key_destroy(const struct bt_mesh_key *key);
int bt_mesh_key_compare(const uint8_t raw_key[16], const struct bt_mesh_key *mesh_key);
