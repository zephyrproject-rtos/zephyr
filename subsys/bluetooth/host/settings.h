/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

#include <zephyr/settings/settings.h>
#include <common/bt_settings_commit.h>

/* Max settings key length (with all components) */
#define BT_SETTINGS_KEY_MAX 36

/* Base64-encoded string buffer size of in_size bytes */
#define BT_SETTINGS_SIZE(in_size) ((((((in_size) - 1) / 3) * 4) + 4) + 1)

#define BT_SETTINGS_DEFINE(_hname, _subtree, _set, _commit)                                        \
	SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(bt_##_hname, "bt/" _subtree, NULL, _set, _commit,\
						  NULL, BT_SETTINGS_CPRIO_1)


#define ID_DATA_LEN(array) (bt_dev.id_count * sizeof(array[0]))

int bt_settings_store(const char *key, uint8_t id, const bt_addr_le_t *addr, const void *value,
		      size_t val_len);
int bt_settings_delete(const char *key, uint8_t id, const bt_addr_le_t *addr);

void bt_testing_settings_store_hook(const char *key, const void *value, size_t val_len);
void bt_testing_settings_delete_hook(const char *key);

/* Helpers for keys containing a bdaddr */
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
			    const bt_addr_le_t *addr, const char *key);
int bt_settings_decode_key(const char *key, bt_addr_le_t *addr);

void bt_settings_save_id(void);

int bt_settings_init(void);

int bt_settings_store_sc(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len);
int bt_settings_delete_sc(uint8_t id, const bt_addr_le_t *addr);

int bt_settings_store_cf(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len);
int bt_settings_delete_cf(uint8_t id, const bt_addr_le_t *addr);

int bt_settings_store_ccc(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len);
int bt_settings_delete_ccc(uint8_t id, const bt_addr_le_t *addr);

int bt_settings_store_hash(const void *value, size_t val_len);
int bt_settings_delete_hash(void);

int bt_settings_store_name(const void *value, size_t val_len);
int bt_settings_delete_name(void);

int bt_settings_store_appearance(const void *value, size_t val_len);
int bt_settings_delete_appearance(void);

int bt_settings_store_id(void);
int bt_settings_delete_id(void);

int bt_settings_store_irk(void);
int bt_settings_delete_irk(void);

int bt_settings_store_link_key(const bt_addr_le_t *addr, const void *value, size_t val_len);
int bt_settings_delete_link_key(const bt_addr_le_t *addr);

int bt_settings_store_keys(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len);
int bt_settings_delete_keys(uint8_t id, const bt_addr_le_t *addr);
