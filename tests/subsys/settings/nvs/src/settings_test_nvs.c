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

int c1_handle_get(const char *name, char *val, int val_len_max);
int c1_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg);
int c1_handle_commit(void);
int c1_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len));

struct settings_handler c_test_handlers[] = {
	{
		.name = "myfoo",
		.h_get = c1_handle_get,
		.h_set = c1_handle_set,
		.h_commit = c1_handle_commit,
		.h_export = c1_handle_export
	},
};

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
			      "value length: %zd, ought equal 1", val_len);
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

ZTEST_SUITE(settings_config, NULL, settings_config_setup, NULL, NULL,
	    settings_config_teardown);
