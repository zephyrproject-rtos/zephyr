/*
 * Copyright (c) 2026 Elyorbek Kholikov
 * SPDX-License-Identifier: Apache-2.0
 */

/* A settings backend that keeps the entries in RAM and counts how they are
 * loaded, to check which settings calls the ITS store implementation makes.
 */
#include <errno.h>
#include <string.h>

#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <psa/internal_trusted_storage.h>

#define ENTRY_COUNT    8
#define ENTRY_MAX_SIZE (CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE + 64)

struct entry {
	char name[SETTINGS_MAX_NAME_LEN + 1];
	uint8_t data[ENTRY_MAX_SIZE];
	size_t len;
};

static struct entry entries[ENTRY_COUNT];
static unsigned int load_calls;
static unsigned int load_one_calls;

static struct entry *find_entry(const char *name)
{
	ARRAY_FOR_EACH_PTR(entries, entry) {
		if (entry->len != 0 && strcmp(entry->name, name) == 0) {
			return entry;
		}
	}
	return NULL;
}

static ssize_t read_entry(void *cb_arg, void *data, size_t len)
{
	const struct entry *entry = cb_arg;

	len = MIN(len, entry->len);
	memcpy(data, entry->data, len);
	return len;
}

static int backend_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	ARG_UNUSED(cs);

	++load_calls;

	ARRAY_FOR_EACH_PTR(entries, entry) {
		int ret;

		if (entry->len == 0) {
			continue;
		}
		ret = settings_call_set_handler(entry->name, entry->len, read_entry, entry, arg);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

static ssize_t backend_load_one(struct settings_store *cs, const char *name, char *buf,
				size_t buf_len)
{
	const struct entry *entry = find_entry(name);

	ARG_UNUSED(cs);

	++load_one_calls;

	if (entry == NULL) {
		return 0;
	}
	memcpy(buf, entry->data, MIN(buf_len, entry->len));
	return entry->len;
}

static int backend_save(struct settings_store *cs, const char *name, const char *value,
			size_t val_len)
{
	struct entry *entry = find_entry(name);

	ARG_UNUSED(cs);

	if (value == NULL || val_len == 0) {
		if (entry != NULL) {
			entry->len = 0;
		}
		return 0;
	}
	if (val_len > ENTRY_MAX_SIZE || strlen(name) > SETTINGS_MAX_NAME_LEN) {
		return -ENOSPC;
	}
	if (entry == NULL) {
		ARRAY_FOR_EACH_PTR(entries, free_entry) {
			if (free_entry->len == 0) {
				entry = free_entry;
				break;
			}
		}
	}
	if (entry == NULL) {
		return -ENOSPC;
	}

	strcpy(entry->name, name);
	memcpy(entry->data, value, val_len);
	entry->len = val_len;
	return 0;
}

static const struct settings_store_itf backend_itf = {
	.csi_load = backend_load,
	.csi_load_one = backend_load_one,
	.csi_save = backend_save,
};

static struct settings_store backend_store = {.cs_itf = &backend_itf};

int settings_backend_init(void)
{
	settings_dst_register(&backend_store);
	settings_src_register(&backend_store);
	return 0;
}

/* Reading an entry must not make the settings backend go through the other
 * stored settings.
 */
ZTEST(secure_storage_psa_its, test_get_loads_one_setting)
{
	const psa_storage_uid_t uid = 2 << 16;
	const uint8_t written_data[] = {1, 2, 3, 4};
	uint8_t read_data[sizeof(written_data)];
	size_t data_length;
	psa_status_t ret;

	ret = psa_its_set(uid, sizeof(written_data), written_data, PSA_STORAGE_FLAG_NONE);
	zassert_equal(ret, PSA_SUCCESS);

	load_calls = 0;
	load_one_calls = 0;

	ret = psa_its_get(uid, 0, sizeof(read_data), read_data, &data_length);
	zassert_equal(ret, PSA_SUCCESS);
	zassert_equal(data_length, sizeof(written_data));
	zassert_mem_equal(read_data, written_data, sizeof(written_data));

	zassert_equal(load_one_calls, 1, "%u loads of one setting", load_one_calls);
	zassert_equal(load_calls, 0, "%u loads of all the settings", load_calls);

	ret = psa_its_remove(uid);
	zassert_equal(ret, PSA_SUCCESS);

	ret = psa_its_get(uid, 0, sizeof(read_data), read_data, &data_length);
	zassert_equal(ret, PSA_ERROR_DOES_NOT_EXIST);
	zassert_equal(load_calls, 0, "%u loads of all the settings", load_calls);
}
