/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kvss/nvs.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/ztest.h>

#include <settings/settings_nvs.h>

#define TEST_SUBTREE  "crc"
#define TEST_KEY_NAME TEST_SUBTREE "/name"
#define TEST_KEY_LONG TEST_SUBTREE "/long"

/* Long enough that the loader's one-byte probe cannot verify it: NVS only
 * checks the data CRC when the whole element is read.
 */
static const uint8_t long_value[] = {0x5a, 0xa5, 0x3c, 0xc3, 0x69, 0x96, 0x0f, 0xf0};

/* Scoped to one key: settings_load() walks every entry, including a
 * corrupt one left by an earlier test.
 */
static const char *watched_key;
static int handler_calls;
static ssize_t handler_read_rc;

static int test_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	uint8_t buf[sizeof(long_value)];
	ssize_t rc;

	rc = read_cb(cb_arg, buf, MIN(len, sizeof(buf)));

	/* The handler is given the key without its subtree prefix. */
	if ((watched_key == NULL) || (strcmp(name, watched_key + strlen(TEST_SUBTREE) + 1) != 0)) {
		return 0;
	}

	handler_calls++;
	handler_read_rc = rc;

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(test_crc, TEST_SUBTREE, NULL, test_set, NULL, NULL);

static struct nvs_fs *test_storage(void)
{
	void *storage = NULL;

	zassert_ok(settings_storage_get(&storage), "no settings storage");
	zassert_not_null(storage, "no settings storage");

	return (struct nvs_fs *)storage;
}

static uint16_t test_name_id(struct nvs_fs *fs, const char *key)
{
	char name[SETTINGS_MAX_NAME_LEN + 1];

	for (uint16_t id = NVS_NAMECNT_ID + 1; id < NVS_NAMECNT_ID + NVS_NAME_ID_OFFSET; id++) {
		ssize_t rc = nvs_read(fs, id, name, sizeof(name) - 1);

		if (rc <= 0) {
			continue;
		}

		name[rc] = '\0';
		if (strcmp(name, key) == 0) {
			return id;
		}
	}

	zassert_unreachable("stored name entry not found");

	return 0;
}

/* Clears one bit of the given pattern on the partition, so NVS data CRC
 * rejects it on the next read. No erase needed - flash writes only clear bits.
 */
static void corrupt_stored_bytes(const void *pattern, size_t pattern_len)
{
	static uint8_t partition[PARTITION_SIZE(storage_partition)];
	const struct flash_area *fa;
	const uint8_t zero = 0;
	size_t hit;
	bool found = false;

	zassert_ok(flash_area_open(PARTITION_ID(storage_partition), &fa));
	zassert_ok(flash_area_read(fa, 0, partition, sizeof(partition)));

	for (hit = 0; hit + pattern_len <= sizeof(partition); hit++) {
		if (memcmp(&partition[hit], pattern, pattern_len) == 0) {
			found = true;
			break;
		}
	}
	zassert_true(found, "stored bytes not found on the partition");

	zassert_ok(flash_area_write(fa, hit, &zero, sizeof(zero)));
	flash_area_close(fa);
}

/* A corrupt name entry is dropped by settings_load(), along with its
 * value entry.
 */
ZTEST(settings_nvs_data_crc, test_corrupt_name_is_deleted)
{
	struct nvs_fs *fs;
	uint16_t name_id;
	char name[SETTINGS_MAX_NAME_LEN + 1];
	uint8_t value = 0;
	const uint8_t stored = 0x42;

	zassert_ok(settings_save_one(TEST_KEY_NAME, &stored, sizeof(stored)));

	fs = test_storage();
	name_id = test_name_id(fs, TEST_KEY_NAME);

	corrupt_stored_bytes(TEST_KEY_NAME, strlen(TEST_KEY_NAME));

	/* The injection took effect: the name no longer reads back. */
	zassert_equal(nvs_read(fs, name_id, name, sizeof(name) - 1), -EIO,
		      "corruption was not injected");

	watched_key = TEST_KEY_NAME;
	handler_calls = 0;
	zassert_ok(settings_load());
	zassert_equal(handler_calls, 0, "corrupted entry was handed to a handler");

	zassert_equal(nvs_read(fs, name_id, name, sizeof(name) - 1), -ENOENT,
		      "corrupted name entry survived settings_load()");
	zassert_equal(nvs_read(fs, name_id + NVS_NAME_ID_OFFSET, &value, sizeof(value)), -ENOENT,
		      "orphaned value entry survived settings_load()");
}

/* Same but for a value longer than one byte: the loader's one-byte probe
 * succeeds without a CRC check, so the failure surfaces only when the
 * handler reads the value; both entries must still be dropped.
 */
ZTEST(settings_nvs_data_crc, test_corrupt_long_value_is_deleted)
{
	struct nvs_fs *fs;
	uint16_t name_id;
	char name[SETTINGS_MAX_NAME_LEN + 1];
	uint8_t value[sizeof(long_value)];

	zassert_ok(settings_save_one(TEST_KEY_LONG, long_value, sizeof(long_value)));

	fs = test_storage();
	name_id = test_name_id(fs, TEST_KEY_LONG);

	corrupt_stored_bytes(long_value, sizeof(long_value));

	/* Full read fails but the loader's one-byte probe still succeeds -
	 * that gap is what this test targets.
	 */
	zassert_equal(nvs_read(fs, name_id + NVS_NAME_ID_OFFSET, value, sizeof(value)), -EIO,
		      "corruption was not injected");
	zassert_equal(nvs_read(fs, name_id + NVS_NAME_ID_OFFSET, value, 1),
		      (ssize_t)sizeof(long_value), "the one-byte probe did not stay silent");

	watched_key = TEST_KEY_LONG;
	handler_calls = 0;
	handler_read_rc = 0;
	zassert_ok(settings_load());
	zassert_equal(handler_calls, 1, "handler was not called for a readable name");
	zassert_equal(handler_read_rc, -EIO, "the handler's read did not fail");

	zassert_equal(nvs_read(fs, name_id, name, sizeof(name) - 1), -ENOENT,
		      "name entry of a corrupt value survived settings_load()");
	zassert_equal(nvs_read(fs, name_id + NVS_NAME_ID_OFFSET, value, sizeof(value)), -ENOENT,
		      "corrupted value entry survived settings_load()");
}

static void *setup(void)
{
	zassert_ok(settings_subsys_init());

	return NULL;
}

ZTEST_SUITE(settings_nvs_data_crc, NULL, setup, NULL, NULL, NULL);
