/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/internal_trusted_storage.h>
#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

static struct k_work_delayable worker;

#define MAX_CHUNK_SIZE   CONFIG_TFM_ITS_MAX_ASSET_SIZE
#define MAX_SETTINGS     CONFIG_SETTINGS_ITS_NUM_ENTRIES
#define ENTRIES_BASE_UID 0x1

struct setting_entry {
	char name[SETTINGS_MAX_NAME_LEN];
	char value[SETTINGS_MAX_VAL_LEN];
	size_t val_len;
};

static struct setting_entry entries[MAX_SETTINGS];
static int entries_count;

static int settings_its_load(struct settings_store *cs, const struct settings_load_arg *arg);
static int settings_its_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len);

static const struct settings_store_itf settings_its_itf = {
	.csi_load = settings_its_load,
	.csi_save = settings_its_save,
};

static struct settings_store default_settings_its = {
	.cs_itf = &settings_its_itf
};

static int store_entries(void)
{
	psa_status_t status;
	psa_storage_uid_t uid = ENTRIES_BASE_UID;
	size_t remaining = sizeof(entries);
	size_t chunk_size = MAX_CHUNK_SIZE;
	const uint8_t *metadata_bytes = (const uint8_t *)&entries;

	/*
	 * Each ITS UID is treated like a sector. Data is written to each ITS node until
	 * that node is full, before incrementing the UID. This is done to minimize the
	 * number of allocated ITS nodes and to avoid wasting allocated bytes.
	 */
	while (remaining > 0) {
		size_t write_size = (remaining > chunk_size) ? chunk_size : remaining;

		status = psa_its_set(uid, write_size, metadata_bytes, PSA_STORAGE_FLAG_NONE);
		if (status) {
			LOG_ERR("Error storing %d bytes of metadata! Bytes Remaining: %d, status: "
				"%d",
				write_size, remaining, status);
			return status;
		}

		metadata_bytes += write_size;
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
	psa_storage_uid_t uid = ENTRIES_BASE_UID;
	size_t remaining = sizeof(entries);
	size_t chunk_size = MAX_CHUNK_SIZE;
	uint8_t *metadata_bytes = (uint8_t *)&entries;

	/*
	 * Each ITS UID is treated like a sector. Data is written to each ITS node until
	 * that node is full, before incrementing the UID. This is done to minimize the
	 * number of allocated ITS nodes and to avoid wasting allocated bytes.
	 */
	while (remaining > 0) {
		size_t to_read = (remaining > chunk_size) ? chunk_size : remaining;

		status = psa_its_get(uid, 0, to_read, metadata_bytes, &bytes_read);
		if (status) {
			return status;
		}

		metadata_bytes += bytes_read;
		remaining -= bytes_read;
		uid++;
	}

	for (int i = 0; i < MAX_SETTINGS; i++) {
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
	LOG_DBG("ITS Read - index: %d", *(int *)back_end);

	memcpy(data, entries[*(int *)back_end].value, len);

	/*
	 * Callback expects return value of the number of bytes read
	 */
	return entries[*(int *)back_end].val_len;
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
	if (entries_count >= MAX_SETTINGS) {
		LOG_ERR("%s: Max settings reached: %d", __func__, MAX_SETTINGS);
		return -ENOMEM;
	}

	if (val_len > SETTINGS_MAX_VAL_LEN) {
		LOG_ERR("%s: Invalid settings size - val_len: %d", __func__, val_len);
		return -EINVAL;
	}

	int index;

	/* Search metadata to see if entry already exists */
	for (index = 0; index < MAX_SETTINGS; index++) {
		if (strncmp(entries[index].name, name, SETTINGS_MAX_NAME_LEN) == 0) {
			break;
		} else if (strnlen(entries[index].name, SETTINGS_MAX_NAME_LEN) == 0) {
			/* New setting being entered */
			entries_count++;
			break;
		}
	}

	LOG_DBG("ITS Save - index %d: name %s, val_len %d", index, name, val_len);

	/* Update metadata */
	strncpy(entries[index].name, name, SETTINGS_MAX_NAME_LEN);
	memcpy(entries[index].value, value, val_len);
	entries[index].val_len = val_len;

	k_work_schedule(&worker, K_MSEC(CONFIG_SETTINGS_ITS_LAZY_PERSIST_DELAY_MS));

	return 0;
}

void worker_persist_entries_struct_fn(struct k_work *work)
{
	store_entries();
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
