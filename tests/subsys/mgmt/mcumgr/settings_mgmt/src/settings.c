/*
 * Copyright (c) 2023-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

static bool set_called;
static bool get_called;
static bool export_called;
static bool commit_called;
static uint8_t val_aa[4];
static uint8_t val_bb;

struct save_single_data single_data;

static int val_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg);
static int val_handle_commit(void);
static int val_handle_export(int (*cb)(const char *name, const void *value, size_t val_len));
static int val_handle_get(const char *name, char *val, int val_len_max);

SETTINGS_STATIC_HANDLER_DEFINE(val, "test_val", val_handle_get, val_handle_set, val_handle_commit,
			       val_handle_export);

static int val_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;

	set_called = true;

	if (settings_name_steq(name, "aa", &next) && !next) {
		if (len != sizeof(val_aa)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, val_aa, sizeof(val_aa));

		return 0;
	} else if (settings_name_steq(name, "bb", &next) && !next) {
		if (len != sizeof(val_bb)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &val_bb, sizeof(val_bb));

		return 0;
	}

	return -ENOENT;
}

static int val_handle_commit(void)
{
	commit_called = true;
	return 0;
}

static int val_handle_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	export_called = true;

	(void)cb("test_val/aa", val_aa, sizeof(val_aa));
	(void)cb("test_val/bb", &val_bb, sizeof(val_bb));

	return 0;
}

static int val_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	get_called = true;

	if (val_len_max < 0) {
		return -EINVAL;
	}

	if (settings_name_steq(name, "aa", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val_aa));
		memcpy(val, val_aa, val_len_max);
		return val_len_max;
	} else if (settings_name_steq(name, "bb", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(val_bb));
		memcpy(val, &val_bb, val_len_max);
		return val_len_max;
	}

	return -ENOENT;
}

void settings_state_reset(void)
{
	set_called = false;
	get_called = false;
	export_called = false;
	commit_called = false;
}

void settings_state_get(bool *set, bool *get, bool *export, bool *commit)
{
	*set = set_called;
	*get = get_called;
	*export = export_called;
	*commit = commit_called;
}

static int first_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;

	if (settings_name_steq(name, "value1", &next) && !next) {
		if (len != sizeof(single_data.first_val)) {
			return -EINVAL;
		}
		(void)read_cb(cb_arg, &single_data.first_val, sizeof(single_data.first_val));
		single_data.first_set_called = true;
		return 0;
	}
	if (settings_name_steq(name, "value2", &next) && !next) {
		if (len != sizeof(single_data.second_val)) {
			return -EINVAL;
		}
		(void)read_cb(cb_arg, &single_data.second_val, sizeof(single_data.second_val));
		single_data.second_set_called = true;
		return 0;
	}

	return -ENOENT;
}

static int first_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	if (val_len_max < 0) {
		return -EINVAL;
	}

	if (settings_name_steq(name, "value1", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(single_data.first_val));
		memcpy(val, &single_data.first_val, val_len_max);
		single_data.first_get_called = true;
		return val_len_max;
	} else if (settings_name_steq(name, "value2", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(single_data.second_val));
		memcpy(val, &single_data.second_val, val_len_max);
		single_data.second_get_called = true;
		return val_len_max;
	}

	return -ENOENT;
}

static int first_commit(void)
{
	single_data.first_second_commit_called = true;
	return 0;
}

static int first_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	(void)cb("first/value1", &single_data.first_val, sizeof(single_data.first_val));
	(void)cb("first/value2", &single_data.second_val, sizeof(single_data.second_val));

	single_data.first_second_export_called = true;
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(first, "first", first_get, first_set, first_commit, first_export);

static int third_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;

	if (settings_name_steq(name, "value3", &next) && !next) {
		if (len != sizeof(single_data.third_val)) {
			return -EINVAL;
		}
		(void)read_cb(cb_arg, &single_data.third_val, sizeof(single_data.third_val));
		single_data.third_set_called = true;
		return 0;
	}

	return -ENOENT;
}

static int third_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	if (val_len_max < 0) {
		return -EINVAL;
	}

	if (settings_name_steq(name, "value3", &next) && !next) {
		val_len_max = MIN(val_len_max, sizeof(single_data.third_val));
		memcpy(val, &single_data.third_val, val_len_max);
		single_data.third_get_called = true;
		return val_len_max;
	}

	return -ENOENT;
}

static int third_commit(void)
{
	single_data.third_commit_called = true;
	return 0;
}

static int third_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	(void)cb("first/other/value3", &single_data.third_val, sizeof(single_data.third_val));

	single_data.third_export_called = true;
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(third, "first/other", third_get, third_set, third_commit,
			       third_export);

static int forth_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;

	if (settings_name_steq(name, "value4", &next) && !next) {
		if (len != sizeof(single_data.forth_val)) {
			return -EINVAL;
		}
		(void)read_cb(cb_arg, &single_data.forth_val, sizeof(single_data.forth_val));
		single_data.forth_set_called = true;
		return 0;
	}

	return -ENOENT;
}

static int forth_commit(void)
{
	single_data.forth_commit_called = true;
	return 0;
}

static int forth_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	(void)cb("first/expected_fail/value4", &single_data.forth_val,
		 sizeof(single_data.forth_val));

	single_data.forth_export_called = true;
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(forth, "first/expected_fail", NULL, forth_set, forth_commit,
			       forth_export);

void single_modification_reset(void)
{
	single_data.first_second_export_called = false;
	single_data.first_second_commit_called = false;
	single_data.first_get_called = false;
	single_data.first_set_called = false;
	single_data.second_get_called = false;
	single_data.second_set_called = false;
	single_data.third_export_called = false;
	single_data.third_commit_called = false;
	single_data.third_get_called = false;
	single_data.third_set_called = false;
	single_data.forth_export_called = false;
	single_data.forth_commit_called = false;
	single_data.forth_get_called = false;
	single_data.forth_set_called = false;
}
