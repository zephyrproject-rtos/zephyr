/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include "settings_test.h"
#include "settings_priv.h"
#include <zephyr/storage/flash_map.h>

#define TEST_PARTITION		storage_partition
#define TEST_PARTITION_ID	FIXED_PARTITION_ID(TEST_PARTITION)

uint8_t val8;
uint8_t val8_un;
uint32_t val32;
uint64_t val64;

int test_get_called;
int test_set_called;
int test_commit_called;
int test_export_block;

int c2_var_count = 1;

int c1_handle_get(const char *name, char *val, int val_len_max);
int c1_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg);
int c1_handle_commit(void);
int c1_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len));

int c2_handle_get(const char *name, char *val, int val_len_max);
int c2_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg);
int c2_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len));

int c3_handle_get(const char *name, char *val, int val_len_max);
int c3_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg);
int c3_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len));

struct settings_handler c_test_handlers[] = {
	{
		.name = "myfoo",
		.h_get = c1_handle_get,
		.h_set = c1_handle_set,
		.h_commit = c1_handle_commit,
		.h_export = c1_handle_export
	},
	{
		.name = "2nd",
		.h_get = c2_handle_get,
		.h_set = c2_handle_set,
		.h_commit = NULL,
		.h_export = c2_handle_export
	},
	{
		.name = "3",
		.h_get = c3_handle_get,
		.h_set = c3_handle_set,
		.h_commit = NULL,
		.h_export = c3_handle_export
	}
};

char val_string[SETTINGS_TEST_FCB_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];
char test_ref_value[SETTINGS_TEST_FCB_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];

int c1_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	test_get_called = 1;

	if (settings_name_steq(name, "mybar", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val8));
		memcpy(val, &val8, MIN(val_len_max, sizeof(val8)));
		return val_len_max;
	}

	if (settings_name_steq(name, "mybar64", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val64));
		memcpy(val, &val64, MIN(val_len_max, sizeof(val64)));
		return val_len_max;
	}

	return -ENOENT;
}

