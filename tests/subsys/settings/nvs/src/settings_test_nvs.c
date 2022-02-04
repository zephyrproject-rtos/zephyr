/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include "settings_priv.h"
#include "settings_test.h"

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

char val_string[SETTINGS_TEST_NVS_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];
char test_ref_value[SETTINGS_TEST_NVS_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];

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
		     "var index greather than any exporter");

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

void test_config_empty_lookups(void);
void test_config_insert(void);
void test_config_getset_unknown(void);
void test_config_getset_int(void);
void test_config_getset_int64(void);
void test_config_commit(void);

void test_main(void)
{
	ztest_test_suite(test_config_nvs,
			 /* Config tests */
			 ztest_unit_test(test_config_empty_lookups),
			 ztest_unit_test(test_config_insert),
			 ztest_unit_test(test_config_getset_unknown),
			 ztest_unit_test(test_config_getset_int),
			 ztest_unit_test(test_config_getset_int64),
			 ztest_unit_test(test_config_commit)
			);

	ztest_run_test_suite(test_config_nvs);
}
