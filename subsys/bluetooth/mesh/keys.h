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

#if defined CONFIG_BT_MESH_USES_MBEDTLS_PSA || defined CONFIG_BT_MESH_USES_TFM_PSA

int bt_mesh_key_import(enum bt_mesh_key_type type, const uint8_t in[16], struct bt_mesh_key *out);
int bt_mesh_key_export(uint8_t out[16], const struct bt_mesh_key *in);
void bt_mesh_key_assign(struct bt_mesh_key *dst, const struct bt_mesh_key *src);
int bt_mesh_key_destroy(const struct bt_mesh_key *key);
int bt_mesh_key_compare(const uint8_t raw_key[16], const struct bt_mesh_key *mesh_key);

#elif defined CONFIG_BT_MESH_USES_TINYCRYPT

static inline int bt_mesh_key_import(enum bt_mesh_key_type type, const uint8_t in[16],
				     struct bt_mesh_key *out)
{
	memcpy(out, in, 16);
	return 0;
}

static inline int bt_mesh_key_export(uint8_t out[16], const struct bt_mesh_key *in)
{
	memcpy(out, in, 16);
	return 0;
}

static inline void bt_mesh_key_assign(struct bt_mesh_key *dst, const struct bt_mesh_key *src)
{
	memcpy(dst, src, sizeof(struct bt_mesh_key));
}

static inline int bt_mesh_key_destroy(const struct bt_mesh_key *key)
{
	return 0;
}

static inline int bt_mesh_key_compare(const uint8_t raw_key[16], const struct bt_mesh_key *mesh_key)
{
	return memcmp(mesh_key, raw_key, 16);
}

#endif
