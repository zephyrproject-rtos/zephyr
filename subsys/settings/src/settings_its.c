/*
 * Copyright (c) 2019 Laczen
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/internal_trusted_storage.h>
#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/psa/its_ids.h>

LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

K_MUTEX_DEFINE(worker_mutex);
static struct k_work_delayable worker;

struct setting_entry {
	char name[SETTINGS_MAX_NAME_LEN];
	char value[SETTINGS_MAX_VAL_LEN];
	size_t val_len;
};

static struct setting_entry entries[CONFIG_SETTINGS_TFM_ITS_NUM_ENTRIES];
static int entries_count;

static int settings_its_load(struct settings_store *cs, const struct settings_load_arg *arg);
static int settings_its_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len);

static const struct settings_store_itf settings_its_itf = {
	.csi_load = settings_its_load,
	.csi_save = settings_its_save,
};

static struct settings_store default_settings_its = {.cs_itf = &settings_its_itf};

/* Ensure Key configured max size does not exceed reserved Key range */
BUILD_ASSERT(sizeof(entries) / CONFIG_TFM_ITS_MAX_ASSET_SIZE <=
	     ZEPHYR_PSA_SETTINGS_TFM_ITS_UID_RANGE_SIZE,
	     "entries array exceeds reserved ITS UID range");

static int store_entries(void)
{
	psa_status_t status;
	psa_storage_uid_t uid = ZEPHYR_PSA_SETTINGS_TFM_ITS_UID_RANGE_BEGIN;
	size_t remaining = sizeof(entries);
	size_t chunk_size = CONFIG_TFM_ITS_MAX_ASSET_SIZE;
	const uint8_t *data_ptr = (const uint8_t *)&entries;

	/*
	 * Each ITS UID is treated like a sector. Data is written to each ITS node until
	 * that node is full, before incrementing the UID. This is done to minimize the
	 * number of allocated ITS nodes and to avoid wasting allocated bytes.
	 */
	while (remaining > 0) {
		size_t write_size = (remaining > chunk_size) ? chunk_size : remaining;

		status = psa_its_set(uid, write_size, data_ptr, PSA_STORAGE_FLAG_NONE);
		if (status) {
			LOG_ERR("Error storing %d bytes of metadata! Bytes Remaining: %d, status: "
				"%d",
				write_size, remaining, status);
			return status;
		}

		data_ptr += write_size;
		remaining -= write_size;
		uid++;
	}

	LOG_DBG("ITS entries stored successfully - bytes_saved: %d num_entries: %d max_uid: %lld",
		sizeof(entries), entries_count, uid);

	return 0;
}

static int load_entries(void)
{
	psa_status_t status;
	size_t bytes_read;
	psa_storage_uid_t uid = ZEPHYR_PSA_SETTINGS_TFM_ITS_UID_RANGE_BEGIN;
	size_t remaining = sizeof(entries);
	size_t chunk_size = CONFIG_TFM_ITS_MAX_ASSET_SIZE;
	uint8_t *data_ptr = (uint8_t *)&entries;

	/*
	 * Each ITS UID is treated like a sector. Data is written to each ITS node until
	 * that node is full, before incrementing the UID. This is done to minimize the
	 * number of allocated ITS nodes and to avoid wasting allocated bytes.
	 */
	while (remaining > 0) {
		size_t to_read = (remaining > chunk_size) ? chunk_size : remaining;

		status = psa_its_get(uid, 0, to_read, data_ptr, &bytes_read);
		if (status) {
			return status;
		}

		data_ptr += bytes_read;
		remaining -= bytes_read;
		uid++;
	}

	for (int i = 0; i < CONFIG_SETTINGS_TFM_ITS_NUM_ENTRIES; i++) {
		if (strnlen(entries[i].name, SETTINGS_MAX_NAME_LEN) != 0) {
			entries_count++;
		}
	}

	LOG_DBG("ITS entries restored successfully - bytes_loaded: %d, num_entries: %d",
		sizeof(entries), entries_count);

	return 0;
}

