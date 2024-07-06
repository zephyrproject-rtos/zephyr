/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 *  PSA ITS emulator over settings.
 */

#include <stdlib.h>

#include <zephyr/bluetooth/mesh.h>
#include <../library/psa_crypto_its.h>

#define LOG_MODULE_NAME pts_its_emu

#include <zephyr/logging/log.h>
#include "mesh/net.h"
#include "mesh/settings.h"

LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

/* The value of 52 bytes was measured practically in the mbedTLS psa security storage. */
#define MAX_ITEM_LENGTH 52
#define MAX_ITEM_NUMBER MBEDTLS_PSA_KEY_SLOT_COUNT

typedef struct {
	uint32_t size;
	psa_storage_create_flags_t flags;
	uint8_t data[MAX_ITEM_LENGTH];
} psa_its_pst_item_t;

typedef struct {
	psa_storage_uid_t uid;
	psa_its_pst_item_t pst_item;
} psa_its_item_t;

static psa_its_item_t item[MAX_ITEM_NUMBER];

static psa_its_item_t *get_item_by_uid(psa_storage_uid_t uid)
{
	for (int i = 0; i < MAX_ITEM_NUMBER; i++) {
		if (uid == item[i].uid) {
			return &item[i];
		}
	}

	return NULL;
}

static int itsemul_set(const char *name, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	k_ssize_t len;
	uint64_t uid;
	psa_its_item_t *p_item;

	LOG_DBG("read out uid: %s", name);

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	uid = strtoull(name, NULL, 10);
	if (uid == ULLONG_MAX) {
		LOG_ERR("Invalid format for uid");
		return -EINVAL;
	}

	p_item = get_item_by_uid(uid);
	if (p_item == NULL) {
		p_item = get_item_by_uid(0ull);
	}

	if (p_item == NULL) {
		LOG_ERR("Insufficient sources for %llu", uid);
		return -EINVAL;
	}

	p_item->uid = uid;

	len = read_cb(cb_arg, &p_item->pst_item, len_rd);
	if (len < 0) {
		LOG_ERR("Failed to read value (err %zd)", len);
		return -EINVAL;
	}

	LOG_HEXDUMP_DBG(&p_item->pst_item, len, "pst_item:");

	if (len != len_rd) {
		LOG_ERR("Unexpected length (%zd != %zu)", len, len_rd);
		return -EINVAL;
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(psa_its_emu, "itsemul", NULL, itsemul_set, NULL, NULL);

psa_status_t psa_its_get_info(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
	psa_its_item_t *p_item;

	LOG_DBG("get info uid: %llu", uid);

	p_item = get_item_by_uid(uid);
	if (p_item == NULL) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	p_info->flags = p_item->pst_item.flags;
	p_info->size = p_item->pst_item.size;

	LOG_DBG("flags: %lu, size: %lu", p_info->flags, p_info->size);

	return PSA_SUCCESS;
}

psa_status_t psa_its_get(psa_storage_uid_t uid, uint32_t data_offset, uint32_t data_length,
			 void *p_data, size_t *p_data_length)
{
	psa_its_item_t *p_item;
	psa_its_pst_item_t *p_pst_item;

	LOG_DBG("get uid: %llu", uid);

	p_item = get_item_by_uid(uid);
	if (p_item == NULL) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	p_pst_item = &p_item->pst_item;

	if (data_offset > p_pst_item->size) {
		return PSA_ERROR_DATA_CORRUPT;
	}

	*p_data_length = MIN(p_pst_item->size - data_offset, data_length);
	memcpy(p_data, p_pst_item->data + data_offset, *p_data_length);

	return PSA_SUCCESS;
}

psa_status_t psa_its_set(psa_storage_uid_t uid, uint32_t data_length, const void *p_data,
			 psa_storage_create_flags_t create_flags)
{
	char path[40];
	psa_its_item_t *p_item;
	psa_its_pst_item_t *p_pst_item;
	psa_status_t status = PSA_SUCCESS;

	LOG_DBG("Set uid: %llu, len: %lu", uid, data_length);

	if (data_length > MAX_ITEM_LENGTH) {
		LOG_ERR("Too long item data: %lu > " STRINGIFY(MAX_ITEM_LENGTH), data_length);
	}

	p_item = get_item_by_uid(uid);
	if (p_item == NULL) {
		p_item = get_item_by_uid(0ull);
	}

	if (p_item == NULL) {
		return PSA_ERROR_STORAGE_FAILURE;
	}

	snprintk(path, sizeof(path), "itsemul/%llu", uid);

	p_item->uid = uid;
	p_pst_item = &p_item->pst_item;
	p_pst_item->size = data_length;
	p_pst_item->flags = create_flags;
	memcpy(p_pst_item->data, p_data, data_length);

	if (settings_save_one(path, p_pst_item, sizeof(psa_its_pst_item_t))) {
		LOG_ERR("Failed to store its item: %s", path);
		status = PSA_ERROR_STORAGE_FAILURE;
	} else {
		LOG_DBG("Stored its item: %s", path);
	}

	return status;
}

psa_status_t psa_its_remove(psa_storage_uid_t uid)
{
	char path[40];
	psa_status_t status = PSA_SUCCESS;
	psa_its_item_t *p_item;

	LOG_DBG("remove uid: %llu", uid);

	p_item = get_item_by_uid(uid);
	if (p_item == NULL) {
		return status;
	}
	memset(p_item, 0, sizeof(psa_its_item_t));

	snprintk(path, sizeof(path), "itsemul/%llu", uid);

	if (settings_delete(path)) {
		LOG_ERR("Failed to remove its item: %s", path);
		status = PSA_ERROR_STORAGE_FAILURE;
	} else {
		LOG_DBG("Removed its item: %s", path);
	}

	return status;
}
