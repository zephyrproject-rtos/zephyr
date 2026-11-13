/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kvss/nvs.h>
#include <zephyr/settings/settings.h>
#include <zephyr/ztest.h>

#include <settings/settings_nvs.h>

#define NAME_ID  (NVS_NAMECNT_ID + 1U)
#define VALUE_ID (NAME_ID + NVS_NAME_ID_OFFSET)

static const char setting_name[] = "test/value";
static const char unreadable_name[] = "unreadable/value";
static const uint32_t setting_value = 0x12345678U;
static struct settings_nvs *backend;
static bool fail_read;
static uint16_t fail_id;
static uint32_t failed_reads;
static uint32_t writes;
static uint32_t deletes;
static uint32_t callbacks;

ssize_t __real_nvs_read(struct nvs_fs *fs, uint16_t id, void *data, size_t len);
ssize_t __real_nvs_write(struct nvs_fs *fs, uint16_t id, const void *data, size_t len);
int __real_nvs_delete(struct nvs_fs *fs, uint16_t id);

ssize_t __wrap_nvs_read(struct nvs_fs *fs, uint16_t id, void *data, size_t len)
{
	if (fail_read && id == fail_id) {
		failed_reads++;
		return -EIO;
	}
	return __real_nvs_read(fs, id, data, len);
}

ssize_t __wrap_nvs_write(struct nvs_fs *fs, uint16_t id, const void *data, size_t len)
{
	writes++;
	return __real_nvs_write(fs, id, data, len);
}

int __wrap_nvs_delete(struct nvs_fs *fs, uint16_t id)
{
	deletes++;
	return __real_nvs_delete(fs, id);
}

static int read_setting(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			void *param)
{
	uint32_t value;

	ARG_UNUSED(param);
	zassert_str_equal(key, "value");
	zassert_equal(len, sizeof(value));
	zassert_equal(read_cb(cb_arg, &value, sizeof(value)), sizeof(value));
	zassert_equal(value, setting_value);
	callbacks++;
	return 0;
}

static int set_setting(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	return read_setting(key, len, read_cb, cb_arg, NULL);
}

SETTINGS_STATIC_HANDLER_DEFINE(test, "test", NULL, set_setting, NULL, NULL);

static void *setup(void)
{
	void *storage;

	zassert_ok(settings_subsys_init());
	zassert_ok(settings_storage_get(&storage));
	backend = CONTAINER_OF(storage, struct settings_nvs, cf_nvs);
	return NULL;
}

static void clear_cache(void)
{
#if defined(CONFIG_SETTINGS_NVS_NAME_CACHE)
	memset(backend->cache, 0, sizeof(backend->cache));
	backend->cache_next = 0U;
	backend->cache_total = 0U;
	backend->loaded = false;
#endif
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	fail_read = false;
	failed_reads = 0U;
	callbacks = 0U;
	zassert_ok(nvs_clear(&backend->cf_nvs));
	clear_cache();
	zassert_ok(settings_nvs_backend_init(backend));
	writes = 0U;
	deletes = 0U;
}

static void seed_setting(void)
{
	zassert_ok(settings_save_one(setting_name, &setting_value, sizeof(setting_value)));
	zassert_equal(backend->last_name_id, NAME_ID);
	writes = 0U;
	deletes = 0U;
}

static void inject_read_error(uint16_t id)
{
	fail_id = id;
	fail_read = true;
}

static void assert_preserved(void)
{
	uint16_t last_name_id;
	uint32_t value;
	char name[sizeof(setting_name)];

	zassert_true(failed_reads > 0U, "read failure was not injected");
	zassert_equal(writes, 0U, "failed read caused a write");
	zassert_equal(deletes, 0U, "failed read caused a deletion");
	zassert_equal(backend->last_name_id, NAME_ID);
	fail_read = false;
	zassert_equal(
		nvs_read(&backend->cf_nvs, NVS_NAMECNT_ID, &last_name_id, sizeof(last_name_id)),
		sizeof(last_name_id));
	zassert_equal(last_name_id, NAME_ID);
	zassert_equal(nvs_read(&backend->cf_nvs, NAME_ID, name, sizeof(name)),
		      sizeof(setting_name) - 1U);
	zassert_mem_equal(name, setting_name, sizeof(setting_name) - 1U);
	zassert_equal(nvs_read(&backend->cf_nvs, VALUE_ID, &value, sizeof(value)), sizeof(value));
	zassert_equal(value, setting_value);
	zassert_ok(settings_load_subtree_direct("test", read_setting, NULL));
	zassert_equal(callbacks, 1U);
}