/* void *back_end is the index of the entry in metadata entries struct */
static ssize_t settings_its_read_fn(void *back_end, void *data, size_t len)
{
	int index = *(int *)back_end;

	LOG_DBG("ITS Read - index: %d", index);

	if (index < 0 || index >= CONFIG_SETTINGS_TFM_ITS_NUM_ENTRIES) {
		LOG_ERR("Invalid index %d in ITS metadata", index);
		return 0;
	}

	memcpy(data, entries[index].value, len);

	/*
	 * Callback expects return value of the number of bytes read
	 */
	return entries[index].val_len;
}

static int settings_its_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	int ret;

	for (int i = 0; i < entries_count; i++) {
		if (strnlen(entries[i].name, SETTINGS_MAX_NAME_LEN) != 0) {
			/*
			 * Pass the key to the settings handler with it's index as an argument,
			 * to be read during callback function later.
			 */
			ret = settings_call_set_handler(entries[i].name, entries[i].val_len,
							settings_its_read_fn, (void *)&i,
							(void *)arg);
			if (ret) {
				return ret;
			}
		}
	}

	return 0;
}

static int settings_its_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len)
{
	if (entries_count >= CONFIG_SETTINGS_TFM_ITS_NUM_ENTRIES) {
		LOG_ERR("%s: Max settings reached: %d", __func__,
			CONFIG_SETTINGS_TFM_ITS_NUM_ENTRIES);
		return -ENOMEM;
	}

	if (val_len > SETTINGS_MAX_VAL_LEN) {
		LOG_ERR("%s: Invalid settings size - val_len: %d", __func__, val_len);
		return -EINVAL;
	}

	int index;
	bool delete;

	/* Find out if we are doing a delete */
	delete = ((value == NULL) || (val_len == 0));

	/* Lock mutex before manipulating settings array */
	k_mutex_lock(&worker_mutex, K_FOREVER);

	/*
	 * Search metadata to see if entry already exists. Array is compacted, so first blank entry
	 * signals end of settings.
	 */
	for (index = 0; index < CONFIG_SETTINGS_TFM_ITS_NUM_ENTRIES; index++) {
		if (strncmp(entries[index].name, name, SETTINGS_MAX_NAME_LEN) == 0) {
			break;
		} else if (entries[index].val_len == 0) {

			/* Setting already deleted */
			if (delete) {
				LOG_DBG("%s: %s Already deleted!", __func__, name);
				k_mutex_unlock(&worker_mutex);
				return 0;
			}

			/* New setting being entered */
			entries_count++;
			break;
		}
	}

	LOG_DBG("ITS Save - index %d: name %s, val_len %d", index, name, val_len);

	if (delete) {
		/* Clear metadata */
		memset(entries[index].name, 0, SETTINGS_MAX_NAME_LEN);
		memset(entries[index].value, 0, SETTINGS_MAX_VAL_LEN);
		entries[index].val_len = 0;

		/* If setting not at end of array, shift entries */
		if (index < entries_count - 1) {
			memcpy(&entries[index], &entries[index + 1],
			       (entries_count - index - 1) * sizeof(struct setting_entry));
			/* Remove duplicate entry at end of array */
			memset(&entries[entries_count - 1], 0, sizeof(struct setting_entry));
		}

		entries_count--;
	} else {
		/* Update metadata */
		strncpy(entries[index].name, name, SETTINGS_MAX_NAME_LEN);
		memcpy(entries[index].value, value, val_len);
		entries[index].val_len = val_len;
	}

	k_mutex_unlock(&worker_mutex);
	k_work_schedule(&worker, K_MSEC(CONFIG_SETTINGS_TFM_ITS_LAZY_PERSIST_DELAY_MS));

	return 0;
}

void worker_persist_entries_struct_fn(struct k_work *work)
{
	k_mutex_lock(&worker_mutex, K_FOREVER);
	store_entries();
	k_mutex_unlock(&worker_mutex);
}

int settings_backend_init(void)
{
	psa_status_t status;

	/* Load ITS metadata */
	status = load_entries();

	/* If resource DNE, we need to allocate it */
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		status = store_entries();
		if (status) {
			LOG_ERR("Error storing metadata in %s: (status %d)", __func__, status);
			return -EIO;
		}
	} else if (status) {
		LOG_ERR("Error loading metadata in %s: (status %d)", __func__, status);
		return -EIO;
	}

	settings_dst_register(&default_settings_its);
	settings_src_register(&default_settings_its);

	k_work_init_delayable(&worker, worker_persist_entries_struct_fn);

	return 0;
}