int c1_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	size_t val_len;
	int rc;
	const char *next;

	test_set_called = 1;
	if (settings_name_steq(name, "mybar", &next) && !next) {
		rc = read_cb(cb_arg, &val8, sizeof(val8));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	if (settings_name_steq(name, "mybar64", &next) && !next) {
		rc = read_cb(cb_arg, &val64, sizeof(val64));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	if (settings_name_steq(name, "unaligned", &next) && !next) {
		val_len = len;
		zassert_equal(val_len, sizeof(val8_un),
			      "value length: %d, ought equal 1", val_len);
		rc = read_cb(cb_arg, &val8_un, sizeof(val8_un));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	return -ENOENT;
}

int c1_handle_commit(void)
{
	test_commit_called = 1;
	return 0;
}

int c1_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	if (test_export_block) {
		return 0;
	}

	(void)cb("myfoo/mybar", &val8, sizeof(val8));

	(void)cb("myfoo/mybar64", &val64, sizeof(val64));

	(void)cb("myfoo/unaligned", &val8_un, sizeof(val8_un));

	return 0;
}

void ctest_clear_call_state(void)
{
	test_get_called = 0;
	test_set_called = 0;
	test_commit_called = 0;
}

int ctest_get_call_state(void)
{
	return test_get_called + test_set_called + test_commit_called;
}

void config_wipe_srcs(void)
{
	sys_slist_init(&settings_load_srcs);
	settings_save_dst = NULL;
}

struct flash_sector fcb_sectors[SETTINGS_TEST_FCB_FLASH_CNT] = {
	[0] = {
		.fs_off = 0x00000000,
		.fs_size = 16 * 1024
	},
	[1] = {
		.fs_off = 0x00004000,
		.fs_size = 16 * 1024
	},
	[2] = {
		.fs_off = 0x00008000,
		.fs_size = 16 * 1024
	},
	[3] = {
		.fs_off = 0x0000c000,
		.fs_size = 16 * 1024
	}
};

void config_wipe_fcb(struct flash_sector *fs, int cnt)
{
	const struct flash_area *fap;
	int rc;
	int i;

	rc = flash_area_open(TEST_PARTITION_ID, &fap);

	for (i = 0; i < cnt; i++) {
		rc = flash_area_flatten(fap, fs[i].fs_off, fs[i].fs_size);
		zassert_true(rc == 0, "Can't get flash area");
	}
}

void
test_config_fill_area(char test_value[SETTINGS_TEST_FCB_VAL_STR_CNT]
				     [SETTINGS_MAX_VAL_LEN],
		      int iteration)
{
	int i, j;

	for (j = 0; j < 64; j++) {
		for (i = 0; i < SETTINGS_MAX_VAL_LEN; i++) {
			test_value[j][i] = ((j * 2) + i + iteration) % 10 + '0';
		}
		test_value[j][sizeof(test_value[j]) - 1] = '\0';
	}
}

char *c2_var_find(char *name)
{
	int idx = 0;
	int len;
	char *eptr;

	len = strlen(name);
	zassert_true(len > 6, "string type expected");
	zassert_true(!strncmp(name, "string", 6), "string type expected");

	idx = strtoul(&name[6], &eptr, 10);
	zassert_true(*eptr == '\0', "EOF");
	zassert_true(idx < c2_var_count,
		     "var index greater than any exporter");

	return val_string[idx];
}

int c2_handle_get(const char *name, char *val, int val_len_max)
{
	int len;
	char *valptr;
	const char *next;
	char argv[32];

	len = settings_name_next(name, &next);
	if (len && !next) {
		strncpy(argv, name, len);
		argv[len] = '\0';
		valptr = c2_var_find(argv);
		if (!valptr) {
			return -ENOENT;
		}

		len = strlen(val_string[0]);
		if (len > val_len_max) {
			len = val_len_max;
		}

		len = MIN(strlen(valptr), len);
		memcpy(val, valptr, len);
		return len;
	}

	return -ENOENT;
}

int c2_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	char *valptr;
	const char *next;
	char argv[32];

	int rc;

	len = settings_name_next(name, &next);
	if (len && !next) {
		strncpy(argv, name, len);
		argv[len] = '\0';
		valptr = c2_var_find(argv);
		if (!valptr) {
			return -ENOENT;
		}

		rc = read_cb(cb_arg, valptr, sizeof(val_string[0]));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		if (rc == 0) {
			(void)memset(valptr, 0, sizeof(val_string[0]));
		}

		return 0;
	}

	return -ENOENT;
}

int c2_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	int i;
	char name[32];

	for (i = 0; i < c2_var_count; i++) {
		snprintf(name, sizeof(name), "2nd/string%d", i);
		(void)cb(name, val_string[i], strlen(val_string[i]));
	}

	return 0;
}

int c3_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	if (settings_name_steq(name, "v", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val32));
		memcpy(val, &val32, MIN(val_len_max, sizeof(val32)));
		return val_len_max;
	}
	return -EINVAL;
}

int c3_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	int rc;
	size_t val_len;
	const char *next;

	if (settings_name_steq(name, "v", &next) && !next) {
		val_len = len;
		zassert_true(val_len == 4, "bad set-value size");

		rc = read_cb(cb_arg, &val32, sizeof(val32));
		zassert_true(rc >= 0, "SETTINGS_VALUE_SET callback");
		return 0;
	}

	return -ENOENT;
}

int c3_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	(void)cb("3/v", &val32, sizeof(val32));


	return 0;
}

void *settings_config_fcb_setup(void)
{
	const struct flash_area *fap;
	int rc;
	uint8_t wbs;

	rc = flash_area_open(TEST_PARTITION_ID, &fap);
	zassume_true(rc == 0, "Can't open storage flash area");

	wbs = flash_area_align(fap);
	zassume_true(wbs <= 32,
		"Flash driver is not compatible with the settings fcb-backend");
	return NULL;
}

ZTEST(settings_config_fcb, test_config_insert_handler2)
{
	test_config_insert2();
}

ZTEST(settings_config_fcb, test_config_insert_handler3)
{
	test_config_insert3();
}

ZTEST_SUITE(settings_config, NULL, settings_config_setup, NULL, NULL,
	    settings_config_teardown);
ZTEST_SUITE(settings_config_fcb, NULL, settings_config_fcb_setup,
	    NULL, NULL, NULL);