/** @brief Missing metadata initializes an empty store. */
ZTEST(settings_nvs_read_error, test_init_missing_metadata)
{
	zassert_ok(settings_nvs_backend_init(backend));
	zassert_equal(backend->last_name_id, NVS_NAMECNT_ID);
	zassert_equal(writes, 0U);
	zassert_equal(deletes, 0U);
}

/** @brief Valid metadata is restored after initialization. */
ZTEST(settings_nvs_read_error, test_init_valid_metadata)
{
	seed_setting();
	backend->last_name_id = NVS_NAMECNT_ID;
	zassert_ok(settings_nvs_backend_init(backend));
	zassert_equal(backend->last_name_id, NAME_ID);
}

/** @brief A metadata read failure cannot reset the largest allocated ID. */
ZTEST(settings_nvs_read_error, test_init_read_error)
{
	seed_setting();
	inject_read_error(NVS_NAMECNT_ID);
	zassert_equal(settings_nvs_backend_init(backend), -EIO);
	assert_preserved();
}

/** @brief Truncated and oversized metadata are rejected. */
ZTEST(settings_nvs_read_error, test_init_invalid_metadata_size)
{
	uint8_t value[sizeof(uint16_t) + 1U] = {0};
	const size_t lengths[] = {1U, sizeof(value)};

	for (size_t i = 0; i < ARRAY_SIZE(lengths); i++) {
		zassert_equal(nvs_write(&backend->cf_nvs, NVS_NAMECNT_ID, value, lengths[i]),
			      lengths[i]);
		zassert_equal(settings_nvs_backend_init(backend), -EINVAL);
	}
}

/** @brief Metadata IDs must belong to the reserved name range. */
ZTEST(settings_nvs_read_error, test_init_invalid_metadata_id)
{
	const uint16_t ids[] = {NVS_NAMECNT_ID - 1U, NVS_NAMECNT_ID + NVS_NAME_ID_OFFSET};

	for (size_t i = 0; i < ARRAY_SIZE(ids); i++) {
		zassert_equal(nvs_write(&backend->cf_nvs, NVS_NAMECNT_ID, &ids[i], sizeof(ids[i])),
			      sizeof(ids[i]));
		zassert_equal(settings_nvs_backend_init(backend), -EINVAL);
	}
}

static void check_load_error(uint16_t id, bool direct)
{
	uint16_t last_name_id;
	uint32_t value;
	uint32_t replacement = setting_value + 1U;
	char name[sizeof(unreadable_name)];

	seed_setting();
	/* NVS scans descending IDs, so the unreadable entry precedes the valid one. */
	zassert_ok(settings_save_one(unreadable_name, &setting_value, sizeof(setting_value)));
	clear_cache();
	writes = 0U;
	deletes = 0U;
	inject_read_error(id);
	if (direct) {
		zassert_equal(settings_load_subtree_direct("test", read_setting, NULL), -EIO);
	} else {
		zassert_ok(settings_load());
	}
	zassert_equal(callbacks, 1U, "valid setting was not loaded after the read error");
	zassert_true(failed_reads > 0U);
	zassert_equal(writes, 0U);
	zassert_equal(deletes, 0U);
	zassert_equal(backend->last_name_id, NAME_ID + 1U);
	fail_read = false;
	zassert_equal(
		nvs_read(&backend->cf_nvs, NVS_NAMECNT_ID, &last_name_id, sizeof(last_name_id)),
		sizeof(last_name_id));
	zassert_equal(last_name_id, NAME_ID + 1U);
	zassert_equal(nvs_read(&backend->cf_nvs, NAME_ID + 1U, name, sizeof(name)),
		      sizeof(unreadable_name) - 1U);
	zassert_mem_equal(name, unreadable_name, sizeof(unreadable_name) - 1U);
	zassert_equal(nvs_read(&backend->cf_nvs, VALUE_ID + 1U, &value, sizeof(value)),
		      sizeof(value));
	zassert_equal(value, setting_value);

	/* An incomplete name cache must not cause a duplicate when updating this entry. */
	zassert_ok(settings_save_one(unreadable_name, &replacement, sizeof(replacement)));
	zassert_equal(backend->last_name_id, NAME_ID + 1U);
	zassert_equal(nvs_read(&backend->cf_nvs, NAME_ID + 2U, name, sizeof(name)), -ENOENT);
	zassert_equal(nvs_read(&backend->cf_nvs, VALUE_ID + 1U, &value, sizeof(value)),
		      sizeof(value));
	zassert_equal(value, replacement);
}

