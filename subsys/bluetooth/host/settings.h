/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../common/settings.h"

#define ID_DATA_LEN(array) (bt_dev.id_count * sizeof(array[0]))

int bt_settings_store(const char *key, uint8_t id, const bt_addr_le_t *addr, const void *value,
		      size_t val_len);
int bt_settings_delete(const char *key, uint8_t id, const bt_addr_le_t *addr);

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