/** @brief A failed name read preserves the record and permits later settings to load. */
ZTEST(settings_nvs_read_error, test_load_name_error)
{
	check_load_error(NAME_ID + 1U, true);
}

/** @brief A failed value read preserves the record and permits later settings to load. */
ZTEST(settings_nvs_read_error, test_load_value_error)
{
	check_load_error(VALUE_ID + 1U, true);
}

/** @brief Normal loading still calls handlers after a name read fails. */
ZTEST(settings_nvs_read_error, test_normal_load_name_error)
{
	check_load_error(NAME_ID + 1U, false);
}

/** @brief Normal loading still calls handlers after a value read fails. */
ZTEST(settings_nvs_read_error, test_normal_load_value_error)
{
	check_load_error(VALUE_ID + 1U, false);
}

static int stop_loading(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg,
			void *param)
{
	ARG_UNUSED(key);
	ARG_UNUSED(len);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(cb_arg);
	callbacks++;
	return *(int *)param;
}

/** @brief A direct callback can still stop the backend scan. */
ZTEST(settings_nvs_read_error, test_callback_stop)
{
	int results[] = {-EACCES, 1};

	seed_setting();
	zassert_ok(settings_save_one(unreadable_name, &setting_value, sizeof(setting_value)));
	for (size_t i = 0; i < ARRAY_SIZE(results); i++) {
		callbacks = 0U;
		zassert_equal(settings_load_subtree_direct(NULL, stop_loading, &results[i]),
			      results[i] < 0 ? results[i] : 0);
		zassert_equal(callbacks, 1U);
	}
}

/** @brief A callback stop does not hide an earlier storage error. */
ZTEST(settings_nvs_read_error, test_callback_stop_after_error)
{
	int result = 1;

	seed_setting();
	zassert_ok(settings_save_one(unreadable_name, &setting_value, sizeof(setting_value)));
	inject_read_error(NAME_ID + 1U);
	zassert_equal(settings_load_subtree_direct(NULL, stop_loading, &result), -EIO);
	zassert_equal(callbacks, 1U);
}

/** @brief A genuinely missing name still allows cleanup of its orphaned value. */
ZTEST(settings_nvs_read_error, test_load_missing_name)
{
	uint32_t value;

	seed_setting();
	zassert_ok(nvs_delete(&backend->cf_nvs, NAME_ID));
	zassert_ok(settings_load_subtree_direct("test", read_setting, NULL));
	zassert_equal(callbacks, 0U);
	zassert_equal(backend->last_name_id, NVS_NAMECNT_ID);
	zassert_equal(nvs_read(&backend->cf_nvs, VALUE_ID, &value, sizeof(value)), -ENOENT);
}

static void check_save_error(bool warm_cache, bool delete)
{
	uint32_t replacement = setting_value + 1U;

	seed_setting();
	if (warm_cache) {
		zassert_ok(settings_load_subtree_direct("test", read_setting, NULL));
		callbacks = 0U;
	} else {
		clear_cache();
	}
	inject_read_error(NAME_ID);
	if (delete) {
		zassert_equal(settings_delete(setting_name), -EIO);
	} else {
		zassert_equal(settings_save_one(setting_name, &replacement, sizeof(replacement)),
			      -EIO);
	}
	assert_preserved();
}

/** @brief A failed name scan cannot create a duplicate setting. */
ZTEST(settings_nvs_read_error, test_save_scan_error)
{
	check_save_error(false, false);
}

/** @brief A failed cached name read cannot create a duplicate setting. */
ZTEST(settings_nvs_read_error, test_save_cached_error)
{
	check_save_error(true, false);
}

/** @brief A failed name scan must not report successful deletion. */
ZTEST(settings_nvs_read_error, test_delete_scan_error)
{
	check_save_error(false, true);
}

/** @brief A failed cached name read must not report successful deletion. */
ZTEST(settings_nvs_read_error, test_delete_cached_error)
{
	check_save_error(true, true);
}

/** @brief A genuinely missing entry can be reused when saving a setting. */
ZTEST(settings_nvs_read_error, test_save_missing_name)
{
	seed_setting();
	zassert_ok(nvs_delete(&backend->cf_nvs, NAME_ID));
	zassert_ok(nvs_delete(&backend->cf_nvs, VALUE_ID));
	clear_cache();
	zassert_ok(settings_save_one(setting_name, &setting_value, sizeof(setting_value)));
	zassert_equal(backend->last_name_id, NAME_ID);
	zassert_ok(settings_load_subtree_direct("test", read_setting, NULL));
	zassert_equal(callbacks, 1U);
}

ZTEST_SUITE(settings_nvs_read_error, NULL, setup, before, NULL, NULL);
